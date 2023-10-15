import math
from abc import ABC, abstractmethod
from collections import deque
from typing import Dict, Union, Tuple
from math import sqrt

import numpy as np
import pandas as pd
from filterpy.kalman import KalmanFilter
from numpy import trace, multiply
from numpy.linalg import inv, matrix_power

from ..sensor_functions.fft import compute_nyquist_and_energy


default_measurements = [
    1.04202710058, 1.10726790452, 1.2913511148, 1.48485250951, 1.72825901034,
    1.74216489744, 2.11672039768, 2.14529225112, 2.16029641405, 2.21269371128,
    2.57709350237, 2.6682215744, 2.51641839428, 2.76034056782, 2.88131780617,
    2.88373786518, 2.9448468727, 2.82866600131, 3.0006601946, 3.12920591669,
    2.858361783, 2.83808170354, 2.68975330958, 2.66533185589, 2.81613499531,
    2.81003612051, 2.88321849354, 2.69789264832, 2.4342229249, 2.23464791825,
    2.30278776224, 2.02069770395, 1.94393985809, 1.82498398739, 1.52526230354,
    1.86967808173, 1.18073207847, 1.10729605087, 0.916168349913, 0.678547664519,
    0.562381751596, 0.355468474885, 500,
    300, 200, 0.355468474885, 0.572381751596, 0.658547664519, 0.916168349913,
    0.926168349913
]


class IntervalKalmanFilter(ABC, KalmanFilter):
    def __init__(self, dim_x=3, dim_z=1, window_size=10, nyquist_window=10, use_filter=True, only_filter=False,
                 constrain_analyzer: bool = False, max_oversampling: float = 1., fused=False):
        super().__init__(dim_x=dim_x, dim_z=dim_z)
        self.fused = fused
        if not self.fused:
            self._set_default_values()
        else:
            self._set_default_fusion_values()
        self.window_size = window_size
        self.kf_error_window = deque([0] * self.window_size, maxlen=self.window_size)
        self.last_values_window = deque(maxlen=nyquist_window)
        self.tracedResult = None
        self.innovXvalRevSquared = None
        self.innovXvalRev = None
        self.total_estimation_error_divider = 0 if self.window_size > 0 else 1
        self._calculate_total_estimation_error_divider()
        self.lamda = 0.6
        self.theta = 2
        self.euler_constant = np.exp(1.0)
        self.gathering_interval = 1000
        self.gathering_interval_received = 1000
        self.initial_freq = 1000
        self.gathering_interval_range = 8000
        self.decrease_counter = 1
        self.increase_counter = 1
        self.use_filter = use_filter
        self.only_filter = only_filter
        self.constrain_analyzer = constrain_analyzer
        self.max_oversampling = max_oversampling

    def _set_default_values(self):
        self.x = np.array([0., default_measurements[0], default_measurements[0]])  # initial state vector
        self.F = np.array([[1., 0.0333, 0.], [0., 1., 0.0333], [0., 0., 1.]])  # state transition matrix
        self.H = np.array([[1., 0., 0.]])  # measurements function
        self.Q = np.array([[0.05, 0.05, 0.], [0.05, 0.05, 0.], [0., 0., 0.]])  # process noise
        self.R = 5  # measurement noise
        self.P = np.array([[0.1, 0.1, 0.1], [0.1, 10000, 10], [0.1, 10, 100]])  # covariance matrix

    # TODO: add sensible defaults that do not break m, n
    def _set_default_fusion_values(self):
        self.H.fill(1)  # all sensors contribute equally
        self.R = self.R * 0.64  # measurement noise for every sensor
        self.Q = self.Q * 0.005  # process noise co-variance
        self.x = np.array([(0. + default_measurements[0]) / 2])  # handle in init

    def _calculate_total_estimation_error_divider(self):
        for idx in range(1, self.window_size + 1):
            self.total_estimation_error_divider += (1.0 / idx)

    def _calculate_total_estimation_error(self):
        j = 1.0
        total_error = 0.0
        for error_value in self.kf_error_window:
            total_error += (error_value / j)
            j += 1
        return total_error / self.total_estimation_error_divider

    def get_current_gathering_interval(self):
        return self.gathering_interval

    def get_current_gathering_interval_seconds(self):
        return self.gathering_interval / 1000

    def get_total_estimation_error(self):
        return self._calculate_total_estimation_error()

    def get_estimation_error(self):
        return self.kf_error_window[0]

    def _get_estimation_error_difference(self):
        return self.kf_error_window[0] - self.kf_error_window[1]

    def get_estimation_error_difference(self):
        return self._get_estimation_error_difference()

    def _get_exponential_decay_frequency(self):
        try:
            new_candidate = self.initial_freq * math.pow((1 - .25), self.decrease_counter)
            if math.isinf(new_candidate):
                return self.gathering_interval
            self._decay_counters()
        except OverflowError:
            return self.gathering_interval
        return new_candidate

    def get_exponential_decay_frequency(self):
        return self._get_exponential_decay_frequency()

    def _decay_counters(self):
        self.decrease_counter += 1
        self.increase_counter = 1

    def decay_counters(self):
        self._decay_counters()

    def _get_exponential_growth_frequency(self):
        try:
            new_candidate = self.initial_freq * math.pow((1 + .25), self.increase_counter)
            if math.isinf(new_candidate):
                return self.gathering_interval
            self._growth_counters()
        except OverflowError:
            return self.gathering_interval
        return new_candidate

    def get_exponential_growth_frequency(self):
        return self._get_exponential_growth_frequency()

    def _growth_counters(self):
        self.increase_counter += 1
        self.decrease_counter = 1

    def growth_counters(self):
        self._growth_counters()

    def init(self, initial_state):
        if self.fused:
            # TODO: adapt matlab's xhat(:,1) = H \ z(:,1) for multi-col input
            # if all(len(row) == len(self.H) for row in self.H):  # is_square
            #     self.x = np.linalg.solve(self.H, initial_state)
            # else:
            self.x = np.linalg.lstsq(self.H, initial_state, rcond=None)[0][0]  # initial_state is always 2-D
        else:
            self.x = initial_state
        # self.last_values_window.appendleft(self.x[-1])
        # assert isinstance(self.last_values_window[-1], float)

    def predict_and_update(self, timestamp, new_measurement):
        self.last_values_window.append((timestamp, new_measurement))
        measurement_array = np.array([[new_measurement]]) if new_measurement != 0.0 else np.array([[1.0]])
        self.predict()
        self.update(new_measurement)
        # eq. 8 calculated outside update phase (same thing)
        innovXvalRev = multiply(self.y, inv(measurement_array))
        innovXvalRevSquared = matrix_power(innovXvalRev, 2)
        tracedResult = trace(innovXvalRevSquared)
        self.kf_error_window.appendleft(sqrt(tracedResult))

    def set_gathering_interval(self, new_gathering_interval):
        self.gathering_interval = new_gathering_interval
        self.gathering_interval_received = new_gathering_interval
        self.initial_freq = new_gathering_interval

    def get_gathering_interval_seconds(self):
        return self.gathering_interval / 1000

    def set_gathering_interval_range(self, new_gathering_interval_range):
        if new_gathering_interval_range < 1000:
            new_gathering_interval_range *= 1000
        self.gathering_interval_range = new_gathering_interval_range

    def calc_upper_bound(self) -> Tuple[bool, float]:
        if len(self.last_values_window) == self.last_values_window.maxlen:
            df = pd.DataFrame(list(self.last_values_window), columns=['timestamp', 'value'])
            df['timestamp'] = pd.to_datetime(df['timestamp'])
            df.set_index('timestamp', inplace=True)
            nyq_result = compute_nyquist_and_energy(df)
            if nyq_result[0]:
                if self.constrain_analyzer and nyq_result[-2] > self.max_oversampling:
                    return True, ((nyq_result[-1] / nyq_result[-2]) * self.max_oversampling)
                return True, nyq_result[-1] * 1000
        return False, self.initial_freq + (self.initial_freq / 2)

    @abstractmethod
    def get_new_gathering_interval(self) -> float:
        pass

    @abstractmethod
    def get_name(self) -> str:
        pass


