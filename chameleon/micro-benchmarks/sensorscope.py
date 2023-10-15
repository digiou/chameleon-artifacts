import argparse
import datetime
import glob
import json
from collections import Counter
from enum import Enum
from multiprocessing import Pool
from pathlib import Path
from typing import Dict, Any, List, Union, Optional

import shortuuid
import numpy as np
import pandas as pd
from pandas import DataFrame

from experiments.io.dataset_stats import compute_detailed_stats
from experiments.io.files import IOConfiguration
from experiments.io.experiment_logger import prepare_logger
from experiments.io.resource_counters import get_physical_cores
from experiments.models.kalman_filters import init_filters
from experiments.io.serialization import NpEncoder
from experiments.plotting.formatting import closest_time_unit
from experiments.sensor_functions.accuracy import calculate_agg_accuracy_windowed
from experiments.sensor_functions.fft import compute_nyquist_and_energy
from experiments.sensor_functions.gathering import gather_values_from_df
from experiments.sensor_functions.interpolation import interpolate_df
from experiments.emulator.scalability import measure_scalability


def get_all_dataframes(path: Path, data_name='meteo') -> List[DataFrame]:
    """Column definitions for meteo:
    1. Station ID
    2. Year
    3. Month
    4. Day
    5. Hour
    6. Minute
    7. Second
    8. Time since the epoch [s]
    9. Sequence Number
    10. Ambient Temperature [°C]
    11. Surface Temperature [°C]
    12. Solar Radiation [W/m^2]
    13. Relative Humidity [%]
    14. Soil Moisture [%]
    15. Watermark [kPa]
    16. Rain Meter [mm]
    17. Wind Speed [m/s]
    18. Wind Direction [°]
    Row: 101 2007 08 06 13 04 22 1186398262 16.720 17.350 0.000 94.747 NaN NaN NaN NaN NaN

    Column definitions for monitor:
    1. Station ID
    2. Year
    3. Month
    4. Day
    5. Hour
    6. Minute
    7. Second
    8. Time since the epoch [s]
    9. Sequence Number
    10. Config Sampling Time [s]
    11. Data Sampling Time [s]
    12. Radio Duty Cycle [%]
    13. Radio Transmission Power [dBm]
    14. Radio Transmission Frequency [MHz]
    15. Primary Buffer Voltage [V]
    16. Secondary Buffer Voltage [V]
    17. Solar Panel Current [mA]
    18. Global Current [mA]
    19. Energy Source
    Row: 2 2007 08 27 17 43 10 1188229390 3.243 4.003 6.202 1.807 2.000"""
    all_dfs = []
    meteo_non_sequenced = ['fishnet-meteo', 'pdg2008-meteo', 'stbernard-meteo', 'genepi-meteo']
    monitor_non_sequenced = []
    non_sequenced = monitor_non_sequenced
    time_col_names = ['station_id', 'year', 'month', 'day', 'hour', 'minute',
                      'second', 'timestamp', 'sequence_no']
    meteo_measurement_cols = ['ambient_temp', 'surface_temp', 'solar_radiation', 'rel_humidity',
                        'soil_moisture', 'watermark', 'rain_meter', 'wind_speed', 'wind_direction']
    monitor_measurement_cols = ['config_sampling', 'data_sampling', 'radio_cycle', 'radio_tx_pwr', 'radio_tx_frq',
                                'primary_vltg', 'secondary_vltg', 'solar_current', 'global_current', 'source']
    measurement_cols = monitor_measurement_cols
    if data_name == "meteo":
        non_sequenced = meteo_non_sequenced
        measurement_cols = meteo_measurement_cols
    dtype = {**{col_name: 'Int64' for col_name in time_col_names if col_name != 'timestamp'},
             **{col_name: np.float64 for col_name in measurement_cols}}
    avg_len = 0.
    logger.info("Loading csvs from disk...")
    search_str = str(path.joinpath("**").joinpath(f"*{data_name}*[0-9].txt"))
    for filename in glob.glob(search_str, recursive=True):
        logger.debug(f"Loading file {filename} from disk...")
        col_names = time_col_names + measurement_cols
        read_csv_options = {'filepath_or_buffer': str(filename), 'header': None, 'names': col_names,
                            'index_col': None, 'dtype': dtype.copy(), 'sep': " ", 'encoding': 'utf-8-sig'}
        for non_sequenced_name in non_sequenced:
            if non_sequenced_name in filename:
                read_csv_options['names'] = time_col_names[:-1] + measurement_cols
                read_csv_options['dtype'].pop('sequence_no')
                break
        raw_dataframe = pd.read_csv(**read_csv_options)
        raw_dataframe.dropna(subset=['timestamp'], inplace=True)
        raw_dataframe['timestamp'] = pd.to_datetime(raw_dataframe['timestamp'], unit='s', utc=True)
        raw_dataframe.set_index('timestamp', drop=True, inplace=True)
        raw_dataframe.index = raw_dataframe.index.tz_convert('Europe/Berlin')
        raw_dataframe = raw_dataframe[~raw_dataframe.index.isna()]
        raw_dataframe = raw_dataframe[~raw_dataframe.index.duplicated(keep='last')]
        raw_dataframe.index.name = filename
        start_date = raw_dataframe.index.min()
        if raw_dataframe.index.max() - start_date > pd.Timedelta(days=15):
            try:
                end_date = start_date + pd.DateOffset(days=12)
                raw_dataframe = raw_dataframe[start_date:end_date]
            except KeyError:
                logger.info(f"{raw_dataframe.index.name} is missing values in first 12 days, skipping...")
                continue
        for measurement_col in measurement_cols:
            raw_dataframe[measurement_col] = raw_dataframe[measurement_col].fillna(method='ffill')
            raw_dataframe[measurement_col] = raw_dataframe[measurement_col].fillna(method='bfill')
        if len(raw_dataframe) == 0:
            continue
        avg_len += len(raw_dataframe)
        all_dfs.append(raw_dataframe)
        logger.debug("Loaded csv!")
    avg_len /= len(all_dfs)
    logger.info(f"Average DF size: {avg_len}")
    logger.info("Done indexing all dataframes!")
    return all_dfs


