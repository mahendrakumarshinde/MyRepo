#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# IDE Firmware Flashing Utility v2.1
# Copyright (C) Infinite Uptime Inc.
# Author: John Wang<john@infinite-uptime.com.cn>


import sys
import os
import re
import serial
import serial.tools.list_ports
import win32com.client
import time
import esptool
import subprocess
import ctypes
import enum
import traceback
import logging
import argparse
import sys
from endpoint_utilities import endpoints
from utils.log import logger
import hashlib
import shutil
__version__ = 'v2.1'


# ShellExecute Errors
class ShellExecuteError(enum.IntEnum):
    ZERO = 0
    FILE_NOT_FOUND = 2
    PATH_NOT_FOUND = 3
    BAD_FORMAT = 11
    ACCESS_DENIED = 5
    ASSOC_INCOMPLETE = 27
    DDE_BUSY = 30
    DDE_FAIL = 29
    DDE_TIMEOUT = 28
    DLL_NOT_FOUND = 32
    NO_ASSOC = 31
    OOM = 8
    SHARE = 26


# Device IDs
VID_RUNTIME = 0x1209
VID_RUNTIME_STR = '1209'
PID_RUNTIME = 0x6666
PID_RUNTIME_STR = '6666'
VID_DFU = 0x0483
VID_DFU_STR = '0483'
PID_DFU = 0xdf11
PID_DFU_STR = 'df11'
RUNTIME_ID_STRING = 'VID_1209&PID_6666'
DFU_ID_STRING = 'VID_0483&PID_DF11'

# Reduce the Retries to 20
SERIAL_MAX_TRIES = 5000
RESET_MAX_TRIES = 5000
DETECT_MAX_TRIES = 5000
FLASH_MAX_TRIES = 5000
WIFI_MAX_TRIES = 5000
RESTART_MAX_RETIES = 5000

# Baudrates
RESET_BAUDRATE = 1200
FLASH_BAUDRATE = 115200

total_ides = 0

# Directories
FIRMWARE_DIR = os.path.join(os.getcwd(), 'firmware_binaries')
FIRMWARE_VERSION_FILEPATH = os.path.join(os.getcwd(), 'firmware_binaries/version.json')
UTIL_DIR = os.path.join(os.getcwd(), 'utils')
CLI_DIR = os.path.join(os.getcwd(), 'utils/CLI')
REPORT_DIR = os.path.join(os.getcwd(), 'reports')
CUSTOM_BACKEND_FILEPATH = os.path.join(os.getcwd(), 'backends.json')

MAIN_FW_BIN_FILE        = os.path.join(FIRMWARE_DIR,"vEdge_main.bin")
MAIN_FW_MD5_FILE        = os.path.join(FIRMWARE_DIR,"vEdge_main.md5")
WIFI_FW_BIN_FILE        = os.path.join(FIRMWARE_DIR,"vEdge_wifi.bin")
WIFI_FW_MD5_FILE        = os.path.join(FIRMWARE_DIR,"vEdge_wifi.md5")

WIFI_BOOT_BIN_FILE      = os.path.join(FIRMWARE_DIR,"vEdge_wifi_boot.bin")
WIFI_BOOT_MD5_FILE      = os.path.join(FIRMWARE_DIR,"vEdge_wifi_boot.md5")
WIFI_BOOTLOADER_BIN_FILE = os.path.join(FIRMWARE_DIR,"vEdge_wifi_bootloader.bin")
WIFI_BOOTLOADER_MD5_FILE = os.path.join(FIRMWARE_DIR,"vEdge_wifi_bootloader.md5")
WIFI_PARTITION_BIN_FILE  = os.path.join(FIRMWARE_DIR,"vEdge_wifi_partition.bin")
WIFI_PARTITION_MD5_FILE  = os.path.join(FIRMWARE_DIR,"vEdge_wifi_partition.md5")


serial_numbers = None

class DeviceException(Exception):
    def __init__(self, code, message):
        self.code = code
        self.message = message
        time.sleep(10)


def make_dirs():
    os.makedirs(FIRMWARE_DIR, exist_ok=True)
    os.makedirs(REPORT_DIR, exist_ok=True)


def detect_file(filename):
    """
    Check whether specfic file exists
    :param filename:
    :return: None or raise an Exception
    """
    if not os.path.isfile(filename):
        raise DeviceException('005', 'Missing file: %s' % filename)


