package com.idragonit.bleexplorer;

import android.content.Context;
import android.content.SharedPreferences;

public class AppData {
    static final String TAG = "BLEMonitor_AppData";
    static final String PREFERENCE_THRESHOLD = TAG + "_threshold";
    static final String PREFERENCE_FEATURE_START_TIME = TAG + "_feature_start_time";

    public static void setThresholdValue(Context context, int index, int value) {
        try{
            SharedPreferences pref = context.getSharedPreferences(TAG, Context.MODE_PRIVATE);
            SharedPreferences.Editor editor = pref.edit();

            editor.putInt(PREFERENCE_THRESHOLD + index, value);

            editor.commit();
        }catch (Exception e){
            e.printStackTrace();
        }
    }

    public static int getThresholdValue(Context context, int index, int defaultValue) {
        try{
            SharedPreferences pref = context.getSharedPreferences(TAG, Context.MODE_PRIVATE);
            return pref.getInt(PREFERENCE_THRESHOLD + index, defaultValue);
        } catch (Exception e){
            e.printStackTrace();
        }

        return defaultValue;
    }

    public static void setFeatureStartTime(Context context, String id, long value) {
        try{
            SharedPreferences pref = context.getSharedPreferences(TAG, Context.MODE_PRIVATE);
            SharedPreferences.Editor editor = pref.edit();

            editor.putLong(PREFERENCE_FEATURE_START_TIME + id, value);

            editor.commit();
        }catch (Exception e){
            e.printStackTrace();
        }
    }

    public static long getFeatureStartTime(Context context, String id) {
        try{
            SharedPreferences pref = context.getSharedPreferences(TAG, Context.MODE_PRIVATE);
            return pref.getLong(PREFERENCE_FEATURE_START_TIME + id, 0);
        } catch (Exception e){
            e.printStackTrace();
        }

        return 0;
    }
}
