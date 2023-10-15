import argparse
import json
import math

import numpy as np
import time
from collections import Counter
from enum import Enum
from datetime import datetime
from multiprocessing import Pool
from pathlib import Path
from typing import Dict, List, Union, Any, Tuple, Optional

import plotly.graph_objects as go
import shortuuid
import pandas as pd
from dtaidistance import dtw, preprocessing
from fastdtw import fastdtw
from numpy import ndarray
from pandas import DataFrame
from scipy import stats

from experiments.models.kalman_filters import init_filters
from experiments.io.dataset_stats import compute_detailed_stats
from experiments.io.files import IOConfiguration, download_raw_csv
from experiments.io.experiment_logger import prepare_logger
from experiments.io.resource_counters import get_physical_cores
from experiments.io.serialization import NpEncoder
from experiments.plotting.formatting import closest_time_unit
from experiments.sensor_functions.accuracy import calculate_agg_accuracy_windowed
from experiments.sensor_functions.fft import compute_nyquist_and_energy
from experiments.sensor_functions.interpolation import interpolate_df
from experiments.sensor_functions.gathering import gather_values_from_df
from experiments.emulator.scalability import measure_scalability


class IntelIOConfiguration(Enum):
    RAW_FILE = 'data.txt'
    SMALL_RAW_FILE = 'data-small.txt'
    RAW_FILE_DOWNLOADED_NAME = 'data.zip'
    RESULTS_DIR = 'intel-by-sensor'
    ORIGIN = 'http://db.csail.mit.edu/labdata/data.txt.gz'


def get_single_dataframe(path: Path) -> DataFrame:
    useful_cols = ['date', 'time', 'epoch', 'moteid', 'temperature',
                   'humidity', 'light', 'voltage']
    read_csv_options = {'filepath_or_buffer': str(path), 'encoding': 'utf-8-sig',
                        'header': None, 'index_col': None,
                        'sep': '\s+', 'on_bad_lines': 'warn',
                        'names': useful_cols, 'parse_dates': [['date', 'time']]}
    logger.info("Loading main csv from disk...")
    raw_dataframe = pd.read_csv(**read_csv_options)
    logger.debug("Loaded csv!")
    raw_dataframe.index = pd.to_datetime(raw_dataframe['date_time'], unit='s', utc=True, format='%Y-%m-%d %H:%M:%S.%fZ')
    del raw_dataframe['date_time']
    raw_dataframe.index = raw_dataframe.index.tz_convert('Europe/Berlin')
    raw_dataframe.sort_index(inplace=True)
    raw_dataframe.dropna(subset=['moteid'], inplace=True)
    raw_dataframe[["moteid"]] = raw_dataframe[["moteid"]].astype(int)
    raw_dataframe.dropna(subset=useful_cols[4:], how='all', inplace=True)
    raw_dataframe.drop(raw_dataframe[raw_dataframe.moteid > 58].index, inplace=True)
    logger.info("Done indexing main dataframe!")
    return raw_dataframe


def create_per_sensor_dataframes(raw_dataframe: DataFrame, print_stats=False) -> Dict[str, List[DataFrame]]:
    value_dfs = {'temperature': [], 'humidity': [], 'light': [], 'voltage': []}
    necessary_cols = ['epoch', 'moteid']
    logger.info("Creating per-sensor dataframes...")
    mote_ids = raw_dataframe.moteid.unique()
    for mote in mote_ids:
        for value_key in value_dfs.keys():
            cols_to_keep = necessary_cols + [value_key]  # keep info + val col.
            value_df = raw_dataframe[raw_dataframe.moteid == mote][cols_to_keep]
            value_df.dropna(subset=cols_to_keep[2:], inplace=True)
            if len(value_df) > 0:
                value_df = value_df[~value_df.index.duplicated(keep='last')]
                value_dfs[value_key].append(value_df)
    logger.info(f"Found {len(mote_ids)} unique motes, for {len(value_dfs.keys())} values!")
    if print_stats:
        for value_key, value_df_list in value_dfs.items():
            results = compute_detailed_stats(dataframes=value_df_list, col_name=value_key)
            mean_dur_str = closest_time_unit(results["mean_duration"])
            print(f"{value_key}: meandur: {mean_dur_str}, stats: {results}")
    return value_dfs


def get_single_sensor_sampling_stats(sensor_df: DataFrame, value_type: str, nyquist_window=10, use_filter=True,
                                     use_only_filter=False, constrain_analyzer: bool = False, max_oversampling: float = 1.,
                                     initial_interval=4, further_interpolate=True,
                                     only_necessary=True) -> Dict[str, Union[int, DataFrame]]:
    logger.debug("Preparing sensor_df for processing...")
    sensor_df.drop(columns=['moteid', 'epoch'], inplace=True, errors='ignore')
    if "value" not in sensor_df.columns:
        sensor_df.rename(columns={f"{value_type}": "value"}, inplace=True)
    sensor_df[["value"]] = sensor_df[["value"]].astype(float)
    if further_interpolate:
        sensor_interpolated = interpolate_df(sensor_df=sensor_df)
    else:
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


