import glob
import time
from os import sep
from typing import List

from matplotlib import pyplot as plt
import pandas as pd
import seaborn as sns
import plotly.graph_objects as go


def create_scatterplots(df_list: List[any], path='/home/digiou/test-results/'):
    fig_descriptor = go.Figure()
    for df in df_list:
        fig_descriptor.add_trace(
            go.Scatter(x=df["Date"],
                       y=df["Interval"],
                       hoverinfo='x+y',
                       mode='markers+lines',
                       name=df.index.name.replace(path, "").split("-")[0])
        )

    fig_descriptor.update_layout(title="<b>Chameleon Behavior Micro-Benchmark -- Fixed Input</b>")
    fig_descriptor.update_layout(legend_title_text = "<b>Strategy</b>")
    fig_descriptor.update_xaxes(title_text="<b>Event time (ms)</b>")
    fig_descriptor.update_yaxes(title_text="<b>Interval (ms)</b>")
    fig_descriptor.write_image(f"{path}{sep}results.pdf")
    time.sleep(2)  # wait and overwrite graph for mathjax bug
    fig_descriptor.write_image(f"{path}{sep}results.pdf")
    return fig_descriptor


def create_lineplots(df_list: List[any], path='/home/digiou/test-results/'):
    for df in df_list:
        sns.lineplot(data=df, x="Date", y="Interval")
        plt.xticks(rotation=15)
        pdf_name = df.index.name.replace(path, "").replace("csv", "pdf")
        split_strings = pdf_name.split("-")
        plt.title(split_strings[0])
        plt.xlabel("Time (s)")
        plt.ylabel("Interval (ms)")
        plt.tight_layout()
        plt.savefig(f"{path}{sep}{pdf_name}")
        plt.clf()


def create_pd_dataframes(file_list: List[str]) -> List[any]:
    dfs = []
    for file_path in file_list:
        current_df = pd.read_csv(file_path,
                                 header=0,
                                 parse_dates=["Date"],
                                 date_parser=lambda x: pd.to_datetime(x, format='%Y-%m-%dT%H:%M:%S.%fZ'),
                                 infer_datetime_format=True)
        current_df.index.name = file_path
        dfs.append(current_df)
    return dfs


def list_result_files(path='/home/digiou/test-results/', suffix="csv") -> List[str]:
    return glob.glob(f'{path}{sep}*.{suffix}')


if __name__ == '__main__':
    files = list_result_files()
    dataframes = create_pd_dataframes(files)
    fig = create_scatterplots(dataframes)
    fig.show()
