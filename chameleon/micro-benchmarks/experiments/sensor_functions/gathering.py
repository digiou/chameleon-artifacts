from datetime import timedelta
from typing import List

import numpy as np
import pandas as pd
from pandas import DataFrame

from .interpolation import interpolate_two_dfs
from ..models.kalman_filters import IntervalKalmanFilter, FusedKalmanFilter


def gather_values_from_df(original_df: DataFrame, initial_interval: int, strategy: IntervalKalmanFilter,
                          gathering_interval_range=8, artificial_cuttoff=False) -> DataFrame:
    observation_list = []
    delta = timedelta(seconds=initial_interval)
    delta_millis = timedelta(milliseconds=0)
    first_timestamp = original_df.head(1).index[0]
    current_timestamp = first_timestamp
    old_timestamp = first_timestamp
    last_timestamp = original_df.tail(1).index[0]
    if strategy is not None:
        strategy.set_gathering_interval_range(gathering_interval_range)
        delta = timedelta(seconds=strategy.get_gathering_interval_seconds())

    while current_timestamp < last_timestamp:
        values = original_df[old_timestamp:current_timestamp]
        if values.empty:  # when no interpolation in the original 1s stream
            values = original_df[:current_timestamp + delta_millis]
        single_val = values.iloc[-1]["value"]
        observation_list.append({"timestamp": current_timestamp,
                                 "value": single_val})
        if strategy is not None:
            strategy.predict_and_update(current_timestamp, single_val)
            new_interval = strategy.get_new_gathering_interval()
            delta = timedelta(seconds=int(new_interval))
            delta_millis = timedelta(milliseconds=int((new_interval - np.fix(new_interval)) * 1000))
        old_timestamp = current_timestamp
        current_timestamp = current_timestamp + delta + delta_millis

        if artificial_cuttoff and len(observation_list) >= len(original_df):
            break
    observation_df = pd.DataFrame(observation_list, columns=["timestamp", "value"])
    observation_df.set_index("timestamp", inplace=True)
    observation_df.index.name = "fixed" if strategy is None else strategy.get_name()
    return observation_df


def get_fused_vals_with_ts(original_df_list: List[DataFrame], current_items: np.ndarray,
                           old_ts, new_ts):
    assert np.shape(current_items)[0] == len(original_df_list)
    idx = 0
    for each_df in original_df_list:
        time_slice = each_df[old_ts:new_ts]
        try:
            current_items[idx][0] = time_slice.iloc[-1]["value"]
        except IndexError:
            # TODO: iloc[-1] fails here, investigate
            current_items[idx][0] = 0
        idx += 1


def gather_fused_dfs(original_df_list: List[DataFrame], initial_interval: int,
                     gathering_interval_range=8, artificial_cuttoff=False) -> DataFrame:
    observation_list = []
    delta = timedelta(seconds=initial_interval)
    delta_millis = timedelta(milliseconds=0)
    first_timestamp = min([df.index.min() for df in original_df_list])
    current_timestamp = old_timestamp = first_timestamp
    last_timestamp = max([df.index.max() for df in original_df_list])
    current_observations = np.zeros((len(original_df_list), 1), dtype=np.float)
    max_len = max([len(df) for df in original_df_list])
    original_name = "+".join([df.index.name for df in original_df_list])

    # do initialization from first values
    original_df_list = interpolate_two_dfs(original_df_list)
    get_fused_vals_with_ts(original_df_list, current_observations,
                           old_timestamp, current_timestamp + delta + delta_millis)
    fused_filter = FusedKalmanFilter(current_observations)  # init and prepare filter for 1st time
    fused_filter.set_gathering_interval(initial_interval * 1000)
    fused_filter.set_gathering_interval_range(gathering_interval_range)
    observation_list.append({"timestamp": current_timestamp,
                             "value": fused_filter.x[-1]})  # filtered val.
    old_timestamp = current_timestamp
    current_timestamp = current_timestamp + delta + delta_millis  # advance first-read idxs

    while current_timestamp < last_timestamp:
        get_fused_vals_with_ts(original_df_list, current_observations,
                               old_timestamp, current_timestamp)
        fused_filter.predict_and_update(current_observations)
        observation_list.append({"timestamp": current_timestamp,
                                 "value": fused_filter.x[-1]})  # filtered val.
        new_interval = fused_filter.get_new_gathering_interval()
        delta = timedelta(seconds=int(new_interval))
        delta_millis = timedelta(milliseconds=int((new_interval - np.fix(new_interval)) * 1000))
        old_timestamp = current_timestamp
        current_timestamp = current_timestamp + delta + delta_millis

        if artificial_cuttoff and len(observation_list) >= max_len:
            break
    observation_df = pd.DataFrame(observation_list, columns=["timestamp", "value"])
    observation_df.set_index("timestamp", inplace=True)
    observation_df.index.name = original_name
    return observation_df
