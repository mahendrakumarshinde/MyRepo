package com.idragonit.bleexplorer;

import android.annotation.TargetApi;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.support.design.widget.TabLayout;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentStatePagerAdapter;
import android.support.v4.view.ViewPager;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ExpandableListView;
import android.widget.SimpleExpandableListAdapter;
import android.widget.TextView;

import com.idragonit.bleexplorer.dao.BLEStatus;
import com.idragonit.bleexplorer.dao.DaoUtils;
import com.idragonit.bleexplorer.fragment.MonitoringFragment;
import com.idragonit.bleexplorer.fragment.UtilizationFragment;
import com.idragonit.bleexplorer.fragment.ThresholdsFragment;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.UUID;

@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR2)
public class ReadDataActivity extends FragmentActivity implements IReceiveData {
    public static final String EXTRAS_DEVICE_ADDRESS = "DEVICE_ADDRESS";
    public static final String EXTRAS_DEVICE_NAME = "DEVICE_NAME";
    public static final long MONITOR_TIME = 1000L;
    public static final int REFRESH_MONITOR = 1;

    private static final String TAG = ReadDataActivity.class.getSimpleName();
    private final String LIST_NAME = "NAME";
    private final String LIST_PERMISSION = "PERMISSION";
    private final String LIST_PROPERTIES = "PROPERTIES";
    private final String LIST_UUID = "UUID";
    public static BluetoothLeService mBluetoothLeService;
    public static boolean mConnected = false;
    public static String mDeviceAddress;
    public static String mDeviceName;
    public static BluetoothGattCharacteristic mTargetCharacteristic;
    private TextView mConnectionState;
    private ArrayList mGattCharacteristics;
    private ExpandableListView mGattServicesList;
    private Button mBtnConnect;
    private View mLayoutChart;
    private TextView mTxtData;

    private IReceiveData mMonitoringListener = null;
    private IReceiveData mUtilizationListener = null;
    private IReceiveData mThresholdsListener = null;

    public static int uartIndex = -1;

    public ArrayList<BLEStatus> mStatusList = new ArrayList<BLEStatus>();
    public BLEStatus mLastStatus = null;
    public long[] mDurations = new long[4];

    public Thread mMonitorThread = null;
    public boolean isRunning = false;

    private final BroadcastReceiver mGattUpdateReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            if (BluetoothLeService.ACTION_GATT_CONNECTED.equals(action)) {
                mConnected = true;
                updateConnectionState(R.string.connected);
            } else if (BluetoothLeService.ACTION_GATT_DISCONNECTED.equals(action)) {
                mConnected = false;
                mTargetCharacteristic = null;
                updateConnectionState(R.string.disconnected);
                clearUI();
                disconnect();
            } else if (BluetoothLeService.ACTION_GATT_SERVICES_DISCOVERED.equals(action))
                displayGattServices(mBluetoothLeService.getSupportedGattServices());
            else if (DeviceScanActivity.DEVICE_DATA_AVAILABLE.equals(action)) {
                mDeviceName = intent.getStringExtra("DEVICE_NAME");
                mDeviceAddress = intent.getStringExtra("DEVICE_ADDRESS");
                //displayData(intent.getStringExtra(BluetoothLeService.EXTRA_DATA));
            } else if (BluetoothLeService.ACTION_DATA_AVAILABLE.equals(action)) {
                final byte data[] = intent.getByteArrayExtra(BluetoothLeService.EXTRA_DATA);
                Log.d(TAG, "Uart BroadcastReceiver :" + (new DataManager()).byteArrayToHex(data));

                runOnUiThread(new Runnable() {

                    public void run() {
                        try {
                            String state = new String(data, "UTF-8");
                            displayData(state);
                        } catch (Exception e) {

                        }
                    }
                });
            }

