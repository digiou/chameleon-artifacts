from collections import Counter
import datetime
import json
from multiprocessing import Pool
from pathlib import Path
from typing import Dict, Union, List, Callable
from pandas import DataFrame
import pandas as pd
import numpy as np


from ..io.serialization import NpEncoder


def calculate_agg_accuracy_windowed(original_df: DataFrame, sampled_df: DataFrame, query_window: str,
                                    sampler_rate: Union[str, float]) -> Dict[str, Dict[str, float]]:
    agg_map = {"value": ["sum", "count", "mean", "median", "max", "min", "std", "var"]}
    agg_original = original_df.groupby(pd.Grouper(level=0, freq=query_window, sort=True)).agg(agg_map)
    agg_sampled = sampled_df.groupby(pd.Grouper(level=0, freq=query_window, sort=True)).agg(agg_map)
    # rolling_original = original_df.rolling(window=window_freq).agg(agg_map)
    # rolling_sampled = sampled_df.rolling(window=window_freq).agg(agg_map)
    res = {"mape": {}.fromkeys(agg_map["value"], 0.), "max": {}.fromkeys(agg_map["value"], 0.)}
    for agg_type in agg_map["value"]:
        max_err = float('-inf')
        total_err = 0
        total = 0
        for idx, val in agg_original["value"][agg_type].items():
            val_sampled = agg_sampled["value"][agg_type].get(idx)
            if val_sampled is None:
                total_err += (np.abs(val) * 100)
                max_err = max(max_err, (np.abs(val) * 100))
            else:
                if np.abs(val) == np.inf or np.abs(val_sampled) == np.inf:
                    continue
                if np.isnan(val) or np.isnan(val_sampled):
                    continue
                if val != 0:
                    total_err += (np.abs((val - val_sampled) / val) * 100)
                    max_err = max(max_err, (np.abs((val - val_sampled) / val) * 100))
                else:
                    total_err += (np.abs(val_sampled) * 100)
                    max_err = max(max_err, (np.abs(val_sampled) * 100))
            total += 1
        if total == 0:
            res["max"][agg_type] = 100
            res["mape"][agg_type] = 100
        else:
            res["max"][agg_type] = max_err
            res["mape"][agg_type] = (1/total) * total_err
    rounded_res = {
        outer_key: {inner_key: round(value, 2) for inner_key, value in inner_dict.items()}
        for outer_key, inner_dict in res.items()
    }
    return rounded_res


def measure_accuracy_fn(sensor_dfs: List[DataFrame], col_names: List[str], sensor_files_path: Path,
                     current_uuid: str, task_pool: Pool, calculate_agg_accuracy: Callable, dataset: str,
                     nyquist_window=10, use_filter=True, only_necessary: bool = True):
    total_results = {}
    strategies = ["fixed", "chameleon"] if only_necessary else ["baseline", "fixed", "chameleon"]
    file_name = f"accuracy-{dataset}-{current_uuid}-{datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S')}.json"
    with open(sensor_files_path.joinpath(file_name), 'w', encoding='utf-8') as fp:
        for stream_type in col_names:
            mape_accuracy = Counter({"sum": 0., "count": 0., "mean": 0., "median": 0., "max": 0., "min": 0., "std": 0., "var": 0.})
            mape_accuracies = {k: mape_accuracy.copy() for k in strategies}
            max_accuracies = {k: mape_accuracy.copy() for k in strategies}
            promises = [task_pool.apply_async(calculate_agg_accuracy,
                        (raw_sensor, stream_type, nyquist_window, use_filter,)) for raw_sensor in sensor_dfs]
            aggregation_results = [res.get() for res in promises]
            for result in filter(None, aggregation_results):
                for approach, stats in result.items():
                    mape = Counter(stats["mape"])
                    mape_accuracies[approach] += mape
                    max = max_accuracies[approach] | Counter(stats["max"])
                    for key, max_val in max.items():
                        max_accuracies[approach][key] = max_val
            total_results[stream_type] = \
                {"accuracy": dict(mape_accuracies), "max_accuracy": dict(max_accuracies)}
        json.dump(total_results, fp, ensure_ascii=False, indent=4, cls=NpEncoder)
