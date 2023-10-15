from typing import List

import pandas as pd
from pandas import DataFrame


def interpolate_df(sensor_df: DataFrame, desired_data_rate='.05S') -> DataFrame:
    """
    Original df is irregular time-series
    """
    new_idx = pd.date_range(sensor_df.index.min(), sensor_df.index.max(), freq=desired_data_rate)
    return sensor_df.reindex(sensor_df.index.union(new_idx)).interpolate('index').reindex(new_idx)


def interpolate_df_simple(sensor_df: DataFrame, desired_data_rate='.05S') -> DataFrame:
    """
    Original df is regular time-series
    """
    if not sensor_df.index.is_unique:
        sensor_df = sensor_df[~sensor_df.index.duplicated(keep='last')]
    return sensor_df.resample(desired_data_rate).interpolate()


def interpolate_two_dfs(df_list: List[DataFrame]):
    """
    Interpolate with each other, make all of them have a common
    index and interpolated values
    """
    if len(df_list) == 2:
        combined_idx = df_list[0].index.union(df_list[1].index)
        df_list[0] = df_list[0].reindex(combined_idx).interpolate(method='time')
        df_list[1] = df_list[1].reindex(combined_idx).interpolate(method='time')
    return df_list
