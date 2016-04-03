package com.idragonit.bleexplorer.dao;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabaseLockedException;

import java.util.ArrayList;
import java.util.HashMap;

public class DaoUtils {

    public static BLEStatusDao getBLEStatusDao(Context context) {
        return (new DaoMaster(getDevOpenHelper(context).getWritableDatabase())).newSession().getBLEStatusDao();
    }

    public static BLEFeatureDao getBLEFeatureDao(Context context) {
        return (new DaoMaster(getDevOpenHelper(context).getWritableDatabase())).newSession().getBLEFeatureDao();
    }

    private static DaoMaster.DevOpenHelper getDevOpenHelper(Context context) {
        return new DaoMaster.DevOpenHelper(context, "BLESTATUS", null);
    }

    public static long[] getStatusDuration(Context context, String deviceName, String deviceAddress) {
        BLEStatusDao bleStatusDao;
        String columns[];
        String selection;
        Cursor cursor;

        long[] durations = new long[4];

        try {
            bleStatusDao = getBLEStatusDao(context);
        }
        catch(NullPointerException nullpointerexception) {
            return durations;
        }

        columns = new String[2];
        columns[0] = BLEStatusDao.Properties.Status.columnName;
        columns[1] = "sum(" + BLEStatusDao.Properties.Duration.columnName + ")";

        selection = BLEStatusDao.Properties.DeviceName.columnName + " = '" + deviceName + "' AND " + BLEStatusDao.Properties.DeviceAddress.columnName + " = '" + deviceAddress + "'";
        cursor = bleStatusDao.getDatabase().query("BLESTATUS", columns, selection, null, BLEStatusDao.Properties.Status.columnName, null, null);

        cursor.moveToFirst();

        while (!cursor.isAfterLast()) {
            try {
                durations[(int) cursor.getLong(0) - 1] = cursor.getLong(1);
            } catch (Exception e) {

            }

            cursor.moveToNext();
        }

        cursor.close();
        bleStatusDao.getDatabase().close();

        return durations;
    }

    public static void addBLEStatus(Context context, ArrayList<BLEStatus> data) {
        BLEStatusDao bleStatusDao;
        BLEStatus bleStatus;

        try {
            bleStatusDao = getBLEStatusDao(context);
        }
        catch(SQLiteDatabaseLockedException sqlitedatabaselockedexception) {
            return;
        }

        for (int i = 0; i < data.size(); i++) {
            bleStatus = data.get(i);
            bleStatusDao.insert(bleStatus);
        }

        bleStatusDao.getDatabase().close();
    }

    public static HashMap<String, ArrayList<BLEFeature>> getFeatures(Context context, String deviceName, String deviceAddress) {
        BLEFeatureDao bleFeatureDao;
        String columns[];
        String selection;
        Cursor cursor;

        HashMap<String, ArrayList<BLEFeature>> hashMap = new HashMap<String, ArrayList<BLEFeature>>();

        try {
            bleFeatureDao = getBLEFeatureDao(context);
        }
        catch(NullPointerException nullpointerexception) {
            return hashMap;
        }

        columns = new String[4];
        columns[0] = BLEFeatureDao.Properties.Status.columnName;
        columns[1] = BLEFeatureDao.Properties.Duration.columnName;
        columns[2] = BLEFeatureDao.Properties.FeatureId.columnName;
        columns[3] = BLEFeatureDao.Properties.FeatureValue.columnName;

        selection = BLEFeatureDao.Properties.DeviceName.columnName + " = '" + deviceName + "' AND " + BLEFeatureDao.Properties.DeviceAddress.columnName + " = '" + deviceAddress + "'";
        //cursor = bleFeatureDao.getDatabase().query("BLEFEATURE", columns, selection, null, BLEStatusDao.Properties.StartTime.columnName, null, null);
        cursor = bleFeatureDao.getDatabase().query("BLEFEATURE", columns, selection, null, null, null, null);

        cursor.moveToFirst();

        while (!cursor.isAfterLast()) {
            try {
                ArrayList<BLEFeature> featureList = hashMap.get(cursor.getString(2));
                if (featureList == null) {
                    featureList = new ArrayList<BLEFeature>();
                    hashMap.put(cursor.getString(2), featureList);
                }

                BLEFeature bleFeature = new BLEFeature();

                bleFeature.setStatus(cursor.getInt(0));
                bleFeature.setDuration(cursor.getLong(1));
                bleFeature.setFeatureId(cursor.getString(2));
                bleFeature.setFeatureValue(cursor.getFloat(3));

                featureList.add(bleFeature);

            } catch (Exception e) {

            }

            cursor.moveToNext();
        }

        cursor.close();
        bleFeatureDao.getDatabase().close();

        return hashMap;
    }

    public static void addBLEFeature(Context context, ArrayList<BLEFeature> data) {
        BLEFeatureDao bleFeatureDao;
        BLEFeature bleFeature;

        try {
            bleFeatureDao = getBLEFeatureDao(context);
        }
        catch(SQLiteDatabaseLockedException sqlitedatabaselockedexception) {
            return;
        }

        for (int i = 0; i < data.size(); i++) {
            bleFeature = data.get(i);
            bleFeatureDao.insert(bleFeature);
        }

        bleFeatureDao.getDatabase().close();
    }
}