def check_dependencies():
    """
    Ensure dependencies exist.
    :return: None or raise an Exception
    """
    logger.info('Checking dependencies and firmware binaries...')

    #detect_file(os.path.join(UTIL_DIR, 'dfu-util-static.exe'))
    detect_file(os.path.join(CLI_DIR, 'STM32_Programmer_CLI.exe'))
    detect_file(os.path.join(UTIL_DIR, 'install-dfu-driver.cmd'))
    detect_file(os.path.join(UTIL_DIR, 'zadic.exe'))
    detect_file(os.path.join(UTIL_DIR, 'esptool.exe'))
    detect_file(os.path.join(FIRMWARE_DIR, 'vEdge_main.bin'))
    detect_file(os.path.join(FIRMWARE_DIR, 'vEdge_init.bin'))
    #detect_file(os.path.join(FIRMWARE_DIR, 'boot_app0.bin'))
    #detect_file(os.path.join(FIRMWARE_DIR, 'bootloader_qio_80m.bin'))
    #detect_file(os.path.join(FIRMWARE_DIR, 'wifi_station_ESP32.ino.partitions.bin'))
    detect_file(os.path.join(FIRMWARE_DIR, 'vEdge_wifi.bin'))
    detect_file(os.path.join(FIRMWARE_DIR, 'vEdge_bl1.bin'))
    detect_file(os.path.join(FIRMWARE_DIR, 'vEdge_bl2.bin'))
    detect_file(os.path.join(FIRMWARE_DIR, 'vEdge_fact.bin'))

    detect_file(os.path.join(FIRMWARE_DIR, 'vEdge_wifi_boot.bin'))    
    detect_file(os.path.join(FIRMWARE_DIR, 'vEdge_wifi_bootloader.bin'))
    detect_file(os.path.join(FIRMWARE_DIR, 'vEdge_wifi_partition.bin'))    


    logger.info('All dependencies and firmware binaries are ready!')


def check_dfu_driver():
    """
    Check if DFU USB driver is installed.
    :return: None or raise an Exception
    """
    logger.info('Checking and installing DFU driver...')

    cmd = '"%s" "%s"' % (os.path.join(UTIL_DIR, 'install-dfu-driver.cmd'), UTIL_DIR)
    rc = subprocess.call(cmd)
    if rc != 0:
        raise DeviceException('009', 'Unable to install libwdi DFU driver.')
    else:
        logger.info('Libwdi DFU driver is allready installed.')


def get_serial_ports():
    """
    Get all IDE serial devices
    :return: list of device instances
    """
    logger.info("Detecting serial devices...")
    ide_ports = []
    serail_tries_count = 0
    while serail_tries_count < SERIAL_MAX_TRIES:
        serail_tries_count += 1
        port_list = list(serial.tools.list_ports.comports())
        for port in port_list:
            if port.vid == VID_RUNTIME and port.pid == PID_RUNTIME:
                logger.info('Found serial device %s: %s' % (port.device, port.usb_info()))
                ide_ports.append(port)
        if len(ide_ports) > 0:
            return ide_ports
        time.sleep(2)

    raise DeviceException('003', 'No serial port detected!')


