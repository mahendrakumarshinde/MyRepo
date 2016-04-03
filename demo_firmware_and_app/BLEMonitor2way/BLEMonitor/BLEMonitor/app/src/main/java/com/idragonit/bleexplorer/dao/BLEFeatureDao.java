package com.idragonit.bleexplorer.dao;

import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteStatement;

import de.greenrobot.dao.AbstractDao;
import de.greenrobot.dao.Property;
import de.greenrobot.dao.internal.DaoConfig;

/** 
 * DAO for table BLESTATUS.
*/
public class BLEFeatureDao extends AbstractDao<BLEFeature, Long> {

    public static final String TABLENAME = "BLEFEATURE";

    /**
     * Properties of entity TRAFFICS.<br/>
     * Can be used for QueryBuilder and for referencing column names.
    */
    public static class Properties {
        public static final Property Id = new Property(0, Long.class, "id", true, "_id");
        public static final Property DeviceName = new Property(1, String.class, "deviceName", false, "DEVICE_NAME");
        public static final Property DeviceAddress = new Property(2, String.class, "deviceAddress", false, "DEVICE_ADDRESS");
        public static final Property ReceivedDeviceAddress = new Property(3, String.class, "receivedDeviceAddress", false, "RECEIVED_DEVICE_ADDRESS");
        public static final Property Status = new Property(4, Integer.class, "status", false, "STATUS");
        public static final Property FeatureId = new Property(5, String.class, "featureId", false, "FEATURE_ID");
        public static final Property FeatureValue = new Property(6, Float.class, "featureValue", false, "FEATURE_VALUE");
        public static final Property StartTime = new Property(7, Long.class, "startTime", false, "START_TIME");
        public static final Property EndTime = new Property(8, Long.class, "endTime", false, "END_TIME");
        public static final Property Duration = new Property(9, Long.class, "duration", false, "DURATION");
    };

    public BLEFeatureDao(DaoConfig config) {
        super(config);
    }

    public BLEFeatureDao(DaoConfig config, DaoSession daoSession) {
        super(config, daoSession);
    }
    
    /** Creates the underlying database table. */
    public static void createTable(SQLiteDatabase db, boolean ifNotExists) {
        String constraint = ifNotExists? "IF NOT EXISTS ": "";
        db.execSQL("CREATE TABLE " + constraint + "'BLEFEATURE' (" + //
                "'_id' INTEGER PRIMARY KEY AUTOINCREMENT ," + // 0: id
                "'DEVICE_NAME' TEXT ," + // 1: device name
                "'DEVICE_ADDRESS' TEXT ," + // 2: device address
                "'RECEIVED_DEVICE_ADDRESS' TEXT ," + // 3: received device address
                "'STATUS' INTEGER ," + // 4: status
                "'FEATURE_ID' TEXT ," + // 5: feature id
                "'FEATURE_VALUE' REAL ," + // 6: feature value
                "'START_TIME' INTEGER ," + // 7: start time
                "'END_TIME' INTEGER ," + // 8: end time
                "'DURATION' INTEGER);"); // 9: duration
    }

    /** Drops the underlying database table. */
    public static void dropTable(SQLiteDatabase db, boolean ifExists) {
        String sql = "DROP TABLE " + (ifExists ? "IF EXISTS " : "") + "'BLEFEATURE'";
        db.execSQL(sql);
    }

    /** @inheritdoc */
    @Override
    protected void bindValues(SQLiteStatement stmt, BLEFeature entity) {
        stmt.clearBindings();
 
        Long id = entity.getId();
        if (id != null) {
            stmt.bindLong(1, id);
        }
        
        if (entity.getDeviceName() != null)
        	stmt.bindString(2, entity.getDeviceName());

        if (entity.getDeviceAddress() != null)
            stmt.bindString(3, entity.getDeviceAddress());

        if (entity.getReceivedDeviceAddress() != null)
            stmt.bindString(4, entity.getReceivedDeviceAddress());

        stmt.bindLong(5, entity.getStatus());

        if (entity.getFeatureId() != null)
            stmt.bindString(6, entity.getFeatureId());

        stmt.bindDouble(7, entity.getFeatureValue());
        stmt.bindLong(8, entity.getStartTime());
        stmt.bindLong(9, entity.getEndTime());
        stmt.bindLong(10, entity.getDuration());
    }

    /** @inheritdoc */
    @Override
    public Long readKey(Cursor cursor, int offset) {
        return cursor.isNull(offset + 0) ? null : cursor.getLong(offset + 0);
    }    

    /** @inheritdoc */
    @Override
    public BLEFeature readEntity(Cursor cursor, int offset) {
        BLEFeature entity = new BLEFeature(
            cursor.isNull(offset + 0) ? null : cursor.getLong(offset + 0), // id
            cursor.isNull(offset + 1) ? null : cursor.getString(offset + 1), // device name
            cursor.isNull(offset + 2) ? null : cursor.getString(offset + 2), // device address
            cursor.isNull(offset + 3) ? null : cursor.getString(offset + 3), // received device address
            cursor.isNull(offset + 4) ? null : cursor.getInt(offset + 4), // status
            cursor.isNull(offset + 5) ? null : cursor.getString(offset + 5), // feature id
            cursor.isNull(offset + 6) ? null : cursor.getFloat(offset + 6), // feature value
			cursor.isNull(offset + 7) ? null : cursor.getLong(offset + 7), // start time
			cursor.isNull(offset + 8) ? null : cursor.getLong(offset + 8) // end time
        );
        
        return entity;
    }
     
    /** @inheritdoc */
    @Override
    protected Long updateKeyAfterInsert(BLEFeature entity, long rowId) {
        entity.setId(rowId);
        return rowId;
    }
    
    /** @inheritdoc */
    @Override
    public Long getKey(BLEFeature entity) {
        if(entity != null) {
            return entity.getId();
        } else {
            return null;
        }
    }

    /** @inheritdoc */
    @Override    
    protected boolean isEntityUpdateable() {
        return true;
    }

	@Override
	protected void readEntity(Cursor arg0, BLEFeature arg1, int arg2) {
		// TODO Auto-generated method stub
	}
    
}
