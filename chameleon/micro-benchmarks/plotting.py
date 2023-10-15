import datetime
import json
import os
import time

import numpy as np
import pandas as pd

from glob import glob
from pathlib import Path
from typing import List, Dict

import plotly.graph_objects as go
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from wfdb.io import rdrecord, rdheader
from pandas import DataFrame

from experiments.plotting.pareto import front_plot

def smartgrid_input_size_comparison():
    fig = go.Figure()
    x_names = ['<b>Dual KF</b>', '<b>Fixed Rate</b>', '<b>Chameleon</b>']

    fig.add_trace(go.Bar(
        name='1 GB',
        x=x_names, y=[8871497, 8860540, 5939652],
        error_y=dict(type='data', array=[354859, 0, 178189])
    ))

    fig.add_trace(go.Bar(
        name='4 GB',
        x=x_names, y=[59441413, 35442163, 23747258],
        error_y=dict(type='data', array=[1064577, 0, 949890])
    ))

    fig.add_trace(go.Bar(
        name='20 GB',
        x=x_names, y=[297762619, 177210815, 118736292],
        error_y=dict(type='data', array=[11910504, 0, 4749451])
    ))

    fig.add_trace(go.Bar(
        name='40 GB',
        x=x_names, y=[595525241, 354421630, 237472580],
        error_y=dict(type='data', array=[23821009, 0, 9498903])
    ))

    fig.add_trace(go.Bar(
        name='130 GB',
        x=x_names, y=[1815423030, 1063264890, 712417741],
        error_y=dict(type='data', array=[72616921, 0, 28496709])
    ))

    title={
        'text': "<b>SmartGrid Dataset, Dynamic + Idle streams</b>",
        'x': 0.5,
        'xanchor': 'center'}
    fig.update_layout(barmode='group', legend_title_text='<b>Input Size</b>', title=title)
    fig.update_xaxes(title_text="<b>Strategy</b>")
    fig.update_yaxes(title_text="<b>Number of Samples</b>", type='log')
    fig_name = f"sampling-smartgrid-by-size.pdf"
    fig.write_image(fig_name)
    time.sleep(2)
    fig.write_image(fig_name)


def smartgrid_input_size_group_input():
    fig = go.Figure()
    x_names = ['<b>Small (4 GB)</b>', '<b>Medium (40 GB)</b>',
               '<b>Large (130 GB)</b>']

    fig.add_trace(go.Bar(
        name='<b>Dual KF</b>',
        x=x_names, y=[59441413, 595525241, 1815423030],
        error_y=dict(type='data', array=[1064577, 23821009, 72616921])
    ))

    fig.add_trace(go.Bar(
        name='<b>Fixed Rate</b>',
        x=x_names, y=[35442163, 354421630, 1063264890]
    ))

    fig.add_trace(go.Bar(
        name='<b>Chameleon</b>',
        x=x_names, y=[23747258, 237472580, 712417741],
        error_y=dict(type='data', array=[949890, 9498903, 28496709])
    ))

    title={
        'text': "<b>SmartGrid, Dynamic + Idle streams</b>",
        'x': 0.5,
        'xanchor': 'center'}
    fig.update_layout(barmode='group', legend_title_text='<b>Strategy</b>', title=title)
    fig.update_xaxes(title_text="<b>Dataset Size</b>")
    fig.update_yaxes(title_text="<b>Number of Samples</b>", type='log')
    fig_name = f"sampling-smartgrid-by-size.pdf"
    fig.write_image(fig_name)
    time.sleep(2)
    fig.write_image(fig_name)