def copy_fw_files(LogicalDisk_DeviceID):
    try:
        ROLLBACK_FW_DIR = os.path.join(LogicalDisk_DeviceID,"iuRollbackFirmware")
        BACKUP_FW_DIR = os.path.join(LogicalDisk_DeviceID,"iuBackupFirmware")
        TEMP_FW_DIR = os.path.join(LogicalDisk_DeviceID,"iuTempFirmware")
        os.makedirs(ROLLBACK_FW_DIR, exist_ok=True)
        os.makedirs(BACKUP_FW_DIR, exist_ok=True)
        print("Copying files, Please wait....")

        #print("Copying WIFI_BOOT BIN file to iuTempFirmware")
        shutil.copy(WIFI_BOOT_BIN_FILE,TEMP_FW_DIR)
        #print("Copying WIFI_BOOT MD5 file to iuTempFirmware")
        shutil.copy(WIFI_BOOT_MD5_FILE,TEMP_FW_DIR)
        #print("Copying WIFI_BOOT_LOADER BIN file to iuTempFirmware")
        shutil.copy(WIFI_BOOTLOADER_BIN_FILE,TEMP_FW_DIR)
        #print("Copying WIFI_BOOT_LOADER MD5 file to iuTempFirmware")
        shutil.copy(WIFI_BOOTLOADER_MD5_FILE,TEMP_FW_DIR)
        #print("Copying WiFi FW MD5 file to iuTempFirmware")
        shutil.copy(WIFI_PARTITION_BIN_FILE,TEMP_FW_DIR)
        #print("Copying WiFi FW MD5 file to iuTempFirmware")
        shutil.copy(WIFI_PARTITION_MD5_FILE,TEMP_FW_DIR)

        #print("Copying Main FW Bin file to Rollback")
        shutil.copy(MAIN_FW_BIN_FILE,ROLLBACK_FW_DIR)
        #print("Copying Main FW MD5 file to Rollback")
        shutil.copy(MAIN_FW_MD5_FILE,ROLLBACK_FW_DIR)
        #print("Copying WiFi FW Bin file to Rollback")
        shutil.copy(WIFI_FW_BIN_FILE,ROLLBACK_FW_DIR)
        #print("Copying WiFi FW MD5 file to Rollback")
        shutil.copy(WIFI_FW_MD5_FILE,ROLLBACK_FW_DIR)

        #print("Copying WIFI_BOOT BIN file to Rollback")
        shutil.copy(WIFI_BOOT_BIN_FILE,ROLLBACK_FW_DIR)
        #print("Copying WIFI_BOOT MD5 file to Rollback")
        shutil.copy(WIFI_BOOT_MD5_FILE,ROLLBACK_FW_DIR)
        #print("Copying WIFI_BOOT_LOADER BIN file to Rollback")
        shutil.copy(WIFI_BOOTLOADER_BIN_FILE,ROLLBACK_FW_DIR)
        #print("Copying WIFI_BOOT_LOADER MD5 file to Rollback")
        shutil.copy(WIFI_BOOTLOADER_MD5_FILE,ROLLBACK_FW_DIR)
        #print("Copying WiFi FW MD5 file to Rollback")
        shutil.copy(WIFI_PARTITION_BIN_FILE,ROLLBACK_FW_DIR)
        #print("Copying WiFi FW MD5 file to Rollback")
        shutil.copy(WIFI_PARTITION_MD5_FILE,ROLLBACK_FW_DIR)

        #print("Copying Main FW Bin file to Backup")
        shutil.copy(MAIN_FW_BIN_FILE,BACKUP_FW_DIR)
        #print("Copying Main FW MD5 file to Backup")
        shutil.copy(MAIN_FW_MD5_FILE,BACKUP_FW_DIR)
        #print("Copying WiFi FW Bin file to Backup")
        shutil.copy(WIFI_FW_BIN_FILE,BACKUP_FW_DIR)
        #print("Copying WiFi FW MD5 file to Backup")
        shutil.copy(WIFI_FW_MD5_FILE,BACKUP_FW_DIR)
        
        #print("Copying WIFI_BOOT BIN file to Backup")
        shutil.copy(WIFI_BOOT_BIN_FILE,BACKUP_FW_DIR)
        #print("Copying WIFI_BOOT MD5 file to Backup")
        shutil.copy(WIFI_BOOT_MD5_FILE,BACKUP_FW_DIR)
        #print("Copying WIFI_BOOT_LOADER BIN file to Backup")
        shutil.copy(WIFI_BOOTLOADER_BIN_FILE,BACKUP_FW_DIR)
        #print("Copying WIFI_BOOT_LOADER MD5 file to Backup")
        shutil.copy(WIFI_BOOTLOADER_MD5_FILE,BACKUP_FW_DIR)
        #print("Copying WiFi FW MD5 file to Backup")
        shutil.copy(WIFI_PARTITION_BIN_FILE,BACKUP_FW_DIR)
        #print("Copying WiFi FW MD5 file to Backup")
        shutil.copy(WIFI_PARTITION_MD5_FILE,BACKUP_FW_DIR)

        print("Copying files...OK")

    except Exception:
        raise DeviceException('001', 'Failed to Access USB devices')
    except IOError as e:
        print("Unable to copy file. %s" % e)
    except:
        print("Unexpected error:", sys.exc_info())

