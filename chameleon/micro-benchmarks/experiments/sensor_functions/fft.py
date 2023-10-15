import logging

import numpy as np
import pandas as pd
import scipy as sp

from typing import Tuple, Union
from pandas import DataFrame
from scipy import fft, signal

from .interpolation import interpolate_df

logger = logging.getLogger(__name__)


def compute_nyquist_and_energy(signal: DataFrame) -> Tuple[bool, float, float, float]:
    """
    Given a datafram signal, we prepare the inferred signal frequency.
    We then use the values of the signal to compute the FFT, the
    frequency bins, and finally the total signal energy.
    :param signal: a signal of a single value as a DataFrame
    :return: 4-tuple of (true/false if oversampled, the total energy, curr. oversampling ratio, proposed nyquist in s)
    """
    inferred_freq, diff_val = infer_signal_frequency(signal=signal)
    if inferred_freq is None:
        return False, 0., 0., 0.
    samples_per_second, new_nyquist_rate = per_second_stats(inferred_freq=inferred_freq, diff_val=diff_val)
    if pd.infer_freq(signal.index) is None:  # common inferred freq. is 2S
        try:
            signal = interpolate_df(signal, inferred_freq)
        except:  # timestamp sequence is too out-of-order to work
            return False, 0., 0., 0.
    vals = signal["value"].values
    # Compute DFT, detrend the 0th freq. by removing the mean
    sensor_fft = sp.fft.fft(vals - np.nanmean(vals))
    # Compute corresponding frequencies
    sensor_freqfft = sp.fft.fftfreq(len(vals), d=samples_per_second)
    # PSD is also the square of fft in the literature
    psd = np.abs(sensor_fft) ** 2
    # Energy obtained via "integrating" DFT components over frequency.
    E_fft = np.sum(np.abs(sensor_fft) ** 2) / len(vals)
    aliased, proposed_new_rate_idx = check_aliased_and_new_nyq_freq(psd, E_fft)
    assert sensor_freqfft[0] == 0.
    if not aliased and E_fft > 0. and proposed_new_rate_idx != 0:
        assert proposed_new_rate_idx != len(psd)
        new_nyquist_rate = 2 * np.abs(sensor_freqfft[proposed_new_rate_idx])
        logger.debug(f"Oversampled: {new_nyquist_rate < samples_per_second}, old rate: {samples_per_second}, new rate: {new_nyquist_rate}, energy: {E_fft}, oversampling ratio: {samples_per_second / new_nyquist_rate}")
    return new_nyquist_rate < samples_per_second, E_fft, samples_per_second / new_nyquist_rate, 1/new_nyquist_rate


def check_aliased_and_new_nyq_freq(psd_array: np.ndarray, total_energy: float) -> Tuple[bool, int]:
    """
    From https://www.microsoft.com/en-us/research/uploads/prod/2021/10/DSP_HotNets2021-18.pdf,
    used if the signal is already sampled in higher frequencies. Returns True if
    we use all bins to find >=99% of the energy (accumulate all bins).
    :param psd_array: the array of psd components of the signal
    :param total_energy: the total energy of the signal
    :return: true if we used all bins to find >=99% of the energy, the proposed new nyquist freq. idx
    """
    cutoff_percent = (total_energy * 99.0) / 100.0
    current_level = 0
    bin_idx = 0
    while current_level < cutoff_percent and bin_idx < len(psd_array):
        current_level += psd_array[bin_idx]
        bin_idx += 1
    return bin_idx == len(psd_array), bin_idx - 1


def computer_power_welch(signal: np.ndarray, samples_per_second: float) -> float:
    # Estimate PSD `S_xx_welch` at discrete frequencies `f_welch`
    f_welch, S_xx_welch = sp.signal.welch(signal, fs=samples_per_second)
    # Integrate PSD over spectral bandwidth
    # to obtain signal power `P_welch`
    df_welch = f_welch[1] - f_welch[0]
    P_welch = np.sum(S_xx_welch) * df_welch
    return P_welch


def infer_signal_frequency(signal: DataFrame) -> Tuple[Union[str, None], Union[int, None]]:
    try:
        inferred_freq = pd.infer_freq(signal.index)
    except ValueError:
        inferred_freq = None
    try:
        diff = signal.index[1] - signal.index[0]
        diff_val = diff.seconds
    except IndexError:
        return None, None
    if inferred_freq is None or diff_val == 0:  # common inferred freq. is 2S
        if diff.seconds > 0:
            diff_val = diff.seconds
            inferred_freq = str(diff_val) + "S"
        elif diff.components.milliseconds > 0:
            diff_val = diff.components.milliseconds
            inferred_freq = str(diff_val) + "L"
        elif diff.microseconds > 0:  # this will cover millis, no diff.milliseconds
            diff_val = diff.microseconds
            inferred_freq = str(diff_val) + "U"
        elif diff.nanoseconds > 0:
            diff_val = diff.nanoseconds
            inferred_freq = str(diff_val) + "N"
        elif diff.components.minutes > 0:
            diff_val = diff.components.minutes
            inferred_freq = str(diff.components.minutes) + "T"
            assert int(float(inferred_freq[:-1])) * 60 == diff_val
    if inferred_freq == "T" and diff.seconds == 60 and diff.components.minutes >= 1:
        inferred_freq = str(diff.components.minutes) + "T"
        diff_val = diff.components.minutes
    return inferred_freq, diff_val


def per_second_stats(inferred_freq: str, diff_val: int):
    samples_per_second = 1
    new_nyquist_rate = 1
    seconds = diff_val
    if 'S' not in inferred_freq and diff_val > 0:
        divider = 1
        if 'L' in inferred_freq:
            divider = 1000.0
        elif 'U' in inferred_freq:
            divider = 1000000.0
        elif 'N' in inferred_freq:
            divider = 1000000000.0
        elif 'T' in inferred_freq:
            assert int(float(inferred_freq[:-1])) == diff_val
        seconds /= divider
    samples_per_second /= seconds
    new_nyquist_rate /= seconds
    return samples_per_second, new_nyquist_rate