def smartgrid_input_size_lineplot():
    fig = go.Figure()
    x_names = ['Small (4 GB)', 'Medium (40 GB)',
               'Large (135 GB)']

    fig.add_trace(go.Scatter(
        name='Dual KF', x=x_names, y=[59441413, 590122814, 1993638922],
        error_y=dict(type='data', array=[1064577, 23821009, 72616921])
    ))

    fig.add_trace(go.Scatter(
        name='Fixed Rate', x=x_names, y=[35442163, 360707284, 1306350516]
    ))

    fig.add_trace(go.Scatter(
        name='Chameleon', x=x_names, y=[23747258, 240496641, 870958781],
        error_y=dict(type='data', array=[949890, 9498903, 28496709])
    ))

    # fig.add_trace(go.Scatter(
    #     name='Dataset', x=x_names,
    #     y=[120000000, 1200000000, 4055508721], showlegend=False,
    #     text=['', 'Dataset', ''], mode='lines+markers+text',
    #     textposition='top left', textfont=dict(color='firebrick'),
    #     line=dict(color='firebrick', width=1, dash='dash')
    # ))

    title={
        'text': "<b>SmartGrid Dataset</b>",
        'x': 0.5,
        'y': 0.85,
        'xanchor': 'center'}
    fig.update_layout(barmode='group', legend_title_text='<b>Strategy</b>', title=title)
    fig.update_xaxes(title_text="<b>Dataset Size</b>")
    fig.update_yaxes(title_text="<b>Number of Samples</b>")
    fig_name = f"sampling-smartgrid-by-size-lineplot.pdf"
    fig.write_image(fig_name)
    time.sleep(2)
    fig.write_image(fig_name)


def oversampling_barplot():
    fig = go.Figure()
    x_names = ['Motion', 'Voltage', 'Humidity', 'Light', 'Temperature']
    y_values = [44, 25, 18, 15, 14]
    width_values = [0.7] * len(x_names)

    fig.add_trace(go.Bar(
        name='Measurement', x=x_names, y=y_values, width=width_values,
        text=[str(y_val) + 'x' for y_val in y_values], textposition="outside",
        marker={"color": y_values, "colorscale": "viridis"}
    ))

    fig.update_layout(barmode='group', bargap=0.15, xaxis_tickangle=-45)
    fig.update_yaxes(title_text="<b>Oversampling Ratio</b>")
    fig_name = f"oversampling-barplot.pdf"
    fig.write_image(fig_name)
    time.sleep(2)
    fig.write_image(fig_name)


def smartgrid_dtw_similarity():
    fig = go.Figure()
    x_names = ['<b>Dynamic</b>', '<b>Idle</b>']

    baseline_y_data = [892, 277]
    fig.add_trace(go.Bar(
        name='Dual KF',
        x=x_names, y=baseline_y_data,
        error_y=dict(type='data', array=[12, 10]),
        text=baseline_y_data, textposition='outside'
    ))

    fixed_y_data = [124, 102]
    fig.add_trace(go.Bar(
        name='Fixed Rate',
        x=x_names, y=fixed_y_data,
        text=fixed_y_data, textposition='outside'
    ))

    chameleon_y_data = [68, 55]
    fig.add_trace(go.Bar(
        name='Chameleon',
        x=x_names, y=chameleon_y_data,
        error_y=dict(type='data', array=[2, 1]),
        text=chameleon_y_data, textposition='outside'
    ))

    title={
        'text': "<b>SmartGrid DataSet</b>",
        'x': 0.5,
        'y': 0.85,
        'xanchor': 'center'}
    fig.update_layout(barmode='group', legend_title_text='<b>Strategy</b>', title=title)
    fig.update_xaxes(title_text="<b>Sub-Dataset</b>")
    fig.update_yaxes(title_text="<b>DTW Distance</b>")
    fig_name = f"similarity-smartgrid.pdf"
    fig.write_image(fig_name)
    time.sleep(2)
    fig.write_image(fig_name)


