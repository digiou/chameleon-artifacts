import argparse
import datetime
import json
import os
from enum import Enum
import glob
from multiprocessing import Pool
from pathlib import Path
from typing import Dict, Any, List, Union, Optional

import shortuuid
import numpy as np
import pandas as pd
from pandas import DataFrame
from wfdb.io import rdrecord

from experiments.io.files import IOConfiguration
from experiments.io.experiment_logger import prepare_logger
from experiments.io.resource_counters import get_physical_cores
from experiments.models.kalman_filters import init_filters
from experiments.sensor_functions.accuracy import calculate_agg_accuracy_windowed, measure_accuracy_fn
from experiments.sensor_functions.fft import compute_nyquist_and_energy
from experiments.sensor_functions.gathering import gather_values_from_df
from experiments.sensor_functions.interpolation import interpolate_df
from experiments.emulator.scalability import measure_scalability


class ArrhythmiaIOConfiguration(Enum):
    RAW_FILE = 'mit-bih-supraventricular-arrhythmia-database-1.0.0'
    SMALL_RAW_FILE = 'arrhythmia-database'
    RAW_FILE_DOWNLOADED_NAME = 'mit-bih-supraventricular-arrhythmia-database-1.0.0.zip'
    RESULTS_DIR = 'arrhythmia'
    ORIGIN = 'https://physionet.org/static/published-projects/svdb/mit-bih-supraventricular-arrhythmia-database-1.0.0.zip'


def plot_fft(sensor_dfs: List[DataFrame], task_pool: Pool):
    logger.info(f"Computing FFT and energy levels per sensor...")
    oversampled = 0
    zero_energy = 0
    oversampling_ratio = 0.
    promises = [task_pool.apply_async(compute_nyquist_and_energy, (sensor_df,)) for sensor_df in sensor_dfs]
    nyquist_results = [res.get() for res in promises]
    for nyquist_result in nyquist_results:
        if nyquist_result[0]:
            oversampled += 1
            oversampling_ratio += nyquist_result[2]
        if nyquist_result[1] == 0.0:
            zero_energy += 1
    oversampling_ratio /= oversampled
    logger.info(
        f"Total streams: {len(sensor_dfs)}, oversampled total: {oversampled}, oversampling ratio: {oversampling_ratio}, zero energy: {zero_energy}")
    logger.info(f"Done computing FFTs!")


def calculate_agg_accuracy(sensor_df: DataFrame, stream_type: str, nyquist_window=10, use_filter=True,
                           window_freq: str='1min', sampling_rate: Union[float, str] = 4., only_necessary=True) -> Optional[Dict[str, Dict[str, Dict[str, float]]]]:
    original_sampler_rate = sampling_rate
    single_df = sensor_df[[stream_type]]
    single_df.rename(columns={f"{stream_type}": "value"}, inplace=True, errors='ignore')
    interpolated_df = interpolate_df(sensor_df=sensor_df)
    samples = get_single_sensor_sampling_stats(sensor_df=interpolated_df,
                                               nyquist_window=nyquist_window, use_filter=use_filter,
                                               initial_interval=sampling_rate, further_interpolate=False)
    results = {"baseline": None, 'fixed': None, 'chameleon': None}
    if only_necessary:
        results.pop("baseline")
    for result_name in results.keys():
        results[result_name] = calculate_agg_accuracy_windowed(single_df,
                                                               samples[result_name + "_df"],
                                                               window_freq=window_freq,
                                                               sampler_rate=original_sampler_rate)
    return results


def get_single_sensor_sampling_stats(sensor_df: DataFrame, nyquist_window=10, use_filter=True,
                                     use_only_filter=False, constrain_analyzer: bool = False, max_oversampling: float = 1.,
                                     initial_interval=4, further_interpolate=True,
                                     only_necessary=True) -> Dict[str, Union[int, DataFrame]]:
    logger.debug("Preparing sensor_df for processing...")
    if further_interpolate:
        sensor_interpolated = interpolate_df(sensor_df=sensor_df)
    else:
        sensor_interpolated = sensor_df
    if len(sensor_df) > len(sensor_interpolated):
        sensor_interpolated = sensor_df
    logger.debug(f"Interpolated length {len(sensor_interpolated)}...")
    strategies = init_filters(initial_interval=1000 * initial_interval, nyquist_window=nyquist_window, use_filter=use_filter,
                              use_only_filter=use_only_filter, constrain_analyzer=constrain_analyzer,
                              max_oversampling=max_oversampling)
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
                       task_pool: Pool, short_uuid: str, nyquist_window: int=10,
                       use_only_filter=False, constrain_analyzer: bool = False, max_oversampling: float = 1.,
                       use_filter: bool=True, get_avg_stats=False) -> Dict[str, Dict[str, float]]:
    results = {}
    logger.info(f"Sampling stats on arrhythmia ECG streams...")
    overall_stats = {"fixed": 0, "baseline": 0, "chameleon": 0}
    per_strategy_sizes = {k: [] for k in overall_stats.keys()}
    param_list = [(sensor_df, nyquist_window, use_filter, use_only_filter, constrain_analyzer, max_oversampling) for sensor_df in per_sensor_df]
    promises = [task_pool.apply_async(get_single_sensor_sampling_stats, (i)) for i in param_list]
    sensor_results = [res.get() for res in promises]
    for sensor_result in sensor_results:
        allowed_items = {k: v for k, v in sensor_result.items() if not k.endswith('_df')}
        for key in allowed_items.keys():
            overall_stats[key] += allowed_items[key]
            per_strategy_sizes[key].append(allowed_items[key])
    results['ecg'] = {**overall_stats}
    if get_avg_stats:
        results['ecg']["stats"] = {}
        detailed_stats = {"nyquist_window": nyquist_window,
                          "mean_size": np.mean(per_strategy_sizes["chameleon"]),
                          "median_size": np.median(per_strategy_sizes["chameleon"]),
                          "sensor_num": len(per_sensor_df)}
        results['ecg']["stats"] = {**detailed_stats}
    file_name = f"sampling-results-range-arrhythmia-{short_uuid}-{datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
    with open(sensor_files_path.joinpath(file_name), 'w') as fp:
        json.dump(results, fp, indent=4)
    return results


