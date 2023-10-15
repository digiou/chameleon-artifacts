import argparse
import json
import math
import pathlib
import time
from collections import Counter
from datetime import datetime
from enum import Enum
from multiprocessing import Pool
from pathlib import Path
from random import random
from typing import Dict, List, Union, Any, Tuple, Optional

# import modin.pandas as pd
import numpy as np
import pandas as pd
import plotly.graph_objects as go
import shortuuid
from fastdtw import fastdtw
from pandas import DataFrame

from experiments.io.files import IOConfiguration
from experiments.io.experiment_logger import prepare_logger
from experiments.io.resource_counters import get_physical_cores
from experiments.io.serialization import NpEncoder
from experiments.models.kalman_filters import init_filters
from experiments.plotting.df_plots import plot_fusion_comparison
from experiments.sensor_functions.accuracy import calculate_agg_accuracy_windowed
from experiments.sensor_functions.fft import compute_nyquist_and_energy
from experiments.sensor_functions.gathering import gather_values_from_df, gather_fused_dfs
from experiments.sensor_functions.interpolation import interpolate_df, interpolate_df_simple
from experiments.emulator.scalability import measure_scalability


nice_stream_candidates = [
    "s1-hh6-h36",
    "s2-hh6-h36",
    "s0-hh9-h38"
]


class SmartgridIOConfiguration(Enum):
    RAW_FILE = 'smartgrid-full.csv'
    RAW_FILE_DOWNLOADED_NAME = 'smartgrid-full.csv.gz'
    RESULTS_DIR = 'smartgrid-by-sensor'
    ORIGIN = 'http://www.doc.ic.ac.uk/~mweidlic/sorted.csv.gz'


def get_single_dataframe(path: str, has_no_headers=False) -> DataFrame:
    useful_cols = ['timestamp', 'value', 'property', 'plug_id', 'household_id', 'house_id']
    read_csv_options = {'filepath_or_buffer': path, 'encoding': 'utf-8-sig', 'header': 0, 'index_col': "timestamp",
                        'usecols': useful_cols}
    if has_no_headers:
        read_csv_options['names'] = ['id'] + useful_cols
        read_csv_options['header'] = None
        read_csv_options['index_col'] = None
        logger.info("Loading main csv from disk...")
    raw_dataframe = pd.read_csv(**read_csv_options)
    logger.debug("Loaded csv!")
    if has_no_headers:
        logger.info("Indexing main dataframe from csv...")
        raw_dataframe.index = pd.to_datetime(raw_dataframe['timestamp'], unit='s', utc=True)
        logger.info("Done indexing main dataframe!")
    else:
        raw_dataframe.index = pd.to_datetime(raw_dataframe.index, unit='s', utc=True)
    raw_dataframe.index = raw_dataframe.index.tz_convert('Europe/Berlin')
    raw_dataframe.drop('timestamp', axis=1, inplace=True, errors='ignore')
    return raw_dataframe


def create_per_sensor_dataframes(raw_dataframe: DataFrame) -> List[DataFrame]:
    dfs = []
    logger.info("Creating per-sensor dataframes...")
    house_ids = raw_dataframe.house_id.unique()
    avg_len = 0.
    total_sensor_df = 0.
    for house_id in house_ids:
        house_mask = raw_dataframe['house_id'].values == house_id
        house_df = raw_dataframe[house_mask]
        household_ids = house_df.household_id.unique()
        for household_id in household_ids:
            household_mask = house_df['household_id'].values == household_id
            household_df = house_df[household_mask]
            sensor_ids = household_df.plug_id.unique()
            for sensor_id in sensor_ids:
                logger.debug(f"Creating dataframe for s{household_id}_hh{household_id}_h{house_id}...")
                sensor_mask = household_df['plug_id'].values == sensor_id
                sensor_df = household_df[sensor_mask]
                avg_len += len(sensor_df)
                dfs.append(sensor_df)
                total_sensor_df += 1
    avg_len /= total_sensor_df
    logger.info(f"Average DF size: {avg_len}")
    logger.info("Done creating per-sensor dataframes!")
    return dfs


