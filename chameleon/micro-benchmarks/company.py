import argparse
import datetime
import glob
import json

from collections import Counter
from enum import Enum
from pathlib import Path
from multiprocessing import Pool
from typing import Dict, Any, List, Union, Optional

import shortuuid

import numpy as np
import pandas as pd
from pandas import DataFrame

from experiments.io.dataset_stats import compute_detailed_stats
from experiments.io.experiment_logger import prepare_logger
from experiments.io.files import IOConfiguration
from experiments.io.resource_counters import get_physical_cores
from experiments.models.kalman_filters import init_filters
from experiments.io.serialization import NpEncoder
from experiments.plotting.formatting import closest_time_unit
from experiments.sensor_functions.accuracy import calculate_agg_accuracy_windowed
from experiments.sensor_functions.fft import compute_nyquist_and_energy
from experiments.sensor_functions.interpolation import interpolate_df
from experiments.sensor_functions.gathering import gather_values_from_df
from experiments.emulator.scalability import measure_scalability


class companyIOConfiguration(Enum):
    RAW_FILE = '0917-619-54.csv'
    SMALL_RAW_FILE = '0917-619-30.csv'
    RAW_FILE_DOWNLOADED_NAME = 'canbus-with-gps.zip'
    RESULTS_DIR = 'company-by-sensor'
    ORIGIN = ''  # private dataset unfortunately


def measure_accuracy(sensor_dfs: List[DataFrame], col_names: List[str], sensor_files_path: Path,
                     current_uuid: str, task_pool: Pool, nyquist_window=10, query_window='1min',
                     use_filter=True, use_only_filter=False,
                     constrain_analyzer: bool = False, max_oversampling: float = 1., sampling_rate: float = 4.,
                     only_necessary: bool = True):
    total_results = {}
    strategies = ["fixed", "chameleon"] if only_necessary else ["baseline", "fixed", "chameleon"]
    file_name = f"qs-company-{current_uuid}-{datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
    with open(sensor_files_path.joinpath(file_name), 'w', encoding='utf-8') as fp:
        for stream_type in col_names:
            mape_accuracy = Counter({"sum": 0., "count": 0., "mean": 0., "median": 0., "max": 0., "min": 0., "std": 0., "var": 0.})
            mape_accuracies = {k: mape_accuracy.copy() for k in strategies}
            max_accuracies = {k: mape_accuracy.copy() for k in strategies}
            samples = {k: 0 for k in strategies}
            promises = [task_pool.apply_async(calculate_agg_accuracy,
                        (raw_sensor, stream_type, nyquist_window, use_filter, use_only_filter, query_window, constrain_analyzer, max_oversampling, sampling_rate,))
                        for raw_sensor in sensor_dfs]
            aggregation_results = [res.get() for res in promises]
            for result in filter(None, aggregation_results):
                for approach, stats in result.items():
                    mape = Counter(stats["mape"])
                    mape_accuracies[approach] += mape
                    samples[approach] += stats["samples"]
                    max = max_accuracies[approach] | Counter(stats["max"])
                    for key, max_val in max.items():
                        max_accuracies[approach][key] = max_val
            logger.info(f"Stream: {stream_type}, len: {len(aggregation_results)}")
            total_results[stream_type] = \
                {"accuracy": dict(mape_accuracies), "samples": dict(samples),
                 "nyq_size": nyquist_window, "q_window": query_window, "sampling_rate": sampling_rate}
        json.dump(total_results, fp, ensure_ascii=False, indent=4, cls=NpEncoder)
        logger.info(f"Wrote data in {sensor_files_path.joinpath(file_name)}")


def calculate_agg_accuracy(sensor_df: DataFrame, stream_type: str, nyquist_window=10, use_filter=True,
                           use_only_filter=False, window_freq: str='1min', constrain_analyzer: bool = False,
                           max_oversampling: float = 1., sampling_rate: Union[float, str] = 4., only_necessary=True) -> Optional[Dict[str, Dict[str, Dict[str, float]]]]:
    filtered_df = sensor_df[[stream_type]].copy()
    filtered_df.rename(columns={f"{stream_type}": "value"}, inplace=True)
    interpolated_df = interpolate_df(sensor_df=filtered_df)
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


