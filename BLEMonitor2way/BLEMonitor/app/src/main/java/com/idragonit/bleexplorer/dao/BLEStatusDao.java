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
public class BLEStatusDao extends AbstractDao<BLEStatus, Long> {

    public static final String TABLENAME = "BLESTATUS";

    /**
     * Properties of entity TRAFFICS.<br/>
     * Can be used for QueryBuilder and for referencing column names.
    */
    public static class Properties {
        public static final Property Id = new Property(0, Long.class, "id", true, "_id");
        public static final Property DeviceName = new Property(1, String.class, "deviceName", false, "DEVICE_NAME");
        public static final Property DeviceAddress = new Property(2, String.class, "deviceAddress", false, "DEVICE_ADDRESS");
        public static final Property Status = new Property(3, Integer.class, "status", false, "STATUS");
        public static final Property StartTime = new Property(4, Long.class, "startTime", false, "START_TIME");
        public static final Property EndTime = new Property(5, Long.class, "endTime", false, "END_TIME");
        public static final Property Duration = new Property(6, Long.class, "duration", false, "DURATION");
    };

    public BLEStatusDao(DaoConfig config) {
        super(config);
    }
    
    public BLEStatusDao(DaoConfig config, DaoSession daoSession) {
        super(config, daoSession);
    }
    
    /** Creates the underlying database table. */
    public static void createTable(SQLiteDatabase db, boolean ifNotExists) {
        String constraint = ifNotExists? "IF NOT EXISTS ": "";
        db.execSQL("CREATE TABLE " + constraint + "'BLESTATUS' (" + //
                "'_id' INTEGER PRIMARY KEY AUTOINCREMENT ," + // 0: id
                "'DEVICE_NAME' TEXT ," + // 1: device name
                "'DEVICE_ADDRESS' TEXT ," + // 2: device address
                "'STATUS' INTEGER ," + // 3: status
                "'START_TIME' INTEGER ," + // 4: start time
                "'END_TIME' INTEGER ," + // 5: end time
                "'DURATION' INTEGER);"); // 6: duration
    }

    /** Drops the underlying database table. */
    public static void dropTable(SQLiteDatabase db, boolean ifExists) {
        String sql = "DROP TABLE " + (ifExists ? "IF EXISTS " : "") + "'BLESTATUS'";
        db.execSQL(sql);
    }

    /** @inheritdoc */
    @Override
    protected void bindValues(SQLiteStatement stmt, BLEStatus entity) {
        stmt.clearBindings();
 
        Long id = entity.getId();
        if (id != null) {
            stmt.bindLong(1, id);
        }
        
        if (entity.getDeviceName() != null)
        	stmt.bindString(2, entity.getDeviceName());

        if (entity.getDeviceAddress() != null)
            stmt.bindString(3, entity.getDeviceAddress());

        stmt.bindLong(4, entity.getStatus());
        stmt.bindLong(5, entity.getStartTime());
        stmt.bindLong(6, entity.getEndTime());
        stmt.bindLong(7, entity.getDuration());
    }

    /** @inheritdoc */
    @Override
    public Long readKey(Cursor cursor, int offset) {
        return cursor.isNull(offset + 0) ? null : cursor.getLong(offset + 0);
    }    

    /** @inheritdoc */
    @Override
    public BLEStatus readEntity(Cursor cursor, int offset) {
    	BLEStatus entity = new BLEStatus( //
            cursor.isNull(offset + 0) ? null : cursor.getLong(offset + 0), // id
            cursor.isNull(offset + 1) ? null : cursor.getString(offset + 1), // device name
            cursor.isNull(offset + 2) ? null : cursor.getString(offset + 2), // device address
            cursor.isNull(offset + 3) ? null : cursor.getInt(offset + 3), // status
			cursor.isNull(offset + 4) ? null : cursor.getLong(offset + 4), // start time
			cursor.isNull(offset + 5) ? null : cursor.getLong(offset + 5) // end time
        );
        
        return entity;
    }
     
    /** @inheritdoc */
    @Override
    protected Long updateKeyAfterInsert(BLEStatus entity, long rowId) {
        entity.setId(rowId);
        return rowId;
    }
    
    /** @inheritdoc */
    @Override
    public Long getKey(BLEStatus entity) {
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
	protected void readEntity(Cursor arg0, BLEStatus arg1, int arg2) {
		// TODO Auto-generated method stub
	}
    
}
