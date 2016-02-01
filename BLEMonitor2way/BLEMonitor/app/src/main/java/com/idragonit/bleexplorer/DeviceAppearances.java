package com.idragonit.bleexplorer;

import java.util.HashMap;

public class DeviceAppearances {
    private static HashMap appearances;

    static {
        appearances = new HashMap();
        appearances.put(Integer.valueOf(0), "Unknown");
        appearances.put(Integer.valueOf(64), "Generic Phone");
        appearances.put(Integer.valueOf(128), "Generic Computer");
        appearances.put(Integer.valueOf(192), "Generic Watch");
        appearances.put(Integer.valueOf(193), "Watch: Sports Watch");
        appearances.put(Integer.valueOf(256), "Generic Clock");
        appearances.put(Integer.valueOf(320), "Generic Display");
        appearances.put(Integer.valueOf(384), "Generic Remote Control");
        appearances.put(Integer.valueOf(448), "Generic Eye-glasses");
        appearances.put(Integer.valueOf(512), "Generic Tag");
        appearances.put(Integer.valueOf(576), "Generic Keyring");
        appearances.put(Integer.valueOf(640), "Generic Media Player");
        appearances.put(Integer.valueOf(704), "Generic Barcode Scanner");
        appearances.put(Integer.valueOf(768), "Generic Thermometer");
        appearances.put(Integer.valueOf(769), "Thermometer: Ear");
        appearances.put(Integer.valueOf(832), "Generic Heart rate Sensor");
        appearances.put(Integer.valueOf(833), "Heart Rate Sensor: Heart Rate Belt");
        appearances.put(Integer.valueOf(896), "Generic Blood Pressure");
        appearances.put(Integer.valueOf(897), "Blood Pressure: Arm");
        appearances.put(Integer.valueOf(898), "Blood Pressure: Wrist");
        appearances.put(Integer.valueOf(960), "Human Interface Device (HID)");
        appearances.put(Integer.valueOf(961), "Keyboard (HID)");
        appearances.put(Integer.valueOf(962), "Mouse (HID)");
        appearances.put(Integer.valueOf(963), "Joystiq (HID)");
        appearances.put(Integer.valueOf(964), "Gamepad (HID)");
        appearances.put(Integer.valueOf(965), "Digitizer Tablet (HID)");
        appearances.put(Integer.valueOf(966), "Card Reader (HID)");
        appearances.put(Integer.valueOf(967), "Digital Pen (HID)");
        appearances.put(Integer.valueOf(968), "Barcode Scanner (HID )");
        appearances.put(Integer.valueOf(1024), "Generic Glucose Meter");
        appearances.put(Integer.valueOf(1088), "Generic Running Walking Sensor");
        appearances.put(Integer.valueOf(1089), "Running Walking Sensor: In-Shoe");
        appearances.put(Integer.valueOf(1090), "Running Walking Sensor: On-Shoe");
        appearances.put(Integer.valueOf(1091), "Running Walking Sensor: On-Hip");
        appearances.put(Integer.valueOf(1152), "Generic Cycling");
        appearances.put(Integer.valueOf(1153), "Cycling: Cycling Computer");
        appearances.put(Integer.valueOf(1154), "Cycling: Speed Sensor");
        appearances.put(Integer.valueOf(1155), "Cycling: Cadence Sensor");
        appearances.put(Integer.valueOf(1156), "Cycling: Power Sensor");
        appearances.put(Integer.valueOf(1157), "Cycling: Speed and Cadence Sensor");
        appearances.put(Integer.valueOf(3136), "Generic Pulse Oximeter");
        appearances.put(Integer.valueOf(3137), "Fingertip (Pulse Oximeter)");
        appearances.put(Integer.valueOf(3138), "Wrist Worn(Pulse Oximeter)");
        appearances.put(Integer.valueOf(3200), "Generic Weight Scale");
        appearances.put(Integer.valueOf(5184), "Generic Outdoor Sports Activity");
        appearances.put(Integer.valueOf(5185), "Location Display Device (Outdoor Sports Activity)");
        appearances.put(Integer.valueOf(5186), "Location and Navigation Display Device (Outdoor Sports Activitye)");
        appearances.put(Integer.valueOf(5187), "Location Pod (Outdoor Sports Activity)");
        appearances.put(Integer.valueOf(5188), "Location and Navigation Pod (Outdoor Sports Activity)");
        appearances.put(Integer.valueOf(21862), "iCareBox");
    }

    public DeviceAppearances() {
    }

    public static String lookup(int index, String defaultDevice) {
        String device = (String) appearances.get(index);
        if (device == null)
            return defaultDevice;

        return device;
    }
}