def plot_fft(sensor_df_list: List[DataFrame], task_pool: Pool, metric='Dist'):
    metric_dfs = []
    for single_df in sensor_df_list:
        sensor_df = single_df[[metric]]
        sensor_df.dropna(subset=[metric], inplace=True)
        sensor_df.rename(columns={f"{metric}": "value"}, inplace=True)
        metric_dfs.append(sensor_df)
    promises = [task_pool.apply_async(compute_nyquist_and_energy, (sensor_df,)) for sensor_df in metric_dfs if
                len(sensor_df.index) > 2]
    nyquist_results = [res.get() for res in promises]
    oversampled = 0
    zero_energy = 0
    oversampling_ratio = 0.
    for nyquist_result in nyquist_results:
        if nyquist_result[0]:
            oversampled += 1
            oversampling_ratio += nyquist_result[2]
        if nyquist_result[1] == 0.0:
            zero_energy += 1
    oversampling_ratio /= oversampled
    logger.info(
        f"Total streams for {metric}: {len(sensor_df_list)}, oversampled total: {oversampled}, oversampling ratio: {oversampling_ratio}")
    logger.debug(f"Done computing FFTs!")


def get_all_dataframes(path: Path) -> List[DataFrame]:
    all_dfs = []
    useful_cols = ["Time", "Dist", "ABS_Front_Wheel_Press", "ABS_Rear_Wheel_Press",
                   "ABS_Front_Wheel_Speed", "ABS_Rear_Wheel_Speed", "V_GPS", "MMDD",
                   "HHMM", "LAS_Ax1", "LAS_Ay1", "LAS_Az_Vertical_Acc", "ABS_Lean_Angle",
                   "ABS_Pitch_Info", "ECU_Gear_Position", "ECU_Accel_Position",
                   "ECU_Engine_Rpm", "ECU_Water_Temperature", "ECU_Oil_Temp_Sensor_Data",
                   "ECU_Side_StanD", "Longitude", "Latitude", "Altitude"]
    read_csv_options = {'encoding': 'utf-8-sig', 'header': None, "names": useful_cols,
                        "sep": ";", "decimal": ",", "skiprows": range(0, 2)}
    search_str = str(path.joinpath("**").joinpath(f"*.csv"))
    now_dt = datetime.datetime(2023, 7, 6, 15, 30, 45, 123000)
    avg_len = 0.
    for filename in glob.glob(search_str, recursive=True):
        logger.debug(f"Loading {filename}...")
        read_csv_options['filepath_or_buffer'] = str(filename)
        raw_dataframe = pd.read_csv(**read_csv_options)
        raw_dataframe['Time'] = raw_dataframe['Time'].apply(lambda x: now_dt + pd.to_timedelta(x, unit='s'))
        raw_dataframe.index = pd.to_datetime(raw_dataframe['Time'], unit='s')
        raw_dataframe.drop('Time', axis=1, inplace=True)
        avg_len += len(raw_dataframe)
        all_dfs.append(raw_dataframe)
        logger.debug("Loaded csv!")
    avg_len /= len(all_dfs)
    logger.info(f"Average DF size: {avg_len}")
    return all_dfs


def get_single_sensor_sampling_stats(sensor_df: DataFrame, value_type='value', nyquist_window=10, use_filter=True,
                                     use_only_filter=False, constrain_analyzer: bool = False, max_oversampling: float = 1.,
                                     initial_interval=4, further_interpolate=True, only_necessary=True) -> \
        Dict[str, Union[int, DataFrame]]:
    logger.debug("Preparing sensor_df for processing...")
    filtered_df = sensor_df[[value_type]].copy()
    filtered_df.rename(columns={f"{value_type}": "value"}, inplace=True, errors='ignore')
    filtered_df[["value"]] = filtered_df[["value"]].astype(float)
    if further_interpolate:
        sensor_interpolated = interpolate_df(sensor_df=filtered_df)
    else:
        sensor_interpolated = sensor_df
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
                       column_names: List[str], task_pool: Pool, short_uuid: str, nyquist_window: int=10,
                       use_filter: bool=True, use_only_filter=False, constrain_analyzer: bool = False,
                       max_oversampling: float = 1., sampling_rate: float = 4., get_avg_stats: bool=False) -> Dict[str, Dict[str, float]]:
    results = {}
    for column in column_names:
        logger.info(f"Sampling stats on {column} streams...")
        overall_stats = {"fixed": 0, "baseline": 0, "chameleon": 0}
        per_strategy_sizes = {k: [] for k in overall_stats.keys()}
        param_list = [(sensor_df, column, nyquist_window, use_filter, use_only_filter, constrain_analyzer,
                       max_oversampling, sampling_rate)
                      for sensor_df in per_sensor_df]
        promises = [task_pool.apply_async(get_single_sensor_sampling_stats, (i)) for i in param_list]
        sensor_results = [res.get() for res in promises]
        for sensor_result in sensor_results:
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
                              "sensor_num": len(per_sensor_df)}
            results[column]["stats"] = {**detailed_stats}
    file_name = f"sampling-results-range-company-{short_uuid}-{datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
    with open(sensor_files_path.joinpath(file_name), 'w') as fp:
        json.dump(results, fp, indent=4)
    return results


