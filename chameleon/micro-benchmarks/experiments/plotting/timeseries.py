import functools

import matplotlib.pyplot as plt
import pandas as pd


def ts_display(signal, fn_name: str, **kwargs):
    plt.style.use("tableau-colorblind10")
    signal_idx = None
    if type(signal) == pd.DataFrame:
        signal_idx = signal.index
        signal = signal.values

    if signal.ndim == 1:
        signal = signal.reshape(-1, 1)
    n_samples, n_features = signal.shape

    # let's set a sensible defaut size for the subplots
    matplotlib_options = {
        "figsize": (10, 2 * n_features),  # figure size
        # figsize = set_size(506 / 2) TODO: this should match the paper
    }
    # add/update the options given by the user
    matplotlib_options.update(kwargs)

    # create plots
    fig, axarr = plt.subplots(n_features, sharex=True, **matplotlib_options)
    if n_features == 1:
        axarr = [axarr]

    for axe, sig in zip(axarr, signal.T):
        # plot s
        if signal_idx is not None:
            axe.plot(signal_idx, sig)
        else:
            axe.plot(range(n_samples), sig)
    plt.title(fn_name)
    fig.tight_layout()
    return fig, axarr


def plottable(_func=None, *, show_plot=False, current_uuid=""):
    def decorator_plottable(func):
        @functools.wraps(func)
        def wrapper_plot(*args, **kwargs):
            ts_result = func(*args, **kwargs)
            ts_display(ts_result, fn_name=func.__name__)
            if show_plot:
                plt.show()
            else:
                plt.savefig(f"{func.__name__}-{current_uuid}.png", format='png')
            plt.close()
            return ts_result
        return wrapper_plot
    if _func is None:
        return decorator_plottable
    else:
        return decorator_plottable(_func)