def get_sampling_stats(sensor_files_path: Path, per_sensor_df: Dict[str, List[DataFrame]],
                       task_pool: Pool, short_uuid: str, nyquist_window: int=10, use_filter: bool=True,
                       use_only_filter=False, constrain_analyzer: bool = False, max_oversampling: float = 1.,
                       sampling_rate: float = 4, get_avg_stats: bool=False) -> Dict[str, Dict[str, float]]:
    value_types = ['temperature', 'humidity', 'light', 'voltage']
    results = {}
    for value_type in value_types:
        logger.info(f"Sampling stats on {value_type} streams...")
        overall_stats = {"fixed": 0, "baseline": 0, "chameleon": 0}
        per_strategy_sizes = {k: [] for k in overall_stats.keys()}
        param_list = [(sensor_df, value_type, nyquist_window, use_filter, use_only_filter, constrain_analyzer,
                       max_oversampling, sampling_rate)
                      for sensor_df in per_sensor_df.get(value_type)]
        promises = [task_pool.apply_async(get_single_sensor_sampling_stats, (i)) for i in param_list]
        sensor_results = [res.get() for res in promises]
        for sensor_result in sensor_results:
            allowed_items = {k: v for k, v in sensor_result.items() if not k.endswith('_df')}
            for key in allowed_items.keys():
                overall_stats[key] += allowed_items[key]
                per_strategy_sizes[key].append(allowed_items[key])
        results[value_type] = {**overall_stats}
        if get_avg_stats:
            results[value_type]["stats"] = {}
            detailed_stats = {"nyquist_window": nyquist_window,
                              "sampling_rate": sampling_rate,
                              "mean_size": np.mean(per_strategy_sizes["chameleon"]),
                              "median_size": np.median(per_strategy_sizes["chameleon"]),
                              "sensor_num": len(per_sensor_df.get(value_type))}
            results[value_type]["stats"] = {**detailed_stats}
    file_name = f"sampling-results-range-intel-{short_uuid}-{datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
    with open(sensor_files_path.joinpath(file_name), 'w') as fp:
        json.dump(results, fp, indent=4)
    return results


def plot_sampling_stats(sensor_results: Dict[str, Dict[str, float]], results_path: Path, short_uuid: str, fuse=False):
    strategies = ["baseline", "fixed", "chameleon"]
    overall_data = []
    fused_data = []
    y_max = 0
    y_max_fused = 0
    for workload in sensor_results.keys():
        results = sensor_results[workload]
        y_values = [results[x] for x in strategies]
        y_max = max(y_max, max(y_values))
        legend_name = f"Dataset: {workload}"
        overall_data.append(go.Bar(name=legend_name, x=strategies, y=y_values))
        if fuse:
            y_values_fused = y_values.copy()
            y_values_fused.append(results["fused"])
            y_max_fused = max(y_max_fused, max(y_values_fused))
            fused_data.append(go.Bar(name=legend_name, x=strategies + ["fused"], y=y_values_fused))
    fig_total = go.Figure(data=overall_data)
    fig_total.update_layout(barmode="group")
    fig_total.update_xaxes(title_text="<b>Strategy</b>")
    fig_total.update_yaxes(title_text="<b># of Samples (total)</b>", type='log',
                           range=[0, math.ceil(math.log10(y_max))])
    fil_total_name = f"sampling-results-total-intel-{short_uuid}.pdf"
    fig_total.write_image(results_path.joinpath(fil_total_name))
    time.sleep(2)  # wait and overwrite graph for mathjax bug
    fig_total.write_image(results_path.joinpath(fil_total_name))
    if fuse:
        fig_fused = go.Figure(data=fused_data)
        fig_fused.update_layout(barmode="group")
        fig_fused.update_xaxes(title_text="<b>Strategy</b>")
        fig_fused.update_yaxes(title_text="<b># of Samples (total)</b>", type='log',
                               range=[0, math.ceil(math.log10(y_max_fused))])
        fig_fused_name = f"sampling-results-fused-total-intel-{short_uuid}.pdf"
        fig_fused.write_image(results_path.joinpath(fig_fused_name))
        time.sleep(2)  # wait and overwrite graph for mathjax bug
        fig_fused.write_image(results_path.joinpath(fig_fused_name))