def intel_lab_sampling():
    fig = go.Figure()
    x_names = ['Temperature', 'Humidity', 'Light', 'Voltage']

    baseline_y_data = [286521687, 282574778, 282607804, 277221898]
    fig.add_trace(go.Bar(
        name='Dual KF',
        x=x_names, y=baseline_y_data,
        error_y=dict(type='data', array=[1146086, 1130299, 1130431, 1108887]),
        text=baseline_y_data, textposition='outside'
    ))

    fixed_y_data = [35943967, 35451828, 35451828, 34750018]
    fig.add_trace(go.Bar(
        name='Fixed Rate',
        x=x_names, y=fixed_y_data,
        text=fixed_y_data, textposition='outside'
    ))

    chameleon_y_data = [24003102, 23647779, 23854749, 23167479]
    fig.add_trace(go.Bar(
        name='Chameleon',
        x=x_names, y=[24003102, 23647779, 23854749, 23167479],
        error_y=dict(type='data', array=[945911, 945911, 954189, 926699]),
        text=chameleon_y_data, textposition='outside'
    ))

    title={
        'text': "<b>Intel Lab Dataset</b>",
        'x': 0.5,
        'y': 0.85,
        'xanchor': 'center'}
    fig.update_layout(barmode='group', legend_title_text='<b>Strategy</b>', title=title)
    fig.update_traces(texttemplate='%{text:.2s}', textposition='outside')
    fig.update_xaxes(title_text="<b>Stream Name</b>")
    fig.update_yaxes(title_text="<b>Number of Samples</b>")
    fig_name = f"sampling-intel.pdf"
    fig.write_image(fig_name)
    time.sleep(2)
    fig.write_image(fig_name)


def link_saturation():
    fig = go.Figure()
    x_names = ['<b>4s</b>', '<b>2s</b>', '<b>1s</b>', '<b>0.5s</b>']
    fig.add_trace(go.Bar(
        name='Sampling Rates',
        x=x_names, y=[0.1, 0.224, 0.5, 1]
    ))
    fig.add_trace(go.Scatter(x=x_names, y=[0.5, 0.5, 0.5, 0.5], name='Max. Link Bandwidth', mode='lines',
                             line=dict(color='firebrick', width=2, dash='dash')))
    fig.update_xaxes(title_text="<b>Sampling Rate</b>")
    fig.update_yaxes(title_text="<b>Bandwidth % of Gateway</b>")
    fig_name = f"saturating-link.pdf"
    fig.write_image(fig_name)
    time.sleep(2)
    fig.write_image(fig_name)


def link_saturation_number_nodes():
    fig = go.Figure()
    x_names = ['<b>10 node</b>', '<b>50 nodes</b>', '<b>130 nodes</b>']
    fig.add_trace(go.Bar(
        name='Num. of LoRaWaN Nodes',
        x=x_names, y=[10.2, 60, 160], showlegend=False
    ))
    fig.add_trace(go.Scatter(x=x_names, y=[144, 144, 144], name='Max. Gateway Bandwidth', mode='lines+text',
                             line=dict(color='firebrick', width=1, dash='dot'),
                             showlegend=False,
                             text=['', 'Limit: 120 nodes @ 1.2 Bytes/min', ''],
                             textposition='top center',
                             textfont=dict(color='firebrick')))
    title={
        'text': "<b>LoRaWaN Limits</b>",
        'x': 0.5,
        'y': 0.85,
        'xanchor': 'center'}
    fig.update_xaxes(title_text="<b>Concurrent LoRaWaN Nodes</b>")
    fig.update_yaxes(title_text="<b>Gateway Usage [B/min]</b>")
    fig.update_layout(title=title)
    fig_name = f"saturating-link-number-nodes.pdf"
    fig.write_image(fig_name)
    time.sleep(2)
    fig.write_image(fig_name)