def create_per_house_dataframes(raw_dataframe: DataFrame) -> Dict[str, Dict[str, Dict[str, List[DataFrame]]]]:
    dfs = {}
    logger.info("Creating per-house dataframes...")
    house_ids = raw_dataframe.house_id.unique()
    for house_id in house_ids:
        dfs[house_id] = {}
        house_df = raw_dataframe[raw_dataframe.house_id == house_id]
        household_ids = house_df.household_id.unique()
        for household_id in household_ids:
            dfs[house_id][household_id] = {"work": [], "load": []}
            household_df = house_df[house_df.household_id == household_id]
            sensor_ids = household_df.plug_id.unique()
            for sensor_id in sensor_ids:
                logger.debug(f"Creating dataframe for s{household_id}-hh{household_id}-h{house_id}...")
                sensor_df = household_df[household_df.plug_id == sensor_id]
                work_df = sensor_df[sensor_df.property == 0]
                work_df = work_df[~work_df.index.duplicated(keep='last')]
                load_df = sensor_df[sensor_df.property == 1]
                load_df = load_df[~load_df.index.duplicated(keep='last')]
                df_name = 's' + str(sensor_id) + "-" + "hh" + str(household_id) + "-" + "h" + str(house_id)
                work_df.index.name = load_df.index.name = df_name
                dfs[house_id][household_id]["work"].append(work_df)
                dfs[house_id][household_id]["load"].append(load_df)
    return dfs


def write_sensor_df_csv(df_dict: Dict[str, List[DataFrame]],
                        task_pool: Pool,
                        base_path='/home/digiou/test-results/smartgrid-by-sensor/'):
    file_configurations = []
    Path(base_path).mkdir(parents=True, exist_ok=True)
    for workload_key in df_dict.keys():
        df_list = df_dict[workload_key]  # keys are workload types
        logger.info(f"Preprocessing {workload_key} dataframes...")
        Path(base_path).joinpath(workload_key).mkdir(parents=True, exist_ok=True)
        for data_frame in df_list:
            file_id = "s" + str(int(data_frame.iloc[0]["plug_id"])) + "-" \
                      + "hh" + str(int(data_frame.iloc[0]["household_id"])) + "-" \
                      + "h" + str(int(data_frame.iloc[0]["house_id"]))
            data_frame.index.name = file_id
            file_configurations.append((data_frame, str(Path(base_path).joinpath(workload_key))))
        logger.info(f"Preprocessed {workload_key} dataframes!")
    write_promises = [task_pool.apply_async(write_single_sensor_df_csv, i) for (i) in file_configurations]
    for write_promise in write_promises:
        write_promise.get()
    logger.info(f"Wrote all dataframes in {Path(base_path)}!")


def write_single_sensor_df_csv(df: DataFrame,
                               base_path: str):
    logger.debug(f"Writing file {df.index.name}...")
    file_path = pathlib.Path(base_path).joinpath(df.index.name + ".csv")
    df.to_csv(index=True, path_or_buf=file_path, date_format='%s', encoding='ascii', mode='a', header=False)
    logger.debug(f"Wrote new csv!")


def split_data_by_sensor(big_file_path: Path) -> List[DataFrame]:
    dataframe = get_single_dataframe(path=str(big_file_path), has_no_headers=True)
    return create_per_sensor_dataframes(dataframe)


def split_data_by_household(big_file_path: Path) -> Dict[str, Dict[str, Dict[str, List[DataFrame]]]]:
    dataframe = get_single_dataframe(path=str(big_file_path), has_no_headers=True)
    dfs = create_per_house_dataframes(dataframe)
    return dfs