def get_similarity_stats(per_value_df_lists: Dict[str, List[DataFrame]],
                         sensor_files_path: Path, short_uuid: str, task_pool: Pool,
                         desired_data_rate='.05s', initial_interval=4) -> Dict[
    str, Dict[str, Dict[str, Union[Dict[str, float], float]]]]:
    logger.info(f"Distance calculation...")
    results = {"fast_dtw": {}}
    for stream_type, sensor_list in per_value_df_lists.items():
        similarity_results = signal_similarity(sensor_list=sensor_list, work_type=stream_type,
                                               desired_data_rate=desired_data_rate,
                                               initial_interval=initial_interval, task_pool=task_pool)
        results["fast_dtw"][stream_type] = similarity_results
    file_name = f"similarity-intel-{short_uuid}-{datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}-{initial_interval}.json"
    with open(sensor_files_path.joinpath(file_name), 'w') as fp:
        json.dump(results, fp, indent=4)
    return results


def signal_similarity(sensor_list: List[DataFrame], work_type: str, task_pool: Pool,
                      desired_data_rate='.05s', initial_interval=4) -> Dict[str, Dict[str, float]]:
    similarity_results_overall = {
        "baseline": {"dist": 0, "samples": 0},
        "fixed": {"dist": 0, "samples": 0},
        "chameleon": {"dist": 0, "samples": 0}
    }
    total_interval = 0
    param_list = []
    for sensor in sensor_list:
        param_list.append((sensor, work_type, desired_data_rate, initial_interval))
    idx = len(param_list)
    promises = [task_pool.apply_async(interpolate_sample_and_compare, (i)) for i in param_list]
    all_similarity_results = [res.get() for res in promises]
    for single_sensor_similarity, used_interval in all_similarity_results:
        for results_key in similarity_results_overall.keys():
            if single_sensor_similarity[results_key]["dist"] != -1:
                similarity_results_overall[results_key]["dist"] += single_sensor_similarity[results_key]["dist"]
                similarity_results_overall[results_key]["samples"] += single_sensor_similarity[results_key]["samples"]
                total_interval += used_interval
            else:
                idx -= 1
                break
    for results_key in similarity_results_overall.keys():
        similarity_results_overall[results_key]["dist"] /= idx
        similarity_results_overall[results_key]["samples"] /= idx
    similarity_results_overall["used_interval"] = total_interval / idx
    return similarity_results_overall


def interpolate_sample_and_compare(raw_sensor_df: DataFrame, work_type: str, data_rate='.05s',
                                   initial_interval=4) -> Tuple[Dict[str, Dict[str, float]], float]:
    similarity_results = {
        "baseline": {"dist": 0, "samples": 0},
        "fixed": {"dist": 0, "samples": 0},
        "chameleon": {"dist": 0, "samples": 0}
    }
    drop_list = list(['moteid', 'epoch'])
    clean_sensor = raw_sensor_df.drop(columns=drop_list, errors='ignore')
    sensor_interpolated = interpolate_df(clean_sensor, desired_data_rate=data_rate)
    if initial_interval == 'nyquist':
        clean_sensor.rename(columns={f"{work_type}": "value"}, inplace=True)
        nyq_result = compute_nyquist_and_energy(clean_sensor)
        if not nyq_result[0]:
            return {x: {"dist": -1, "samples": -1} for x in similarity_results}, initial_interval
        initial_interval = nyq_result[3]
    sensor_interpolated.index.name = work_type
    samples = get_single_sensor_sampling_stats(sensor_interpolated, value_type=work_type,
                                               initial_interval=initial_interval,
                                               only_necessary=True,
                                               further_interpolate=False)
    failed_once = False
    for results_key in samples.keys():
        if results_key.endswith("_df"):
            recorded_stream = samples[results_key]
            try:
                distance_mjc = mjc(prepare_for_mjc(sensor_interpolated, idx_string=sensor_interpolated.index.name),
                                   prepare_for_mjc(recorded_stream, idx_string=results_key[:-3]))
                # distance_fastdtw, _ = fastdtw(sensor_interpolated, recorded_stream, dist=2)
                similarity_results[results_key[:-3]]["dist"] = float(distance_mjc[0])
                if distance_mjc[1]:  # diff exceeded default threshold, somehow
                    logger.info(f"MJC distance exceeded inf, raising...")
                    raise AssertionError
            except AssertionError:
                logger.info(f"MJC errored...")
                failed_once = True
            except IndexError:
                logger.info(f"Fastdtw errored...")
                failed_once = True
                date_t = datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
                sensor_interpolated.to_pickle(f"./interpolated-{date_t}.gz",
                                              compression={'method': 'gzip', 'compresslevel': 1, 'mtime': 1})
                recorded_stream.to_pickle(f"./recorded-{date_t}.gz",
                                          compression={'method': 'gzip', 'compresslevel': 1, 'mtime': 1})
                logger.info(
                    f"IndexError, strat:{results_key}, work: {work_type}, interv: {initial_interval}, interp: {len(sensor_interpolated)}, record: {len(recorded_stream)}")
        else:
            similarity_results[results_key]["samples"] = samples[results_key]
    if failed_once:
        return {x: {"dist": -1, "samples": -1} for x in similarity_results}, initial_interval
    return similarity_results, initial_interval