def similarity_vs_input_size():
    fig = go.Figure()
    x_names = ['20k', '40k', '80k', '120k', '160k', '200k', '300k', '400k', '800k', '1M', '2M', '20M', '40M']

    fig.add_trace(go.Scatter(
        name='Dual KF', x=x_names, y=[54, 121, 291, 481, 631, 830, 1247, 1623, 3485, 4436, 8906, 3149378, 5350104]
    ))

    fig.add_trace(go.Scatter(
        name='Fixed Rate', x=x_names, y=[55, 154, 309, 511, 707, 899, 1328, 1850, 3941, 4979, 10454, 65730, 145932]
    ))

    fig.add_trace(go.Scatter(
        name='Chameleon', x=x_names, y=[44, 113, 232, 430, 630, 818, 1345, 1891, 4558, 5430, 11585, 73981, 129828]
    ))

    title={
        'text': "<b>SmartGrid Dataset</b>",
        'x': 0.5,
        'y': 0.85,
        'xanchor': 'center'}
    fig.update_layout(barmode='group', legend_title_text='<b>Strategy</b>', title=title)
    fig.update_xaxes(title_text="<b>Dataset Size</b>")
    fig.update_yaxes(title_text="<b>FastDTW distance</b>")
    fig_name = f"similarity-by-input-size-lineplot.pdf"
    fig.write_image(fig_name)
    time.sleep(2)
    fig.write_image(fig_name)


def pareto_front_similarity_vs_freq_smartgrid(path_name="/home/digiou/test-results/smartgrid/smartgrid-by-sensor"):
    file_list = glob(f"{path_name}/*.json")
    latest_file = max(file_list, key=os.path.getctime)
    with open(latest_file) as f:
        json_repr = json.load(f)
        load_rates = []
        work_rates = []
        load_list = []
        work_list = []
        for rate, freq_results in json_repr.items():
            for _, workload_results in freq_results.items():
                for workload, strategy_results in workload_results.items():
                    if workload == 'load':
                        load_list.append(strategy_results['fixed']['dist'])
                        if rate == 'nyquist':
                            load_rates.append(1/strategy_results["used_interval"])
                    else:
                        work_list.append(strategy_results['fixed']['dist'])
                        if rate == 'nyquist':
                            work_rates.append(1/strategy_results["used_interval"])
            if rate != 'nyquist':
                load_rates.append(1/float(rate))
                work_rates.append(1/float(rate))
        print(f"Rates (msg/s): {load_rates}, dist: {load_list}")
        # front_plot(Xs=load_rates, Ys=load_list, title="Load", current_uuid="", save_path=path_name)
        print(f"Rates (msg/s): {work_rates}, dist: {work_list}")
        front_plot(Xs=work_rates, Ys=work_list, title="Work", current_uuid="", save_path=path_name)


def get_arrhythmia_dataframe(dataset_dirs=Path("/home/dimitrios/projects/test-results/")) -> List[DataFrame]:
    # interesting: 876
    arrhythmia_str = str(dataset_dirs.joinpath("arrhythmia")
                     .joinpath("mit-bih-supraventricular-arrhythmia-database-1.0.0").joinpath("876.hea"))
    arrhythmia_files = []
    for arrhythmia_file in glob(arrhythmia_str, recursive=True):
        file_no_extension = os.path.splitext(arrhythmia_file)[0]
        records = rdrecord(file_no_extension)
        records.base_datetime = datetime.datetime.now()
        records_df = records.to_dataframe()
        for sub_df_col in ['ECG1', 'ECG2']:
            sub_df = records_df[[sub_df_col]]
            sub_df.rename(columns={sub_df_col: 'value'}, inplace=True)
            sub_df.index.name = f"{sub_df_col.lower()}"
            arrhythmia_files.append(sub_df.copy(deep=True))
    return arrhythmia_files


