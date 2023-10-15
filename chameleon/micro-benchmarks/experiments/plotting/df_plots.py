import math
import time
from os import sep
from pathlib import Path
from typing import List, Dict

import numpy as np
from plotly import graph_objects as go


def create_multiple_stream_scatterplots(df_list: List[any],
                                        path='/home/digiou/test-results/',
                                        write_img=True,
                                        mode="markers+lines") -> go.Figure:
    fig_descriptor = go.Figure()
    initial_mode=mode
    for df in df_list:
        mode=initial_mode
        if "interpolated" in df.index.name:
            mode = "lines"
        scatter_options = {
            "x": df.index,
            "y": df["value"],
            "hoverinfo": 'x+y',
            "mode": mode,
            "name": df.index.name.replace(path, "")
        }
        fig_descriptor.add_trace(
            go.Scatter(**scatter_options)
        )

    fig_descriptor.update_layout(title="<b>Chameleon Behavior Micro-Benchmark -- Single Plug Observations Comparison</b>")
    fig_descriptor.update_layout(legend_title_text="<b>Workload</b>")
    fig_descriptor.update_xaxes(title_text="<b>Observation time (ms)</b>")
    fig_descriptor.update_yaxes(title_text="<b>Wattage (watts)</b>")
    if write_img:
        fig_descriptor.write_image(f"{path}{sep}stream-results.pdf")
        time.sleep(2)  # wait and overwrite graph for mathjax bug
        fig_descriptor.write_image(f"{path}{sep}stream-results.pdf")
    return fig_descriptor


def create_multi_stream_scatterplot_horiz_subfigure(df_list: List[List[any]],
                                              path='/home/digiou/test-results/',
                                              mode='markers') -> go.Figure:

    layout = go.Layout(
        yaxis=dict(
            domain=[0, 0.45]
        ),
        yaxis2=dict(
            domain=[0.55, 1]
        ),
        xaxis2=dict(
            anchor="y2"
        )
    )
    dimension_idx = 0
    data_list = []
    for single_dimension_list in df_list:
        for df in single_dimension_list:
            line_mode = mode
            if "interpolated" in df.index.name:
                line_mode = "marker+lines"
            scatter_options = {
                "x": df.index,
                "y": df["value"],
                "hoverinfo": 'x+y',
                "mode": line_mode,
                "name": df.index.name.replace(path, "")
            }
            if dimension_idx > 0:
                scatter_options['xaxis'] = 'x2'
                scatter_options['yaxis'] = 'y2'
            data_list.append(go.Scatter(**scatter_options))
        dimension_idx += 1

    fig_descriptor = go.Figure(data=data_list, layout=layout)
    fig_descriptor.update_layout(title="<b>Chameleon Behavior Micro-Benchmark -- Initialization (wrong vs right)</b>")
    fig_descriptor.update_layout(legend_title_text="<b>Workload</b>")
    fig_descriptor.update_xaxes(title_text="<b>Observation time (ms)</b>")
    fig_descriptor.update_yaxes(title_text="<b>Wattage (watts)</b>")
    return fig_descriptor


# TODO: merge with previous fn and pass layout as param instead
def create_stream_and_barplot_vertical_subfigure(df_list: Dict[str, List[any]],
                                                 path='/home/digiou/test-results/',
                                                 mode='markers') -> go.Figure:
    layout = go.Layout(
        xaxis=dict(
            domain=[0, 0.75]
        ),
        xaxis2=dict(
            domain=[0.80, 1]
        ),
        yaxis2=dict(
            anchor="x2"
        )
    )
    data_list = []
    for df in df_list['scatter']:
        line_mode = mode
        if "interpolated" in df.index.name:
            line_mode = "marker+lines"
        scatter_options = {
            "x": df.index,
            "y": df["value"],
            "hoverinfo": 'x+y',
            "mode": line_mode,
            "name": df.index.name.replace(path, ""),
            'xaxis': 'x1',
            'yaxis': 'y1'
        }
        data_list.append(go.Scatter(**scatter_options))
    for df in df_list['bar']:
        bar_options = {
            "x": df.index,
            "y": df["value"],
            "hoverinfo": 'x+y',
            "name": df.index.name.replace(path, ""),
            'xaxis': 'x2',
            'yaxis': 'y2'
        }
        data_list.append(go.Bar(**bar_options))
    fig_descriptor = go.Figure(data=data_list, layout=layout)
    fig_descriptor.update_layout(title="<b>Chameleon Behavior Micro-Benchmark -- Plot and # of measurements</b>")
    fig_descriptor.update_layout(legend_title_text="<b>Workload</b>")
    fig_descriptor.update_xaxes(title_text="<b>Observation time (ms)</b>")
    fig_descriptor.update_yaxes(title_text="<b>Wattage (watts)</b>")
    return fig_descriptor


