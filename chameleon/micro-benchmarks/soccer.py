import argparse
import datetime
import json
from collections import  Counter
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
from experiments.plotting.formatting import closest_time_unit
from experiments.sensor_functions.accuracy import calculate_agg_accuracy_windowed
from experiments.io.serialization import NpEncoder
from experiments.sensor_functions.fft import compute_nyquist_and_energy
from experiments.sensor_functions.gathering import gather_values_from_df
from experiments.sensor_functions.interpolation import interpolate_df
from experiments.emulator.scalability import measure_scalability


class SoccerIOConfiguration(Enum):
    RAW_FILE = 'full-game'
    SMALL_RAW_FILE = 'full-game-small'
    RAW_FILE_DOWNLOADED_NAME = 'full-game.gz'
    RESULTS_DIR = 'soccer'
    ORIGIN = 'http://www2.iis.fraunhofer.de/sports-analytics/full-game.gz'


def plot_fft(per_value_df_lists: Dict[str, List[DataFrame]], task_pool: Pool):
    logger.info(f"Computing FFT and energy levels per sensor...")
    for workload, sensor_df_list in per_value_df_lists.items():
        oversampled = 0
        zero_energy = 0
        oversampling_ratio = 0.
        promises = [task_pool.apply_async(compute_nyquist_and_energy, (sensor_df,)) for sensor_df in sensor_df_list]
        nyquist_results = [res.get() for res in promises]
        for nyquist_result in nyquist_results:
            if nyquist_result[0]:
                oversampled += 1
                oversampling_ratio += nyquist_result[2]
            if nyquist_result[1] == 0.0:
                zero_energy += 1
        oversampling_ratio /= oversampled
        logger.info(f"Total streams for {workload}: {len(sensor_df_list)}, oversampled total: {oversampled}, oversampling ratio: {oversampling_ratio}, zero energy: {zero_energy}")
    logger.info(f"Done computing FFTs!")


def calculate_agg_accuracy(sensor_df: DataFrame, stream_type: str, nyquist_window=10, use_filter=True,
                           use_only_filter=False, window_freq: str='1min', constrain_analyzer: bool = False,
                           max_oversampling: float = 1., sampling_rate: Union[float, str] = 4., only_necessary=True) -> Optional[Dict[str, Dict[str, Dict[str, float]]]]:
    interpolated_df = interpolate_df(sensor_df=sensor_df)
    samples = get_single_sensor_sampling_stats(sensor_df=interpolated_df,
                                               nyquist_window=nyquist_window, use_filter=use_filter,
                                               use_only_filter=use_only_filter, constrain_analyzer=constrain_analyzer,
                                               max_oversampling=max_oversampling, initial_interval=sampling_rate,
                                               further_interpolate=False)
    results = {"baseline": None, 'fixed': None, 'chameleon': None}
    if only_necessary:
        results.pop("baseline")
    for result_name in results.keys():
        results[result_name] = calculate_agg_accuracy_windowed(sensor_df,
                                                               samples[result_name + "_df"],
                                                               query_window=window_freq,
                                                               sampler_rate=sampling_rate)
        results[result_name]["samples"] = samples[result_name]
    return results


def get_single_dataframe(path: Path) -> DataFrame:
    col_names = ['sid', 'ts', 'x', 'y', 'z', 'v_abs', 'a_abs',
                 'vx', 'vy', 'vz', 'ax', 'ay', 'az']
    dtype = {col_name: np.int64 for col_name in col_names if col_name != 'ts'}
    read_csv_options = {'filepath_or_buffer': str(path), 'header': None, 'names': col_names,
                        'index_col': None, 'dtype': dtype}
    logger.info("Loading main csv from disk...")
    raw_dataframe = pd.read_csv(**read_csv_options)
    logger.debug("Loaded csv!")
    raw_dataframe['ts'] = pd.to_datetime(raw_dataframe['ts'], unit='ns', utc=True)
    raw_dataframe.set_index('ts', drop=True, inplace=True)
    logger.info("Done indexing main dataframe!")
    return raw_dataframe


def get_ball_x_dataframe(raw_dataframe: DataFrame) -> DataFrame:
    # from https://www.iis.fraunhofer.de/content/dam/iis/en/doc/lv/ok/config.txt
    ball_ids = [4]
    ball_df = raw_dataframe[raw_dataframe['sid'].isin(ball_ids)]
    ball_df.sort_index(inplace=True)
    x_val_df = ball_df[['x']].rename(columns={'x': 'value'})
    return x_val_df


def get_referee_x_dataframe(raw_dataframe: DataFrame) -> DataFrame:
    # from https://www.iis.fraunhofer.de/content/dam/iis/en/doc/lv/ok/config.txt
    referee_ids = [106]
    referee_df = raw_dataframe[raw_dataframe['sid'].isin(referee_ids)]
    referee_df.sort_index(inplace=True)
    x_val_df = referee_df[['x']].rename(columns={'x': 'value'})
    return x_val_df