class JainKalmanFilter(IntervalKalmanFilter):
    def __init__(self, dim_x=3, dim_z=1, window_size=10):
        super().__init__(dim_x=dim_x, dim_z=dim_z, window_size=window_size)

    def get_new_gathering_interval(self) -> float:
        total_estimation_error = self._calculate_total_estimation_error()
        power_of_euler = (total_estimation_error + self.lamda) / self.lamda
        theta_part = self.theta * (1 - np.power(self.euler_constant, power_of_euler))
        new_gathering_interval_candidate = self.gathering_interval + theta_part
        if (self.gathering_interval_received - (self.gathering_interval_range / 2)
                <= new_gathering_interval_candidate <=
                self.gathering_interval_received + (self.gathering_interval_range / 2)):
            self.gathering_interval = math.trunc(new_gathering_interval_candidate)
        return self.get_current_gathering_interval_seconds()

    def get_name(self) -> str:
        return "baseline"


class ChameleonKalmanFilter(IntervalKalmanFilter):
    def __init__(self, dim_x=3, dim_z=1, window_size=10, nyquist_window=10, fused=False):
        super().__init__(dim_x=dim_x, dim_z=dim_z, window_size=window_size, nyquist_window=nyquist_window, fused=fused)

    def get_new_gathering_interval(self) -> float:
        if self.only_filter:
            self._use_profiler_only()
        elif self.use_filter:
            self._use_analyzer_with_profiler()
        else:
            self._use_analyzer_only()
        return self.get_current_gathering_interval_seconds()

    def _calculate_new_gathering_interval(self, lower_limit: float, upper_limit: float):
        if self.get_estimation_error_difference() > 0.6:  # indicates immediate change
            frequency_candidate = self.get_exponential_decay_frequency()
            if frequency_candidate < lower_limit:
                self.gathering_interval = lower_limit
            else:
                self.gathering_interval = frequency_candidate
        elif self.get_total_estimation_error() < 0.24:  # indicates consecutive similarity
            frequency_candidate = self.get_exponential_growth_frequency()
            if frequency_candidate > upper_limit:
                self.gathering_interval = upper_limit
            else:
                self.gathering_interval = frequency_candidate

    def _use_analyzer_only(self):
        oversampled, upper_limit = self.calc_upper_bound()
        if oversampled:
            self.gathering_interval = upper_limit

    def _use_profiler_only(self):
        upper_limit = (self.initial_freq + (self.initial_freq / 2))
        lower_limit = self.initial_freq / 2
        self._calculate_new_gathering_interval(upper_limit=upper_limit, lower_limit=lower_limit)

    def _use_analyzer_with_profiler(self):
        oversampled, upper_limit = self.calc_upper_bound()
        lower_limit = self.initial_freq / 2
        self._calculate_new_gathering_interval(upper_limit=upper_limit, lower_limit=lower_limit)

    def set_use_filter(self, use_filter: bool):
        self.use_filter = use_filter

    def set_only_filter(self, only_filter: bool):
        self.only_filter = only_filter
        if only_filter:
            self.use_filter = True

    def set_constrained_analyzer(self, constrain_analyzer: bool, max_oversampling: float):
        self.constrain_analyzer = constrain_analyzer
        self.max_oversampling = max_oversampling
        if self.constrain_analyzer:
            self.use_filter = True

    def get_name(self) -> str:
        return "chameleon"


