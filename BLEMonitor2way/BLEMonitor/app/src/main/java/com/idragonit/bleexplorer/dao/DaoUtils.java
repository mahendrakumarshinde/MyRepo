package com.idragonit.bleexplorer.dao;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabaseLockedException;

import java.util.ArrayList;

public class DaoUtils {

    public static BLEStatusDao getBLEStatusDao(Context context) {
        return (new DaoMaster(getDevOpenHelper(context).getWritableDatabase())).newSession().getBLEStatusDao();
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
}
