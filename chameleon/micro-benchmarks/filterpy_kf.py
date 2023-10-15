from math import sqrt
import numpy as np
from filterpy.kalman import KalmanFilter
from numpy import dot, trace, multiply
from numpy.linalg import inv, matrix_power


measurements = [
    1.04202710058, 1.10726790452, 1.2913511148, 1.48485250951, 1.72825901034,
    1.74216489744, 2.11672039768, 2.14529225112, 2.16029641405, 2.21269371128,
    2.57709350237, 2.6682215744, 2.51641839428, 2.76034056782, 2.88131780617,
    2.88373786518, 2.9448468727, 2.82866600131, 3.0006601946, 3.12920591669,
    2.858361783, 2.83808170354, 2.68975330958, 2.66533185589, 2.81613499531,
    2.81003612051, 2.88321849354, 2.69789264832, 2.4342229249, 2.23464791825,
    2.30278776224, 2.02069770395, 1.94393985809, 1.82498398739, 1.52526230354,
    1.86967808173, 1.18073207847, 1.10729605087, 0.916168349913, 0.678547664519,
    0.562381751596, 0.355468474885, 500, 900, 900,
    500, 0.355468474885, 0.572381751596, 0.658547664519, 0.916168349913,
    0.926168349913
]

kFilter = KalmanFilter(dim_x=3, dim_z=1) # 2-dim state vector, 1-dim measurement
kFilter.x = np.array([0., measurements[0], measurements[0]]) # initial state vector
kFilter.F = np.array([[1., 0.0333, 0.], [0., 1., 0.0333], [0., 0., 1.]]) # state transition matrix
kFilter.H = np.array([[1., 0., 0.]]) # measurements function
kFilter.Q = np.array([[0.05, 0.05, 0.], [0.05, 0.05, 0.], [0., 0., 0.]]) # process noise
kFilter.R = 5 # measurement noise
kFilter.P = np.array([[0.1, 0.1, 0.1], [0.1, 10000, 10], [0.1, 10, 100]]) # covariance matrix

for measurement in measurements[1:]:
    measurementArray = np.array([[measurement]])
    # normal steps
    kFilter.predict()
    kFilter.update(measurement)
    # eq. 8 calculated outside update phase (same thing)
    innovXvalRev = multiply(kFilter.y, inv(measurementArray))
    innovXvalRevSquared = matrix_power(innovXvalRev, 2)
    tracedResult = trace(innovXvalRevSquared)
    # result
    print(f"Measurement: {measurementArray}, innovationError: {kFilter.y}, tracedResult: {sqrt(tracedResult)}")

print("\n\nDone!")