def get_single_sensor_sampling_stats(sensor_df: DataFrame, value_type: int, nyquist_window=10, use_filter=True,
                                     use_only_filter=False, constrain_analyzer: bool = False, max_oversampling: float = 1.,
                                     initial_interval=4, further_interpolate=True,
                                     only_necessary=True) -> Dict[str, Union[int, DataFrame]]:
    file_path = sensor_df.index.name
    filtered_df = sensor_df[sensor_df.property == value_type]
    filtered_df = filtered_df[~filtered_df.index.duplicated(keep='last')]
    if further_interpolate:
        sensor_interpolated = interpolate_df(sensor_df=filtered_df)
    else:
        sensor_interpolated = filtered_df
    strategies = init_filters(initial_interval=1000 * initial_interval, nyquist_window=nyquist_window,
                              use_filter=use_filter, use_only_filter=use_only_filter,
                              constrain_analyzer=constrain_analyzer, max_oversampling=max_oversampling)
    logger.debug("Getting sampling stats for fixed interval...")
    fixed_df = gather_values_from_df(original_df=sensor_interpolated, initial_interval=initial_interval, strategy=None)
    fixed_df.index.name = f"{file_path}-observed-fixed-{initial_interval}s"
    logger.debug("Getting sampling stats for chameleon...")
    chameleon_df = gather_values_from_df(original_df=sensor_interpolated, initial_interval=initial_interval, strategy=strategies["chameleon"],
                                         gathering_interval_range=2*initial_interval)
    chameleon_df.index.name = f"{file_path}-observed-chameleon-range-{2*initial_interval}s"
    if only_necessary:
        return {"fixed": len(fixed_df), "fixed_df": fixed_df,
                "chameleon": len(chameleon_df), "chameleon_df": chameleon_df}
    logger.debug("Getting sampling stats for baseline...")
    baseline_df = gather_values_from_df(original_df=sensor_interpolated, initial_interval=initial_interval, strategy=strategies["baseline"],
                                        gathering_interval_range=2*initial_interval, artificial_cuttoff=True)
    baseline_df.index.name = f"{file_path}-observed-baseline-range-{2*initial_interval}s"
    return {"fixed": len(fixed_df),
            "fixed_df": fixed_df,
            "baseline": len(baseline_df),
            "baseline_df": baseline_df,
            "chameleon": len(chameleon_df),
            "chameleon_df": chameleon_df}


def get_single_sensor_sampling_stats_async(sensor_df: DataFrame, task_pool: Pool,
                                           gathering_interval_range=8) -> Dict[str, Union[int, DataFrame]]:
    filters = init_filters()
    filters.pop("fused")
    strategies = list(filters.keys()) + ['fixed']
    should_cutoff = lambda x: True if x == "baseline" else False
    param_tuplist = [(sensor_df, 4, filters.get(strategy, None), gathering_interval_range, should_cutoff(strategy))
                     for strategy in strategies]
    execution_results = [task_pool.apply_async(gather_values_from_df, (i)) for i in param_tuplist]
    materialized_results = [res.get() for res in execution_results]
    results = {}
    for materialized_result in materialized_results:
        results[materialized_result.index.name] = len(materialized_result)
        results[f"{materialized_result.index.name}_df"] = materialized_result
    return results


def prepare_filter_and_sample(sensor_list: List[DataFrame], gathering_interval_range=8) -> DataFrame:
    return gather_fused_dfs(sensor_list, 4, gathering_interval_range, False)


