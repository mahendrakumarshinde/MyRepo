package com.idragonit.bleexplorer.dao;

import com.idragonit.bleexplorer.IReceiveData;

/**
 * Entity mapped to table BLEStatus.
 */
public class BLEStatus implements IReceiveData {

    private Long id;
    private String deviceName = "";
    private String deviceAddress = "";
    private int status = STATUS_NONE;
    private Long startTime = 0L;
    private Long endTime = 0L;
    private Long duration = 0L;

    public BLEStatus() {
    }

    public BLEStatus(Long id) {
        this.id = id;
    }

    public BLEStatus(String deviceName, String deviceAddress, int status, Long startTime, Long endTime) {
        this.deviceName = deviceName;
        this.deviceAddress = deviceAddress;
        this.status = status;
        this.startTime = startTime;
        this.endTime = endTime;
        this.duration = endTime - startTime;
    }

    public BLEStatus(Long id, String deviceName, String deviceAddress, int status, Long startTime, Long endTime) {
        this.id = id;
        this.deviceName = deviceName;
        this.deviceAddress = deviceAddress;
        this.status = status;
        this.startTime = startTime;
        this.endTime = endTime;
        this.duration = endTime - startTime;
    }

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public String getDeviceName() {
        return deviceName;
    }

    public void setDeviceName(String deviceName) {
        this.deviceName = deviceName;
    }

    public String getDeviceAddress() {
        return deviceAddress;
    }

    public void setDeviceAddress(String deviceAddress) {
        this.deviceAddress = deviceAddress;
    }

    public int getStatus() {
        return status;
    }

    public void setStatus(int status) {
        this.status = status;
    }

    public Long getStartTime() {
        return startTime;
    }

    public void setStartTime(Long startTime) {
        this.startTime = startTime;
    }

    public Long getEndTime() {
        return endTime;
    }

    public void setEndTime(Long endTime) {
        this.endTime = endTime;
        this.duration = endTime - startTime;
    }

    public Long getDuration() {
        return duration;
    }

    public void setDuration(Long duration) {
        this.duration = duration;
    }

    @Override
    public void readData(int status, long readTime) {

    }
}