def detect_mount():
    strComputer = "."
    objWMIService = win32com.client.Dispatch("WbemScripting.SWbemLocator")
    objSWbemServices = objWMIService.ConnectServer(strComputer,"root\cimv2")
    # 1. Win32_DiskDrive
    try:
        colItems = objSWbemServices.ExecQuery("SELECT * FROM Win32_DiskDrive WHERE InterfaceType = \"USB\"")
        DiskDrive_DeviceID = colItems[0].DeviceID.replace('\\', '').replace('.', '')
        DiskDrive_Caption = colItems[0].Caption
        #print ("DiskDrive DeviceID:" + DiskDrive_DeviceID)

        if(DiskDrive_Caption == "Tlera DOSFS USB Device"):
            print("Found DOSFS Partition")
        else:
            print("Invalid DOSFS partition, exiting flashing !")
            raise DeviceException('001', 'Failed to Access USB devices')

        # 2. Win32_DiskDriveToDiskPartition
        colItems = objSWbemServices.ExecQuery("SELECT * from Win32_DiskDriveToDiskPartition")
        for objItem in colItems:
            if DiskDrive_DeviceID in str(objItem.Antecedent):
                DiskPartition_DeviceID = objItem.Dependent.split('=')[1].replace('"', '')

        #print ("DiskPartition DeviceID:" + DiskPartition_DeviceID)

        # 3. Win32_LogicalDiskToPartition
        colItems = objSWbemServices.ExecQuery("SELECT * from Win32_LogicalDiskToPartition")
        for objItem in colItems:
            if DiskPartition_DeviceID in str(objItem.Antecedent):
                LogicalDisk_DeviceID = objItem.Dependent.split('=')[1].replace('"', '')

        #print ("LogicalDisk DeviceID:" + LogicalDisk_DeviceID)

        # 4. Win32_LogicalDisk
        colItems = objSWbemServices.ExecQuery("SELECT * from Win32_LogicalDisk WHERE DeviceID=\"" + LogicalDisk_DeviceID + "\"")
        #print ("LogicalDisk VolumeName:" + colItems[0].VolumeName)

        # putting it together
        #print(DiskDrive_Caption)
        #print(colItems[0].VolumeName)
        #print("(" + LogicalDisk_DeviceID +  ")")
        return LogicalDisk_DeviceID
    except Exception:
        raise DeviceException('001', 'Failed to Access USB devices')