def get_smartgrid_dataframe(dataset_dir=Path("/home/dimitrios/projects/test-results/smartgrid-by-sensor/")) -> List[DataFrame]:
    # interesting: s5-hh2-h18
    useful_cols = ['timestamp', 'value', 'property', 'plug_id', 'household_id', 'house_id']
    smartgrid_files = []
    smartgrid_str = str(dataset_dir.joinpath("total").joinpath("s5-hh2-h18.csv"))
    for smartgrid_file in glob(smartgrid_str):
        read_csv_options = {'filepath_or_buffer': smartgrid_file, 'encoding': 'utf-8-sig', 'header': None, 'index_col': None,
                            'usecols': useful_cols, 'names': useful_cols}
        raw_dataframe = pd.read_csv(**read_csv_options)
        raw_dataframe.index = pd.to_datetime(raw_dataframe['timestamp'], unit='s', utc=True)
        raw_dataframe.index = raw_dataframe.index.tz_convert('Europe/Berlin')
        raw_dataframe.drop('timestamp', axis=1, inplace=True, errors='ignore')
        raw_dataframe.drop(['plug_id', 'household_id', 'house_id'], axis=1, inplace=True, errors='ignore')
        work_df = raw_dataframe[raw_dataframe['property'] == 0]
        work_df = work_df[~work_df.index.duplicated(keep='last')]
        work_df.drop(['property'], axis=1, inplace=True, errors='ignore')
        work_df.index.name = f"work"
        load_df = raw_dataframe[raw_dataframe['property'] == 1]
        load_df = load_df[~load_df.index.duplicated(keep='last')]
        load_df.drop(['property'], axis=1, inplace=True, errors='ignore')
        load_df.index.name = f"load"
        smartgrid_files.append(work_df)
        smartgrid_files.append(load_df)
    return smartgrid_files


def get_intel_dataframe(dataset_dir=Path("/home/dimitrios/projects/test-results/intel-by-sensor/data.txt.bk")) -> Dict[str, List[DataFrame]]:
    interesting_streams = {"temperature": [50], "humidity": [50], "light": [43], "voltage": [36]}
    useful_cols = ['date', 'time', 'epoch', 'moteid', 'temperature',
                   'humidity', 'light', 'voltage']
    read_csv_options = {'filepath_or_buffer': str(dataset_dir), 'encoding': 'utf-8-sig',
                        'header': None, 'index_col': None,
                        'sep': '\s+', 'on_bad_lines': 'warn',
                        'names': useful_cols, 'parse_dates': [['date', 'time']]}
    value_dfs = {'temperature': [], 'humidity': [], 'light': [], 'voltage': []}
    necessary_cols = ['epoch', 'moteid']
    raw_dataframe = pd.read_csv(**read_csv_options)
    raw_dataframe.index = pd.to_datetime(raw_dataframe['date_time'], unit='s', utc=True, format='%Y-%m-%d %H:%M:%S.%fZ')
    del raw_dataframe['date_time']
    raw_dataframe.index = raw_dataframe.index.tz_convert('Europe/Berlin')
    raw_dataframe.sort_index(inplace=True)
    raw_dataframe.dropna(subset=['moteid'], inplace=True)
    raw_dataframe[["moteid"]] = raw_dataframe[["moteid"]].astype(int)
    raw_dataframe.dropna(subset=useful_cols[4:], how='all', inplace=True)
    raw_dataframe.drop(raw_dataframe[raw_dataframe.moteid > 58].index, inplace=True)
    for value_key in value_dfs.keys():
        cols_to_keep = necessary_cols + [value_key]  # keep info + val col.
        for interesting_mote in interesting_streams.get(value_key):
            value_df = raw_dataframe[raw_dataframe.moteid == interesting_mote][cols_to_keep]
            value_df.dropna(subset=cols_to_keep[2:], inplace=True)
            if len(value_df) > 0:
                value_df = value_df[~value_df.index.duplicated(keep='last')]
                value_df.drop(necessary_cols, axis=1, inplace=True, errors='ignore')
                value_df.index.name = f"{value_key}"
                value_dfs[value_key].append(value_df)
    return value_dfs


