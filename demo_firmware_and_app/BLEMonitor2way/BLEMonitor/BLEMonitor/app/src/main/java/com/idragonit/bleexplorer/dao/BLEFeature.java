package com.idragonit.bleexplorer.dao;

import com.idragonit.bleexplorer.IReceiveData;

/**
 * Entity mapped to table BLEFeature.
 */
public class BLEFeature implements IReceiveData {

    private Long id;
    private String deviceName = "";
    private String deviceAddress = "";
    private String receivedDeviceAddress = "";
    private int status = STATUS_NONE;
    private String featureId = "";
    private Float featureValue = 0.0f;
    private Long startTime = 0L;
    private Long endTime = 0L;
    private Long duration = 0L;

    public BLEFeature() {
    }

    public BLEFeature(Long id) {
        this.id = id;
    }

    public BLEFeature(String deviceName, String deviceAddress, String receivedDeviceAddress, int status, String featureId, Float featureValue, Long startTime, Long endTime) {
        this.deviceName = deviceName;
        this.deviceAddress = deviceAddress;
        this.receivedDeviceAddress = receivedDeviceAddress;
        this.status = status;
        this.featureId = featureId;
        this.featureValue = featureValue;
        this.startTime = startTime;
        this.endTime = endTime;
        this.duration = endTime - startTime;
    }

    public BLEFeature(Long id, String deviceName, String deviceAddress, String receivedDeviceAddress, int status, String featureId, Float featureValue, Long startTime, Long endTime) {
        this.id = id;
        this.deviceName = deviceName;
        this.deviceAddress = deviceAddress;
        this.receivedDeviceAddress = receivedDeviceAddress;
        this.status = status;
        this.featureId = featureId;
        this.featureValue = featureValue;
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

    public String getReceivedDeviceAddress() {
        return receivedDeviceAddress;
    }

    public void setReceivedDeviceAddress(String deviceAddress) {
        this.receivedDeviceAddress = deviceAddress;
    }

    public int getStatus() {
        return status;
    }

    public void setStatus(int status) {
        this.status = status;
    }

    public String getFeatureId() {
        return featureId;
    }

    public void setFeatureId(String featureId) {
        this.featureId = featureId;
    }

    public Float getFeatureValue() {
        return featureValue;
    }

    public void setFeatureValue(Float featureValue) {
        this.featureValue = featureValue;
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
