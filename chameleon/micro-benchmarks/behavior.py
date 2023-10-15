import math

import numpy as np
import pandas as pd

from experiments.models.kalman_filters import init_filters
from experiments.sensor_functions.gathering import interpolate_df_simple, gather_values_from_df
from experiments.plotting.df_plots import create_multiple_stream_scatterplots, \
    create_multi_stream_scatterplot_horiz_subfigure


def sine_behavior(window_size=10):
    x_values = np.arange(0, 4 * np.pi, 0.1)
    y_data = [np.sin(x) for x in x_values]
    idx = pd.date_range(start="2018-01-01", periods=len(y_data),
                        freq="1S", tz='Europe/Berlin')
    ts = pd.Series(data=y_data, index=idx)
    ts_interpolated = interpolate_df_simple(ts.to_frame('value'))
    ts_interpolated.index.name = "sine-interpolated"
    strategies = init_filters()
    # TODO: the logic in the baseline never stops making the interval smaller
    baseline_df = gather_values_from_df(original_df=ts_interpolated,
                                        initial_interval=strategies['baseline'].get_gathering_interval_seconds(),
                                        strategy=strategies['baseline'],
                                        artificial_cuttoff=True)
    baseline_df.index.name = "sine-baseline"
    chameleon_df = gather_values_from_df(original_df=ts_interpolated,
                                         initial_interval=strategies["chameleon"].get_gathering_interval_seconds(),
                                         strategy=strategies["chameleon"])
    chameleon_df.index.name = "sine-chameleon"
    fixed_df = gather_values_from_df(original_df=ts_interpolated,
                                     initial_interval=4,
                                     strategy=None)
    fixed_df.index.name = "sine-fixed"
    plot_fig = create_multiple_stream_scatterplots(df_list=[ts_interpolated,
                                                            baseline_df,
                                                            fixed_df,
                                                            chameleon_df],
                                                   write_img=False,
                                                   mode="markers")
    plot_fig.show()


def cosine_behavior(window_size=10):
    x_values = np.arange(0, 4 * np.pi, 0.1)
    y_data = [np.cos(x) for x in x_values]
    idx = pd.date_range(start="2018-01-01", periods=len(y_data),
                        freq="1S", tz='Europe/Berlin')
    ts = pd.Series(data=y_data, index=idx)
    ts_interpolated = interpolate_df_simple(ts.to_frame('value'))
    ts_interpolated.index.name = "cosine-interpolated"
    strategies = init_filters()
    baseline_df = gather_values_from_df(original_df=ts_interpolated,
                                        initial_interval=strategies['baseline'].get_gathering_interval_seconds(),
                                        strategy=strategies['baseline'],
                                        artificial_cuttoff=True)
    baseline_df.index.name = "cosine-baseline"
    chameleon_df = gather_values_from_df(original_df=ts_interpolated,
                                         initial_interval=strategies["chameleon"].get_gathering_interval_seconds(),
                                         strategy=strategies["chameleon"])
    chameleon_df.index.name = "cosine-chameleon"
    fixed_df = gather_values_from_df(original_df=ts_interpolated,
                                     initial_interval=4,
                                     strategy=None)
    fixed_df.index.name = "cosine-fixed"
    plot_fig = create_multiple_stream_scatterplots(df_list=[ts_interpolated,
                                                            baseline_df,
                                                            fixed_df,
                                                            chameleon_df],
                                                   write_img=False,
                                                   mode="markers")
    plot_fig.show()


def sigmoid_incr(x):
    return 1 / (1 + math.exp(-x))