def plot_fft(per_value_df_lists: Dict[str, List[DataFrame]], task_pool: Pool):
    """
    Consolidate power calculations for all signals.
    From https://stackoverflow.com/questions/29429733/cant-find-the-right-energy-using-scipy-signal-welch
    """
    logger.info(f"Computing FFT and energy levels per sensor...")
    for workload, sensor_df_list in per_value_df_lists.items():
        for sensor_df in sensor_df_list:
            sensor_df.drop(columns=['moteid', 'epoch'], inplace=True)
            sensor_df.rename(columns={f"{workload}": "value"}, inplace=True)
            sensor_df[["value"]] = sensor_df[["value"]].astype(float)
        oversampled = 0
        zero_energy = 0
        oversampling_ratio = 0.
        promises = [task_pool.apply_async(compute_nyquist_and_energy, (sensor_df,)) for sensor_df in sensor_df_list if
                    len(sensor_df.index) > 2]
        nyquist_results = [res.get() for res in promises]
        for nyquist_result in nyquist_results:
            if nyquist_result[0]:
                oversampled += 1
                oversampling_ratio += nyquist_result[2]
            if nyquist_result[1] == 0.0:
                zero_energy += 1
        oversampling_ratio /= oversampled
        logger.info(
            f"Total streams for {workload}: {len(sensor_df_list)}, oversampled total: {oversampled}, oversampling ratio: {oversampling_ratio}, zero energy: {zero_energy}")
    logger.info(f"Done computing FFTs!")


def pareto_calc(per_sensor_dfs: Dict[str, List[DataFrame]], sensor_files_path: Path, current_uuid: str,
                task_pool: Pool, data_rate='.01S'):
    pareto_results = {}
    sampler_rates = [.05, .1, 1 / 9, .2, .3, .5, 1, 2, 3, 5, 9, 10, 20, 'nyquist']  # in seconds
    for sampler_rate in sampler_rates:
        stats = get_similarity_stats(per_value_df_lists=per_sensor_dfs,
                                     sensor_files_path=sensor_files_path, short_uuid=current_uuid,
                                     desired_data_rate=data_rate,
                                     initial_interval=sampler_rate, task_pool=task_pool)
        pareto_results[str(sampler_rate)] = stats
        logger.info(f"Stats for {sampler_rate} (s): {stats}")
    file_name = f"pareto-intel-{current_uuid}-{datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
    with open(sensor_files_path.joinpath(file_name), 'w', encoding='utf-8') as f:
        json.dump(pareto_results, f, ensure_ascii=False, indent=4)
        logger.info(f"Wrote data in {sensor_files_path.joinpath(file_name)}")


def bench_mjc(per_sensor_dfs: Dict[str, List[DataFrame]], initial_data_rates: List[str]):
    logger.info("Benchmarking different MJC impls...")
    test_df = per_sensor_dfs["temperature"][0]
    intervals = [10, 5, 2, 1, .5, .05]
    for initial_data_rate in initial_data_rates:
        start = time.time()
        interpolated_test = interpolate_df(sensor_df=test_df, desired_data_rate=initial_data_rate)
        logger.info(f"Interpolation with rate={initial_data_rate} took {time.time() - start}")
        results = {}
        for interval in intervals:
            interval_results = []
            start = time.time()
            samples = get_single_sensor_sampling_stats(interpolated_test, value_type="temperature",
                                                       initial_interval=interval, further_interpolate=False)
            logger.info(f"Sampling took {time.time() - start} at {interval}s interval...")
            interpolated = prepare_for_mjc(interpolated_test)
            sampled = prepare_for_mjc(samples['fixed_df'], idx_string='fixed')
            chameleon = prepare_for_mjc(samples['chameleon_df'], idx_string='chameleon')
            baseline = prepare_for_mjc(samples['baseline_df'], idx_string='baseline')
            start = time.time()
            distance = mjc(interpolated, sampled)
            interval_results.append(('pymjc.mjc.sampled', time.time() - start, float(distance[0])))
            start = time.time()
            distance = mjc(interpolated, sampled, override_checks=True)
            interval_results.append(('pymjc.mjc.sampled.override_checks', time.time() - start, float(distance[0])))
            start = time.time()
            distance = mjc(list(interpolated), list(sampled))
            interval_results.append(('pymjc.mjc.sampled.list', time.time() - start, float(distance[0])))
            start = time.time()
            distance = mjc(interpolated[1], sampled[1])
            interval_results.append(('pymjc.mjc.sampled.no_time', time.time() - start, float(distance[0])))
            start = time.time()
            distance = mjc(interpolated, chameleon)
            interval_results.append(('pymjc.mjc.chameleon', time.time() - start, float(distance[0])))
            start = time.time()
            try:
                distance = mjc(interpolated, baseline)
                interval_results.append(('pymjc.mjc.baseline', time.time() - start, float(distance[0])))
            except AssertionError:
                interval_results.append(('pymjc.mjc.baseline', time.time() - start, math.inf))
            results[str(interval)] = interval_results
        logger.info(f"Finished benchmarking MJC for rate {initial_data_rate}!")
        file_name = f"bench-mjc-{initial_data_rate}-{datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
        with open(sensor_files_path.joinpath(file_name), 'w', encoding='utf-8') as f:
            json.dump(results, f, ensure_ascii=False, indent=4)
            logger.info(f"Wrote data in {sensor_files_path.joinpath(file_name)}")
        logger.info(f"Rate: {initial_data_rate}, Results:\n{results}")