def plot_fft(sensor_df_list: List[DataFrame], task_pool: Pool, metric="ambient_temp"):
    """
    Consolidate power calculations for all signals.
    From https://stackoverflow.com/questions/29429733/cant-find-the-right-energy-using-scipy-signal-welch
    """
    logger.debug(f"Computing FFT and energy levels per sensor for {metric}...")
    metric_dfs = []
    for sensor_df in sensor_df_list:
        new_df = sensor_df[[metric]]
        new_df.dropna(subset=[metric], inplace=True)
        new_df.rename(columns={f"{metric}": "value"}, inplace=True)
        metric_dfs.append(new_df)
    promises = [task_pool.apply_async(compute_nyquist_and_energy, (sensor_df,)) for sensor_df in metric_dfs if
                len(sensor_df.index) > 2]
    nyquist_results = [res.get() for res in promises]
    oversampled = 0
    zero_energy = 0
    oversampling_ratio = 0.
    immediately_zeroed = 0
    for nyquist_result in nyquist_results:
        if nyquist_result[0]:
            oversampled += 1
            oversampling_ratio += nyquist_result[2]
        elif not nyquist_result[0] and nyquist_result[1] == 0.0 and nyquist_result[2] == 0.0 and nyquist_result[3] == 0.0:
            immediately_zeroed += 1
        if nyquist_result[1] == 0.0:
            zero_energy += 1
    oversampling_ratio /= oversampled
    logger.info(
        f"Total streams for {metric}: {len(sensor_df_list)}, oversampled total: {oversampled}, oversampling ratio: {oversampling_ratio}, immediately zeroed: {immediately_zeroed}")
    logger.debug(f"Done computing FFTs!")