def get_company_dataframes(path: Path=Path("/home/dimitrios/projects/test-results/company-by-sensor")) -> Dict[str, List[DataFrame]]:
    interesting_streams = {"ABS_Front_Wheel_Press": ["0917-619-33"], "ABS_Lean_Angle": ["0917-619-35"]}
    useful_cols = ["Time", "Dist", "ABS_Front_Wheel_Press", "ABS_Rear_Wheel_Press",
                   "ABS_Front_Wheel_Speed", "ABS_Rear_Wheel_Speed", "V_GPS", "MMDD",
                   "HHMM", "LAS_Ax1", "LAS_Ay1", "LAS_Az_Vertical_Acc", "ABS_Lean_Angle",
                   "ABS_Pitch_Info", "ECU_Gear_Position", "ECU_Accel_Position",
                   "ECU_Engine_Rpm", "ECU_Water_Temperature", "ECU_Oil_Temp_Sensor_Data",
                   "ECU_Side_StanD", "Longitude", "Latitude", "Altitude"]
    read_csv_options = {'encoding': 'utf-8-sig', 'header': None, "names": useful_cols,
                        "sep": ";", "decimal": ",", "skiprows": range(0, 2)}
    now_dt = datetime.datetime.now()
    cols = ['Dist', "ABS_Front_Wheel_Press", "ABS_Rear_Wheel_Press", "ABS_Front_Wheel_Speed",
            "ABS_Rear_Wheel_Speed", "ABS_Lean_Angle", "ECU_Water_Temperature", "ECU_Oil_Temp_Sensor_Data",
            "Longitude"]
    per_stream_lists = {stream_name: [] for stream_name in interesting_streams.keys()}
    for interesting_stream, files in interesting_streams.items():
        for file in files:
            search_str = str(path.joinpath("**").joinpath(f"{file}.csv"))
            for filename in glob(search_str, recursive=True):
                read_csv_options['filepath_or_buffer'] = str(filename)
                raw_dataframe = pd.read_csv(**read_csv_options)
                raw_dataframe['Time'] = raw_dataframe['Time'].apply(lambda x: now_dt + pd.to_timedelta(x, unit='s'))
                raw_dataframe.index = pd.to_datetime(raw_dataframe['Time'], unit='s')
                raw_dataframe.drop('Time', axis=1, inplace=True)
                df_view = raw_dataframe[[interesting_stream]]
                df_view.index.name = f"{interesting_stream}"
                df_copy = df_view.copy(deep=True)
                per_stream_lists[interesting_stream].append(df_copy)
    return per_stream_lists


def get_daphnet_dataframes(path: Path=Path("/home/dimitrios/projects/test-results/daphnet")) -> Dict[str, List[DataFrame]]:
    # interesting: ankle_hori_fwd_accel-S04R01
    col_names = ['timestamp', 'ankle_hori_fwd_accel', 'ankle_vertical', 'ankle_hori_lateral',
                 'thigh_hori_fwd_accel', 'thigh_vertical', 'thigh_hori_lateral',
                 'trunk_hori_fwd_accel', 'trunk_vertical', 'trunk_hori_lateral', 'annotation']
    dtype = {col_name: np.int64 for col_name in col_names if col_name != 'timestamp'}
    search_str = str(path.joinpath("**").joinpath(f"S04R01.txt"))
    read_csv_options = {'header': None, 'names': col_names, 'index_col': None,
                        'dtype': dtype, 'sep': " ", 'encoding': 'utf-8-sig'}
    per_stream_dfs = {stream_name: [] for stream_name in col_names[1:-1]}
    now_dt = datetime.datetime.now()
    for filename in glob(search_str, recursive=True):
        read_csv_options['filepath_or_buffer'] = str(filename)
        raw_dataframe = pd.read_csv(**read_csv_options)
        raw_dataframe.dropna(subset=['timestamp'], inplace=True)
        raw_dataframe['timestamp'] = raw_dataframe['timestamp'].apply(lambda x: now_dt + pd.to_timedelta(x, unit='ms'))
        raw_dataframe.index = pd.to_datetime(raw_dataframe['timestamp'], unit='s')
        raw_dataframe.drop('timestamp', axis=1, inplace=True)
        raw_dataframe.index = raw_dataframe.index.tz_localize('Europe/Berlin')
        raw_dataframe.sort_index(inplace=True)
        for col in ['ankle_hori_fwd_accel']:
            view_df = raw_dataframe[[col]]
            copy_df = view_df.copy(deep=True)
            copy_df.index.name = f"{col}"
            per_stream_dfs[col].append(copy_df)
    return per_stream_dfs