def get_sampling_stats(sensor_files_path: Path, per_sensor_df: List[DataFrame],
                       short_uuid: str, task_pool: Pool, nyquist_window: int=10, use_filter: bool=True,
                       use_only_filter=False, constrain_analyzer: bool = False, max_oversampling: float = 1.,
                       sampling_rate: float = 4, get_avg_stats: bool=False) -> Dict[str, Dict[str, float]]:
    work_types = {'work': 0, 'load': 1}
    results = {}
    for stream_name, stream_value in work_types.items():
        logger.info(f"Sampling stats on {stream_name} streams...")
        overall_stats = {"fixed": 0, "baseline": 0, "chameleon": 0}
        per_strategy_sizes = {k: [] for k in overall_stats.keys()}
        param_list = []
        for sensor in per_sensor_df:
            param_list.append((sensor, stream_value, nyquist_window, use_filter, use_only_filter, constrain_analyzer,
                               max_oversampling, sampling_rate))
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
                              "sensor_num": len(per_sensor_df)}
            results[stream_name]["stats"] = {**detailed_stats}
    file_name = f"sampling-results-range-smartgrid-{short_uuid}-{datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
    with open(sensor_files_path.joinpath(file_name), 'w') as fp:
        json.dump(results, fp, indent=4)
    return results


def get_fused_sampling_stats(sorted_dfs: Dict[str, Dict[str, Dict[str, List[DataFrame]]]],
                             task_pool: Pool, fusion_selectivity=.5) -> Dict[str, Dict[str, float]]:
    results = {"work": {"fused": 0}, "load": {"fused": 0}}
    logger.info(f"Sampling stats with fused streams...")
    for house_id, house in sorted_dfs.items():
        for household_id, household in house.items():
            for workload_id, workload_dfs in household.items():
                sensors = workload_dfs
                lower_bound = 0
                upper_bound = 1
                task_list = []
                while upper_bound <= len(sensors):
                    if fuse_multiple(fusion_selectivity):
                        upper_bound += 1
                    sensors_to_use = sensors[lower_bound:upper_bound]
                    task_list.append(task_pool.apply_async(prepare_filter_and_sample, (sensors_to_use, 8)))
                    lower_bound = upper_bound
                    upper_bound += 1
                household_results = [res.get() for res in task_list]
                for sensor_result in household_results:
                    results[workload_id]["fused"] += len(sensor_result)
    return results


def fuse_multiple(fusion_selectivity=.5) -> bool:
    return random() < fusion_selectivity


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
    fil_total_name = f"sampling-results-total-{short_uuid}.pdf"
    fig_total.write_image(results_path.joinpath(fil_total_name))
    time.sleep(2)  # wait and overwrite graph for mathjax bug
    fig_total.write_image(results_path.joinpath(fil_total_name))
    if fuse:
        fig_fused = go.Figure(data=fused_data)
        fig_fused.update_layout(barmode="group")
        fig_fused.update_xaxes(title_text="<b>Strategy</b>")
        fig_fused.update_yaxes(title_text="<b># of Samples (total)</b>", type='log',
                               range=[0, math.ceil(math.log10(y_max_fused))])
        fig_fused_name = f"sampling-results-fused-total-{short_uuid}.pdf"
        fig_fused.write_image(results_path.joinpath(fig_fused_name))
        time.sleep(2)  # wait and overwrite graph for mathjax bug
        fig_fused.write_image(results_path.joinpath(fig_fused_name))


def signal_similarity(sensor_files_path: Path, work_type: str, task_pool: Pool, in_memory=False,
                      desired_data_rate='.05s', initial_interval=4) -> Dict[str, Union[Dict[str, float], float]]:
    similarity_results_overall = {
        "baseline": {"dist": 0, "samples": 0},
        "fixed": {"dist": 0, "samples": 0},
        "chameleon": {"dist": 0, "samples": 0}
    }
    total_interval = 0
    param_list = []
    for sensor in get_sensor_df_generator(precomputed_results_path=str(sensor_files_path),
                                          work_type=work_type, in_memory=in_memory):
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
    drop_list = list(["id", "plug_id", "property", "household_id", "house_id"])
    clean_sensor = raw_sensor_df.drop(columns=drop_list, errors='ignore')
    sensor_interpolated = interpolate_df_simple(clean_sensor, desired_data_rate=data_rate)
    if initial_interval == 'nyquist':
        nyq_result = compute_nyquist_and_energy(clean_sensor)
        if not nyq_result[0]:
            return {x: {"dist": -1, "samples": -1} for x in similarity_results}, initial_interval
        initial_interval = nyq_result[3]
    sensor_interpolated.index.name = work_type
    samples = get_single_sensor_sampling_stats(sensor_interpolated, initial_interval=initial_interval)
    for results_key in samples.keys():
        if results_key.endswith("_df"):
            recorded_stream = samples[results_key]
            distance_fastdtw, _ = fastdtw(sensor_interpolated, recorded_stream, dist=2)
            similarity_results[results_key[:-3]]["dist"] = distance_fastdtw
        else:
            similarity_results[results_key]["samples"] = samples[results_key]
    return similarity_results, initial_interval