def sigmoid_behavior(value_range=range(-20, 20), window_size=10):
    x_values = np.arange(-20, 20, 0.1)
    y_data = [sigmoid_incr(x) for x in x_values]
    idx = pd.date_range(start="2018-01-01", periods=len(y_data),
                        freq="1S", tz='Europe/Berlin')
    ts = pd.Series(data=y_data, index=idx)
    ts_interpolated = interpolate_df_simple(ts.to_frame('value'))
    ts_interpolated.index.name = "sigmoid-interpolated"
    strategies = init_filters()
    baseline_df = gather_values_from_df(original_df=ts_interpolated,
                                        initial_interval=strategies['baseline'].get_gathering_interval_seconds(),
                                        strategy=strategies['baseline'],
                                        artificial_cuttoff=True)
    baseline_df.index.name = "sigmoid-baseline"
    chameleon_df = gather_values_from_df(original_df=ts_interpolated,
                                         initial_interval=strategies["chameleon"].get_gathering_interval_seconds(),
                                         strategy=strategies["chameleon"])
    chameleon_df.index.name = "sigmoid-chameleon"
    fixed_df = gather_values_from_df(original_df=ts_interpolated,
                                     initial_interval=4,
                                     strategy=None)
    fixed_df.index.name = "sigmoid-fixed"
    strategies = init_filters(start_with_different_measurements=True)
    baseline_df_init_wrong = gather_values_from_df(original_df=ts_interpolated,
                                        initial_interval=strategies['baseline'].get_gathering_interval_seconds(),
                                        strategy=strategies['baseline'],
                                        artificial_cuttoff=True)
    baseline_df_init_wrong.index.name = "sigmoid-baseline-wrong-init"
    chameleon_df_init_wrong = gather_values_from_df(original_df=ts_interpolated,
                                         initial_interval=strategies["chameleon"].get_gathering_interval_seconds(),
                                         strategy=strategies["chameleon"])
    chameleon_df_init_wrong.index.name = "sigmoid-chameleon-wrong-init"
    plot_fig = create_multiple_stream_scatterplots(df_list=[ts_interpolated,
                                                            baseline_df,
                                                            fixed_df,
                                                            chameleon_df,
                                                            baseline_df_init_wrong,
                                                            chameleon_df_init_wrong],
                                                   write_img=False,
                                                   mode="markers")
    plot_fig.show()


def sigmoid_decr(x):
    return 1 / (1 + math.exp(x))


def sigmoid_decr_behavior(value_range=range(-20, 20), window_size=10):
    x_values = np.arange(-20, 20, 0.1)
    y_data = [sigmoid_decr(x) for x in x_values]
    idx = pd.date_range(start="2018-01-01", periods=len(y_data),
                        freq="1S", tz='Europe/Berlin')
    ts = pd.Series(data=y_data, index=idx)
    ts_interpolated = interpolate_df_simple(ts.to_frame('value'))
    ts_interpolated.index.name = "sigmoid-decr-interpolated"
    strategies = init_filters()
    baseline_df = gather_values_from_df(original_df=ts_interpolated,
                                        initial_interval=strategies['baseline'].get_gathering_interval_seconds(),
                                        strategy=strategies['baseline'],
                                        artificial_cuttoff=True)
    baseline_df.index.name = "sigmoid-decr-baseline"
    chameleon_df = gather_values_from_df(original_df=ts_interpolated,
                                         initial_interval=strategies["chameleon"].get_gathering_interval_seconds(),
                                         strategy=strategies["chameleon"])
    chameleon_df.index.name = "sigmoid-decr-chameleon"
    fixed_df = gather_values_from_df(original_df=ts_interpolated,
                                     initial_interval=4,
                                     strategy=None)
    fixed_df.index.name = "sigmoid-decr-fixed"
    plot_fig = create_multiple_stream_scatterplots(df_list=[ts_interpolated,
                                                            baseline_df,
                                                            fixed_df,
                                                            chameleon_df],
                                                   write_img=False,
                                                   mode="markers")
    plot_fig.show()


