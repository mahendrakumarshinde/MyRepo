package com.idragonit.bleexplorer;

public interface IReceiveFeatureData {
    void readData(int status, String id, float value, long readTime, boolean isNew);

    final static int STATUS_NONE = 0;
    final static int STATUS_IDLE = 1;
    final static int STATUS_NORMAL_CUTTING = 2;
    final static int STATUS_WARNING = 3;
    final static int STATUS_DANGER = 4;

    final static int STATUS_IDLE_COLOR = 0xFF0070C0;
    final static int STATUS_NORMAL_CUTTING_COLOR = 0xFF00B050;
    final static int STATUS_WARNING_COLOR = 0xFFFFC000;
    final static int STATUS_DANGER_COLOR = 0xFFFF0000;

    final static String STATUS_BATTERY_NAME = "BAT";
    final static String STATUS_IDLE_NAME = "Idle";
    final static String STATUS_NORMAL_CUTTING_NAME = "Normal Cutting";
    final static String STATUS_WARNING_NAME = "Warning";
    final static String STATUS_DANGER_NAME = "Danger";
}