            Log.d(ReadDataActivity.TAG, "BroadcastReceiver.onReceive():action=" + action);
        }
    };

    private final ServiceConnection mServiceConnection = new ServiceConnection() {

        public void onServiceConnected(ComponentName componentname, IBinder ibinder) {
            mBluetoothLeService = ((BluetoothLeService.LocalBinder) ibinder).getService();
            if (!mBluetoothLeService.initialize()) {
                Log.e(ReadDataActivity.TAG, "Unable to initialize Bluetooth");
                finish();
            }

            mBluetoothLeService.connect(mDeviceAddress);
        }

        public void onServiceDisconnected(ComponentName componentname) {
            mBluetoothLeService = null;
        }
    };

    private final ExpandableListView.OnChildClickListener servicesListClickListner = new ExpandableListView.OnChildClickListener() {

        public boolean onChildClick(ExpandableListView expandablelistview, View view, int groupPosition, int childPosition, long id) {
            if (mGattCharacteristics == null) {
                mTargetCharacteristic = null;
                return false;
            }

            mTargetCharacteristic = (BluetoothGattCharacteristic) ((ArrayList) mGattCharacteristics.get(groupPosition)).get(childPosition);
            UUID uuid = mTargetCharacteristic.getUuid();

            Log.d(TAG, "OnChildClickListener, UUID="+uuid.toString());

            if (GattAttributes.IsUartCharacteristic(uuid) >= 0) {
                uartIndex = GattAttributes.IsUartCharacteristic(uuid);
                mBluetoothLeService.enableTXNotification();
                Log.d(TAG, "OnChildClickListener, UART=" + uartIndex);
            } else {
                Log.d(TAG, "OnChildClickListener, NO UART");
            }

            mLayoutChart.setVisibility(View.VISIBLE);
            mGattServicesList.setVisibility(View.GONE);

            return true;
        }
    };

    Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == REFRESH_MONITOR) {
                if (mLastStatus != null) {
                    long curTime = System.currentTimeMillis();
                    mLastStatus.setEndTime(curTime);

                    if (mMonitoringListener != null)
                        mMonitoringListener.readData(mLastStatus.getStatus(), curTime);
                    if (mUtilizationListener != null)
                        mUtilizationListener.readData(mLastStatus.getStatus(), curTime);
                    if (mThresholdsListener != null)
                        mThresholdsListener.readData(mLastStatus.getStatus(), curTime);
                }
            }
        }
    };

    public ReadDataActivity() {
        mGattCharacteristics = new ArrayList();
        mConnected = false;
    }

    private void clearUI() {
        mGattServicesList.setAdapter((SimpleExpandableListAdapter) null);
    }

    public long getStatusDuration(int status) {
        long duration = mDurations[status - 1];
        for (int i = 0; i < mStatusList.size(); i++) {
            BLEStatus bleData = mStatusList.get(i);

            if (bleData.getStatus() == status)
                duration += bleData.getEndTime() - bleData.getStartTime();
        }

        return duration;
    }

    public long getTotalDuration() {
        long duration = 0;
        for (int i = 0; i < 4; i++)
            duration += getStatusDuration(i + 1);

        return duration;
    }

    public int getStatusPercent(int status) {
        long totalDuration = getTotalDuration();
        long duration = getStatusDuration(status);

        if (totalDuration == 0)
            return 0;

        return (int) (duration * 100L / totalDuration);
    }

    private void startMonitorThread() {
        if (mMonitorThread != null)
            return;

        mMonitorThread = new Thread() {
            public void run() {
                while (isRunning) {
                    try {
                        sleep(MONITOR_TIME);
                    }
                    catch (Exception e) {

                    }

                    if (mLastStatus != null) {
                        mHandler.sendEmptyMessage(REFRESH_MONITOR);
                    }
                }

                mMonitorThread = null;
            }
        };

        isRunning = true;
        mMonitorThread.start();
    }

    private void endMonitorThread() {
        if (mMonitorThread == null)
            return;

        isRunning = false;
    }

    private void disconnect() {
        if (mLastStatus != null) {
            mLastStatus.setEndTime(System.currentTimeMillis());
            mLastStatus = null;
        }

        addStatus();
    }

    private void displayData(String data) {
        long curTime = System.currentTimeMillis();
        int status = STATUS_NONE;

        if (data.contains(STATUS_IDLE_NAME)) {
            status = STATUS_IDLE;
        }
        else if (data.contains(STATUS_NORMAL_CUTTING_NAME)) {
            status = STATUS_NORMAL_CUTTING;
        }
        else if (data.contains(STATUS_WARNING_NAME)) {
            status = STATUS_WARNING;
        }
        else if (data.contains(STATUS_DANGER_NAME)) {
            status = STATUS_DANGER;
        }

        if (status != STATUS_NONE) {
            mLastStatus = new BLEStatus(mDeviceName, mDeviceAddress, status, curTime, curTime);
            mStatusList.add(mLastStatus);

            if (mMonitoringListener != null)
                mMonitoringListener.readData(status, curTime);
            if (mUtilizationListener != null)
                mUtilizationListener.readData(status, curTime);
            if (mThresholdsListener != null)
                mThresholdsListener.readData(status, curTime);

            mTxtData.setText(data);
        }
    }

    public void addStatus() {
        for (int i = 0; i < mStatusList.size(); i++) {
            BLEStatus status = mStatusList.get(i);
            mDurations[status.getStatus() - 1] += status.getEndTime() - status.getStartTime();
        }

        DaoUtils.addBLEStatus(this, mStatusList);
        mStatusList.clear();
        mLastStatus = null;
    }

    private void displayGattServices(List gattList) {
        if (gattList == null)
            return;

        String unknownService = getResources().getString(R.string.unknown_service);
        String unknownCharacteristic = getResources().getString(R.string.unknown_characteristic);

        ArrayList gattServiceArrayList = new ArrayList();
        ArrayList devInfoList = new ArrayList();
        mGattCharacteristics = new ArrayList();

        for (int i = 0; i < gattList.size(); i++) {
            BluetoothGattService bluetoothgattservice = (BluetoothGattService) gattList.get(i);
            HashMap hashmap = new HashMap();
            String uuid = bluetoothgattservice.getUuid().toString();

            hashmap.put(LIST_NAME, GattAttributes.lookup(uuid, unknownService));
            hashmap.put(LIST_UUID, uuid);

            gattServiceArrayList.add(hashmap);

            ArrayList deviceInfoArrayList = new ArrayList();
            List characteristicList = bluetoothgattservice.getCharacteristics();
            ArrayList characterArrayList = new ArrayList();

            for (int k = 0; k < characteristicList.size(); k++) {
                BluetoothGattCharacteristic characteristic = (BluetoothGattCharacteristic) characteristicList.get(k);
                characterArrayList.add(characteristic);

                HashMap deviceInfoMap = new HashMap();
                String characterUUID = characteristic.getUuid().toString();

                deviceInfoMap.put(LIST_NAME, GattAttributes.lookup(characterUUID, unknownCharacteristic));
                deviceInfoMap.put(LIST_UUID, characterUUID);

                int properties = characteristic.getProperties();
                String propertiesData = String.format("[%04X]", properties);

                if ((properties & 1) > 0)
                    propertiesData = propertiesData + " Broadcast";
                if ((properties & 0x20) > 0)
                    propertiesData = propertiesData + " Indicate";
                if ((properties & 0x80) > 0)
                    propertiesData = propertiesData + " ExtendedProps";
                if ((properties & 2) > 0)
                    propertiesData = propertiesData + " Read";
                if ((properties & 8) > 0)
                    propertiesData = propertiesData + " Write";
                if ((properties & 0x40) > 0)
                    propertiesData = propertiesData + " SignedWrite";
                if ((properties & 4) > 0)
                    propertiesData = propertiesData + " WriteNoResponse";
                if ((properties & 0x10) > 0)
                    propertiesData = propertiesData + " Notify";

                deviceInfoMap.put(LIST_PROPERTIES, propertiesData);

                int permissions = characteristic.getPermissions();
                String permissionsData = String.format("[%04X]", permissions);

                if ((permissions & 1) > 0)
                    permissionsData = permissionsData + " Read";
                if ((permissions & 2) > 0)
                    permissionsData = permissionsData + " ReadEncrypted";
                if ((permissions & 4) > 0)
                    permissionsData = permissionsData + " ReadEncryptedMitm";
                if ((permissions & 0x10) > 0)
                    permissionsData = permissionsData + " Write";
                if ((permissions & 0x20) > 0)
                    permissionsData = permissionsData + " WriteEncrypted";
                if ((permissions & 0x40) > 0)
                    permissionsData = permissionsData + "WriteEncryptedMitm";
                if ((permissions & 0x80) > 0)
                    permissionsData = permissionsData + " WriteSigned";
                if ((permissions & 0x100) > 0)
                    permissionsData = permissionsData + " WriteSignedMitm";

                deviceInfoMap.put(LIST_PERMISSION, permissionsData);
                deviceInfoArrayList.add(deviceInfoMap);
            }

            mGattCharacteristics.add(characterArrayList);
            devInfoList.add(deviceInfoArrayList);
        }

        SimpleExpandableListAdapter simpleexpandablelistadapter = new SimpleExpandableListAdapter(this, gattServiceArrayList, 0x1090007, new String[]{
                LIST_NAME, LIST_UUID
        }, new int[]{
                0x1020014, 0x1020015
        }, devInfoList, R.layout.characteristics_item_view, new String[]{
                LIST_NAME, LIST_UUID, LIST_PROPERTIES, LIST_PERMISSION
        }, new int[]{
                R.id.characteristic_name, R.id.characteristic_uuid, R.id.characteristic_properties, R.id.characteristic_permission
        });

        mGattServicesList.setAdapter(simpleexpandablelistadapter);
    }

    private static IntentFilter makeGattUpdateIntentFilter() {
        IntentFilter intentfilter = new IntentFilter();

        intentfilter.addAction(BluetoothLeService.ACTION_GATT_CONNECTED);
        intentfilter.addAction(BluetoothLeService.ACTION_GATT_DISCONNECTED);
        intentfilter.addAction(BluetoothLeService.ACTION_GATT_SERVICES_DISCOVERED);
        intentfilter.addAction(DeviceScanActivity.DEVICE_DATA_AVAILABLE);
        intentfilter.addAction(BluetoothLeService.RSSI_DATA_AVAILABLE);
        intentfilter.addAction(BluetoothLeService.ACTION_DATA_AVAILABLE);

        return intentfilter;
    }

    private void updateConnectionState(final int resourceId) {
        runOnUiThread(new Runnable() {
            public void run() {
                mConnectionState.setText(resourceId);
                if (resourceId == R.string.connected)
                    mBtnConnect.setText(R.string.menu_disconnect);
                else
                    mBtnConnect.setText(R.string.menu_connect);
            }
        });
    }

    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);

        setContentView(R.layout.activity_read_data);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        Intent intent = getIntent();

        mDeviceName = intent.getStringExtra(EXTRAS_DEVICE_NAME);
        mDeviceAddress = intent.getStringExtra(EXTRAS_DEVICE_ADDRESS);

        mDurations = DaoUtils.getStatusDuration(this, mDeviceName, mDeviceAddress);

        ((TextView) findViewById(R.id.device_name)).setText(mDeviceName);

        mTxtData = (TextView) findViewById(R.id.data_value);

        mGattServicesList = (ExpandableListView) findViewById(R.id.gatt_services_list);
        mGattServicesList.setOnChildClickListener(servicesListClickListner);

        mLayoutChart = findViewById(R.id.layout_chart);
        mLayoutChart.setVisibility(View.GONE);
        mGattServicesList.setVisibility(View.VISIBLE);

        mConnectionState = (TextView) findViewById(R.id.connection_state);
        mBtnConnect = (Button) findViewById(R.id.btn_connect);
        mBtnConnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mConnected) {
                    mBluetoothLeService.disconnect();
                } else {
                    mBluetoothLeService.connect(mDeviceAddress);
                }
            }
        });

        TabLayout tabLayout = (TabLayout) findViewById(R.id.tab_layout);
        tabLayout.addTab(tabLayout.newTab().setText("Monitoring"));
        tabLayout.addTab(tabLayout.newTab().setText("Utilization"));
        tabLayout.addTab(tabLayout.newTab().setText("Thresholds"));
        tabLayout.setTabGravity(TabLayout.GRAVITY_FILL);

        final ViewPager viewPager = (ViewPager) findViewById(R.id.pager);
        final PagerAdapter adapter = new PagerAdapter(getSupportFragmentManager(), tabLayout.getTabCount());
        viewPager.setAdapter(adapter);
        viewPager.addOnPageChangeListener(new TabLayout.TabLayoutOnPageChangeListener(tabLayout));
        tabLayout.setOnTabSelectedListener(new TabLayout.OnTabSelectedListener() {
            @Override
            public void onTabSelected(TabLayout.Tab tab) {
                viewPager.setCurrentItem(tab.getPosition());
            }

            @Override
            public void onTabUnselected(TabLayout.Tab tab) {

            }

            @Override
            public void onTabReselected(TabLayout.Tab tab) {

            }
        });

        bindService(new Intent(this, BluetoothLeService.class), mServiceConnection, BIND_AUTO_CREATE);
    }

    protected void onDestroy() {
        super.onDestroy();
        unbindService(mServiceConnection);
        mBluetoothLeService = null;
    }

    protected void onPause() {
        super.onPause();

        endMonitorThread();

        if (mLastStatus != null) {
            mLastStatus.setEndTime(System.currentTimeMillis());
        }

        addStatus();
        unregisterReceiver(mGattUpdateReceiver);
    }

    protected void onResume() {
        super.onResume();

        registerReceiver(mGattUpdateReceiver, makeGattUpdateIntentFilter());

        if (mBluetoothLeService != null) {
            boolean result = mBluetoothLeService.connect(mDeviceAddress);
            Log.d(TAG, "Connect request result=" + result);
        }

        startMonitorThread();
    }

    @Override
    public void readData(int status, long readTime) {

    }

    public class PagerAdapter extends FragmentStatePagerAdapter {
        int mNumOfTabs;

        public PagerAdapter(FragmentManager fm, int NumOfTabs) {
            super(fm);
            this.mNumOfTabs = NumOfTabs;
        }

        @Override
        public Fragment getItem(int position) {

            switch (position) {
                case 0:
                    MonitoringFragment monitoringFragment = new MonitoringFragment();
                    mMonitoringListener = monitoringFragment;
                    return monitoringFragment;
                case 1:
                    UtilizationFragment utilizationFragment = new UtilizationFragment();
                    mUtilizationListener = utilizationFragment;
                    return utilizationFragment;
                case 2:
                    ThresholdsFragment thresholdsFragment = new ThresholdsFragment();
                    mThresholdsListener = thresholdsFragment;
                    return thresholdsFragment;
                default:
                    return null;
            }
        }

        @Override
        public int getCount() {
            return mNumOfTabs;
        }
    }
}