def straight_line():
    y_data = 60 * [1]
    idx = pd.date_range(start="2018-01-01", periods=len(y_data),
                        freq="1S", tz='Europe/Berlin')
    ts = pd.Series(data=y_data, index=idx)
    ts_interpolated = interpolate_df_simple(ts.to_frame('value'))
    ts_interpolated.index.name = "straight-line-val-1-init-0-interpolated"
    strategies = init_filters(interval=2000)
    chameleon_df_2s = gather_values_from_df(original_df=ts_interpolated,
                                            initial_interval=strategies["chameleon"].get_gathering_interval_seconds(),
                                            strategy=strategies["chameleon"])
    chameleon_df_2s.index.name = "straight-line-val-1-init-0-chameleon-2s"
    strategies = init_filters()
    chameleon_df_4s = gather_values_from_df(original_df=ts_interpolated,
                                            initial_interval=strategies["chameleon"].get_gathering_interval_seconds(),
                                            strategy=strategies["chameleon"])
    chameleon_df_4s.index.name = "straight-line-val-1-init-0-chameleon-4s"
    baseline_df = gather_values_from_df(original_df=ts_interpolated,
                                        initial_interval=strategies["baseline"].get_gathering_interval_seconds(),
                                        strategy=strategies["baseline"])
    baseline_df.index.name = "straight-line-val-1-init-0-baseline-4s"
    upper_data = [chameleon_df_2s, chameleon_df_4s, baseline_df]
    y_data_right_init = 60 * [0]
    idx = pd.date_range(start="2018-01-01", periods=len(y_data_right_init),
                        freq="1S", tz='Europe/Berlin')
    ts = pd.Series(data=y_data_right_init, index=idx)
    ts_interpolated_right_init = interpolate_df_simple(ts.to_frame('value'))
    ts_interpolated_right_init.index.name = "straight-line-val-0-init-0-interpolated"
    strategies = init_filters(interval=2000)
    chameleon_df_2s_right_init = gather_values_from_df(original_df=ts_interpolated_right_init,
                                                       initial_interval=strategies[
                                                           "chameleon"].get_gathering_interval_seconds(),
                                                       strategy=strategies["chameleon"])
    chameleon_df_2s_right_init.index.name = "straight-line-val-0-init-0-chameleon-2s"
    strategies = init_filters()
    chameleon_df_4s_right_init = gather_values_from_df(original_df=ts_interpolated_right_init,
                                                       initial_interval=strategies[
                                                           "chameleon"].get_gathering_interval_seconds(),
                                                       strategy=strategies["chameleon"])
    chameleon_df_4s_right_init.index.name = "straight-line-val-0-init-0-chameleon-4s"
    baseline_df_right_init = gather_values_from_df(original_df=ts_interpolated_right_init,
                                                   initial_interval=strategies[
                                                       "baseline"].get_gathering_interval_seconds(),
                                                   strategy=strategies["baseline"])
    baseline_df_right_init.index.name = "straight-line-val-0-init-0-baseline-4s"
    lower_data = [chameleon_df_2s_right_init, chameleon_df_4s_right_init, baseline_df_right_init]
    fig = create_multi_stream_scatterplot_horiz_subfigure(df_list=[lower_data, upper_data])
    fig.show()


def straight_line_with_len(line_len=60, initial_value=0):
    y_data = line_len * [initial_value]
    idx = pd.date_range(start="2018-01-01", periods=len(y_data),
                        freq="1S", tz='Europe/Berlin')
    ts = pd.Series(data=y_data, index=idx)
    ts_interpolated = interpolate_df_simple(ts.to_frame('value'))
    ts_interpolated.index.name = f"straight-line-val-{initial_value}-interpolated"
    strategies = init_filters()
    chameleon_df_4s = gather_values_from_df(original_df=ts_interpolated,
                                            initial_interval=strategies["chameleon"].get_gathering_interval_seconds(),
                                            strategy=strategies["chameleon"])
    chameleon_df_4s.index.name = f"straight-line-val-{initial_value}-len-{line_len}-chameleon-4s"
    upper_data = [chameleon_df_4s]
    y_data_below = (2*line_len) * [initial_value]
    idx = pd.date_range(start="2018-01-01", periods=len(y_data_below),
                        freq="1S", tz='Europe/Berlin')
    ts = pd.Series(data=y_data_below, index=idx)
    ts_interpolated_double = interpolate_df_simple(ts.to_frame('value'))
    ts_interpolated_double.index.name = f"straight-line-val-{initial_value}-interpolated"
    strategies = init_filters()
    chameleon_df_4s_double = gather_values_from_df(original_df=ts_interpolated_double,
                                            initial_interval=strategies["chameleon"].get_gathering_interval_seconds(),
                                            strategy=strategies["chameleon"])
    chameleon_df_4s_double.index.name = f"straight-line-val-{initial_value}-len-{2*line_len}-chameleon-4s"
    lower_data = [chameleon_df_4s_double]
    fig = create_multi_stream_scatterplot_horiz_subfigure(df_list=[lower_data, upper_data])
    fig.show()


def run_all(window_size=10):
    # default_measurements_behavior(window_size=window_size)
    sine_behavior(window_size=window_size)
    # cosine_behavior(window_size=window_size)
    sigmoid_behavior(window_size=window_size)
    # sigmoid_decr_behavior(window_size=window_size)
    straight_line()
    straight_line_with_len(line_len=60, initial_value=1)


if __name__ == '__main__':
    run_all()
