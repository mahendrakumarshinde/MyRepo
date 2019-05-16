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
import csv
import numpy as np
from collections import defaultdict
import matplotlib.pyplot as plt
import hamming_window as hw
from matplotlib import interactive
interactive(True)

WD = os.getcwd()

# Firmware constants
AXIS = ['X', 'Y', 'Z']
SAMPLING_FREQ = 1000
SAMPLE_COUNT = 512
DF = float(SAMPLING_FREQ) / float(SAMPLE_COUNT)
DT = 1000. / float(SAMPLING_FREQ)
TS = [i * DT for i in range(0, SAMPLE_COUNT)]
FS = [i * DF for i in range(0, int(SAMPLE_COUNT / 2 + 1))]
LOW_FREQ_FILTER = 0  # Hz
HIGH_FREQ_FILTER = 1000  # Hz

# =============================================================================
# Generic Functions
# =============================================================================


def q4_11_to_float(q4_11_val):
    return float(q4_11_val) / 2048


def q15_to_float(q15_val):
    return float(q15_val) / 32768


def g_to_ms2(value):
    return 9.80665 * value


def g_to_mms2(value):
    return 9806.65 * value


def get_error(value, ref_value, round_to=2):
    if ref_value == 0:
        return np.NAN
    return round(100 * ((value - ref_value) / ref_value), round_to)


def get_windowing_func_and_gain(windowing_name):
    if windowing_name == 'HAMMING':
        return hw.get_hamming_coef, hw.hamming_window_gain
    elif windowing_name == 'HANN':
        return hw.get_hann_coef, hw.hann_window_gain
    else:
        return None, 1

# =============================================================================
# Load data
# =============================================================================


def read_accel_data_inline(filepath, transform_func=lambda x: x):
    accel = dict(X=[], Y=[], Z=[])
    with open(filepath, 'r') as f:
        csv_reader = csv.reader(f)
        for row in csv_reader:
            accel[row[0]].append(transform_func(float(row[1])))
    return accel


def read_accel_data(filepath, transform_func=lambda x: x, delimiter=','):
    accel = dict(X=[], Y=[], Z=[])
    with open(filepath, 'r') as f:
        csv_reader = csv.reader(f, delimiter=delimiter)
        for row in csv_reader:
            accel['X'].append(transform_func(float(row[0])))
            accel['Y'].append(transform_func(float(row[1])))
            accel['Z'].append(transform_func(float(row[2])))
    return accel

# =============================================================================
# Load data
# =============================================================================