def get_players_x_dataframes(raw_dataframe: DataFrame) -> List[DataFrame]:
    # from https://www.iis.fraunhofer.de/content/dam/iis/en/doc/lv/ok/config.txt
    player_dfs = []
    player_ids = [16, 88, 52, 54, 24, 58, 28,
                  64, 66, 68, 38, 40, 74, 44]  # no goalkeepers
    for player_id in player_ids:
        player_df = raw_dataframe[raw_dataframe['sid'] == player_id]
        player_df.sort_index(inplace=True)
        x_val_df = player_df[['x']].rename(columns={'x': 'value'})
        player_dfs.append(x_val_df)
    return player_dfs


def get_goalkeeper_x_dataframes(raw_dataframe: DataFrame) -> List[DataFrame]:
    # from https://www.iis.fraunhofer.de/content/dam/iis/en/doc/lv/ok/config.txt
    player_dfs = []
    player_ids = [14, 62]  # goalkeepers
    for player_id in player_ids:
        player_df = raw_dataframe[raw_dataframe['sid'] == player_id]
        player_df.sort_index(inplace=True)
        x_val_df = player_df[['x']].rename(columns={'x': 'value'})
        player_dfs.append(x_val_df)
    return player_dfs


def get_single_sensor_sampling_stats(sensor_df: DataFrame, nyquist_window: int=10, use_filter=True,
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


def get_sampling_stats(sensor_files_path: Path, per_value_df_lists: Dict[str, List[DataFrame]],
                       task_pool: Pool, short_uuid: str, nyquist_window: int=10, use_filter: bool=True,
                       use_only_filter=False, constrain_analyzer: bool = False, max_oversampling: float = 1.,
                       sampling_rate: float = 4., get_avg_stats: bool=False) -> Dict[str, Dict[str, float]]:
    results = {}
    for stream_name, stream_items in per_value_df_lists.items():
        logger.info(f"Sampling stats on {stream_name} streams...")
        overall_stats = {"fixed": 0, "baseline": 0, "chameleon": 0}
        per_strategy_sizes = {k: [] for k in overall_stats.keys()}
        param_list = [(sensor_df, nyquist_window, use_filter, use_only_filter, constrain_analyzer, max_oversampling,
                       sampling_rate) for sensor_df in stream_items]
        promises = [task_pool.apply_async(get_single_sensor_sampling_stats, (i)) for i in param_list]
        sensor_results = [res.get() for res in promises]
        for sensor_result in sensor_results:
            allowed_items = {k: v for k, v in sensor_result.items() if not k.endswith('_df')}
            for key in allowed_items.keys():
                overall_stats[key] += allowed_items[key]
                per_strategy_sizes[key].append(allowed_items[key])
        results[stream_name] = {**overall_stats}
        if get_avg_stats:
            results[stream_name]["stats"] = {}
            detailed_stats = {"nyquist_window": nyquist_window,
                              "sampling_rate": sampling_rate,
                              "mean_size": np.mean(per_strategy_sizes["chameleon"]),
                              "median_size": np.median(per_strategy_sizes["chameleon"]),
                              "sensor_num": len(stream_items)}
            results[stream_name]["stats"] = {**detailed_stats}
    file_name = f"sampling-results-range-soccer-{short_uuid}-{datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
    with open(sensor_files_path.joinpath(file_name), 'w') as fp:
        json.dump(results, fp, indent=4)
    return results


def measure_accuracy(per_value_df_lists: Dict[str, List[DataFrame]], sensor_files_path: Path,
                     current_uuid: str, task_pool: Pool, nyquist_window=10, query_window='1min',
                     use_filter=True, use_only_filter=False,
                     constrain_analyzer: bool = False, max_oversampling: float = 1., sampling_rate: float = 4.,
                     only_necessary: bool = True):
    total_results = {}
    strategies = ["fixed", "chameleon"] if only_necessary else ["baseline", "fixed", "chameleon"]
    for stream_name, stream_items in per_value_df_lists.items():
        logger.info(f"Measuring similarity on {stream_name} streams...")
        mape_accuracy = Counter({"sum": 0., "count": 0., "mean": 0., "median": 0., "max": 0., "min": 0., "std": 0., "var": 0.})
        mape_accuracies = {k: mape_accuracy.copy() for k in strategies}
        max_accuracies = {k: mape_accuracy.copy() for k in strategies}
        samples = {k: 0 for k in strategies}
        promises = [task_pool.apply_async(calculate_agg_accuracy,
                                          (raw_sensor, stream_name, nyquist_window, use_filter, use_only_filter,
                                           query_window, constrain_analyzer, max_oversampling, sampling_rate,))
                    for raw_sensor in stream_items]
        aggregation_results = [res.get() for res in promises]
        for result in filter(None, aggregation_results):
            for approach, stats in result.items():
                mape = Counter(stats["mape"])
                mape_accuracies[approach] += mape
                samples[approach] += stats["samples"]
                max = max_accuracies[approach] | Counter(stats["max"])
                for key, max_val in max.items():
                    max_accuracies[approach][key] = max_val
        logger.info(f"Stream: {stream_name}, len: {len(aggregation_results)}")
        total_results[stream_name] = \
            {"accuracy": dict(mape_accuracies), "samples": dict(samples),
             "nyq_size": nyquist_window, "q_window": query_window, "sampling_rate": sampling_rate}
    file_name = f"qs-soccer-{current_uuid}-{datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
    with open(sensor_files_path.joinpath(file_name), 'w', encoding='utf-8') as fp:
        json.dump(total_results, fp, ensure_ascii=False, indent=4, cls=NpEncoder)
    logger.info(f"Wrote data in {sensor_files_path.joinpath(file_name)}")


def prepare_parser(io_conf: IOConfiguration) -> argparse.ArgumentParser:
    cli_parser = argparse.ArgumentParser()
    cli_parser.add_argument("--soccer_path", help="The root for the whole experiment",
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
    return cli_parser


def get_parser_args(populated_parser: argparse.ArgumentParser) -> Dict[str, Any]:
    args = populated_parser.parse_args()
    results = {"base_path_str": args.soccer_path,
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
               "print_stats": args.print_stats}
    return results


if __name__ == '__main__':
    # prepare default IO conf
    io_conf = IOConfiguration(raw_file=SoccerIOConfiguration.RAW_FILE.value,
                              small_file=SoccerIOConfiguration.SMALL_RAW_FILE.value,
                              raw_file_download_name=SoccerIOConfiguration.RAW_FILE_DOWNLOADED_NAME.value,
                              results_dir=SoccerIOConfiguration.RESULTS_DIR.value,
                              base_path="/home/digiou/test-results/",
                              origin=SoccerIOConfiguration.ORIGIN.value)
    # parse CLI parameters
    parsed_args = get_parser_args(prepare_parser(io_conf))

    # create logger
    logger = prepare_logger()

    # create UUID for run
    current_uuid = shortuuid.uuid()

    # store any experiment files
    sensor_files_path = Path(parsed_args["base_path_str"]).joinpath(io_conf.results_dir)

    # create df on whole data
    whole_df = get_single_dataframe(sensor_files_path.joinpath(SoccerIOConfiguration.RAW_FILE.value))

    # average length of the dataframes
    avg_len = 0.

    # get X data for only the ball dataset
    ball_df = get_ball_x_dataframe(whole_df)
    avg_len += len(ball_df)

    # get X data for referee dataset
    ref_df = get_referee_x_dataframe(whole_df)
    avg_len += len(ref_df)

    # get X data for all players
    player_dfs = get_players_x_dataframes(whole_df)
    for player_df in player_dfs:
        avg_len += len(player_df)
    avg_len /= (2 + len(player_dfs))
    logger.info(f"Average DF size: {avg_len}")

    # all dfs
    all_dfs = {"ball_movement": [ball_df], "ref_movement": [ref_df], "player_movement": player_dfs}

    with Pool(processes=get_physical_cores()) as task_pool:
        pd.options.mode.chained_assignment = None  # default='warn'
        if parsed_args["print_stats"]:
            for col, values_list in all_dfs.items():
                results = compute_detailed_stats(dataframes=values_list)
                mean_dur_str = closest_time_unit(results["mean_duration"])
                print(f"{col}: meandur: {mean_dur_str}, stats: {results}")
        if parsed_args["plot_fft"]:
            plot_fft(per_value_df_lists=all_dfs, task_pool=task_pool)

        if parsed_args["sampling"]:
            get_sampling_stats(sensor_files_path=sensor_files_path, per_value_df_lists=all_dfs,
                               task_pool=task_pool, short_uuid=current_uuid,
                               nyquist_window=parsed_args["nyquist_window_size"],
                               use_filter=parsed_args["use_filter"],
                               use_only_filter=parsed_args["use_only_filter"],
                               constrain_analyzer=parsed_args["constrain_analyzer"],
                               max_oversampling=parsed_args["max_oversampling"],
                               sampling_rate=parsed_args["sampling_rate"])

        if parsed_args["query_similarity"]:
            measure_accuracy(per_value_df_lists=all_dfs, sensor_files_path=sensor_files_path,
                             current_uuid=current_uuid,task_pool=task_pool,
                             nyquist_window=parsed_args["nyquist_window_size"],
                             query_window=parsed_args["query_window_size"],
                             use_filter=parsed_args["use_filter"],
                             use_only_filter=parsed_args["use_only_filter"],
                             constrain_analyzer=parsed_args["constrain_analyzer"],
                             max_oversampling=parsed_args["max_oversampling"],
                             sampling_rate=parsed_args["sampling_rate"])
        if parsed_args["scalability"]:
            get_sampling_stats(sensor_files_path=sensor_files_path, per_value_df_lists=all_dfs,
                               task_pool=task_pool, short_uuid=current_uuid,
                               nyquist_window=parsed_args["nyquist_window_size"],
                               use_filter=parsed_args["use_filter"],
                               use_only_filter=parsed_args["use_only_filter"],
                               constrain_analyzer=parsed_args["constrain_analyzer"],
                               max_oversampling=parsed_args["max_oversampling"],
                               sampling_rate=parsed_args["sampling_rate"],
                               get_avg_stats=True)
            measure_scalability(sensor_files_path=sensor_files_path)
    logger.info("Done!")
