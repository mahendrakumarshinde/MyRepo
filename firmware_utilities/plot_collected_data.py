#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sat Jan 13 09:20:19 2018

@author: pat
"""

import os
import csv
import matplotlib.pyplot as plt


FILEPATH = "/home/pat/Downloads/Raw data/20June_OLD_Firmware_/Z_Axis_80Hz_5RMS/accel_parsed"


def load_file():
    data = {"x": [],
            "y": [],
            "z": []}
    with open(FILEPATH) as f:
        csv_reader = csv.reader(f, delimiter=' ')
        next(csv_reader)  # Skip 1st line
        for i, row in enumerate(csv_reader):
            try:
                data['x'].append(float(row[0]))
                data['y'].append(float(row[1]))
                data['z'].append(float(row[2]))
            except Exception as e:
                print("Error at row", i, ": ", e)
    return data


def plot_data(data):
    fig = plt.figure()
    ax = fig.add_subplot(111)
    colors = {'x': 'b', 'y': 'g', 'z': 'r'}
    for key, series in data.items():
        ax.plot(range(len(series)), series, color=colors[key], label=key)
    ax.legend()
    fig.show()