def get_all_dataframes(root_path: Path) -> List[DataFrame]:
    logger.info("Loading main csv from disk...")
    data_frames = []
    avg_len = 0.
    dat_files = glob.glob(str(root_path) + os.sep + "*.hea")
    for dat_file in dat_files:
        file_no_extension = os.path.splitext(dat_file)[0]
        records = rdrecord(file_no_extension)
        records.base_datetime = datetime.datetime.now()
        records_df = records.to_dataframe()
        for sub_df_col in ['ECG1', 'ECG2']:
            sub_df = records_df[[sub_df_col]]
            sub_df.rename(columns={sub_df_col: 'value'}, inplace=True)
            avg_len += len(sub_df)
            data_frames.append(sub_df)
    avg_len /= len(data_frames)
    logger.info(f"Average DF size: {avg_len}")
    logger.info("Done indexing main dataframe!")
    return data_frames


def prepare_parser(io_conf: IOConfiguration) -> argparse.ArgumentParser:
    cli_parser = argparse.ArgumentParser()
    cli_parser.add_argument("--arrhythmia_path", help="The root for the whole experiment",
                            default=io_conf.base_path, type=str)
    cli_parser.add_argument("--log_level", help="The log_level as string", type=str, default="info", required=False)
    cli_parser.add_argument("--download", help="Download file", action='store_true')
    cli_parser.set_defaults(download=False)
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
    cli_parser.add_argument("--use_nyquist_only", help="Use only nyquist", action='store_true')
    cli_parser.set_defaults(use_nyquist_only=False)
    cli_parser.add_argument("--use_only_filter", help="Use only filter", action='store_true')
    cli_parser.set_defaults(use_only_filter=False)
    cli_parser.add_argument("--query_similarity", help="Bench accuracy on agg. queries", action='store_true')
    cli_parser.set_defaults(query_similarity=False)
    cli_parser.add_argument("--scalability", help="Bench accuracy on agg. queries", action='store_true')
    cli_parser.set_defaults(scalability=False)
    return cli_parser


def get_parser_args(populated_parser: argparse.ArgumentParser) -> Dict[str, Any]:
    args = populated_parser.parse_args()
    results = {"base_path_str": args.arrhythmia_path,
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
               "scalability": args.scalability,}
    return results


if __name__ == '__main__':
    # prepare default IO conf
    io_conf = IOConfiguration(raw_file=ArrhythmiaIOConfiguration.RAW_FILE.value,
                              small_file=ArrhythmiaIOConfiguration.SMALL_RAW_FILE.value,
                              raw_file_download_name=ArrhythmiaIOConfiguration.RAW_FILE_DOWNLOADED_NAME.value,
                              results_dir=ArrhythmiaIOConfiguration.RESULTS_DIR.value,
                              base_path="/home/digiou/test-results/",
                              origin=ArrhythmiaIOConfiguration.ORIGIN.value)
    # parse CLI parameters
    parsed_args = get_parser_args(prepare_parser(io_conf))

    # create logger
    logger = prepare_logger()

    # create UUID for run
    current_uuid = shortuuid.uuid()

    # store any experiment files
    sensor_files_path = Path(parsed_args["base_path_str"]).joinpath(io_conf.results_dir).joinpath(io_conf.raw_file)

    # create dfs from all data
    sensor_dfs = get_all_dataframes(root_path=sensor_files_path)

    with Pool(processes=get_physical_cores()) as task_pool:
        pd.options.mode.chained_assignment = None  # default='warn'
        if parsed_args["plot_fft"]:
            plot_fft(sensor_dfs, task_pool)

        if parsed_args["sampling"]:
            get_sampling_stats(sensor_files_path=sensor_files_path, per_sensor_df=sensor_dfs,
                               task_pool=task_pool, short_uuid=current_uuid,
                               nyquist_window=parsed_args["nyquist_window_size"],
                               use_filter=parsed_args["use_filter"],
                               use_only_filter=parsed_args["use_only_filter"],
                               constrain_analyzer=parsed_args["constrain_analyzer"],
                               max_oversampling=parsed_args["max_oversampling"])

        if parsed_args["query_similarity"]:
            measure_accuracy_fn(sensor_dfs=sensor_dfs, col_names=['value'], sensor_files_path=sensor_files_path,
                                current_uuid=current_uuid, task_pool=task_pool, calculate_agg_accuracy=calculate_agg_accuracy,
                                dataset="arrhythmia", nyquist_window=parsed_args["nyquist_window_size"],
                                use_filter=parsed_args["use_filter"])
        if parsed_args["scalability"]:
            get_sampling_stats(sensor_files_path=sensor_files_path, per_sensor_df=sensor_dfs,
                               task_pool=task_pool, short_uuid=current_uuid,
                               nyquist_window=parsed_args["nyquist_window_size"],
                               use_filter=parsed_args["use_filter"],
                               use_only_filter=parsed_args["use_only_filter"],
                               constrain_analyzer=parsed_args["constrain_analyzer"],
                               max_oversampling=parsed_args["max_oversampling"],
                               get_avg_stats=True)
            measure_scalability(sensor_files_path=sensor_files_path)
    logger.info("Done!")
