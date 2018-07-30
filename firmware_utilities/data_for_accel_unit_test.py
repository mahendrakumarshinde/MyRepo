#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Apr 16 18:18:34 2017

@author: pat
"""

# =============================================================================
# Imports and constants
# =============================================================================

import os
import numpy as np

from velocity_functions import (
    read_accel_data, q15_to_float, compute_signal_rms, compute_fft_and_scale,
    filter_and_integrate,
    SAMPLE_COUNT, SAMPLING_FREQ, LOW_FREQ_FILTER, HIGH_FREQ_FILTER)

# =============================================================================
# For Arduino unit tests
# =============================================================================


DATA_FOLDER = "/home/pat/workspace/productivity/firmware_utilities/calibration_data/butterfly_data_2017_05_07"

raw_data = read_accel_data(os.path.join(DATA_FOLDER, '80 Hz, 2m per s2.csv'),
                           delimiter=',')
accel_q15 = raw_data['Z'][3*512:4*512]
scaled_accel_q15 = [a * 4 for a in accel_q15]
accel = [q15_to_float(4 * x) for x in accel_q15]

# in Q15
accel_rms_q15 = compute_signal_rms(accel_q15)
accel_rms_no_mean_q15 = compute_signal_rms(accel_q15, remove_mean=True)
accel_fft_q15 = compute_fft_and_scale(accel_q15)
vel_fft_q15 = filter_and_integrate(accel_fft_q15, SAMPLE_COUNT, SAMPLING_FREQ,
                                   LOW_FREQ_FILTER, HIGH_FREQ_FILTER)
velocities_q15 = np.fft.irfft(vel_fft_q15)
vel_rms_q15 = compute_signal_rms(velocities_q15)

# in Q15
scaled_accel_rms_q15 = compute_signal_rms(scaled_accel_q15)
scaled_accel_rms_no_mean_q15 = compute_signal_rms(scaled_accel_q15,
                                                  remove_mean=True)
scaled_accel_fft_q15 = compute_fft_and_scale(scaled_accel_q15)
scaled_vel_fft_q15 = filter_and_integrate(scaled_accel_fft_q15, SAMPLE_COUNT,
                                          SAMPLING_FREQ,
                                          LOW_FREQ_FILTER,
                                          HIGH_FREQ_FILTER)
scaled_velocities_q15 = np.fft.irfft(scaled_vel_fft_q15)
scaled_vel_rms_q15 = compute_signal_rms(scaled_velocities_q15)

# In float in Gs
accel_rms = compute_signal_rms(accel)
accel_rms_no_mean = compute_signal_rms(accel, remove_mean=True)
accel_fft = compute_fft_and_scale(accel)
vel_fft = filter_and_integrate(accel_fft, SAMPLE_COUNT, SAMPLING_FREQ,
                               LOW_FREQ_FILTER, HIGH_FREQ_FILTER)
velocities = np.fft.irfft(vel_fft)
vel_rms = compute_signal_rms(velocities)
