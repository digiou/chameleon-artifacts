import numpy as np
from numpy import sin, linspace, pi, sqrt, size
from numpy.random import randn

from experiments.models.kalman_filters import FusedKalmanFilter
from experiments.plotting.df_plots import plot_fused_results_overall


if __name__ == '__main__':
    bias_1 = 1
    bias_2 = -1

    # measure noise covariance
    R1 = R2 = 0.64

    # process noise covariance
    Q = 0.0005

    # State trans. model
    F = 1

    # Observation model
    H1 = H2 = 1

    # Iteration
    N = 1000

    # generate sine wave signal
    x = 20 + sin(5*linspace(0,1,N)*pi)

    # Add process noise with cov. Q
    np.random.seed(1)
    w = sqrt(Q) * randn(size(x))
    x = x + w

    # Compute noisy sensor values, merge them
    z1 = bias_1 + x + sqrt(R1) * randn(size(x))
    z2 = bias_2 + x + sqrt(R2) * randn(size(x))
    combined_z = np.array([z1, z2])  # so that we can access them as 2-D later
    _, num_of_cols = combined_z.shape

    # single-val kf
    two_d_vals = np.array([combined_z[0, [0]]])  # array of (1,1), not (1,nothing)
    single_val_filter = FusedKalmanFilter(two_d_vals)  # 1 sensor, 1 val each, 1st col = 1st measurement
    xhats_single = [single_val_filter.x[0]]
    for idx in range(1, num_of_cols):
        two_d_val = np.array([combined_z[0, [idx]]])  # array of (1,1), not (1,nothing)
        single_val_filter.predict_and_update(two_d_val)
        xhats_single.append(single_val_filter.x[0])

    # second singl-val kf
    two_d_vals = np.array([combined_z[1, [0]]])  # array of (1,1), not (1,nothing)
    single_val_filter = FusedKalmanFilter(two_d_vals)  # 1 sensor, 1 val each, 1st col = 1st measurement
    xhats_single_secondary = [single_val_filter.x[0]]
    for idx in range(1, num_of_cols):
        two_d_val = np.array([combined_z[1, [idx]]])  # array of (1,1), not (1,nothing)
        single_val_filter.predict_and_update(two_d_val)
        xhats_single_secondary.append(single_val_filter.x[0])

    # create fused kf
    fused_filter = FusedKalmanFilter(combined_z[:, [0]])  # 2 sensor combo, 1 val each, 1st col = 1st measurement
    xhats_fused = [fused_filter.x[0]]
    for idx in range(1, num_of_cols):
        fused_filter.predict_and_update(combined_z[:, [idx]])
        xhats_fused.append(fused_filter.x[0])

    # plot actual + estimated, one sensor + estimated, two sensors + estimated
    plot_fused_results_overall(og_signal=x, results_single=xhats_single,
                               results_second=xhats_single_secondary,
                               results_fused=xhats_fused)
