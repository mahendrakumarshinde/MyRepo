#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Jul 16 15:01:52 2018

@author: pat
"""

# =============================================================================
# Import and constants
# =============================================================================

import pandas as pd
import numpy as np
import json
from collections import defaultdict

from anaximander.utilities import nxtime, functions as fun
from anaximander.utilities.nxrange import TimeInterval, MultiTimeInterval
from dataforge.environment import Account, Device
from dataforge.baseschemas import WiFiLogs
from dataforge.connectivity import ConnectivityTransitionLogs, \
    ConnectivityStatusLogs, ConnectivityStatus, connectivity, \
    ConnectivityAssessment
from dataforge.reporting import OeeSummary5m, OeeSummary30m, OeeSummary1D, \
    OeeSummary1M

from dataforge import LOCAL, logger as LOGGER


ACCOUNTS = []  # The cached list of accounts.
IDE_PLUS_DEVICES = []

NOW = nxtime.now()
ONE_DAY_AGO = NOW - pd.Timedelta(days=1)


# =============================================================================
# Utiliies
# =============================================================================


def is_ide_plus(device):
    return device.mac.startswith("94:54:93")


def get_multi_time_intervals(transition_logs):
    """
    @return: a mapping {state: nxrange.MuliTimeInterval instance}
    """
    if not transition_logs:
        return dict()
    intervals = defaultdict(list)
    rec = transition_logs.iloc[0]
    intervals[rec.prev_state].append(
            TimeInterval(transition_logs.lower, rec.timestamp))
    rec = transition_logs.iloc[-1]
    intervals[rec.next_state].append(
            TimeInterval(rec.timestamp, transition_logs.upper))
    for r0, r1 in fun.pairwise(transition_logs):
        if r0.next_state != r1.prev_state:
            continue  # Inconsistency
        intervals[r0.next_state].append(
                TimeInterval(r0.timestamp, r1.timestamp))
    return dict((k, MultiTimeInterval(v)) for k, v in intervals.items())


def get_multi_time_statistics(multi_time_interval):
    """
    Units: second
    """
    seconds = [ti.duration.total_seconds() for ti in multi_time_interval]
    std_dev = np.std(seconds)
    mean = np.mean(seconds)
    min_, perc_5, median, perc_95, max_ = np.percentile(seconds,
                                                        [0, 5, 50, 95, 100])
    return dict(total_duration=multi_time_interval.duration.total_seconds(),
                std_dev=std_dev, mean=mean, min=min_, max=max_, median=median,
                count=len(seconds), percentile_5=perc_5, percentile_95=perc_95)

# =============================================================================
# Connectivity stats
# =============================================================================


def get_state_repartition(device, state_label, start, end):
    """
    Compute the percentage of time when the device was in each state.

    @param device: A Device instance
    @param state_label: a StateLabel instance or corresponding Status class -
        see Device.statesequence
    @param start: start time
    @param end: end time
    @return: a mapping {state: percentage}
    """
#    percentages =
#    ds = device.statesequence(state_label, start=start, end=end)


def get_connectivity_stats(device, start, end):
    """
    TBD
    Unit for durations: seconds

    @param device: A Device instance
    @param start: start time
    @param end: end time
    """
    conn_logs = device.statesequence("connectivity", start=start, end=end)
    if not conn_logs:
        return dict()  # No connectivity data
    mtis = get_multi_time_intervals(conn_logs)
    stats = get_multi_time_statistics(mtis["Disconnected"])
    new_key_stats = dict()
    for k, v in stats.items():
        new_key_stats['disc_' + k] = v
    total_seconds = (end - start).total_seconds()
    conn_percent = 100 * (1 - stats["total_duration"] / total_seconds)
    new_key_stats["connectivity"] = conn_percent
    return new_key_stats


def get_wifi_stats(device, start, end):
    """
    TBD

    @param device: A Device instance
    @param start: start time
    @param end: end time
    """
    ds = device.datasequence(WiFiLogs, start=start, end=end)
    if not ds:
        return dict()
    ssid = ""
    rssis = []
    for rec in ds:
        try:
            values = json.loads(rec.message)
        except json.JSONDecodeError:
            continue  # Ill-formatted record
        if ssid:
            if ssid != values["ssid"]:
                print("Several wifi detected!")  # Case not handled yet
                #TODO handle several Wifi
                return dict()
        else:
            ssid = values["ssid"]
        rssis.append(values["rssi"])
    min_, perc_5, median, perc_95, max_ = np.percentile(rssis,
                                                        [0, 5, 50, 95, 100])
    return dict(ssid=ssid, rssi_count=len(rssis), rssi_mean=np.mean(rssis),
                rssi_min=min_, rssi_max=max_, rssi_median=median,
                rssi_percentile_5=perc_5, rssi_percentile_95=perc_95)


def get_aggregated_stats(devices, start, end):
    """
    TBD

    @param device: A Device instance
    @param start: start time
    @param end: end time
    """
    df = pd.DataFrame()
    for dev in devices:
        conn_stats = get_connectivity_stats(dev, start, end)
        if not conn_stats:
            continue  # No connectivity data
        wifi_stats = get_wifi_stats(dev, start, end)
        if not wifi_stats:
            continue  # No wifi data
        stats = dict(mac=dev.mac)
        stats.update(conn_stats)
        stats.update(wifi_stats)
        df = df.append([stats])
    return df

# =============================================================================
# Main program
# =============================================================================


if __name__ == '__main__':
    if LOCAL:
        ACCOUNTS = Account.reload_all(local=True)
    else:
        ACCOUNTS = Account.requery_all()
    IDE_PLUS_DEVICES = [dev for acc in ACCOUNTS for dev in acc.devices()
                        if is_ide_plus(dev)]
    ALL_1_DAY_STATS = get_aggregated_stats(IDE_PLUS_DEVICES, ONE_DAY_AGO, NOW)
