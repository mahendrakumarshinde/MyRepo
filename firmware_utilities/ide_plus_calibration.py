#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sat Apr 29 18:45:30 2017

@author: pat
"""

import os
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import interactive
interactive(True)

import hamming_window as hw
from velocity_functions import (
        read_accel_data, compute_signal_rms, compute_full_velocities,
        plot_signal, split_and_compute_velocities, compute_full_velocities)


def compute_signal_range(values, survey_size=50, absolute=True):
    if absolute:
        prep_values = np.absolute(values)
    else:
        prep_values = values
    count = int(len(values) / survey_size)
    peaks = []
    for i in range(count):
        peaks.append(max(prep_values[i*survey_size:(i+1)*survey_size])) # - min(prep_values[i*survey_size:(i+1)*survey_size]))
    return max(peaks)


def plot_stroke_compare(normal_stroke_data, idle_stroke_data, axis='X'):
    fig = plt.figure()
    ax = fig.add_subplot('111')
    ax.plot(normal_stroke_data[axis], color='b', label='normal strokes')
    ax.plot(idle_stroke_data[axis], color='r', label='idle strokes')
    ax.set_xlabel('time (ms)')
    ax.set_ylabel('acceleration (m/s2)')
    ax.set_title('Normal vs idle strokes on {} axis'.format(axis))
    fig.show()


def convert_to_ms2(accel_dict_in_g, axis=['X', 'Y', 'Z']):
    """In place convertion g to m/s2"""
    for x in axis:
        for i, el in enumerate(accel_dict_in_g[x]):
            accel_dict_in_g[x][i] = el * 9.8065


def split_and_compute(accel_dict, compute_func, axis='Z', sample_count=512,
                      **kwargs):
    results = []
    max_iter = min(len(v) // sample_count for v in accel_dict.values())
    for i in range(0, max_iter):
        sample = accel_dict[axis][i * sample_count: (i + 1) * sample_count]
        results.append(compute_func(sample, **kwargs))
    return results

def plot_data(data):
    fig = plt.figure()
    ax = fig.add_subplot(111)
    colors = {'X': 'b', 'Y': 'g', 'Z': 'r'}
    for key, series in data.items():
        ax.plot(range(len(series)), series, color=colors[key], label=key)
    ax.legend()
    fig.show()


def plot_axis(data, axis='Z'):
    fig = plt.figure()
    ax = fig.add_subplot(111)
    colors = {'X': 'b', 'Y': 'g', 'Z': 'r'}
    ax.plot(range(len(data[axis])), data[axis], color=colors[axis], label=axis)
    ax.legend()
    fig.show()


def analayze(accel_dict, sampling_freq=1000, axis='Z', start_idx=512,
             length=512, plot=False):
    accel = accel_dict[axis][start_idx:start_idx + length]
    accel_rms = compute_signal_rms(accel, remove_mean=True) * 9.8065
    vel = compute_full_velocities(accel, factor=1000)
    vel_rms = compute_signal_rms(vel) * 9.8065
    if plot:
        fig = plt.figure(figsize=(12, 6), dpi=240)
        ax = fig.add_subplot('111')
        dt = 1000. / sampling_freq
        ts = [i * dt for i in range(len(accel))]
        ax.plot(ts, accel, color='b', label="Accel (rms={})".format(accel_rms))
        ax.plot(ts, vel, color='r', label="Velocity (rms={})".format(vel_rms))
        ax.legend()
        ax = plot_signal(accel)
        plot_signal(vel, ax=ax)
    return accel_rms, vel_rms


def analyze_all(all_accel_dicts, sampling_freq=1000, axis='Z', start_idx=512,
                length=512, plot=False):
    results = []
    for accel_dict in all_accel_dicts:
        results.append(analayze(accel_dict, sampling_freq=sampling_freq,
                                axis=axis, start_idx=start_idx, length=length,
                                plot=plot))
    return results


# =============================================================================
# Data analysis
# =============================================================================

if __name__ == "__main__":
    WD_OLD_Z_80HZ_5RMS = \
        '/home/pat/Downloads/Raw data/20June_OLD_Firmware/Z_Axis_80Hz_5RMS'
    WD_OLD_Z_80HZ_10RMS = \
        '/home/pat/Downloads/Raw data/20June_OLD_Firmware/Z_Axis_80Hz_10RMS'
    WD_NEW_Z_80HZ_5RMS = \
        '/home/pat/Downloads/Raw data/28June_New_Firmware/Z_Axis_80Hz_5RMS'
    WD_NEW_Z_80HZ_10RMS = \
        '/home/pat/Downloads/Raw data/28June_New_Firmware/Z_AXIS_80Hz_10RMS'
    accel0 = read_accel_data(os.path.join(WD_OLD_Z_80HZ_5RMS, 'accel_parsed'),
                             delimiter=' ')
    accel1 = read_accel_data(os.path.join(WD_OLD_Z_80HZ_10RMS, 'accel_parsed'),
                             delimiter=' ')
    accel2 = read_accel_data(os.path.join(WD_NEW_Z_80HZ_5RMS, 'accel_parsed'),
                             delimiter=' ')
    accel3 = read_accel_data(os.path.join(WD_NEW_Z_80HZ_10RMS, 'accel_parsed'),
                             delimiter=' ')
    all_accels = [accel0, accel1, accel2, accel3]
    for idx in range(1, 11):
        start = idx * 512
        print("From {} to {}:".format(start, start + 512))
        for res in analyze_all(all_accels, start_idx=start):
            print("\t" + str(res))