def measure_accuracy(sensor_dfs: List[DataFrame], col_names: List[str], sensor_files_path: Path,
                     current_uuid: str, task_pool: Pool, nyquist_window=10, query_window='1min',
                     use_filter=True, use_only_filter=False,
                     constrain_analyzer: bool = False, max_oversampling: float = 1., sampling_rate: float = 4.,
                     only_necessary: bool = True):
    total_results = {}
    strategies = ["fixed", "chameleon"] if only_necessary else ["baseline", "fixed", "chameleon"]
    file_name = f"qs-sensorscope-{current_uuid}-{datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
    with open(sensor_files_path.joinpath(file_name), 'w', encoding='utf-8') as fp:
        for stream_type in col_names:
            logger.info(f"Sampling stats on {stream_type} streams...")
            mape_accuracy = Counter({"sum": 0., "count": 0., "mean": 0., "median": 0., "max": 0., "min": 0., "std": 0., "var": 0.})
            mape_accuracies = {k: mape_accuracy.copy() for k in strategies}
            max_accuracies = {k: mape_accuracy.copy() for k in strategies}
            samples = {k: 0 for k in strategies}
            promises = [task_pool.apply_async(calculate_agg_accuracy,
                        (raw_sensor, stream_type, nyquist_window, use_filter, use_only_filter, query_window, constrain_analyzer, max_oversampling, sampling_rate,))
                        for raw_sensor in sensor_dfs]
            aggregation_results = [res.get() for res in promises]
            clean_sensor_results = [sensor_res for sensor_res in aggregation_results if sensor_res is not None]
            for result in clean_sensor_results:
                for approach, stats in result.items():
                    mape = Counter(stats["mape"])
                    mape_accuracies[approach] += mape
                    samples[approach] += stats["samples"]
                    max = max_accuracies[approach] | Counter(stats["max"])
                    for key, max_val in max.items():
                        max_accuracies[approach][key] = max_val
            logger.info(f"Stream: {stream_type}, len: {len(clean_sensor_results)}")
            total_results[stream_type] = \
                {"accuracy": dict(mape_accuracies), "samples": dict(samples),
                 "nyq_size": nyquist_window, "q_window": query_window, "sampling_rate": sampling_rate}
        json.dump(total_results, fp, ensure_ascii=False, indent=4, cls=NpEncoder)
        logger.info(f"Wrote data in {sensor_files_path.joinpath(file_name)}")


def calculate_agg_accuracy(sensor_df: DataFrame, stream_type: str, nyquist_window=10, use_filter=True,
                           use_only_filter=False, window_freq: str='1min', constrain_analyzer: bool = False,
                           max_oversampling: float = 1., sampling_rate: Union[float, str] = 4., only_necessary=True) -> Optional[Dict[str, Dict[str, Dict[str, float]]]]:
    filtered_df = sensor_df[[stream_type]].copy()
    if filtered_df[stream_type].isnull().all():
        return None
    filtered_df.rename(columns={f"{stream_type}": "value"}, inplace=True)
    filtered_df[["value"]] = filtered_df[["value"]].astype(float)
    samples = get_single_sensor_sampling_stats(sensor_df=sensor_df, value_type=stream_type,
                                               nyquist_window=nyquist_window, use_filter=use_filter,
                                               use_only_filter=use_only_filter, constrain_analyzer=constrain_analyzer,
                                               max_oversampling=max_oversampling, initial_interval=sampling_rate,
                                               further_interpolate=True)
    results = {"baseline": None, 'fixed': None, 'chameleon': None}
    if only_necessary:
        results.pop("baseline")
    for result_name in results.keys():
        results[result_name] = calculate_agg_accuracy_windowed(filtered_df,
                                                               samples[result_name + "_df"],
                                                               query_window=window_freq,
                                                               sampler_rate=sampling_rate)
        results[result_name]["samples"] = samples[result_name]
    return results