def prepare_parser(io_conf: IOConfiguration) -> argparse.ArgumentParser:
    cli_parser = argparse.ArgumentParser()
    cli_parser.add_argument("--company_path", help="The root for the whole experiment",
                            default=io_conf.base_path, type=str)
    cli_parser.add_argument("--log_level", help="The log_level as string", type=str, default="info", required=False)
    cli_parser.add_argument("--plot_fft", help="Prepare FFT over/undersampling plot", action='store_true')
    cli_parser.set_defaults(plot_fft=False)
    cli_parser.add_argument("--sampling", help="Skip sampling stats", action='store_true')
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
    cli_parser.add_argument("--calc_pareto", help="Calculate Pareto front", action='store_true')
    cli_parser.set_defaults(calc_pareto=False)
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
    return cli_parser


def get_parser_args(populated_parser: argparse.ArgumentParser) -> Dict[str, Any]:
    args = populated_parser.parse_args()
    results = {"base_path_str": args.company_path,
               "log_level": args.log_level,
               "plot_fft": args.plot_fft,
               "sampling": args.sampling,
               "nyquist_window_size": args.nyquist_window_size,
               "query_window_size": args.query_window_size,
               "constrain_analyzer": args.constrain_analyzer,
               "max_oversampling": args.max_oversampling,
               "sampling_rate": args.sampling_rate,
               "calc_pareto": args.calc_pareto,
               "use_filter": not args.use_nyquist_only,
               "use_only_filter": args.use_only_filter,
               "query_similarity": args.query_similarity,
               "scalability": args.scalability,
               "print_stats": args.print_stats}
    return results


if __name__ == '__main__':
    # prepare default IO conf
    io_conf = IOConfiguration(raw_file=companyIOConfiguration.RAW_FILE.value,
                              small_file=companyIOConfiguration.SMALL_RAW_FILE.value,
                              raw_file_download_name=companyIOConfiguration.RAW_FILE_DOWNLOADED_NAME.value,
                              results_dir=companyIOConfiguration.RESULTS_DIR.value,
                              base_path="/home/dimitrios/projects/test-results",
                              origin=companyIOConfiguration.ORIGIN.value)
    # parse CLI parameters
    parsed_args = get_parser_args(prepare_parser(io_conf))

    # create logger
    logger = prepare_logger(parsed_args["log_level"])

    # create UUID for run
    current_uuid = shortuuid.uuid()

    # store any experiment files
    sensor_files_path = Path(parsed_args["base_path_str"]).joinpath(io_conf.results_dir)

    # create df on whole data
    all_dfs = get_all_dataframes(sensor_files_path.joinpath(sensor_files_path))

    cols = ['Dist', "ABS_Front_Wheel_Press", "ABS_Rear_Wheel_Press", "ABS_Front_Wheel_Speed",
            "ABS_Rear_Wheel_Speed", "ABS_Lean_Angle", "ECU_Water_Temperature", "ECU_Oil_Temp_Sensor_Data",
            "Longitude"]

    with Pool(processes=get_physical_cores()) as task_pool:
        pd.options.mode.chained_assignment = None  # default='warn'
        if parsed_args["print_stats"]:
            for col in cols:
                results = compute_detailed_stats(dataframes=all_dfs, col_name=col)
                mean_dur_str = closest_time_unit(results["mean_duration"])
                print(f"{col}: meandur: {mean_dur_str}, stats: {results}")
        if parsed_args["plot_fft"]:
            for col in cols:
                plot_fft(sensor_df_list=all_dfs, task_pool=task_pool, metric=col)

        if parsed_args["sampling"]:
            get_sampling_stats(sensor_files_path=sensor_files_path, per_sensor_df=all_dfs,
                               column_names=cols, task_pool=task_pool, short_uuid=current_uuid,
                               nyquist_window=parsed_args["nyquist_window_size"],
                               use_filter=parsed_args["use_filter"],
                               use_only_filter=parsed_args["use_only_filter"],
                               constrain_analyzer=parsed_args["constrain_analyzer"],
                               max_oversampling=parsed_args["max_oversampling"])

        # check agg. between raw and sampled data
        if parsed_args["query_similarity"]:
            measure_accuracy(sensor_dfs=all_dfs, col_names=cols, sensor_files_path=sensor_files_path,
                             current_uuid=current_uuid,task_pool=task_pool,
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
