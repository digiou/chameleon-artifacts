import glob
import os
from pathlib import Path
from typing import Dict

import pandas as pd
from pandas import DataFrame


def load_latency_results(base_path=Path('external-logs').joinpath('e2e-latency'),
                         file_suffix: str = '.out') -> Dict[str, DataFrame]:
    col_names = ['window_start', 'window_end', 'id', 'evt_timestamp', 'timestamp']
    dtype = {col_name: 'Int64' for col_name in col_names}
    read_csv_options = {'header': 0, 'names': col_names, 'index_col': None, 'dtype': dtype, 'encoding': 'utf-8-sig'}
    identifiers = ['fixed', 'adaptive']
    all_dfs = {}
    for identifier in identifiers:
        search_str = str(base_path.joinpath(f"*{identifier}{file_suffix}"))
        print(search_str)
        files = glob.glob(search_str)
        if len(files) > 0:
            latest_file = max(files, key=os.path.getctime)
            read_csv_options['filepath_or_buffer'] = str(latest_file)
            res_df = pd.read_csv(**read_csv_options)
            all_dfs[identifier] = res_df
    return all_dfs


def print_evt_ts_stats(all_dfs: Dict[str, DataFrame], exclude_last_row: bool = False):
    for identifier in all_dfs.keys():
        res_df = all_dfs[identifier]
        if exclude_last_row:
            res_df = res_df.iloc[:-1]  # remove last window that might end earlier
        res_df['difference'] = res_df['window_end'] - res_df['evt_timestamp']
        mean_latency = res_df['difference'].mean()
        print(f"Mean latency for {identifier}: {mean_latency}ms")