def bench_dtw_shape(per_sensor_dfs: Dict[str, List[DataFrame]], initial_data_rates: List[str]):
    logger.info("Benchmarking different DTW impls...")
    test_df = per_sensor_dfs["temperature"][0]
    intervals = [10, 5, 2, 1, .5, .05]
    for initial_data_rate in initial_data_rates:
        start = time.time()
        interpolated_test = interpolate_df(sensor_df=test_df, desired_data_rate=initial_data_rate)
        logger.info(f"Interpolation with rate={initial_data_rate} took {time.time() - start}")
        results = {}
        for interval in intervals:
            interval_results = []
            start = time.time()
            samples = get_single_sensor_sampling_stats(interpolated_test, value_type="temperature",
                                                       initial_interval=interval, further_interpolate=False,
                                                       only_necessary=True)
            logger.info(f"Sampling took {time.time() - start} at {interval}s interval...")
            sampled_fix_rate = samples['fixed_df']
            X = np.ascontiguousarray(interpolated_test.to_numpy(dtype=np.float32))
            Y = np.ascontiguousarray(sampled_fix_rate.to_numpy(dtype=np.float32))
            start = time.time()
            Xz = stats.zscore(X)
            Yx = stats.zscore(Y)
            logger.info(f"Zscore took {time.time() - start} at {interval}s interval...")
            start = time.time()
            distance = dtw.distance(Xz, Yx)
            interval_results.append(('dtw.distance.z', time.time() - start, float(distance)))
            start = time.time()
            distance = dtw.distance(Xz, Yx, use_pruning=True)
            interval_results.append(('dtw.distance.pruning.z', time.time() - start, float(distance)))
            start = time.time()
            x_fast = Xz.flatten().astype(np.double)
            y_fast = Yx.flatten().astype(np.double)
            distance = dtw.distance_fast(x_fast, y_fast)
            interval_results.append(('dtw.distance_fast.z', time.time() - start, float(distance)))
            start = time.time()
            distance = dtw.distance_fast(x_fast, y_fast, use_pruning=True)
            interval_results.append(('dtw.distance_fast.pruning.z', time.time() - start, float(distance)))
            start = time.time()
            Xdiff = preprocessing.differencing(X.flatten(), smooth=0.1)
            Ydiff = preprocessing.differencing(Y.flatten(), smooth=0.1)
            logger.info(f"differencing took {time.time() - start} at {interval}s interval...")
            start = time.time()
            distance = dtw.distance(Xdiff, Ydiff)
            interval_results.append(('dtw.distance.diff', time.time() - start, float(distance)))
            start = time.time()
            distance = dtw.distance(Xdiff, Ydiff, use_pruning=True)
            interval_results.append(('dtw.distance.pruning.diff', time.time() - start, float(distance)))
            start = time.time()
            x_fast = Xdiff.flatten().astype(np.double)
            y_fast = Ydiff.flatten().astype(np.double)
            distance = dtw.distance_fast(x_fast, y_fast)
            interval_results.append(('dtw.distance_fast.diff', time.time() - start, float(distance)))
            start = time.time()
            distance = dtw.distance_fast(x_fast, y_fast, use_pruning=True)
            interval_results.append(('dtw.distance_fast.pruning.diff', time.time() - start, float(distance)))
            interval_results.sort(key=lambda tup: tup[1])  # sort on time taken
            results[str(interval)] = interval_results
            break
        logger.info(f"Finished benchmarking DTW shape for rate {initial_data_rate}!")
        file_name = f"bench-dtw-shape-{initial_data_rate}-{datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
        with open(sensor_files_path.joinpath(file_name), 'w', encoding='utf-8') as f:
            json.dump(results, f, ensure_ascii=False, indent=4)
            logger.info(f"Wrote data in {sensor_files_path.joinpath(file_name)}")
        logger.info(f"Rate: {initial_data_rate}, Results:\n{results}")


