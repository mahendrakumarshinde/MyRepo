package com.idragonit.bleexplorer;

import android.content.Context;
import android.content.SharedPreferences;

public class AppData {
    static final String TAG = "BLEMonitor_AppData";
    static final String PREFERENCE_THRESHOLD = TAG + "_threshold";

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
}