def get_single_sensor_sampling_stats(sensor_df: DataFrame, value_type: str, nyquist_window=10, use_filter=True,
                                     use_only_filter=False, constrain_analyzer: bool = False, max_oversampling: float = 1.,
                                     initial_interval=4, further_interpolate=True,
                                     only_necessary=True) -> Optional[Dict[str, Union[int, DataFrame]]]:
    filtered_df = sensor_df[[value_type]].copy()
    if filtered_df[value_type].isnull().all():
        return None
    filtered_df.rename(columns={f"{value_type}": "value"}, inplace=True)
    filtered_df[["value"]] = filtered_df[["value"]].astype(float)
    if further_interpolate:
        sensor_interpolated = interpolate_df(sensor_df=filtered_df)
    else:
        sensor_interpolated = filtered_df
    if len(filtered_df) > len(sensor_interpolated):
        sensor_interpolated = filtered_df
    logger.debug(f"Interpolated length {len(sensor_interpolated)}...")
    strategies = init_filters(initial_interval=1000 * initial_interval, nyquist_window=nyquist_window,
                              use_filter=use_filter, use_only_filter=use_only_filter,
                              constrain_analyzer=constrain_analyzer, max_oversampling=max_oversampling)
    strategies.pop("fused")
    logger.debug("Getting sampling stats for fixed interval...")
    fixed_df = gather_values_from_df(original_df=sensor_interpolated, initial_interval=initial_interval, strategy=None)
    logger.debug("Getting sampling stats for chameleon...")
    chameleon_df = gather_values_from_df(original_df=sensor_interpolated, initial_interval=initial_interval,
                                         strategy=strategies['chameleon'],
                                         gathering_interval_range=2 * initial_interval)
    if only_necessary:
        return {"fixed": len(fixed_df), "fixed_df": fixed_df,
                "chameleon": len(chameleon_df), "chameleon_df": chameleon_df}
    logger.debug("Getting sampling stats for baseline...")
    baseline_df = gather_values_from_df(original_df=sensor_interpolated, initial_interval=initial_interval,
                                        strategy=strategies['baseline'],
                                        gathering_interval_range=2 * initial_interval, artificial_cuttoff=True)
    return {"fixed": len(fixed_df),
            "fixed_df": fixed_df,
            "baseline": len(baseline_df),
            "baseline_df": baseline_df,
            "chameleon": len(chameleon_df),
            "chameleon_df": chameleon_df}


def get_sampling_stats(sensor_files_path: Path, per_sensor_df: List[DataFrame],
                       column_names: List[str], task_pool: Pool, short_uuid: str, nyquist_window: int=10, use_filter: bool=True,
                       use_only_filter=False, constrain_analyzer: bool = False, max_oversampling: float = 1.,
                       sampling_rate: float = 4, get_avg_stats: bool=False) -> Dict[str, Dict[str, float]]:
    results = {}
    for column in column_names:
        logger.info(f"Sampling stats on {column} streams...")
        overall_stats = {"fixed": 0, "baseline": 0, "chameleon": 0}
        per_strategy_sizes = {k: [] for k in overall_stats.keys()}
        param_list = [(sensor_df, column, nyquist_window, use_filter, use_only_filter, constrain_analyzer,
                       max_oversampling, sampling_rate) for sensor_df in per_sensor_df]
        promises = [task_pool.apply_async(get_single_sensor_sampling_stats, (i)) for i in param_list]
        sensor_results = [res.get() for res in promises]
        clean_sensor_results = [sensor_res for sensor_res in sensor_results if sensor_res is not None]
        for sensor_result in clean_sensor_results:
            allowed_items = {k: v for k, v in sensor_result.items() if not k.endswith('_df')}
            for key in allowed_items.keys():
                overall_stats[key] += allowed_items[key]
                per_strategy_sizes[key].append(allowed_items[key])
        results[column] = {**overall_stats}
        if get_avg_stats:
            results[column]["stats"] = {}
            detailed_stats = {"nyquist_window": nyquist_window,
                              "sampling_rate": sampling_rate,
                              "mean_size": np.mean(per_strategy_sizes["chameleon"]),
                              "median_size": np.median(per_strategy_sizes["chameleon"]),
                              "sensor_num": len(clean_sensor_results)}
            results[column]["stats"] = {**detailed_stats}
        logger.info(f"Sampling on {column} done!")
    file_name = f"sampling-results-range-sensorscope-{short_uuid}-{datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
    with open(sensor_files_path.joinpath(file_name), 'w') as fp:
        json.dump(results, fp, indent=4)
    return results