def get_sensorscope_dataframes(path: Path=Path("/home/dimitrios/projects/test-results/sensorscope"), data_set: str = "meteo") -> Dict[str, List[DataFrame]]:
    # interesting: ankle_hori_fwd_accel-S04R01
    meteo_non_sequenced = ['fishnet-meteo', 'pdg2008-meteo', 'stbernard-meteo', 'genepi-meteo']
    monitor_non_sequenced = []
    non_sequenced = monitor_non_sequenced
    time_col_names = ['station_id', 'year', 'month', 'day', 'hour', 'minute',
                      'second', 'timestamp', 'sequence_no']
    meteo_measurement_cols = ['ambient_temp', 'surface_temp', 'solar_radiation', 'rel_humidity',
                              'soil_moisture', 'watermark', 'rain_meter', 'wind_speed', 'wind_direction']
    monitor_measurement_cols = ['config_sampling', 'data_sampling', 'radio_cycle', 'radio_tx_pwr', 'radio_tx_frq',
                                'primary_vltg', 'secondary_vltg', 'solar_current', 'global_current', 'source']
    measurement_cols = monitor_measurement_cols
    if data_set == "meteo":
        non_sequenced = meteo_non_sequenced
        measurement_cols = meteo_measurement_cols
    dtype = {**{col_name: 'Int64' for col_name in time_col_names if col_name != 'timestamp'},
             **{col_name: np.float64 for col_name in measurement_cols}}
    per_stream_dfs = {stream_name: [] for stream_name in measurement_cols}
    search_str = str(path.joinpath("**").joinpath(f"*{data_set}*[0-9].txt"))
    for filename in glob(search_str, recursive=True):
        col_names = time_col_names + measurement_cols
        read_csv_options = {'filepath_or_buffer': str(filename), 'header': None, 'names': col_names,
                            'index_col': None, 'dtype': dtype.copy(), 'sep': " ", 'encoding': 'utf-8-sig'}
        for non_sequenced_name in non_sequenced:
            if non_sequenced_name in filename:
                read_csv_options['names'] = time_col_names[:-1] + measurement_cols
                read_csv_options['dtype'].pop('sequence_no')
                break
        raw_dataframe = pd.read_csv(**read_csv_options)
        raw_dataframe.dropna(subset=['timestamp'], inplace=True)
        raw_dataframe['timestamp'] = pd.to_datetime(raw_dataframe['timestamp'], unit='s', utc=True)
        raw_dataframe.set_index('timestamp', drop=True, inplace=True)
        raw_dataframe.index = raw_dataframe.index.tz_convert('Europe/Berlin')
        raw_dataframe = raw_dataframe[~raw_dataframe.index.isna()]
        raw_dataframe = raw_dataframe[~raw_dataframe.index.duplicated(keep='last')]
        start_date = raw_dataframe.index.min()
        if raw_dataframe.index.max() - start_date > pd.Timedelta(days=15):
            try:
                end_date = start_date + pd.DateOffset(days=12)
                raw_dataframe = raw_dataframe[start_date:end_date]
            except KeyError:
                continue
        for measurement_col in measurement_cols:
            raw_dataframe[measurement_col] = raw_dataframe[measurement_col].fillna(method='ffill')
            raw_dataframe[measurement_col] = raw_dataframe[measurement_col].fillna(method='bfill')
        if len(raw_dataframe) == 0:
            continue
        for measurement_col in measurement_cols:
            view_df = raw_dataframe[[measurement_col]]
            copy_df = view_df.copy(deep=True)
            copy_df.index.name = f"{measurement_col}"
            per_stream_dfs[measurement_col].append(copy_df)
    return per_stream_dfs