def get_similarity_stats(sensor_files_path: Path, short_uuid: str, task_pool: Pool, in_memory=False,
                         desired_data_rate='.05s', initial_interval=4) -> Dict[str, Dict[str, Dict[str, Union[Dict[str, float], float]]]]:
    logger.info(f"Distance calculation...")
    results = {"fast_dtw": {}}
    load_fastdtw_results = signal_similarity(sensor_files_path=sensor_files_path, work_type="load",
                                             in_memory=in_memory, desired_data_rate=desired_data_rate,
                                             initial_interval=initial_interval, task_pool=task_pool)
    work_fastdtw_results = signal_similarity(sensor_files_path=sensor_files_path, work_type="work",
                                             in_memory=in_memory, desired_data_rate=desired_data_rate,
                                             initial_interval=initial_interval, task_pool=task_pool)
    results["fast_dtw"] = {"load": load_fastdtw_results, "work": work_fastdtw_results}
    file_name = f"similarity-results-{short_uuid}-{datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
    with open(sensor_files_path.joinpath(file_name), 'w') as fp:
        json.dump(results, fp)
    return results


def plot_similarity_stats(similarity_stats: Dict[str, Dict[str, Dict[str, float]]],
                          results_path: Path, short_uuid: str, desired_data_rate='.05S'):
    for technique in similarity_stats.keys():
        results = similarity_stats[technique]
        for work_type in results.keys():
            work_type_results = results[work_type]
            x_data = list(work_type_results.keys())
            y_data = list(work_type_results.values())
            plot_max = max(y_data)
            fig = go.Figure(data=go.Bar(x=x_data, y=y_data, name=f"{technique}-{work_type}"))
            title = {'text': "<b>Similarity Using FastDTW</b>", 'x': 0.5, 'xanchor': 'center'}
            fig.update_layout(title=title)
            fig.update_xaxes(title_text="<b>Strategy</b>")
            fig.update_yaxes(title_text=f"<b>Distance from original stream ({technique})</b>", type='log',
                             range=[0, math.ceil(math.log10(plot_max))])
            image_name = f"similarity-{technique}-{work_type}-{desired_data_rate}-{short_uuid}.pdf"
            fig.write_image(results_path.joinpath(image_name))
            time.sleep(2)
            fig.write_image(results_path.joinpath(image_name))