class SensorscopeIOConfiguration(Enum):
    RAW_FILE = 'meteo'
    SMALL_RAW_FILE = 'meteo'
    METRIC_NAME = 'meteo'
    ALTERNATIVE_METRIC_NAME = 'monitoring'
    RAW_FILE_DOWNLOADED_NAME = 'Sensorscope.zip'
    RESULTS_DIR = 'sensorscope'
    ORIGIN = 'https://zenodo.org/record/2654726/files/Sensorscope.zip?download=1'


def prepare_parser(io_conf: IOConfiguration) -> argparse.ArgumentParser:
    cli_parser = argparse.ArgumentParser()
    cli_parser.add_argument("--sensorscope_path", help="The root for the whole experiment",
                            default=io_conf.base_path, type=str)
    cli_parser.add_argument("--log_level", help="The log_level as string", type=str, default="info", required=False)
    cli_parser.add_argument("--download", help="Download file", action='store_true')
    cli_parser.set_defaults(download=False)
    cli_parser.add_argument("--plot_fft", help="Prepare FFT over/undersampling plot", action='store_true')
    cli_parser.set_defaults(plot_fft=False)
    cli_parser.add_argument("--sampling", help="Sampling stats", action='store_true')
    cli_parser.set_defaults(sampling=False)
    cli_parser.add_argument("--nyquist_window_size", help="Set nyquist window size", type=int, default=10,
                            required=False)
    cli_parser.add_argument("--query_window_size", help="Set query window size", type=str, default='1min',
                            required=False)
    cli_parser.add_argument("--constrain_analyzer", help="Constrain oversampling analyzer", action='store_true')
    cli_parser.set_defaults(constrain_analyzer=False)
    cli_parser.add_argument("--max_oversampling", help="Set max oversampling ratio", type=float, default=1.,
                            required=False)
    cli_parser.add_argument("--sampling_rate", help="Set starting sampling rate (s)", type=float, default=4.,
                            required=False)
    cli_parser.add_argument("--use_nyquist_only", help="Use only nyquist", action='store_true')
    cli_parser.set_defaults(use_nyquist_only=False)
    cli_parser.add_argument("--use_only_filter", help="Use only filter", action='store_true')
    cli_parser.set_defaults(use_only_filter=False)
    cli_parser.add_argument("--query_similarity", help="Bench accuracy on agg. queries", action='store_true')
    cli_parser.set_defaults(query_similarity=False)
    cli_parser.add_argument("--scalability", help="Bench accuracy on agg. queries", action='store_true')
    cli_parser.set_defaults(scalability=False)
    cli_parser.add_argument("--print_stats", help="Print raw data stats", action='store_true')
    cli_parser.set_defaults(print_stats=False)
    cli_parser.add_argument("--data_name", help="The data to parse", type=str, default="meteo", required=False)
    return cli_parser


def get_parser_args(populated_parser: argparse.ArgumentParser) -> Dict[str, Any]:
    args = populated_parser.parse_args()
    results = {"base_path_str": args.sensorscope_path,
               "log_level": args.log_level,
               "download": args.download,
               "plot_fft": args.plot_fft,
               "sampling": args.sampling,
               "nyquist_window_size": args.nyquist_window_size,
               "query_window_size": args.query_window_size,
               "constrain_analyzer": args.constrain_analyzer,
               "max_oversampling": args.max_oversampling,
               "sampling_rate": args.sampling_rate,
               "use_filter": not args.use_nyquist_only,
               "use_only_filter": args.use_only_filter,
               "query_similarity": args.query_similarity,
               "scalability": args.scalability,
               "print_stats": args.print_stats,
               "data_name": args.data_name}
    return results