def plot_fused_results(og_signal: np.ndarray,
                       sensed_raw_signal: np.array,
                       results_single: np.ndarray,
                       results_second: np.array,
                       results_fused: np.ndarray) -> go.Figure:
    layout = go.Layout(
        yaxis3=dict(
            domain=[0, 0.3]
        ),
        yaxis2=dict(
            domain=[0.33, 0.63]
        ),
        yaxis=dict(
            domain=[0.66, 1],
        ),
        xaxis2=dict(
            anchor="y2"
        ),
        xaxis3=dict(
            anchor="y3"
        )
    )
    data_list = []
    scatter_options_sensed = {
        "y": sensed_raw_signal,
        "hoverinfo": 'x+y',
        "name": "Sensed (no filter)",
        'xaxis': 'x',
        'yaxis': 'y',
        'line': dict(color='cadetblue')
    }
    data_list.append(go.Scatter(**scatter_options_sensed))
    scatter_options_single = {
        "y": results_single,
        "hoverinfo": 'x+y',
        "name": "Estimated (single)",
        'xaxis': 'x',
        'yaxis': 'y',
        'line': dict(color='red')
    }
    data_list.append(go.Scatter(**scatter_options_single))
    scatter_options_og = {
        "y": og_signal,
        "hoverinfo": 'x+y',
        "name": "Actual (no sensor noise)",
        'xaxis': 'x2',
        'yaxis': 'y2',
        'line': dict(color='black')
    }
    data_list.append(go.Scatter(**scatter_options_og))
    scatter_options_single_secondary = {
        "y": results_second,
        "hoverinfo": 'x+y',
        "name": "2nd Estimated (single)",
        'xaxis': 'x2',
        'yaxis': 'y2',
        'line': dict(color='tomato')
    }
    data_list.append(go.Scatter(**scatter_options_single_secondary))
    scatter_options_estimated_vs_og = scatter_options_single
    scatter_options_estimated_vs_og['xaxis'] = 'x2'
    scatter_options_estimated_vs_og['yaxis'] = 'y2'
    data_list.append(go.Scatter(**scatter_options_estimated_vs_og))
    scatter_options_og_fused = scatter_options_og
    scatter_options_og_fused['xaxis'] = 'x3'
    scatter_options_og_fused['yaxis'] = 'y3'
    data_list.append(go.Scatter(**scatter_options_og_fused))
    scatter_options_sensed_fused = {
        "y": results_fused,
        "hoverinfo": 'x+y',
        "name": "Estimated (fused)",
        'xaxis': 'x3',
        'yaxis': 'y3',
        'line': dict(color='darkred')
    }
    data_list.append(go.Scatter(**scatter_options_sensed_fused))
    fig_descriptor = go.Figure(data=data_list, layout=layout, layout_yaxis_range=[15, 25])
    fig_descriptor.update_layout(title="<b>Chameleon Fusion Micro-Benchmark</b>")
    fig_descriptor.update_layout(legend_title_text="<b>Strategy</b>")
    fig_descriptor.update_yaxes(range=[15, 25])
    fig_descriptor.update_xaxes(title_text="<b>Observation time (s)</b>")
    fig_descriptor.update_yaxes(title_text="<b>Wattage (watts)</b>")
    return fig_descriptor