def bench_dtw(per_sensor_dfs: Dict[str, List[DataFrame]], initial_data_rates: List[str], use_gpu=False):
    logger.info("Benchmarking different DTW impls...")
    test_df = per_sensor_dfs["temperature"][0]
    intervals = [10, 5, 2, 1, .5, .05]
    for initial_data_rate in initial_data_rates:
        start = time.time()
        interpolated_test = interpolate_df(sensor_df=test_df, desired_data_rate=initial_data_rate)
        logger.info(f"Interpolation with rate={initial_data_rate} took {time.time() - start}")
        results = {}
        for interval in intervals:
            interval_results = []
            start = time.time()
            samples = get_single_sensor_sampling_stats(interpolated_test, value_type="temperature",
                                                       initial_interval=interval, further_interpolate=False,
                                                       only_necessary=True)
            logger.info(f"Sampling took {time.time() - start} at {interval}s interval...")
            sampled_fix_rate = samples['fixed_df']
            X = np.ascontiguousarray(interpolated_test.to_numpy(dtype=np.float32))
            Y = np.ascontiguousarray(sampled_fix_rate.to_numpy(dtype=np.float32))
            if not use_gpu:
                start = time.time()
                distance = dtw.distance(X, Y)
                interval_results.append(('dtw.distance', time.time() - start, float(distance)))
                start = time.time()
                distance = dtw.distance(X, Y, use_pruning=True)
                interval_results.append(('dtw.distance.pruning', time.time() - start, float(distance)))
                start = time.time()
                x_fast = X.flatten().astype(np.double)
                y_fast = Y.flatten().astype(np.double)
                distance = dtw.distance_fast(x_fast, y_fast)
                interval_results.append(('dtw.distance_fast', time.time() - start, float(distance)))
                start = time.time()
                distance = dtw.distance_fast(x_fast, y_fast, use_pruning=True)
                interval_results.append(('dtw.distance_fast.pruning', time.time() - start, float(distance)))
                start = time.time()
                dist, _ = fastdtw(interpolated_test, sampled_fix_rate, dist=2, radius=2)
                interval_results.append(('fastdtw', time.time() - start, float(dist)))
                start = time.time()
                dist, _ = linmdtw.fastdtw(X=X, Y=Y, radius=2)
                interval_results.append(('linmdtw.fastdtw', time.time() - start, float(dist)))
                start = time.time()
                dist, _ = linmdtw.cdtw(X=X, Y=Y, radius=2)
                interval_results.append(('linmdtw.cdtw', time.time() - start, float(dist)))
                start = time.time()
                res = linmdtw.dtw_diag(X=X, Y=Y)
                interval_results.append(('linmdtw.dtw_diag', time.time() - start, float(res['cost'])))
            start = time.time()
            cost, _ = linmdtw.linmdtw(X=X, Y=Y, do_gpu=use_gpu)
            interval_results.append(('linmdtw.linmdtw', time.time() - start, float(cost)))
            interval_results.sort(key=lambda tup: tup[1])  # sort on time taken
            results[str(interval)] = interval_results
            start = time.time()
            stats.zscore(X)
            logger.info(f"Zscore took {time.time() - start} at {interval}s interval...")
            start = time.time()
            preprocessing.differencing(X.flatten(), smooth=0.1)
            logger.info(f"differencing took {time.time() - start} at {interval}s interval...")
            break
        logger.info(f"Finished benchmarking DTWs for rate {initial_data_rate}!")
        file_name = f"bench-dtw-{initial_data_rate}-{datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
        with open(sensor_files_path.joinpath(file_name), 'w', encoding='utf-8') as f:
            json.dump(results, f, ensure_ascii=False, indent=4)
            logger.info(f"Wrote data in {sensor_files_path.joinpath(file_name)}")
        logger.info(f"Rate: {initial_data_rate}, Results:\n{results}")


def prepare_for_mjc(dataFrame1: DataFrame, idx_string='index') -> ndarray:
    reset_ts = dataFrame1.reset_index(level=0)
    reset_ts['index'] = reset_ts[idx_string].apply(lambda x: x.value)
    reset_ts.rename(columns={"index": "ts"}, inplace=True)
    interpolated_ndarr = np.vstack((reset_ts['ts'].to_list(), reset_ts['value'].to_list()))
    return interpolated_ndarr