def detect_ide():
    """
    Detect IDE USB devices in both runtime and DFU mode
    :return: list of USB device instances
    """
    logger.info('Detecting IDE...')

    global total_ides

    detect_tries_count = 0
    while detect_tries_count < DETECT_MAX_TRIES:
        dfu_devices = []
        runtime_devices = []

        detect_tries_count += 1
        logger.info('Trying %d' % detect_tries_count)
        usb_list = get_usb_devices()
        for usb in usb_list:
            if RUNTIME_ID_STRING in usb.Dependent:  # and "MI_" not in usb.Dependent: # What dose MI mean here?
                logger.info('Found IDE device in runtime mode: %s' % usb.Dependent)
                runtime_devices.append(usb)
            if DFU_ID_STRING in usb.Dependent:
                logger.info('Found IDE device in DFU mode: %s' % usb.Dependent)
                dfu_devices.append(usb)
        if len(dfu_devices) > 0 or len(runtime_devices) > 0:
            logger.info("Found %d IDE device(s) in DFU mode." % len(dfu_devices))
            logger.info("Found %d IDE device(s) in runtime mode." % (len(runtime_devices) // 2))
            logger.info("Found %d IDE device(s) in total." % (len(dfu_devices) + len(runtime_devices) // 2))
            total_ides = len(dfu_devices) + len(runtime_devices) // 2
            return dfu_devices, runtime_devices
        time.sleep(2)

    total_ides = 0
    raise DeviceException('002',
                          'Failed to detect IDEs. If IDEs were connected, please disconnect and reconnect IDEs. If this error persists, please try to use an USB hub.')


def reset_device(device):
    """
    Reset one device to DFU mode.
    :param device: str, serial device name, like COM1
    :return: None
    """
    logger.info('Reseting IDE(%s) to DFU mode...' % device)

    reset_tries_count = 0
    while reset_tries_count < RESET_MAX_TRIES:
        reset_tries_count += 1

        logger.info('Trying %d' % reset_tries_count)

        try:
            ser = serial.Serial(device, baudrate=RESET_BAUDRATE)
            ser.close()
            logger.info('Reset IDE(%s) to DFU mode successfully!' % device)
            return
        except Exception as e:
            logger.error('Reset IDE(%s) to DFU mode failed!' % device, exc_info=True)
            time.sleep(5)

    raise DeviceException('004', 'Failed to reset IDE(%s)!' % device)


def get_usb_devices():
    """
    Get USB devices
    :return: A list of device instances
    """
    try:
        wmi = win32com.client.GetObject("winmgmts:")
        return wmi.InstancesOf("win32_usbcontrollerdevice")
    except Exception:
        raise DeviceException('001', 'Failed to Access USB devices')


##def get_dfu_device_numbers():
##    """
##    Listing the currently attached DFU capable USB devices of IDE, finding serial numbers.
##    :return: List of serial numbers.
##    """
##    logger.info("Listing the currently attached DFU capable USB devices...")
##    reg = re.compile('serial="(.*?)"')
##    flash_tries_count = 0
##    while flash_tries_count < FLASH_MAX_TRIES:
##        flash_tries_count += 1
##        logger.info('Trying %d' % flash_tries_count)
##        cmd = '"%s" --list' % \
##              (os.path.join(UTIL_DIR, 'dfu-util-static.exe'))
##        logger.info('RUN %s' % cmd)
##        p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
##        out = p.stdout.readlines()
##        serial_numbers = []
##        for line in out:
##            line = line.decode()
##            line = line.strip()
##            if VID_DFU_STR in line and PID_DFU_STR in line:
##                matches = reg.findall(line)
##                if len(matches) > 0:
##                    serial_numbers.append(matches[0])
##        serial_numbers = list(set(serial_numbers))
##        if len(serial_numbers) > 0:
##            for serial_number in serial_numbers:
##                logger.info("Found DFU capable USB device with serial number %s" % serial_number)
##            if "UNKNOWN" not in serial_numbers:
##                return serial_numbers
##
##        time.sleep(5)
##
##    raise DeviceException('010', 'No valid DFU capable USB device found.')

def main_fw_md5():
    mainfw_md5 = hashlib.md5()
    cmd = os.path.join(FIRMWARE_DIR, 'vEdge_main.bin')
    with open(cmd,"rb") as f:
        # Read and update hash in chunks of 512
        for byte_block in iter(lambda: f.read(512),b""):
            mainfw_md5.update(byte_block)
    print("vEdge_main FW MD5 Hash:")
    print(mainfw_md5.hexdigest())
    cmd = os.path.join(FIRMWARE_DIR, 'vEdge_main.md5')
    fw_md5 = open(cmd,"w")
    fw_md5.write(mainfw_md5.hexdigest())
    fw_md5.close()

def wifi_fw_md5():
    wififw_md5 = hashlib.md5()
    cmd = os.path.join(FIRMWARE_DIR, 'vEdge_wifi.bin')
    with open(cmd,"rb") as f:
        # Read and update hash in chunks of 512
        for byte_block in iter(lambda: f.read(512),b""):
            wififw_md5.update(byte_block)
    print("vEdge_main FW MD5 Hash:")
    print(wififw_md5.hexdigest())
    cmd = os.path.join(FIRMWARE_DIR, 'vEdge_wifi.md5')
    fw_md5 = open(cmd,"w")
    fw_md5.write(wififw_md5.hexdigest())
    fw_md5.close()

def wifi_boot_md5():
    mainfw_md5 = hashlib.md5()
    cmd = os.path.join(FIRMWARE_DIR, 'vEdge_wifi_boot.bin')
    with open(cmd,"rb") as f:
        # Read and update hash in chunks of 512
        for byte_block in iter(lambda: f.read(512),b""):
            mainfw_md5.update(byte_block)
    print("vEdge_wifi_boot MD5 Hash:")
    print(mainfw_md5.hexdigest())
    cmd = os.path.join(FIRMWARE_DIR, 'vEdge_wifi_boot.md5')
    fw_md5 = open(cmd,"w")
    fw_md5.write(mainfw_md5.hexdigest())
    fw_md5.close()

def wifi_bootloader_md5():
    mainfw_md5 = hashlib.md5()
    cmd = os.path.join(FIRMWARE_DIR, 'vEdge_wifi_bootloader.bin')
    with open(cmd,"rb") as f:
        # Read and update hash in chunks of 512
        for byte_block in iter(lambda: f.read(512),b""):
            mainfw_md5.update(byte_block)
    print("vEdge_wifi_bootloader MD5 Hash:")
    print(mainfw_md5.hexdigest())
    cmd = os.path.join(FIRMWARE_DIR, 'vEdge_wifi_bootloader.md5')
    fw_md5 = open(cmd,"w")
    fw_md5.write(mainfw_md5.hexdigest())
    fw_md5.close()

def wifi_part_md5():
    mainfw_md5 = hashlib.md5()
    cmd = os.path.join(FIRMWARE_DIR, 'vEdge_wifi_partition.bin')
    with open(cmd,"rb") as f:
        # Read and update hash in chunks of 512
        for byte_block in iter(lambda: f.read(512),b""):
            mainfw_md5.update(byte_block)
    print("vEdge_wifi_partition MD5 Hash:")
    print(mainfw_md5.hexdigest())
    cmd = os.path.join(FIRMWARE_DIR, 'vEdge_wifi_partition.md5')
    fw_md5 = open(cmd,"w")
    fw_md5.write(mainfw_md5.hexdigest())
    fw_md5.close()

def flash_bridge():
    """
    Flashing bridge of one device
    :param serial_number: serial number of the deivce.
    :return: None
    """
    logger.info("FLASHING BRIDGE...")
    time.sleep(2);
    flash_tries_count = 0
    while flash_tries_count < FLASH_MAX_TRIES:
        flash_tries_count += 1
        logger.info('Trying %d' % flash_tries_count)
        cmd = '"%s" -c port=USB1 -d "%s" 0x08000000 -s' % \
              (os.path.join(CLI_DIR, 'STM32_Programmer_CLI.exe'),
               os.path.join(FIRMWARE_DIR, 'vEdge_init.bin'))
        logger.info('RUN %s' % cmd)
        rc = subprocess.call(cmd)
        if rc == 0:
            return
        time.sleep(2)

    raise DeviceException('006', 'Failed to flash bridge.')

def flash_bl1():
    """
    Flashing bridge of one device
    :param serial_number: serial number of the deivce.
    :return: None
    """
    logger.info("FLASHING BOOTLOADER-1...")
    flash_tries_count = 0
    while flash_tries_count < FLASH_MAX_TRIES:
        flash_tries_count += 1
        logger.info('Trying %d' % flash_tries_count)
        cmd = '"%s" -c port=USB1 -d "%s" 0x08000000' % \
              (os.path.join(CLI_DIR, 'STM32_Programmer_CLI.exe'),
               os.path.join(FIRMWARE_DIR, 'vEdge_bl1.bin'))
        
        logger.info('RUN %s' % cmd)
        rc = subprocess.call(cmd)
        if rc == 0:
            return

        time.sleep(3)

    raise DeviceException('006', 'Failed to flash Bootaloader1.')

def flash_bl2():
    """
    Flashing bridge of one device
    :param serial_number: serial number of the deivce.
    :return: None
    """
    logger.info("FLASHING BOOTLOADER-2...")
   # time.sleep(5)
    flash_tries_count = 0
    while flash_tries_count < FLASH_MAX_TRIES:
        flash_tries_count += 1
        logger.info('Trying %d' % flash_tries_count)
        cmd = '"%s" -c port=USB1 -d "%s" 0x08010000 -s' % \
              (os.path.join(CLI_DIR, 'STM32_Programmer_CLI.exe'),
               os.path.join(FIRMWARE_DIR, 'vEdge_bl2.bin'))
        logger.info('RUN %s' % cmd)
        rc = subprocess.call(cmd)
        if rc == 0:
            return
        time.sleep(3)

    raise DeviceException('006', 'Failed to flash Bootaloader2.')

def flash_mcu():
    """
    Flashing MCU of one device
    :param serial_number: serial number of the device
    :return: None
    """
    logger.info("FLASHING MCU FIRMWARE...")
    time.sleep(2);
    flash_tries_count = 0
    
    while flash_tries_count < FLASH_MAX_TRIES:
        flash_tries_count += 1
        logger.info('Trying %d' % flash_tries_count)
        cmd = '"%s" -c port=USB1 -d "%s" 0x08060000' % \
              (os.path.join(CLI_DIR, 'STM32_Programmer_CLI.exe'),
               os.path.join(FIRMWARE_DIR, 'vEdge_main.bin'))
        logger.info('RUN %s' % cmd)
        rc = subprocess.call(cmd)
        if rc == 0:
            return
        time.sleep(3)

    raise DeviceException('007', 'Failed to flash MCU.')


def flash_factfw():
    """
    Flashing MCU of one device
    :param serial_number: serial number of the device
    :return: None
    """
    logger.info("FLASHING MCU Factory FIRMWARE...")

    flash_tries_count = 0

    while flash_tries_count < FLASH_MAX_TRIES:
        flash_tries_count += 1
        logger.info('Trying %d' % flash_tries_count)
        cmd = '"%s" -c port=USB1 -d "%s" 0x08036000' % \
              (os.path.join(CLI_DIR, 'STM32_Programmer_CLI.exe'),
               os.path.join(FIRMWARE_DIR, 'vEdge_fact.bin'))
        logger.info('RUN %s' % cmd)
        rc = subprocess.call(cmd)
        if rc == 0:
            return
        time.sleep(3)

    raise DeviceException('007', 'Failed to flash MCU Factory.')

def flash_wifi(device):
    """
    Flashing WIFI chip of one device
    :param device: serial device name, like COM1
    :return: None
    """
    logger.info("FLASHING WIFI FIRMWARE...")

    wifi_tries_count = 0
    while wifi_tries_count < WIFI_MAX_TRIES:
        wifi_tries_count += 1
        logger.info('Trying %d' % wifi_tries_count)
        try:
            cmd = [os.path.join(UTIL_DIR, 'esptool.exe'),
                  "--port", device,
                   "--baud", str(FLASH_BAUDRATE),
                   "--chip", "esp32",
                   "--before", "default_reset",
                   "--after", "hard_reset",
                   "write_flash", "-z",
                   "0xe000", os.path.join(FIRMWARE_DIR, 'vEdge_wifi_boot.bin'),
                   "0x1000", os.path.join(FIRMWARE_DIR, 'vEdge_wifi_bootloader.bin'),
                   "0x10000", os.path.join(FIRMWARE_DIR, 'vEdge_wifi.bin'),
                   "0x8000", os.path.join(FIRMWARE_DIR, 'vEdge_wifi_partition.bin')]
            logger.info('RUN %s' % " ".join(cmd))
            #esptool.main(cmd)
            rc = subprocess.call(cmd)
            if rc == 0:
                logger.info("Flashing wifi successfully! Serial number: %s." % device)
                return
        except Exception as e:
            logger.error("Flashing wifi failed! Serial number: %s." % device, exc_info=True)

        time.sleep(3)

    raise DeviceException('008', 'Failed to flash wifi firmware.')


def reset_all_devices():
    """
    Reset all devices to DFU mode.
    :return:
    """
    serial_ports = get_serial_ports()
    for p in serial_ports:
        reset_device(p.device)

def flash_all_bridge():
    flash_bridge()
    wait_1_restart()
    LogicalDisk_DeviceID = detect_mount()
    copy_fw_files(LogicalDisk_DeviceID)

def flash_all_bl1():
    flash_bl1()

def flash_all_bl2():
    flash_bl2()

def flash_all_wifi():
    """
    Flash WIFI of all devices.
    :return:
    """
    serial_ports = get_serial_ports()
    for p in serial_ports:
        flash_wifi(p.device)


def flash_all_mcu():
    flash_mcu()

def flash_all_mcu_factfw():
    flash_factfw()

def wait_all_restart():
    """
    Waiting for all devices to restart after reseted to DFU mode.
    :return:
    """
    logger.info("Waiting for all devices to restart...")
    #time.sleep(3)
    restart_tries = 0
    while restart_tries < RESTART_MAX_RETIES:
        restart_tries += 1
        dfu_devices, runtime_devices = detect_ide()
        if len(dfu_devices) > 0:
            time.sleep(5)
        else:
            logger.info("All devices restarted.")
            time.sleep(5)
            return

    raise DeviceException('011', "Found IDE(s) that did not restart.")

def wait_1_restart():
    """
    Waiting for all devices to restart after reseted to DFU mode.
    :return:
    """
    logger.info("Waiting for all devices to restart...")
    #time.sleep(3)
    restart_tries = 0
    while restart_tries < RESTART_MAX_RETIES:
        restart_tries += 1
        dfu_devices, runtime_devices = detect_ide()
        if len(dfu_devices) > 0:
            time.sleep(4)
            return
        else:
            logger.info("All devices restarted.")
            time.sleep(4)
            return

    raise DeviceException('011', "Found IDE(s) that did not restart.")


def set_endpoint(device_port, backend):
    serial_connection = endpoints.get_serial_connection(device_port)
    endpoints.wait_until_ide_boot(serial_connection)
    endpoints.set_endpoints(serial_connection, backend, mqtt=True, http=True)
    serial_connection.close()


def get_endpoint(device_port):
    serial_connection = endpoints.get_serial_connection(device_port)
    endpoints.wait_until_ide_boot(serial_connection)  # block till device has booted up
    serial_connection.close()

    serial_connection = endpoints.get_serial_connection(device_port, timeout=60)
    # timeout of 60 seconds, if device has not responded, indicates either missing config file or corrupted flash
    endpoints.get_endpoints(serial_connection, mqtt=True, http=True)
    serial_connection.close()


def set_all_endpoint(backend):
    """
    :param key:
    :return:
    """
    try:
        endpoints.check_allowed_version(FIRMWARE_VERSION_FILEPATH)
    except FileNotFoundError:
        logger.info("BACKENDS - Firmware version could not be recognized, skipping backend configuration")
        return
    except endpoints.VersionNotAllowedError:
        logger.info("BACKENDS - Firmware version is less than v1.1.3, skipping backend configuration")
        return

    if backend in [backend["key"] for backend in endpoints.backends]:
        serial_ports = get_serial_ports()
        for p in serial_ports:
            set_endpoint(p.device, backend)
            wait_all_restart()
            get_endpoint(p.device)
    elif backend:
        logger.error("BACKENDS - Incorrect backend platform  '{}' specified".format(backend))
    else:
        logger.error("BACKENDS - Please specify backend")


def main(backend=None):
    """
    App Entry
    :return:
    """

    logger.info('*' * 50)
    logger.info('vEdge Firmware Flashing Utility ' + __version__)
    logger.info('*' * 50)

    print(
        'PLEASE DISABLE YOUR ANTIVIRUS DURING FLASHING. IF ANTIVIRUS IS RUNNING, THE UTILITY WILL NOT BE ABLE TO FLASH RELIABLY.')
    input('Please press the Enter key to confirm you have disabled the antivirus.')

    # Check or install DFU USB driver is slow, should be outside of while loop
    try:
        check_dfu_driver()
    except DeviceException as e:
        logger.critical('Error %s: %s' % (e.code, e.message))
        cmd = input('Press <ENTER> to retry.')
    except Exception as e:
        logger.critical(str(e), exc_info=True)
        cmd = input('Press <ENTER> to retry.')

    # Flash multiple IDEs sequentially in a loop
    while True:
        start_time = time.time()
        logger.info('-' * 50)
        try:
            check_dependencies()
            make_dirs()
            dfu_devices, runtime_devices = detect_ide()
            if len(runtime_devices) > 0:
                reset_all_devices()
            main_fw_md5()
            wifi_fw_md5()
            wifi_boot_md5()
            wifi_bootloader_md5()
            wifi_part_md5()
            flash_all_bridge()
            #wait_all_restart()
            flash_all_wifi()
            reset_all_devices()
            flash_all_mcu()
            #reset_all_devices()
            flash_all_mcu_factfw()
            #reset_all_devices()
            flash_all_bl1()
            #reset_all_devices()
            flash_all_bl2()
            
            logger.info("%d IDE device(s) were flashed successfully in total!" % (total_ides))

            # set mqtt & http endpoints
            if backend:
                wait_all_restart()
                endpoints.load_custom_backends(CUSTOM_BACKEND_FILEPATH)
                set_all_endpoint(backend)
            else:
                logger.info("BACKENDS - Firmware version could not be recognized, skipping backend configuration")
            print('--------------Flashing Completed in : %ss--------------' %(time.strftime('%H:%M:%S', time.gmtime(time.time() - start_time))))

            cmd = input("Please unplug all IDEs connected and plug new ones, then press <ENTER> to continue. ")

        except DeviceException as e:
            logger.critical('Error %s: %s' % (e.code, e.message))
            logger.critical('Flashing failed!')

            print('A report was saved to %s' % REPORT_DIR)

            cmd = input('Press <ENTER> to retry.')

        except Exception as e:
            logger.critical('Flashing failed!', exc_info=True)

            print('A report was saved to %s' % REPORT_DIR)

            cmd = input('Press <ENTER> to retry.')


if __name__ == '__main__':

    # try to run as admin user
    # backend
    if len(sys.argv) > 1:
        backend = sys.argv[1]
    else:
        backend = None
    try:
        is_admin = ctypes.windll.shell32.IsUserAnAdmin()
        if is_admin:
            main(backend)
        else:
            logger.info("Trying to run as administrator.")
            # hinstance = ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable,
            #                                                 subprocess.list2cmdline(sys.argv), None, 1)
            hinstance = ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, backend, None, 1)
            if hinstance <= 32:
                raise RuntimeError(ShellExecuteError(hinstance))
            sys.exit(0)
    except RuntimeError as e:
        logger.critical("Failed to run as administrator.", exc_info=False)
        logger.critical("Please rerun and allow the UAC to run as administrator.", exc_info=False)
        input("Press <ENTER> to exit.")
