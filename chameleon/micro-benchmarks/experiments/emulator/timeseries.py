import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import ruptures as rpt

from pandas import DataFrame
from kats.consts import TimeSeriesData
from kats.utils.simulator import Simulator


def kats_manual(n=2000, freq='.5S', start=pd.to_datetime('2022-09-01 00:00:00'), trend_mag=10,
                season_mag=5, trend_season='30S', noise_mag=2) -> DataFrame:
    sim = Simulator(n=n, freq=freq, start=start)
    np.random.seed(100)
    sim.add_trend(magnitude=trend_mag, trend_type='sigmoid')
    sim.add_seasonality(magnitude=season_mag, period=trend_season)
    sim.add_noise(magnitude=noise_mag)
    return _prepare_kats_df(sim.stl_sim())


def _kats_level_shift_sim(n=450, freq='.5MS', start=pd.to_datetime('2022-09-01 00:00:00')) -> DataFrame:
    sim = Simulator(n=n, freq=freq, start=start)
    ts_result = sim.level_shift_sim(cp_arr=[100, 200], level_arr=[3, 20, 2], noise=30,
                        seasonal_period=7, seasonal_magnitude=3,
                        anomaly_arr=[50, 150, 250], z_score_arr=[10, -10, 20])
    return _prepare_kats_df(ts_result)


def _kats_trend_shift_sim(n=450, freq='.5MS', start=pd.to_datetime('2022-09-01 00:00:00')) -> DataFrame:
    sim = Simulator(n=n, freq=freq, start=start)
    ts_result = sim.trend_shift_sim(cp_arr=[100, 200], trend_arr=[3, 20, 2], noise=30,
                        seasonal_period=7, seasonal_magnitude=3,
                        anomaly_arr=[50, 150, 250], z_score_arr=[10, -10, 20])
    return _prepare_kats_df(ts_result)


def _kats_arima_sim(n=100, freq="MS", start=pd.to_datetime('2022-09-01 00:00:00')) -> DataFrame:
    sim = Simulator(n=n, freq=freq, start=start)
    np.random.seed(100)
    ts_result = sim.arima_sim(ar=[0.1, 0.05], ma=[.04, .1], d=1)
    return _prepare_kats_df(ts_result)


def _prepare_kats_df(ts_result: TimeSeriesData) -> DataFrame:
    ts_np_arr = ts_result.to_array()
    ts_df = pd.DataFrame({'value': ts_np_arr[:, 1]}, index=ts_np_arr[:, 0])
    ts_df.index = pd.to_datetime(ts_df.index)
    return ts_df


def use_kats(signal_f='manual', n=2000, freq='.5S', start=pd.to_datetime('2022-09-01 00:00:00')):
    if signal_f == 'manual':
        return kats_manual(n=n, freq=freq, start=start)
    if signal_f == 'level_shift':
        return _kats_level_shift_sim(freq=freq)
    if signal_f == 'trend_shift':
        return _kats_trend_shift_sim(freq=freq)
    if signal_f == 'arima':
        return _kats_arima_sim(freq=freq)


# TODO: decide if we use ruptures
def use_rpt(signal_f='constant', n_samples=1000, n_dim=1, sigma=5, n_bkps=4, show=False):
    if signal_f == 'constant':
        signal, bkps = rpt.pw_constant(n_samples, n_dim, n_bkps, noise_std=sigma)
    elif signal_f == 'linear':
        signal, bkps = rpt.pw_linear(n_samples, n_dim, n_bkps, noise_std=sigma)
    elif signal_f == 'gaussian':
        signal, bkps = rpt.pw_normal(n_samples, n_bkps)
    elif signal_f == 'sinusoid':
        signal, bkps = rpt.pw_wavy(n_samples, n_bkps, noise_std=sigma)
    if show:
        fig, ax_arr = rpt.display(signal, bkps)
        plt.show()
        plt.close()
