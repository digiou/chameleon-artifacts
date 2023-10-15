import glob
import json
import os
from random import uniform
from pathlib import Path


def measure_scalability(sensor_files_path: Path, variance_ratio: float=5., iterations: int=100):
    search_str = str(sensor_files_path.joinpath(f"sampling-results*.json"))
    matching_file = glob.glob(search_str)
    latest_file = max(matching_file, key=os.path.getctime)
    with open(sensor_files_path.joinpath(latest_file), "r") as file:
        data = json.load(file)
    for stream_name, stream_info in data.items():
        total_new_sensors = 0
        data[stream_name]["stats"]["max_additional_sensors"] = 0
        data[stream_name]["stats"]["min_additional_sensors"] = float('inf')
        data[stream_name]["stats"]["min_additional_sensors_pcnt"] = 0
        data[stream_name]["stats"]["max_additional_sensors_pcnt"] = 0
        for iter_cnt in range(iterations):
            if data[stream_name]["fixed"] <= data[stream_name]["chameleon"]:
                continue
            additional_sensors = 0
            initial_size = data[stream_name]["chameleon"]
            mean_size = data[stream_name]["stats"]["mean_size"]
            while initial_size < data[stream_name]["fixed"]:
                num_samples_changed = int(mean_size * (1 + uniform(-variance_ratio, variance_ratio) / 100))
                initial_size += num_samples_changed
                additional_sensors += 1
            total_new_sensors += additional_sensors
            data[stream_name]["stats"]["max_additional_sensors"] = max(
                data[stream_name]["stats"]["max_additional_sensors"], additional_sensors)
            data[stream_name]["stats"]["min_additional_sensors"] = min(
                data[stream_name]["stats"]["min_additional_sensors"], additional_sensors)
        data[stream_name]["stats"]["avg_additional_sensors"] = total_new_sensors / iterations
        data[stream_name]["stats"]["min_additional_sensors_pcnt"] = \
            (data[stream_name]["stats"]["min_additional_sensors"] / data[stream_name]["stats"]["sensor_num"]) / 100
        data[stream_name]["stats"]["max_additional_sensors_pcnt"] = \
            (data[stream_name]["stats"]["max_additional_sensors"] / data[stream_name]["stats"]["sensor_num"]) / 100
    with open(latest_file, "wt") as res_file:
        json.dump(data, res_file, indent=4)
    return data