class FusedKalmanFilter(ChameleonKalmanFilter):
    """
        Code from https://github.com/simondlevy/SensorFusion/blob/master/kalman.m
        Translated using wikipedia notation for matrices.
        Translated to numpy using: https://numpy.org/doc/stable/user/numpy-for-matlab-users.html
        dim_z, dim_x = m, n (num of sensors, num of variables/state values)
    """
    def __init__(self, initial_measurements: np.array, window_size=10):
        shape_tuple = np.shape(initial_measurements)
        super().__init__(dim_x=np.shape(initial_measurements)[1] if len(shape_tuple) > 1 else 1,
                         dim_z=np.shape(initial_measurements)[0],
                         window_size=window_size,
                         fused=True)
        self.init(initial_state=initial_measurements)

    def predict_and_update(self, new_measurements: np.ndarray):
        fused_measurement = np.linalg.lstsq(self.H, new_measurements, rcond=None)[0][0]
        self.last_values_window.append(fused_measurement)
        self.predict()
        self.update(new_measurements)
        # TODO: verify eq. 8 compared to 1 val. filter
        # eq. 8 calculated outside update phase (same thing)
        innovXvalRev = multiply(self.y, new_measurements)
        innovXvalRevSquared = matrix_power(innovXvalRev, 2)
        tracedResult = trace(innovXvalRevSquared)
        self.kf_error_window.appendleft(sqrt(tracedResult))

    def get_name(self) -> str:
        return "fused"


def init_filters(initial_interval=4000, window_size=10, nyquist_window=10, use_filter: bool=True,
                 use_only_filter: bool = False, constrain_analyzer: bool = False, max_oversampling: float = 1.,
                 start_with_different_measurements=False) -> Dict[str, Union[JainKalmanFilter, ChameleonKalmanFilter, FusedKalmanFilter]]:
    initial_state_array = np.array([0.0, default_measurements[0], default_measurements[0]])
    if start_with_different_measurements:
        initial_state_array = np.array([default_measurements[0], default_measurements[0], 0.0])
    initial_state_array_multi_sensor = np.array([[measurement] for measurement in initial_state_array])
    jFilter = JainKalmanFilter(window_size=window_size)
    jFilter.init(initial_state_array)
    jFilter.set_gathering_interval(initial_interval)
    cFilter = ChameleonKalmanFilter(window_size=window_size, nyquist_window=nyquist_window)
    cFilter.init(initial_state_array)
    cFilter.set_gathering_interval(initial_interval)
    cFilter.set_use_filter(use_filter)
    cFilter.set_only_filter(use_only_filter)
    cFilter.set_constrained_analyzer(constrain_analyzer=constrain_analyzer,
                                     max_oversampling=max_oversampling)
    fFilter = FusedKalmanFilter(initial_measurements=initial_state_array_multi_sensor)  # no init here now
    return {"baseline": jFilter, "chameleon": cFilter, "fused": fFilter}