if __name__ == '__main__':
    # prepare default IO conf
    io_conf = IOConfiguration(raw_file=SensorscopeIOConfiguration.RAW_FILE.value,
                              small_file=SensorscopeIOConfiguration.SMALL_RAW_FILE.value,
                              raw_file_download_name=SensorscopeIOConfiguration.RAW_FILE_DOWNLOADED_NAME.value,
                              results_dir=SensorscopeIOConfiguration.RESULTS_DIR.value,
                              base_path="/home/digiou/test-results",
                              origin=SensorscopeIOConfiguration.ORIGIN.value)
    # parse CLI parameters
    parsed_args = get_parser_args(prepare_parser(io_conf))

    # create logger
    logger = prepare_logger(parsed_args["log_level"])

    # create UUID for run
    current_uuid = shortuuid.uuid()

    # store any experiment files
    sensor_files_path = Path(parsed_args["base_path_str"]).joinpath(io_conf.results_dir)

    # create df on all sensors data
    all_dfs = get_all_dataframes(sensor_files_path, data_name=parsed_args["data_name"])

    cols = ['ambient_temp', "surface_temp", "solar_radiation",
            "soil_moisture", "rain_meter", "wind_speed", "wind_direction", "rel_humidity"]

    with Pool(processes=get_physical_cores()) as task_pool:
        pd.options.mode.chained_assignment = None  # default='warn'
        if parsed_args["print_stats"]:
            for col in cols:
                results = compute_detailed_stats(dataframes=all_dfs, col_name=col)
                mean_dur_str = closest_time_unit(results["mean_duration"])
                print(f"{col}: meandur: {mean_dur_str}, stats: {results}")
        if parsed_args["plot_fft"]:
            for col_name in cols:
                plot_fft(all_dfs, task_pool, metric=col_name)

        if parsed_args["sampling"]:
            logger.info(f"We have {len(all_dfs)} dataframes")
            get_sampling_stats(sensor_files_path=sensor_files_path, per_sensor_df=all_dfs,
                               column_names=cols, task_pool=task_pool, short_uuid=current_uuid,
                               nyquist_window=parsed_args["nyquist_window_size"],
                               use_filter=parsed_args["use_filter"],
                               use_only_filter=parsed_args["use_only_filter"],
                               constrain_analyzer=parsed_args["constrain_analyzer"],
                               max_oversampling=parsed_args["max_oversampling"],
                               sampling_rate=parsed_args["sampling_rate"])
            logger.info("Sampling done!")
            biggest_df = all_dfs[0]
            for parsed_df in all_dfs:
                if len(parsed_df) > len(biggest_df):
                    biggest_df = parsed_df
                if parsed_df.ambient_temp.isnull().all():
                    logger.info(f"We found a fully null ambient_temp in {parsed_df.index.name}")
                    break
                if parsed_df.ambient_temp.isnull().any():
                    logger.info(f"We found a df with null ambient_temp in {parsed_df.index.name}")
                    break
            logger.info(f"Biggest df is {biggest_df.index.name} with {len(biggest_df)} samples")

        if parsed_args["query_similarity"]:
            measure_accuracy(sensor_dfs=all_dfs, col_names=cols, sensor_files_path=sensor_files_path,
                             current_uuid=current_uuid, task_pool=task_pool,
                             nyquist_window=parsed_args["nyquist_window_size"],
                             query_window=parsed_args["query_window_size"],
                             use_filter=parsed_args["use_filter"],
                             use_only_filter=parsed_args["use_only_filter"],
                             constrain_analyzer=parsed_args["constrain_analyzer"],
                             max_oversampling=parsed_args["max_oversampling"],
                             sampling_rate=parsed_args["sampling_rate"])
        if parsed_args["scalability"]:
            get_sampling_stats(sensor_files_path=sensor_files_path, per_sensor_df=all_dfs,
                               column_names=cols, task_pool=task_pool, short_uuid=current_uuid,
                               nyquist_window=parsed_args["nyquist_window_size"],
                               use_filter=parsed_args["use_filter"],
                               use_only_filter=parsed_args["use_only_filter"],
                               constrain_analyzer=parsed_args["constrain_analyzer"],
                               max_oversampling=parsed_args["max_oversampling"],
                               sampling_rate=parsed_args["sampling_rate"],
                               get_avg_stats=True)
            measure_scalability(sensor_files_path=sensor_files_path)
    logger.info("Done!")