def get_soccer_dataframes(path: Path=Path("/home/dimitrios/projects/test-results/soccer")) -> Dict[str, List[DataFrame]]:
    file_name = path.joinpath("full-game")
    val_names = ['x', 'y']
    per_stream_dfs = {stream_name: [] for stream_name in val_names}
    col_names = ['sid', 'ts', 'x', 'y', 'z', 'v_abs', 'a_abs',
                 'vx', 'vy', 'vz', 'ax', 'ay', 'az']
    dtype = {col_name: np.int64 for col_name in col_names if col_name != 'ts'}
    read_csv_options = {'filepath_or_buffer': str(file_name), 'header': None, 'names': col_names,
                        'index_col': None, 'dtype': dtype}
    raw_dataframe = pd.read_csv(**read_csv_options)
    raw_dataframe['ts'] = pd.to_datetime(raw_dataframe['ts'], unit='ns', utc=True)
    raw_dataframe.set_index('ts', drop=True, inplace=True)
    referee_ids = [106]
    referee_df = raw_dataframe[raw_dataframe['sid'].isin(referee_ids)]
    referee_df.sort_index(inplace=True)
    for val_name in val_names:
        view_df = raw_dataframe[[val_name]]
        copy_df = view_df.copy(deep=True)
        copy_df.index.name = f"{val_name}"
        per_stream_dfs[val_name].append(copy_df)
    return per_stream_dfs


def create_plot_csvs(csv_results: Path=Path("/home/dimitrios/projects/chameleon-paper/paper/vldb-2022/figures/05-evaluation/example-timeseries")):
    # for stream_name, df_list in get_intel_dataframe().items():
    #     for intel_df in df_list:
    #         to_pdf("Intel Lab", intel_df)
    # for smartgrid_df in get_smartgrid_dataframe():
    #     to_pdf("Smartgrid", smartgrid_df)
    # for stream_name, df_list in get_company_dataframes().items():
    #     for company_df in df_list:
    #         to_pdf("Use Case", company_df)
    for stream_name, df_list in get_daphnet_dataframes().items():
        for daphnet_df in df_list:
            to_pdf("Daphnet", daphnet_df)
    # for stream_name, df_list in get_sensorscope_dataframes().items():
    #     for sensorscope_df in df_list:
    #         to_pdf("Sensorscope", sensorscope_df)
    # for stream_name, df_list in get_soccer_dataframes().items():
    #     for soccer_df in df_list:
    #         to_pdf("DGC13", soccer_df)


def to_pdf(dataset_name: str, df: DataFrame):
    fig, ax = plt.subplots(1, 1)
    ax.plot(df.index, df.values, label=df.index.name)
    ax.set_yticklabels([])
    ax.set_xticklabels([])
    # ax.set_title(dataset_name)
    # fig.autofmt_xdate()
    pdf = PdfPages(f"{dataset_name.lower()}-{df.index.name}.pdf")
    pdf.savefig(fig)
    pdf.close()


if __name__ == "__main__":
    # smartgrid_input_size_comparison()
    # intel_lab_sampling()
    # smartgrid_dtw_similarity()
    # smartgrid_input_size_group_input()
    # link_saturation()
    # link_saturation_number_nodes()
    # smartgrid_input_size_lineplot()
    # similarity_vs_input_size()
    # oversampling_barplot()
    # pareto_front_similarity_vs_freq_smartgrid()
    create_plot_csvs()