def measure_similarity(per_sensor_dfs: Dict[str, List[DataFrame]], sensor_files_path: Path,
                       task_pool: Pool, short_uuid: str, nyquist_window=10, query_window='1min',
                       use_filter=True, use_only_filter=False,
                       constrain_analyzer: bool = False, max_oversampling: float = 1., sampling_rate: float = 4.,
                       only_necessary: bool = True):
    total_results = {}
    strategies = ["fixed", "chameleon"] if only_necessary else ["baseline", "fixed", "chameleon"]
    file_name = f"qs-intel-{short_uuid}-{datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
    with open(sensor_files_path.joinpath(file_name), 'w', encoding='utf-8') as fp:
        for stream_type in per_sensor_dfs.keys():
            logger.info(f"Stream: {stream_type}, measuring qs...")
            mape_accuracy = Counter({"sum": 0., "count": 0., "mean": 0., "median": 0., "max": 0., "min": 0., "std": 0., "var": 0.})
            mape_accuracies = {k: mape_accuracy.copy() for k in strategies}
            max_accuracies = {k: mape_accuracy.copy() for k in strategies}
            samples = {k: 0 for k in strategies}
            sensors = per_sensor_dfs[stream_type]
            promises = [task_pool.apply_async(calculate_agg_accuracy,
                        (raw_sensor, stream_type, nyquist_window, use_filter, use_only_filter, query_window, constrain_analyzer, max_oversampling, sampling_rate,))
                        for raw_sensor in sensors]
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
        total_results["query_window"] = query_window
        json.dump(total_results, fp, ensure_ascii=False, indent=4, cls=NpEncoder)
        logger.info(f"Wrote data in {sensor_files_path.joinpath(file_name)}")


def calculate_agg_accuracy(sensor_df: DataFrame, stream_type: str, nyquist_window=10, use_filter=True,
                           use_only_filter=False, query_window='1min', constrain_analyzer: bool = False,
                           max_oversampling: float = 1., sampling_rate: Union[float, str] = 4., only_necessary=True) -> Optional[Dict[str, Dict[str, Dict[str, float]]]]:
    sensor_df.drop(columns=['moteid', 'epoch'], inplace=True, errors='ignore')
    sensor_df.rename(columns={f"{stream_type}": "value"}, inplace=True)
    sensor_df[["value"]] = sensor_df[["value"]].astype(float)
    interpolated_df = interpolate_df(sensor_df=sensor_df)
    samples = get_single_sensor_sampling_stats(sensor_df=interpolated_df, value_type=stream_type,
                                               nyquist_window=nyquist_window, use_filter=use_filter,
                                               use_only_filter=use_only_filter, constrain_analyzer=constrain_analyzer,
                                               max_oversampling=max_oversampling, initial_interval=sampling_rate,
                                               further_interpolate=False)
    results = {"baseline": None, 'fixed': None, 'chameleon': None}
    if only_necessary:
        results.pop("baseline")
    for result_name in results.keys():
        results[result_name] = calculate_agg_accuracy_windowed(interpolated_df,
                                                               samples[result_name + "_df"],
                                                               query_window=query_window,
                                                               sampler_rate=sampling_rate)
        results[result_name]["samples"] = samples[result_name]
    return results


def prepare_parser(io_conf: IOConfiguration) -> argparse.ArgumentParser:
    cli_parser = argparse.ArgumentParser()
    cli_parser.add_argument("--intel_path", help="The root for the whole experiment",
                            default=io_conf.base_path, type=str)
    cli_parser.add_argument("--download", help="Download file", action='store_true')
    cli_parser.set_defaults(download=False)
    cli_parser.add_argument("--log_level", help="The log_level as string", type=str, default="info", required=False)
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
    cli_parser.add_argument("--calc_pareto", help="Calculate Pareto front", action='store_true')
    cli_parser.set_defaults(calc_pareto=False)
    cli_parser.add_argument("--bench_dtw", help="Bench different dtw approaches", action='store_true')
    cli_parser.set_defaults(bench_dtw=False)
    cli_parser.add_argument("--bench_dtw_shape", help="Bench different dtw shape approaches", action='store_true')
    cli_parser.set_defaults(bench_dtw_shape=False)
    cli_parser.add_argument("--bench_mjc", help="Bench different MJC approaches", action='store_true')
    cli_parser.set_defaults(bench_mjc=False)
    cli_parser.add_argument("--use_gpu", help="Bench dtw on CUDA", action='store_true')
    cli_parser.set_defaults(use_gpu=False)
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
    results = {"base_path_str": args.intel_path,
               "download": args.download,
               "log_level": args.log_level,
               "plot_fft": args.plot_fft,
               "sampling": args.sampling,
               "nyquist_window_size": args.nyquist_window_size,
               "query_window_size": args.query_window_size,
               "constrain_analyzer": args.constrain_analyzer,
               "max_oversampling": args.max_oversampling,
               "sampling_rate": args.sampling_rate,
               "calc_pareto": args.calc_pareto,
               "bench_dtw": args.bench_dtw,
               "bench_dtw_shape": args.bench_dtw_shape,
               "bench_mjc": args.bench_mjc,
               "use_gpu": args.use_gpu,
               "use_filter": not args.use_nyquist_only,
               "use_only_filter": args.use_only_filter,
               "query_similarity": args.query_similarity,
               "scalability": args.scalability,
               "print_stats": args.print_stats}
    return results