def measure_accuracy(sensor_files_path: Path, per_sensor_df: List[DataFrame], task_pool: Pool,
                     short_uuid: str, nyquist_window=10, query_window='1min',
                     use_filter=True, use_only_filter=False, constrain_analyzer: bool = False,
                     max_oversampling: float = 1., sampling_rate: float = 4.,
                     only_necessary: bool = True):
    file_name = f"qs-smartgrid-{short_uuid}-{datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
    with open(sensor_files_path.joinpath(file_name), 'w', encoding='utf-8') as fp:
        total_results = {}
        strategies = ["fixed", "chameleon"] if only_necessary else ["baseline", "fixed", "chameleon"]
        work_types = {'work': 0, 'load': 1}
        for stream_name, stream_value in work_types.items():
            mape_accuracy = Counter(
                {"sum": 0., "count": 0., "mean": 0., "median": 0., "max": 0., "min": 0., "std": 0., "var": 0.})
            mape_accuracies = {k: mape_accuracy.copy() for k in strategies}
            max_accuracies = {k: mape_accuracy.copy() for k in strategies}
            samples = {k: 0 for k in strategies}
            param_list = []
            for sensor in per_sensor_df:
                param_list.append((sensor, stream_value, nyquist_window, use_filter, use_only_filter, query_window, constrain_analyzer, max_oversampling, sampling_rate,))
            promises = [task_pool.apply_async(calculate_agg_accuracy, (params)) for params in param_list]
            aggregation_results = [res.get() for res in promises]
            for result in filter(None, aggregation_results):
                for approach, stats in result.items():
                    mape = Counter(stats["mape"])
                    mape_accuracies[approach] += mape
                    samples[approach] += stats["samples"]
                    max = max_accuracies[approach] | Counter(stats["max"])
                    for key, max_val in max.items():
                        max_accuracies[approach][key] = max_val
            logger.info(
                f"Stream: {stream_name}, len: {len(aggregation_results)}")
            total_results[stream_name] = \
                {"accuracy": dict(mape_accuracies), "samples": dict(samples),
                 "nyq_size": nyquist_window, "q_window": query_window, "sampling_rate": sampling_rate}
        total_results["query_window"] = query_window
        json.dump(total_results, fp, ensure_ascii=False, indent=4, cls=NpEncoder)
        logger.info(f"Wrote data in {sensor_files_path.joinpath(file_name)}")


def calculate_agg_accuracy(sensor_df: DataFrame, stream_type: int, nyquist_window=10, use_filter=True,
                           use_only_filter=False, query_window='1min', constrain_analyzer: bool = False,
                           max_oversampling: float = 1., sampling_rate: Union[float, str] = 4., only_necessary=True) -> Optional[Dict[str, Dict[str, Dict[str, float]]]]:
    filtered_df = sensor_df[sensor_df.property == stream_type]
    filtered_df = filtered_df[~filtered_df.index.duplicated(keep='last')]
    interpolated_df = interpolate_df(sensor_df=filtered_df)
    samples = get_single_sensor_sampling_stats(sensor_df=interpolated_df, value_type=stream_type,
                                               nyquist_window=nyquist_window, use_filter=use_filter,
                                               use_only_filter=use_only_filter, constrain_analyzer=constrain_analyzer,
                                               max_oversampling=max_oversampling, initial_interval=sampling_rate,
                                               further_interpolate=False)
    results = {"baseline": None, 'fixed': None, 'chameleon': None}
    if only_necessary:
        results.pop("baseline")
    for result_name in results.keys():
        results[result_name] = calculate_agg_accuracy_windowed(filtered_df,
                                                               samples[result_name + "_df"],
                                                               query_window=query_window,
                                                               sampler_rate=sampling_rate)
        results[result_name]["samples"] = samples[result_name]
    return results


def plot_fft(big_file_path: Path, sensor_files_path: Path, task_pool: Pool, in_memory=True):
    """
    Consolidate power calculations for all signals.
    From https://stackoverflow.com/questions/29429733/cant-find-the-right-energy-using-scipy-signal-welch
    """
    dfs = split_data_by_sensor(big_file_path=big_file_path, split_sensor_path=sensor_files_path,
                               in_memory=in_memory, task_pool=task_pool)
    logger.info(f"Computing FFT and energy levels per sensor...")
    for workload, sensor_df_list in dfs.items():
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


