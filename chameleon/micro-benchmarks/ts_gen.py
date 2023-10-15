import matplotlib.pyplot as plt
import pandas as pd
import ruptures as rpt

from experiments.plotting.timeseries import plottable
from experiments.emulator.timeseries import use_kats


def use_rpt(signal_f: str, n_samples=1000, n_dim=1, sigma=5, n_bkps=4):
    if signal_f == 'constant':
        signal, bkps = rpt.pw_constant(n_samples, n_dim, n_bkps, noise_std=sigma)
    elif signal_f == 'linear':
        signal, bkps = rpt.pw_linear(n_samples, n_dim, n_bkps, noise_std=sigma)
    elif signal_f == 'gaussian':
        signal, bkps = rpt.pw_normal(n_samples, n_bkps)
    elif signal_f == 'sinusoid':
        signal, bkps = rpt.pw_wavy(n_samples, n_bkps, noise_std=sigma)
    fig, ax_arr = rpt.display(signal, bkps)
    plt.show()


@plottable
def kats_manual(n=2000, freq='.5S', start=pd.to_datetime('2022-09-01 00:00:00')):
    return use_kats(n=n, freq=freq, start=start)


@plottable(show_plot=True)
def kats_level_shift(start=pd.to_datetime('2022-09-01 00:00:00')):
    return use_kats(signal_f='level_shift', n=40000, freq='5min', start=start)


@plottable(show_plot=True)
def kats_trend_shift(start=pd.to_datetime('2022-09-01 00:00:00')):
    return use_kats(signal_f='trend_shift', n=40000, freq='5min', start=start)


@plottable(show_plot=True)
def kats_arima(start=pd.to_datetime('2022-09-01 00:00:00')):
    return use_kats(signal_f='arima', n=40000, freq='5min', start=start)


if __name__ == '__main__':
    # TODO: centralize plot settings
    # TODO: use similar sized-input from intel-lab, adapt anomalies
    gen_ts = [kats_level_shift, kats_trend_shift, kats_arima]
    for ts in gen_ts:
        res = ts()
        ts_std = res['value'].std(ddof=0)
        ts_var = res['value'].var()
        print(f"var={ts_var}, std={ts_std}")