def plot_fused_results_overall(og_signal: np.ndarray,
                               results_single: np.ndarray,
                               results_second: np.array,
                               results_fused: np.ndarray):
    fig_descriptor = go.Figure()
    title = {
        'text': "<b>Chameleon Fusion Micro-Benchmark</b>",
        'x': 0.5,
        'xanchor': 'center'}
    fig_descriptor.update_layout(title=title)
    fig_descriptor.update_xaxes(title_text="<b>Observation time (ms)</b>")
    fig_descriptor.update_yaxes(title_text="<b>Wattage (watts)</b>", range=[15, 25])
    scatter_options_og = {
        "y": og_signal,
        "hoverinfo": 'x+y',
        "name": "Actual Signal",
        'line': dict(color='black')
    }
    fig_descriptor.add_trace(go.Scatter(**scatter_options_og))
    fig_descriptor.update_layout(legend_title_text="<b>Legend</b>")
    fig_descriptor.write_image(f"fusion-showcase-microbench-og.png")
    time.sleep(2)  # wait and overwrite graph for mathjax bug
    fig_descriptor.write_image(f"fusion-showcase-microbench-og.png")
    scatter_options_single = {
        "y": results_single,
        "hoverinfo": 'x+y',
        "name": "Sensor 1",
        'line': dict(color='darkred')
    }
    fig_descriptor.add_trace(go.Scatter(**scatter_options_single))
    fig_descriptor.write_image(f"fusion-showcase-microbench-single.png")
    time.sleep(2)  # wait and overwrite graph for mathjax bug
    fig_descriptor.write_image(f"fusion-showcase-microbench-single.png")
    scatter_options_single_secondary = {
        "y": results_second,
        "hoverinfo": 'x+y',
        "name": "Sensor 2",
        'line': dict(color='tomato')
    }
    fig_descriptor.add_trace(go.Scatter(**scatter_options_single_secondary))
    fig_descriptor.write_image(f"fusion-showcase-microbench-two.png")
    time.sleep(2)  # wait and overwrite graph for mathjax bug
    fig_descriptor.write_image(f"fusion-showcase-microbench-two.png")
    scatter_options_sensed_fused = {
        "y": results_fused,
        "hoverinfo": 'x+y',
        "name": "Fused Signal",
        'line': dict(color='red')
    }
    fig_descriptor.add_trace(go.Scatter(**scatter_options_sensed_fused))
    fig_descriptor.write_image(f"fusion-showcase-microbench-full.png")
    time.sleep(2)  # wait and overwrite graph for mathjax bug
    fig_descriptor.write_image(f"fusion-showcase-microbench-full.png")


def plot_fusion_comparison(selectivity_results: Dict[str, Dict[str, Dict[str, float]]], short_uuid: str,
                           results_path: Path):
    overall_data = []
    selectivities = [str(x) for x in selectivity_results.keys()]
    workload_types = ['work', 'load']
    y_max = 0
    for workload_type in workload_types:
        y_data = []
        for selectivity in selectivities:
            y_data.append(selectivity_results.get(selectivity)[workload_type]["fused"])
        bar_options = {
            "x": selectivities,
            "y": y_data,
            "hoverinfo": 'x+y',
            "name": f"Dataset: {workload_type}"
        }
        overall_data.append(go.Bar(**bar_options))
        y_max = max(y_max, max(y_data))
    fig = go.Figure(data=overall_data)
    title = {'text': "<b>Chameleon Fusion Selectivities Micro-Benchmark</b>", 'x': 0.5, 'xanchor': 'center'}
    fig.update_layout(barmode="group", title=title)
    fig.update_xaxes(title_text="<b>Fusion Selectivities</b>")
    fig.update_yaxes(title_text="<b># of Samples (total)</b>", type='log', range=[0, math.ceil(math.log10(y_max))])
    fig.write_image(results_path.joinpath(f"fusion-selectivity-comparison-{short_uuid}.pdf"))
    time.sleep(2)  # wait and overwrite graph for mathjax bug
    fig.write_image(results_path.joinpath(f"fusion-selectivity-comparison-{short_uuid}.pdf"))