def pareto_calc(sensor_files_path: Path, current_uuid: str, in_memory: bool,
                task_pool: Pool, data_rate='.01S'):
    pareto_results = {}
    sampler_rates = [.05, .1, 1/9, .2, .3, .5, 1, 2, 3, 5, 9, 10, 20, 'nyquist']  # in seconds
    for sampler_rate in sampler_rates:
        stats = get_similarity_stats(sensor_files_path=sensor_files_path, short_uuid=current_uuid,
                                     in_memory=in_memory, desired_data_rate=data_rate,
                                     initial_interval=sampler_rate, task_pool=task_pool)
        pareto_results[str(sampler_rate)] = stats
        logger.info(f"Stats for {sampler_rate} (s): {stats}")
    file_name = f"pareto-smartgrid-{current_uuid}-{datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
    with open(sensor_files_path.joinpath(file_name), 'w', encoding='utf-8') as f:
        json.dump(pareto_results, f, ensure_ascii=False, indent=4)
        logger.info(f"Wrote data in {sensor_files_path.joinpath(file_name)}")


def prepare_parser(io_conf: IOConfiguration) -> argparse.ArgumentParser:
    cli_parser = argparse.ArgumentParser()
    cli_parser.add_argument("--smartgrid_path", help="The root for the whole experiment",
                            default=io_conf.base_path, type=str)
    cli_parser.add_argument("--results_path", help="The path for resulting artifacts",
                            default="smartgrid-by-sensor", type=str)
    cli_parser.add_argument("--skip_download", help="Skip downloading file", action='store_true')
    cli_parser.set_defaults(skip_download=False)
    cli_parser.add_argument("--skip_move", help="Skip moving/renaming the main file", action='store_true')
    cli_parser.set_defaults(skip_move=False)
    cli_parser.add_argument("--skip_splitting", help="Skip splitting file", action='store_true')
    cli_parser.set_defaults(skip_splitting=False)
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
    cli_parser.add_argument("--skip_similarity", help="Skip downloading file", action='store_true')
    cli_parser.set_defaults(skip_similarity=False)
    cli_parser.add_argument("--perform_dtw_similarity", help="Perform slow DTW calculations", action='store_true')
    cli_parser.set_defaults(perform_dtw_similarity=False)
    cli_parser.add_argument("--in_memory", help="Do not write intermediate files", action='store_true')
    cli_parser.set_defaults(in_memory=False)
    cli_parser.add_argument("--log_level", help="The log_level as string", type=str, default="info", required=False)
    cli_parser.add_argument("--sampling_range", help="The allowed sampling range", type=int, required=False)
    cli_parser.set_defaults(sampling_range=8000)
    cli_parser.add_argument("--fusion", help="Do KF with fusion", action='store_true')
    cli_parser.set_defaults(fusion=False)
    cli_parser.add_argument("--fusion_comparison", help="Do KF fusion with many selectivities", action='store_true')
    cli_parser.set_defaults(fusion_comparison=False)
    cli_parser.add_argument("--plot_fft", help="Prepare FFT over/undersampling plot", action='store_true')
    cli_parser.set_defaults(plot_fft=False)
    cli_parser.add_argument("--calc_pareto", help="Calculate Pareto front", action='store_true')
    cli_parser.set_defaults(calc_pareto=False)
    cli_parser.add_argument("--measure_accuracy", help="Bench accuracy on agg. queries", action='store_true')
    cli_parser.set_defaults(measure_accuracy=False)
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
    results = {"base_path_str": args.smartgrid_path,
               "split_sensor_path": args.results_path,
               "skip_download": args.skip_download,
               "skip_move": args.skip_move,
               "skip_splitting": args.skip_move,
               "sampling": args.sampling,
               "nyquist_window_size": args.nyquist_window_size,
               "query_window_size": args.query_window_size,
               "constrain_analyzer": args.constrain_analyzer,
               "max_oversampling": args.max_oversampling,
               "sampling_rate": args.sampling_rate,
               "skip_similarity": args.skip_similarity,
               "in_memory": args.in_memory,
               "log_level": args.log_level,
               "perform_dtw_similarity": args.perform_dtw_similarity,
               "sampling_range": args.sampling_range,
               "fusion": args.fusion,
               "fusion_comparison": args.fusion_comparison,
               "plot_fft": args.plot_fft,
               "calc_pareto": args.calc_pareto,
               "use_filter": not args.use_nyquist_only,
               "use_only_filter": args.use_only_filter,
               "query_similarity": args.query_similarity,
               "scalability": args.scalability,
               "measure_accuracy": args.measure_accuracy}
    return results