def split_and_compute(accel_dict, compute_func, sample_count=512, **kwargs):
    results = dict(X=[], Y=[], Z=[])
    max_iter = min(len(v) // sample_count for v in accel_dict.values())
    for k, v in accel_dict.items():
        for i in range(0, max_iter):
            sample = v[i * sample_count: (i + 1) * sample_count]
            results[k].append(compute_func(sample, **kwargs))
    return results


def plot_fft(fft_values, sample_count=512, sampling_rate=1000,
             normalize=True, ylabel='', max_freq=0, title='', save_plot=False,
             filepath=WD, close_plot=False):
    df = float(sampling_rate) / float(sample_count)
    fft_len = len(fft_values)
    freqs = [i * df for i in range(0, fft_len)]
    magnitudes = np.absolute(fft_values)
    if normalize:
        magnitudes = [m / float(sample_count) for m in magnitudes]
    fig = plt.figure(figsize=(12, 6), dpi=240)
    ax = fig.add_subplot('111')
    ax.bar(freqs, magnitudes)
    ax.set_xlabel('Frequency (Hz)')
    if ylabel:
        ax.set_ylabel(ylabel)
    if max_freq:
        ax.set_xlim(left=0, right=max_freq)
    ax.set_title(title)
    fig.show()
    if save_plot and filepath:
        filename = title or 'fft_plot'
        fig.savefig(os.path.join(filepath, filename + '.png'))
    if close_plot:
        plt.close()


def get_non_zero_magnitudes(values):
    magnitudes = np.absolute(values)
    results = []
    for i, v in enumerate(magnitudes):
        if v > 0:
            results.append((i, v))
    return results


def generate_signal(amplitude, frequency, sampling_frequency, sample_count,
                    base_func=np.cos):
    def signal(t):
        return amplitude * base_func(2 * np.pi * frequency * t)
    dt = 1. / float(sampling_frequency)
    return [signal(i * dt) for i in range(0, sample_count)]


# =============================================================================
# Acceleration specific functions
# =============================================================================


def compute_signal_energy(values, sampling_freq=1000, remove_mean=False):
    avg = np.mean(values) if remove_mean else 0
    total = np.sum((v - avg)**2 for v in values)
    return float(total) / float(sampling_freq)


def compute_signal_power(values, sampling_freq=1000, remove_mean=False):
    energy = compute_signal_energy(values, sampling_freq, remove_mean)
    T = float(len(values)) / float(sampling_freq)
    return energy / T


def compute_signal_rms(values, sampling_freq=1000, remove_mean=False):
    power = compute_signal_power(values, sampling_freq, remove_mean)
    return np.sqrt(power)


def compute_rms_from_fft(fft_values, sample_count=512):
    factor = 1 / (sample_count * np.sqrt(2))
    return np.sqrt(sum((val * factor)**2 for val in np.absolute(fft_values)))


def compute_rms_from_fft_intermediary(fft_values, sample_count=512):
    factor = 1 / (sample_count * np.sqrt(2))
    return [(val * factor)**2 for val in np.absolute(fft_values)]


def compute_fft(values, window_func=None, no_mean=False):
    sample_count = len(values)
    if no_mean:
        avg = np.mean(values)
    else:
        avg = 0
    if window_func is None:
        source_values = [v - avg for v in values]
    else:
        source_values = [(v - avg) * window_func(i, sample_count)
                         for i, v in enumerate(values)]
    return np.fft.rfft(source_values)


def filter_and_integrate(fft_values, sample_count, sampling_rate,
                         freq_lower_bound, freq_higher_bound, twice=False):
    df = float(sampling_rate) / float(sample_count)
    fft_len = len(fft_values)
    min_idx = max(int(np.ceil(float(freq_lower_bound) / df)), 1)
    max_idx = min(int(np.floor(float(freq_higher_bound) / df)),
                  fft_len)
    omega = 2. * np.pi * df
    integrated_fft = []
    # high pass filter
    for i in range(0, min_idx):
        integrated_fft.append(0)

    if twice:
        integral_factor = (1. / np.complex(0, omega))**2
        for i in range(min_idx, max_idx):
            val = fft_values[i] * integral_factor / float(max(i, 1)**2)
            integrated_fft.append(val)
    else:
        integral_factor = 1. / np.complex(0, omega)
        for i in range(min_idx, max_idx):
            val = fft_values[i] * integral_factor / float(max(i, 1))
            integrated_fft.append(val)
    # low pass filter
    for i in range(max_idx, fft_len):
        integrated_fft.append(0)
    return integrated_fft


def compute_full_velocities(accel_values, sampling_rate=1000,
                            freq_lower_bound=5, freq_higher_bound=1000,
                            factor=1, window_func=None):
    sample_count = len(accel_values)
    if window_func is None:
        accel_fft = np.fft.rfft(accel_values)
    else:
        windowed_values = [v * window_func(i, sample_count)
                           for i, v in enumerate(accel_values)]
        accel_fft = np.fft.rfft(windowed_values)
    vel_fft = filter_and_integrate(accel_fft, sample_count, sampling_rate,
                                   freq_lower_bound, freq_higher_bound)
    vel_values = np.fft.irfft(vel_fft)
    if not (window_func is None):
        vel_values = [v / window_func(i, sample_count)
                      for i, v in enumerate(vel_values)]
    return [v * factor for v in vel_values]


def split_and_compute_velocities(accel_dict, sample_count=512,
                                 sampling_rate=1000, freq_lower_bound=5,
                                 freq_higher_bound=1000, factor=1,
                                 window_func=None):
    results = dict(X=[], Y=[], Z=[])
    max_iter = min(len(v) // sample_count for v in accel_dict.values())
    for k, v in accel_dict.items():
        for i in range(0, max_iter):
            sample = v[i * sample_count: (i + 1) * sample_count]
            velocities = compute_full_velocities(
                                    sample,
                                    sampling_rate=sampling_rate,
                                    freq_lower_bound=freq_lower_bound,
                                    freq_higher_bound=freq_higher_bound,
                                    factor=factor,
                                    window_func=window_func)
            results[k].append(compute_signal_rms(velocities, sampling_rate))
    return results


def get_characteristic_frequency(accel_rms, velocity_rms):
    return accel_rms / (2 * np.pi * velocity_rms)

# =============================================================================


def teensy_scaling_function(freq):
    """
    Firmware scaling: 1 + 0.00266 * idx + 0.000106 * idx**2 where idx is the
    idx of the FFT corresponding to the frequency of the highest peak.
    """
    return 1 + 0.00136192 * freq + 2.7787264e-05 * freq**2


def compute_fft_and_scale(values, sample_count=512, sampling_freq=1000,
                          window_func=None, no_mean=False,
                          scaling_function=lambda x: 1):
    df = float(sampling_freq) / float(sample_count)
    ftt_values = compute_fft(values, window_func=window_func, no_mean=no_mean)
    for i, v in enumerate(ftt_values):
        ftt_values[i] = scaling_function(i * df) * v
    return ftt_values


def plot_all_ffts(fft_dict, base_tile, ylabel, sample_count=512, indexes='all',
                  sampling_rate=1000, normalize=True, filepath=WD):
    if indexes == 'all':
        indexes = range(len(indexes))
    if isinstance(indexes, str):
        indexes = [int(idx) for idx in indexes]
    title_format = '{} on axis {} @ {}ms'
    for k, all_values in fft_dict.items():
        for idx in indexes:
            fft_values = all_values[idx]
            title = title_format.format(base_tile, k, str(idx * sample_count))
            plot_fft(fft_values, sample_count=sample_count,
                     sampling_rate=sampling_rate, normalize=normalize,
                     ylabel=ylabel, title=title, save_plot=True,
                     filepath=filepath, close_plot=True)


def integrate_all_ffts(fft_dict, sample_count, sampling_rate, freq_lower_bound,
                       freq_higher_bound, twice=False):
    integrated_ffts = dict()
    for key in fft_dict.keys():
        integrated_ffts[key] = []
    for k, all_values in fft_dict.items():
        for i, fft_values in enumerate(all_values):
            integrated_ffts[k].append(filter_and_integrate(
                    fft_values, sample_count, sampling_rate, freq_lower_bound,
                    freq_higher_bound, twice=twice))
    return integrated_ffts


def compute_all_invert_ffts(fft_dict):
    values_dict = dict()
    for key in fft_dict.keys():
        values_dict[key] = []
    for k, all_values in fft_dict.items():
        for i, fft_values in enumerate(all_values):
            values_dict[k].append(np.fft.irfft(fft_values))
    return values_dict


def compute_all_signal_rms(value_dict, sampling_freq=1000, remove_mean=False):
    rms_dict = dict()
    for key in value_dict.keys():
        rms_dict[key] = []
    for k, all_values in value_dict.items():
        for i, values in enumerate(all_values):
            rms_dict[k].append(compute_signal_rms(
                    values, sampling_freq=sampling_freq,
                    remove_mean=remove_mean))
    return rms_dict


def plot_rms(rms, filtered_rms=[], sample_gap=512, ylabel='', start_time=0,
             title='', filter_freq=0, filepath=WD, save_plot=False,
             close_plot=False):
    fig = plt.figure()
    ax = fig.add_subplot('111')
    ts = [start_time + i * sample_gap for i in range(0, len(rms))]
    if filtered_rms:
        ax.plot(ts, rms, label='Unfiltered RMS')
        if filter_freq:
            label = 'Filtered RMS (high-pass {}Hz)'.format(filter_freq)
        else:
            label = '0-centered Signal RMS'
        ax.plot(ts, filtered_rms, color='r', label=label)
    else:
        ax.plot(ts, rms, label='RMS')
    ax.set_xlabel('time (ms)')
    if ylabel:
        ax.set_ylabel(ylabel)
    ax.legend()
    if title:
        ax.set_title(title)
    fig.show()
    if save_plot and filepath:
        filename = title or 'RMS'
        fig.savefig(os.path.join(filepath, filename + '.png'))
    if close_plot:
        plt.close()


def plot_all_rms(rms_dict, filtered_rms_dict={}, sample_gap=512,
                 ylabel='Velocity (mm/s)', base_title='Velocities RMS',
                 filter_freq=0, filepath=WD):
    title_format = '{} ({}ms intervals) on axis {}'
    for k, rms in rms_dict.items():
        filtered_rms = filtered_rms_dict.get(k)
        title_format.format(base_title, str(sample_gap), k)
        plot_rms(rms, filtered_rms=filtered_rms, sample_gap=sample_gap,
                 ylabel=ylabel, start_time=0,
                 title=title_format.format(base_title, str(sample_gap), k),
                 filter_freq=filter_freq, filepath=filepath,
                 save_plot=True, close_plot=True)


def plot_signal(values, sampling_freq=1000, ylabel='', title='', xmin=0,
                xmax=0, filepath=WD, expected_rms=None,
                save_plot=False, close_plot=False, ax=None):
    dt = 1000. / sampling_freq
    ts = [i * dt for i in range(len(values))]
    if not ax:
        fig = plt.figure(figsize=(12, 6), dpi=240)
        ax = fig.add_subplot('111')
    ax.plot(ts, values)
    if expected_rms:
        avg = np.mean(values)
        ax.axhline(avg, color='r', linestyle='--')
        ax.axhline(avg + expected_rms * np.sqrt(2), color='r', linestyle='--')
        ax.axhline(avg - expected_rms * np.sqrt(2), color='r', linestyle='--')
    if xmin != 0 or xmax != 0:
        xmax = xmax or (dt + 1) * len(values)
        ax.set_xlim(left=xmin, right=xmax)
    ax.set_xlabel('time (ms)')
    if ylabel:
        ax.set_ylabel(ylabel)
    if title:
        ax.set_title(title)
    if not ax:
        fig.show()
    if save_plot and filepath:
        filename = title or 'Signal'
        fig.savefig(os.path.join(filepath, filename + '.png'))
    if close_plot:
        plt.close()
    return ax


def plot_all_signals(accel_dict, sampling_freq=1000, ylabel='', base_title='',
                     expected_rms=dict(), xmin=0, xmax=0, filepath=WD):
    for k, signal in accel_dict.items():
        plot_signal(signal,
                    sampling_freq=SAMPLING_FREQ,
                    ylabel=ylabel,
                    title=base_title + ' on axis ' + k,
                    filepath=WD,
                    xmin=xmin,
                    xmax=xmax,
                    expected_rms=expected_rms.get(k),
                    save_plot=True,
                    close_plot=True)

# =============================================================================
# Complete analysis
# =============================================================================


def get_all_results(all_accels, all_accel_frequencies, all_accel_ref_values,
                    all_vel_ref_values, all_ref_axis, windowing_name="",
                    no_mean=False):
    """


    Example:
    all_accels = [accel_data_16Hz_8rms_1vel_rms, a_80Hz_4rms_2vel_rms]
    all_accel_frequencies = [15.9, 80]
    all_accel_ref_values = [1000, 2000]
    all_vel_ref_values = [10, 4]
    all_ref_axis = ['Z', 'Z']
    """
    all_results = defaultdict(list)
    all_results["all_accel_frequencies"] = all_accel_frequencies
    all_results["all_accel_ref_values"] = all_accel_ref_values
    all_results["all_vel_ref_values"] = all_vel_ref_values
    all_results["all_ref_axis"] = all_ref_axis
    all_results["windowing_name"] = windowing_name
    all_results["no_mean"] = windowing_name
    window_func, window_gain = get_windowing_func_and_gain(windowing_name)
    all_results["window_gain"] = window_gain
    # Accel RMS
    for accel in all_accels:
        rms = split_and_compute(accel, compute_signal_rms,
                                sample_count=SAMPLE_COUNT,
                                sampling_freq=SAMPLING_FREQ,
                                remove_mean=False)
        all_results["all_accel_rms"].append(rms)
        rms_no_mean = split_and_compute(accel, compute_signal_rms,
                                        sample_count=SAMPLE_COUNT,
                                        sampling_freq=SAMPLING_FREQ,
                                        remove_mean=True)
        all_results["all_accel_rms_no_mean"].append(rms_no_mean)
        all_axis_rms = [np.sqrt(sum(axis_rms[i]**2
                                    for axis_rms in rms_no_mean.values()))
                        for i in range(len(rms_no_mean[AXIS[0]]))]
        all_results["all_axis_accel_rms"].append(all_axis_rms)
    # Velocity RMS
    for i, accel_rms in enumerate(all_results["all_accel_rms_no_mean"]):
        all_results["all_vel_rms_theoretical"].append(dict())
        for k, rms_values in accel_rms.items():
            all_results["all_vel_rms_theoretical"][i][k] = \
                [rms / (2. * np.pi * all_accel_frequencies[i])
                 for rms in rms_values]
    for i, all_axis_rms in enumerate(all_results["all_axis_accel_rms"]):
        all_results["all_axis_vel_rms_theoretical"].append(
                [rms / (2. * np.pi * all_accel_frequencies[i])
                 for rms in all_axis_rms])
    # Accel FFTs
    for accel in all_accels:
        accel_fft = split_and_compute(accel,
                                      compute_fft_and_scale,
                                      sample_count=SAMPLE_COUNT,
                                      sampling_freq=SAMPLING_FREQ,
                                      window_func=window_func,
                                      no_mean=no_mean)
        all_results["all_accel_ffts"].append(accel_fft)
    for accel_fft in all_results["all_accel_ffts"]:
        scaled_accel = compute_all_invert_ffts(accel_fft)
        all_results["all_scaled_accel"].append(scaled_accel)
    for i, scaled_accel in enumerate(all_results["all_scaled_accel"]):
        all_results["all_accel_rms_scaled"].append(dict())
        for k, accel_values in scaled_accel.items():
            all_results["all_accel_rms_scaled"][i][k] = [
                    compute_signal_rms(vals, sampling_freq=SAMPLING_FREQ,
                                       remove_mean=True)
                    for vals in accel_values]
    # Filtered FFTs
    filtering_idx = max(int(np.ceil(float(LOW_FREQ_FILTER) / DF)), 1)
    for accel_ffts in all_results["all_accel_ffts"]:
        filtered_accel_fft = dict()
        for k, fft_list in accel_ffts.items():
            filtered_accel_fft[k] = []
            for fft in fft_list:
                filtered_fft = fft.copy()
                for i in range(filtering_idx):
                    filtered_fft[i] = 0
                filtered_accel_fft[k].append(filtered_fft)
        all_results["all_filtered_accel_ffts"].append(filtered_accel_fft)
    for filtered_fft in all_results["all_filtered_accel_ffts"]:
        filtered_accel = compute_all_invert_ffts(filtered_fft)
        all_results["all_filtered_accels"].append(filtered_accel)
    for filtered_accel in all_results["all_filtered_accels"]:
        filtered_rms = dict()
        for k, accel_series in filtered_accel.items():
            filtered_rms[k] = [compute_signal_rms(values, SAMPLING_FREQ)
                               for values in accel_series]
        all_results["all_filtered_rms"].append(filtered_rms)
    # FFT integration
    for accel_fft in all_results["all_accel_ffts"]:
        vel_fft = integrate_all_ffts(accel_fft, SAMPLE_COUNT, SAMPLING_FREQ,
                                     LOW_FREQ_FILTER, HIGH_FREQ_FILTER,
                                     twice=False)
        all_results["all_vel_ffts"].append(vel_fft)
    for vel_fft in all_results["all_vel_ffts"]:
        velocities = compute_all_invert_ffts(vel_fft)
        all_results["all_velocities"].append(velocities)
    for velocities in all_results["all_velocities"]:
        vel_rms = compute_all_signal_rms(velocities)
        all_results["all_vel_rms_experimental"].append(vel_rms)
    for i, vel_rms in enumerate(all_results["all_vel_rms_experimental"]):
        all_axis_rms = [np.sqrt(sum(axis_rms[i]**2
                                    for axis_rms in vel_rms.values()))
                        for i in range(len(vel_rms[AXIS[0]]))]
        all_results["all_axis_vel_rms_experimental"].append(all_axis_rms)
    return all_results


def print_report(all_accels, all_results, index, primary_axis_only=False,
                 output_file=None):
    """

    @param all_accels:
    @param all_results: A dict with all the results, as returned by
        get_all_results.
    @param index: The index of the accel series for which print the report.
    @param primary_axis_only:
    @param output_file: If None, will print the report to the console, else
        will write it to given file.
    """
    if output_file and not output_file.closed and output_file.writable:
        def println(txt):
            output_file.write(txt + "\n")
    else:
        def println(txt):
            print(txt)
    msg = 'Conditions: '
    if all_results["no_mean"]:
        msg += 'Mean removed - '
    if all_results["windowing_name"]:
        msg += 'Windowing {} - '.format(all_results["windowing_name"])
    println(msg + 'Filtering frequencies {}Hz - {}Hz'.format(LOW_FREQ_FILTER,
                                                             HIGH_FREQ_FILTER))
    println('Sample length / N =: {}'.format(SAMPLE_COUNT))
    println('Sample start at (ms): {}'.format(SAMPLE_COUNT * index))
    subtitle_format = 'test at: {}Hz, {}mm.s-2, and {}mm.s-1:'
    for i in range(len(all_accels)):
        println(subtitle_format.format(all_results["all_accel_frequencies"][i],
                                       all_results["all_accel_ref_values"][i],
                                       all_results["all_vel_ref_values"][i]))
        for ax in AXIS:
            if primary_axis_only and all_results["all_ref_axis"][i] != ax:
                continue
            println('Axis {}:'.format(ax))
            println('\tAcceleration RMS:')
            if all_results["all_ref_axis"][i] == ax:
                accel_ref_rms = all_results["all_accel_ref_values"][i]
            else:
                accel_ref_rms = 0
            println('\tReference value (mm/s2): {}'.format(
                round(accel_ref_rms, 4)))
            println('\tExperiment value (mm/s2): {}'.format(round(
                all_results["all_accel_rms_no_mean"][i][ax][index], 4)))
            println('\t\tError (%): {}'.format(
                get_error(all_results["all_accel_rms_no_mean"][i][ax][index],
                          accel_ref_rms)))
            println('')
            println('\tVelocity RMS')
            if all_results["all_ref_axis"][i] == ax:
                vel_ref_rms = all_results["all_vel_ref_values"][i]
            else:
                vel_ref_rms = 0
            window_gain = all_results["window_gain"]
            println('\tReference (mm/s): {}'.format(round(vel_ref_rms, 4)))
            println('\tTheoretical (mm/s): {}'.format(
                round(all_results["all_vel_rms_theoretical"][i][ax][index],
                      4)))
            println('\t\tError (%): {}'.format(
                get_error(all_results["all_vel_rms_theoretical"][i][ax][index],
                          vel_ref_rms)))
            println('\tExperimental (mm/s): {}'.format(
                round(window_gain *
                      all_results["all_vel_rms_experimental"][i][ax][index],
                      4)))
            err = get_error(
                    window_gain *
                    all_results["all_vel_rms_experimental"][i][ax][index],
                    vel_ref_rms)
            println('\t\tError (%): {}'.format(err))
        println('------')
        println('')


def print_all_reports(all_accels, all_results, sample_count=SAMPLE_COUNT,
                      target=WD, base_name='report', primary_axis_only=False):
    max_index = int(min(np.floor(len(accel[ax]) / sample_count)
                        for accel, ax in zip(all_accels,
                                             all_results["all_ref_axis"])))
    for idx in range(max_index):
        filepath = os.path.join(target, '{}_{}.txt'.format(base_name, idx))
        with open(filepath, 'w') as f:
            print_report(all_accels, all_results, idx,
                         primary_axis_only=primary_axis_only,
                         output_file=f)