if __name__ == '__main__':
    # prepare default IO conf
    io_conf = IOConfiguration(raw_file=IntelIOConfiguration.RAW_FILE.value,
                              small_file=IntelIOConfiguration.SMALL_RAW_FILE.value,
                              raw_file_download_name=IntelIOConfiguration.RAW_FILE_DOWNLOADED_NAME.value,
                              results_dir=IntelIOConfiguration.RESULTS_DIR.value,
                              base_path="/home/digiou/test-results",
                              origin=IntelIOConfiguration.ORIGIN.value)

    # parse CLI parameters
    parsed_args = get_parser_args(prepare_parser(io_conf))

    # create logger
    logger = prepare_logger(parsed_args["log_level"])

    # create UUID for run
    current_uuid = shortuuid.uuid()

    # store any experiment files
    sensor_files_path = Path(parsed_args["base_path_str"]).joinpath(io_conf.results_dir)

    # download from origin
    if parsed_args["download"]:
        download_raw_csv(configuration=io_conf, logger=logger, skip_move=parsed_args["print_stats"])

    # create df on whole data
    whole_df = get_single_dataframe(sensor_files_path.joinpath(IntelIOConfiguration.RAW_FILE.value))

    # figure out number of physical cores and create thread pool
    with Pool(processes=get_physical_cores()) as task_pool:
        # split into single sensor dfs for all vals
        per_sensor_dfs = create_per_sensor_dataframes(raw_dataframe=whole_df, print_stats=parsed_args["print_stats"])

        # overall sampling stats
        if parsed_args["sampling"]:
            sampling_stats = get_sampling_stats(sensor_files_path=sensor_files_path,
                                                per_sensor_df=per_sensor_dfs,
                                                task_pool=task_pool, short_uuid=current_uuid,
                                                nyquist_window=parsed_args["nyquist_window_size"],
                                                use_filter=parsed_args["use_filter"],
                                                use_only_filter=parsed_args["use_only_filter"],
                                                constrain_analyzer=parsed_args["constrain_analyzer"],
                                                max_oversampling=parsed_args["max_oversampling"],
                                                sampling_rate=parsed_args["sampling_rate"])
        if parsed_args["plot_fft"]:
            plot_fft(per_value_df_lists=per_sensor_dfs, task_pool=task_pool)

        if parsed_args["calc_pareto"]:
            pareto_calc(per_sensor_dfs=per_sensor_dfs, sensor_files_path=sensor_files_path,
                        current_uuid=current_uuid, task_pool=task_pool)
        rates = ['.2S']
        if parsed_args["bench_dtw_shape"]:
            bench_dtw_shape(per_sensor_dfs=per_sensor_dfs, initial_data_rates=rates)
        if parsed_args["bench_dtw"]:
            import linmdtw
            bench_dtw(per_sensor_dfs=per_sensor_dfs, initial_data_rates=rates, use_gpu=parsed_args["use_gpu"])
        if parsed_args["bench_mjc"]:
            from pymjc import mjc
            bench_mjc(per_sensor_dfs=per_sensor_dfs, initial_data_rates=rates)
        # check agg. between raw and sampled data
        if parsed_args["query_similarity"]:
            measure_similarity(per_sensor_dfs=per_sensor_dfs, sensor_files_path=sensor_files_path,
                             task_pool=task_pool, short_uuid=current_uuid,
                             nyquist_window=parsed_args["nyquist_window_size"],
                             query_window=parsed_args["query_window_size"],
                             use_filter=parsed_args["use_filter"],
                             use_only_filter=parsed_args["use_only_filter"],
                             constrain_analyzer=parsed_args["constrain_analyzer"],
                             max_oversampling=parsed_args["max_oversampling"],
                             sampling_rate=parsed_args["sampling_rate"])
        if parsed_args["scalability"]:
            get_sampling_stats(sensor_files_path=sensor_files_path,
                                per_sensor_df=per_sensor_dfs,
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