if __name__ == '__main__':
    # prepare default IO conf
    io_conf = IOConfiguration(raw_file=SmartgridIOConfiguration.RAW_FILE.value,
                               small_file=None,
                               raw_file_download_name=SmartgridIOConfiguration.RAW_FILE.value,
                               results_dir='smartgrid',
                               base_path="/home/digiou/test-results",
                               origin=SmartgridIOConfiguration.ORIGIN.value)

    # parse CLI parameters
    parsed_args = get_parser_args(prepare_parser(io_conf))

    # prepare logger
    logger = prepare_logger(parsed_args["log_level"])

    # create UUID for run
    current_uuid = shortuuid.uuid()

    # add task pool that uses physical cores only as context
    with Pool(processes=get_physical_cores()) as task_pool:

        sensor_files_path = Path(parsed_args["base_path_str"]).joinpath(io_conf.results_dir)

        # split df to the level of single unique sensors
        logger.info("Start splitting data...")
        dfs = split_data_by_sensor(big_file_path=sensor_files_path.joinpath(io_conf.raw_file))
        logger.info("Done splitting data!")

        # per-sensor sampling stats
        if parsed_args["sampling"]:
            sampling_stats = get_sampling_stats(sensor_files_path=sensor_files_path,
                                                per_sensor_df=dfs,
                                                task_pool=task_pool, short_uuid=current_uuid,
                                                nyquist_window=parsed_args["nyquist_window_size"],
                                                use_filter=parsed_args["use_filter"],
                                                use_only_filter=parsed_args["use_only_filter"],
                                                constrain_analyzer=parsed_args["constrain_analyzer"],
                                                max_oversampling=parsed_args["max_oversampling"],
                                                sampling_rate=parsed_args["sampling_rate"])

        # comparison of different fusion selectivities
        if parsed_args["fusion_comparison"]:
            fusion_selectivities = [.001, .01, .1, .5, 1.]
            fused_results = {}
            dfs = split_data_by_household(big_file_path=big_file_path)
            for selectivity in fusion_selectivities:
                logger.info(f"Calculating samples for fusion selectivity of {selectivity}...")
                fused_results[str(selectivity)] = get_fused_sampling_stats(sorted_dfs=dfs, task_pool=task_pool,
                                                                           fusion_selectivity=selectivity)
            plot_fusion_comparison(selectivity_results=fused_results, short_uuid=current_uuid,
                                   results_path=sensor_files_path)
        # extract and plot FFT energy readings from individual ts
        if parsed_args["plot_fft"]:
            plot_fft(big_file_path=big_file_path, sensor_files_path=sensor_files_path, task_pool=task_pool)

        if parsed_args["query_similarity"]:
            measure_accuracy(sensor_files_path=sensor_files_path, per_sensor_df=dfs,
                             short_uuid=current_uuid, task_pool=task_pool,
                             nyquist_window=parsed_args["nyquist_window_size"],
                             query_window=parsed_args["query_window_size"],
                             use_filter=parsed_args["use_filter"],
                             use_only_filter=parsed_args["use_only_filter"],
                             constrain_analyzer=parsed_args["constrain_analyzer"],
                             max_oversampling=parsed_args["max_oversampling"],
                             sampling_rate=parsed_args["sampling_rate"])

        if parsed_args["scalability"]:
            get_sampling_stats(sensor_files_path=sensor_files_path,
                               per_sensor_df=dfs,
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
