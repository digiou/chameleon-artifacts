from typing import Dict, List, Union

from pandas import DataFrame

from experiments.plotting.formatting import closest_time_unit


def print_data_stats(per_stream_dfs: Dict[str, List[DataFrame]]) -> Dict[str, Dict[str, float]]:
    return_stats = {"total_streams": 0, "total_avg_len": 0}
    for stream_type, stream_sensors in per_stream_dfs.items():
        return_stats["total_streams"] += 1
        max_len = len(stream_sensors[0])
        min_len = len(stream_sensors[0])
        total_len = len(stream_sensors[0])
        max_duration = stream_sensors[0].index[-1] - stream_sensors[0].index[0]
        min_duration = stream_sensors[0].index[-1] - stream_sensors[0].index[0]
        total_duration = stream_sensors[0].index[-1] - stream_sensors[0].index[0]
        max_std = stream_sensors[0][stream_type].std()
        min_std = stream_sensors[0][stream_type].std()
        total_std = stream_sensors[0][stream_type].std()
        max_var = stream_sensors[0][stream_type].var()
        min_var = stream_sensors[0][stream_type].var()
        total_var = stream_sensors[0][stream_type].var()
        for sensor in stream_sensors[1:]:
            max_len = max(max_len, len(sensor))
            min_len = min(min_len, len(sensor))
            total_len += len(sensor)
            max_duration = max(max_duration, sensor.index[-1] - sensor.index[0])
            min_duration = min(min_duration, sensor.index[-1] - sensor.index[0])
            total_duration += (sensor.index[-1] - sensor.index[0])
            max_std = max(max_std, sensor[stream_type].std(ddof=0))
            min_std = min(min_std, sensor[stream_type].std(ddof=0))
            total_std += sensor[stream_type].std()  # TODO: deal with np.nan in var and std
            max_var = max(max_var, sensor[stream_type].var())
            min_var = min(min_var, sensor[stream_type].var())
            total_var += sensor[stream_type].var()
        avg_len = total_len / len(stream_sensors)
        avg_duration = total_duration / len(stream_sensors)
        avg_std = total_std / len(stream_sensors)
        avg_var = total_var / len(stream_sensors)
        return_stats[stream_type] = {
            "max_len": max_len, "min_len": min_len, "avg_len": avg_len,
            "max_duration": max_duration, "min_duration": min_duration,
            "avg_duration": avg_duration, "avg_std": avg_std, "avg_var": avg_var
        }
        return_stats["total_avg_len"] += avg_len
    return_stats["total_avg_len"] /= len(per_stream_dfs.keys())
    return return_stats


def compute_detailed_stats(dataframes: List[DataFrame], col_name: str = 'value') -> Dict[str, float]:
    # Lists to collect statistics from each dataframe
    durations = []
    std_devs = []
    coeffs_of_variation = []

    for df in dataframes:
        # Duration
        durations.append((df.index[-1] - df.index[0]).total_seconds())

        # Standard Deviation
        std_devs.append(df[col_name].std())

        # Coefficient of Variation (Standard Deviation / Mean)
        mean = df[col_name].mean()
        if mean != 0:  # To avoid division by zero
            coeffs_of_variation.append(df[col_name].std() / mean)
        else:
            coeffs_of_variation.append(None)  # Append None if mean is 0

    # Calculate average statistics
    mean_duration = sum(durations) / len(durations)
    avg_std_dev = sum(std_devs) / len(std_devs)
    avg_coeff_of_variation = sum(filter(None, coeffs_of_variation)) / len(coeffs_of_variation)  # filter out None values
    return {"mean_duration": mean_duration, "avg_std_dev": avg_std_dev, "avg_coeff_of_variation": avg_coeff_of_variation}
