#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Utilities for Serial reading, data parsing and dumping.

Copyright (C) Infinite Uptime.
"""


import os
import serial
import csv
import struct
import datetime
import time

import matplotlib.pyplot as plt


# =============================================================================
# Global variables
# =============================================================================

PATH = "/home/pat/workspace/throughput/scripts/test"
AUDIO_FILENAME = "audio_parsed"
AUDIO_RAW_FILENAME = "audio_raw_notimestamp"
ACCEL_FILENAME = "accel_parsed"

COM_PORT = "/dev/ttyACM0"
BAUD_RATE = 115200
INTER_TIMESTAMP = False


# =============================================================================
# Functions
# =============================================================================

def read_value():
    ser = serial.Serial(COM_PORT, BAUD_RATE)

    print("Flushing serial...")
    ser.flush()

    print("Opening files...")
    audio_file = open(os.path.join(PATH, AUDIO_FILENAME), 'wb')
    audio_raw_file = open(os.path.join(PATH, AUDIO_RAW_FILENAME), 'wb')
    accel_file = open(os.path.join(PATH, ACCEL_FILENAME), 'wb')

    print("Marking timestamp...")
    st = str(datetime.datetime.utcnow()) + "\n"

    audio_file.write(st.encode())
    accel_file.write(st.encode())

    totalSize = 0
    BATCH_SIZE = 36
    audio_samp_rate = 8
    accel_samp_rate = 1000

#    if (len(sys.argv) > 7):
#        SetAccelerometerRange(ser,Arange)
#        print("A Range value sent...")
#
#    if (len(sys.argv) > 8):
#        SetRGBLED(ser,RGBLEDcolor)
#        print("A RGBLED value sent...")
#
#    if (len(sys.argv) > 9):
#        audio_samp_rate=SetAcousticSamplingRate(ser,acoSR)
#        print("A new acoustic sampling rate sent...")
#
#    if (len(sys.argv) > 10):
#        accel_samp_rate=SetAccelerometerSamplingRate(ser,accSR)
#        print("A new accelerometer sampling rate sent...")

    ser.write(b"IUCMD_START")
#    SetAccelerometerRange(ser, 1)
#    time.sleep(0.1)
#    SetRGBLED(ser, "100")
#    time.sleep(0.1)
#    audio_samp_rate = SetAcousticSamplingRate(ser, 4)
#    time.sleep(0.1)
#    accel_samp_rate = SetAccelerometerSamplingRate(ser, 7)
    time.sleep(0.1)

    BATCH_SIZE = int(1000.0 * float(audio_samp_rate) /
                     float(accel_samp_rate)) * 3+12
    print("Batch size =", BATCH_SIZE)
    # Initial polling
    print("Flushing serial input again...")
    ser.flush()
    ser.flushInput()
    print("Polling")

    all_ts = []
    all_audio_data = []
    all_audio_data_parsed = []
    all_accel_data_parsed = []

    start_time = time.time()
    try:
        while True:
            bytesToRead = ser.inWaiting()
            if (bytesToRead != 0):
                batchAlignCheck = bytesToRead % BATCH_SIZE
                if (batchAlignCheck == 0):
                    totalSize += bytesToRead
                    all_ts.append(datetime.datetime.utcnow())
                    if INTER_TIMESTAMP:
                        st = str(datetime.datetime.utcnow()) + "\n"
                        audio_file.write(st.encode())
                        accel_file.write(st.encode())
                    print((time.time() - start_time) * 1000,
                          "Read data:", str(bytesToRead),
                          "bytes, total:", str(totalSize))
                    numBatches = bytesToRead / BATCH_SIZE
                    sampling_ratio = int(float(audio_samp_rate) * 1000.0 /
                                         float(accel_samp_rate))

                    for i in range(int(numBatches)):
                        for i in range(sampling_ratio):  # 8000 Hz default
                            audio_data = ser.read(3)
                            # PARSE 3 BYTE INT
                            audio_data_parsed = [struct.unpack('>i', audio_data[i:i+3] + b'\x00')[0] >> 8 for i in range(0, len(audio_data), 3)][0]
                            msg = str(audio_data_parsed) + "\n"

                            all_audio_data_parsed.append(msg.encode())
                            all_audio_data.append(audio_data)
                        li = []
                        for i in range(3):  # 1000 Hz default
                            accel_data = ser.read(4)
                            accel_data_parsed = struct.unpack('<f',
                                                              accel_data)[0]
                            msg = str(accel_data_parsed) + " "
                            li.append(msg.encode())
                        all_accel_data_parsed.append(li)
                else:
                    ser.read_all()
                    print((time.time() - start_time) * 1000, "=> ERROR: THROW AWAY " + str(bytesToRead) + " bytes")
                    ser.reset_input_buffer()
    except KeyboardInterrupt:
        ser.write(b"IUCMD_END")
        ser.close()
        for ts, audio, audio_raw, accel in zip(all_ts, all_audio_data_parsed,
                                               all_audio_data,
                                               all_accel_data_parsed):
            if INTER_TIMESTAMP:
                st = str(ts) + "\n"
                audio_file.write(st.encode())
                accel_file.write(st.encode())
            audio_file.write(audio)
            audio_raw_file.write(audio_raw)
            for a in accel:
                accel_file.write(a)
            accel_file.write(b"\n")
        audio_file.close()
        audio_raw_file.close()
        accel_file.close()


def load_accel_file(filepath):
    data = {"x": [],
            "y": [],
            "z": []}
    with open(filepath) as f:
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


def plot_accel_data(data):
    fig = plt.figure()
    ax = fig.add_subplot(111)
    colors = {'x': 'b', 'y': 'g', 'z': 'r'}
    for key, series in data.items():
        ax.plot(range(len(series)), series, color=colors[key], label=key)
    ax.legend()
    fig.show()


def show_accel_data():
    data = load_accel_file(os.path.join(PATH, ACCEL_FILENAME))
    plot_accel_data(data)
    return data


def convert_q15_to_float(q15_bytes):
    """
    Convert the given Q15 number (as 2 bytes) into a float.

    The function decodes the bytes and then do:
        Float = Q15 / 2**15
    q15_bytes is expected to be 2 bytes long, an exception will be raised if
    not.

    @param q15_bytes: 2 bytes representing a Q15 number.
    """
    q15_int = struct.unpack('>i', q15_bytes + b'\x00\x00')[0] >> 16
    return q15_int / 32768


def convert_q15_array_to_floats(q15_byte_array):
    """
    Convert the Q15 numbers (as array of bytes) into floats.

    see convert_q15_to_float.
    """
    if len(q15_byte_array) % 2 > 0:
        raise ValueError("The Q15 byte array length is expected to be a " +
                         "multiple of 2 (Q15 numbers are 2 bytes long).")
    values = []
    for i in range(len(q15_byte_array) // 2):
        values[i] = convert_q15_to_float(q15_byte_array[i:i + 2])
    return values


def convert_q31_to_float(q31_bytes):
    """
    Convert the given Q31 number (as 4 bytes) into a float.

    The function decodes the bytes and then do:
        Float = Q31 / 2**31
    q31_bytes is expected to be 4 bytes long, an exception will be raised if
    not.

    @param q31_bytes: 4 bytes representing a Q31 number.
    """
    return struct.unpack('>i', q31_bytes)[0] / 2147483648


def convert_q31_array_to_floats(q31_byte_array):
    """
    Convert the Q15 numbers (as array of bytes) into floats.

    see convert_q31_to_float.
    """
    if len(q31_byte_array) % 4 > 0:
        raise ValueError("The Q31 byte array length is expected to be a " +
                         "multiple of 4 (Q31 numbers are 4 bytes long).")
    values = []
    for i in range(len(q31_byte_array) // 4):
        values[i] = convert_q31_to_float(convert_q31_to_float[i:i + 4])
    return values


# =============================================================================
# Execution
# =============================================================================

if __name__ == '__main__':
    read_value()
    data = show_accel_data()
