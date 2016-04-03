package com.idragonit.bleexplorer.dao;

import android.database.sqlite.SQLiteDatabase;

import java.util.Map;

import de.greenrobot.dao.AbstractDao;
import de.greenrobot.dao.AbstractDaoSession;
import de.greenrobot.dao.identityscope.IdentityScopeType;
import de.greenrobot.dao.internal.DaoConfig;

/**
 * {@inheritDoc}
 * 
 * @see de.greenrobot.dao.AbstractDaoSession
 */
public class DaoSession extends AbstractDaoSession {

    private final DaoConfig bleStatusDaoConfig;
    private final DaoConfig bleFeatureDaoConfig;
    private final BLEStatusDao bleStatusDao;
    private final BLEFeatureDao bleFeatureDao;

    public DaoSession(SQLiteDatabase db, IdentityScopeType type, Map<Class<? extends AbstractDao<?, ?>>, DaoConfig>
            daoConfigMap) {
        super(db);

        bleStatusDaoConfig = daoConfigMap.get(BLEStatusDao.class).clone();
        bleStatusDaoConfig.initIdentityScope(type);

        bleStatusDao = new BLEStatusDao(bleStatusDaoConfig, this);

        registerDao(BLEStatus.class, bleStatusDao);

        bleFeatureDaoConfig = daoConfigMap.get(BLEFeatureDao.class).clone();
        bleFeatureDaoConfig.initIdentityScope(type);

        bleFeatureDao = new BLEFeatureDao(bleFeatureDaoConfig, this);

        registerDao(BLEFeature.class, bleFeatureDao);
    }
    
    public void clear() {
        bleStatusDaoConfig.getIdentityScope().clear();
        bleFeatureDaoConfig.getIdentityScope().clear();
    }

    public BLEStatusDao getBLEStatusDao() {
        return bleStatusDao;
    }

    public BLEFeatureDao getBLEFeatureDao() {
        return bleFeatureDao;
    }
}
