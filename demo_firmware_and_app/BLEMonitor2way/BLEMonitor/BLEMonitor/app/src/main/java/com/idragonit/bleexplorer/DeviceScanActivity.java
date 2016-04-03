package com.idragonit.bleexplorer;

import android.annotation.TargetApi;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;

@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR2)
public class DeviceScanActivity extends Activity {
    public static final String DEVICE_DATA_AVAILABLE = "com.idragonit.bleexplorer.DEVICE_DATA_AVAILABLE";
    private static final int MaxDeviceCount = 500;
    private static final int REQUEST_ENABLE_BT = 1;
    private static final long SCAN_PERIOD = 5000L;
    private static final int STATE_NONE = 0;
    private static final int STATE_SCANNING = 1;
    private static final int STATE_STOP = 2;
    private static final String TAG = DeviceScanActivity.class.getSimpleName();
    private BluetoothAdapter mBluetoothAdapter;
    private Handler mHandler;
    private LeDeviceListAdapter mLeDeviceListAdapter;
    private BluetoothAdapter.LeScanCallback mLeScanCallback;
    private boolean mScanning;
    private deviceInfo scanDevice[];
    private int scanIndex;

    private ListView mListView;
    private Button mBtnScan;

    private View mLayoutDevices;
    private View mLayoutScan;
    private View mLayoutNoResult;

    private class LeDeviceListAdapter extends BaseAdapter {
        private LayoutInflater mInflator = getLayoutInflater();
        private ArrayList mLeDevices = new ArrayList();

        public LeDeviceListAdapter() {
            super();
            Log.d(TAG, "*** LeDeviceListAdapter.LeDeviceListAdapter()");
        }

        public void addDevice(BluetoothDevice bluetoothdevice) {
            Log.d(TAG, "*** LeDeviceListAdapter.addDevice()");
            if (!mLeDevices.contains(bluetoothdevice))
                mLeDevices.add(bluetoothdevice);
        }

        public void clear() {
            Log.d(TAG, "*** LeDeviceListAdapter.clear()");
            mLeDevices.clear();
        }

        public int getCount() {
            Log.d(TAG, "*** LeDeviceListAdapter.getCount()");
            return mLeDevices.size();
        }

        public BluetoothDevice getDevice(int i) {
            Log.d(TAG, "*** LeDeviceListAdapter.getDevice()");
            return (BluetoothDevice) mLeDevices.get(i);
        }

        public Object getItem(int i) {
            Log.d(TAG, "*** LeDeviceListAdapter.getItem()");
            return mLeDevices.get(i);
        }

        public long getItemId(int i) {
            Log.d(TAG, "*** LeDeviceListAdapter.getItemId()");
            return (long) i;
        }

        public View getView(int position, View view, ViewGroup viewgroup) {
            Log.d(TAG, String.format("*** LeDeviceListAdapter.getView() position=%d", position));

            ViewHolder viewholder;

            if (view == null) {
                view = mInflator.inflate(R.layout.device_scan, null);
                viewholder = new ViewHolder();
                viewholder.deviceAddress = (TextView) view.findViewById(R.id.device_address);
                viewholder.deviceName = (TextView) view.findViewById(R.id.device_name);
                viewholder.deviceRSSI = (TextView) view.findViewById(R.id.scan_RSSI);
                view.setTag(viewholder);
            } else {
                viewholder = (ViewHolder) view.getTag();
            }

            viewholder.deviceName.setText(scanDevice[position].Name);
            viewholder.deviceAddress.setText(scanDevice[position].Address);

            if (scanDevice[position].RSSI < 65)
                viewholder.deviceRSSI.setText("High");
            else if (scanDevice[position].RSSI < 75)
                viewholder.deviceRSSI.setText("Medium");
            else
                viewholder.deviceRSSI.setText("Low");

            return view;
        }
    }

    static class ViewHolder {

        TextView deviceAddress;
        TextView deviceName;
        TextView deviceRSSI;
    }

    class deviceInfo {
        public String Address;
        public int BondState;
        public String Name;
        public Integer RSSI;
        public int Type;
        public byte[] scanRecord;
    }

    public DeviceScanActivity() {
        scanDevice = new deviceInfo[MaxDeviceCount];
        scanIndex = 0;

        mLeScanCallback = new BluetoothAdapter.LeScanCallback() {

            public void onLeScan(final BluetoothDevice device, int rssi, byte scanRecord[]) {
                Log.d(TAG, String.format("*** BluetoothAdapter.LeScanCallback.onLeScan(): *RSSI=%d", rssi));

                String deviceName = device.getName();
                scanDevice[scanIndex] = new deviceInfo();

                if (deviceName != null && deviceName.length() > 0)
                    scanDevice[scanIndex].Name = deviceName;
                else
                    scanDevice[scanIndex].Name = "unknown device";

                scanDevice[scanIndex].Address = device.getAddress();
                scanDevice[scanIndex].RSSI = Integer.valueOf(rssi);
                scanDevice[scanIndex].Type = device.getType();
                scanDevice[scanIndex].BondState = device.getBondState();

                int length;
                for (length = 0; scanRecord[length] > 0 && length < scanRecord.length; length += 1 + scanRecord[length]) ;

                scanDevice[scanIndex].scanRecord = new byte[length];
                System.arraycopy(scanRecord, 0, scanDevice[scanIndex].scanRecord, 0, length);

                Log.d(TAG, String.format("*** Scan Index=%d, Name=%s, Address=%s, RSSI=%d, scan result length=%d",
                        scanIndex, scanDevice[scanIndex].Name, scanDevice[scanIndex].Address, scanDevice[scanIndex].RSSI, length));

                scanIndex = 1 + scanIndex;

                runOnUiThread(new Runnable() {

                    public void run() {
                        Log.d(TAG, "*** BluetoothAdapter.LeScanCallback.runOnUiThread()");

                        mLeDeviceListAdapter.addDevice(device);
                        mLeDeviceListAdapter.notifyDataSetChanged();
                    }
                });
            }
        };
    }

    private void scanLeDevice(boolean scanning) {
        Log.d(TAG, "*** scanLeDevice()");

        if (scanning) {
            showState(STATE_SCANNING);

            mHandler.postDelayed(new Runnable() {

                @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR2)
                public void run() {
                    if (scanIndex > 0)
                        showState(STATE_STOP);
                    else
                        showState(STATE_NONE);
                    mScanning = false;
                    mBluetoothAdapter.stopLeScan(mLeScanCallback);
                }
            }, SCAN_PERIOD);

            mScanning = true;
            mBluetoothAdapter.startLeScan(mLeScanCallback);
        }
        else {
            if (scanIndex > 0)
                showState(STATE_STOP);
            else
                showState(STATE_NONE);
            mScanning = false;
            mBluetoothAdapter.stopLeScan(mLeScanCallback);
        }
    }

    private void showState(int state) {
        if (state == STATE_NONE) {
            mBtnScan.setText(R.string.menu_scan);

            mLayoutDevices.setVisibility(View.GONE);
            mLayoutScan.setVisibility(View.GONE);
            mLayoutNoResult.setVisibility(View.VISIBLE);
        }
        else if (state == STATE_SCANNING) {
            mBtnScan.setText(R.string.menu_stop);

            mLayoutDevices.setVisibility(View.GONE);
            mLayoutScan.setVisibility(View.VISIBLE);
            mLayoutNoResult.setVisibility(View.GONE);
        }
        else {
            mBtnScan.setText(R.string.menu_scan);

            mLayoutDevices.setVisibility(View.VISIBLE);
            mLayoutScan.setVisibility(View.GONE);
            mLayoutNoResult.setVisibility(View.GONE);
        }
    }

    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        Log.d(TAG, "*** onActivityResult()");

        if (requestCode == REQUEST_ENABLE_BT && resultCode == RESULT_CANCELED) {
            finish();
        }
        else {
            super.onActivityResult(requestCode, resultCode, data);
        }
    }

    public void onCreate(Bundle bundle) {
        Log.d(TAG, "*** onCreate()");

        super.onCreate(bundle);

        setContentView(R.layout.activity_scan);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        mListView = (ListView) findViewById(R.id.list_devices);
        mBtnScan = (Button) findViewById(R.id.btn_scan);

        mLayoutDevices = findViewById(R.id.layout_devices);
        mLayoutScan = findViewById(R.id.layout_scan);
        mLayoutNoResult = findViewById(R.id.layout_no_result);

        mListView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                Log.d(TAG, String.format("*** onListItemClick() position %d", position));

                BluetoothDevice bluetoothdevice = mLeDeviceListAdapter.getDevice(position);

                if (bluetoothdevice == null)
                    return;

                if (mScanning) {
                    mBluetoothAdapter.stopLeScan(mLeScanCallback);
                    if (scanIndex > 0)
                        showState(STATE_STOP);
                    else
                        showState(STATE_NONE);
                    mScanning = false;
                }

                if(scanDevice[position].Type == 3)
                    BluetoothLeService.IsDualMode = true;
                else
                    BluetoothLeService.IsDualMode = false;

                if(BluetoothLeService.IsDualMode)
                    Toast.makeText(getApplication(), "Dual Mode", Toast.LENGTH_LONG).show();

                Intent intent = new Intent(DeviceScanActivity.this, ReadDataActivity.class);

                intent.putExtra("DEVICE_NAME", bluetoothdevice.getName());
                intent.putExtra("DEVICE_ADDRESS", bluetoothdevice.getAddress());

                startActivity(intent);
            }
        });

        mBtnScan.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mScanning) {
                    scanLeDevice(false);
                }
                else {
                    scanIndex = 0;
                    mLeDeviceListAdapter.clear();
                    scanLeDevice(true);
                }

                if (ReadDataActivity.ISTEST) {
                    Intent intent = new Intent(DeviceScanActivity.this, ReadDataActivity.class);
                    startActivity(intent);
                }
            }
        });

        mHandler = new Handler();

        if (!getPackageManager().hasSystemFeature("android.hardware.bluetooth_le")) {
            Toast.makeText(this, R.string.ble_not_supported, Toast.LENGTH_SHORT).show();
            finish();
        }

        mBluetoothAdapter = ((BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE)).getAdapter();

        if (mBluetoothAdapter == null) {
            Toast.makeText(this, R.string.error_bluetooth_not_supported, Toast.LENGTH_SHORT).show();
            finish();
        }
    }

    protected void onPause() {
        Log.d(TAG, "*** onPause()");

        super.onPause();

        scanLeDevice(false);
        mLeDeviceListAdapter.clear();
    }

    protected void onResume() {
        Log.d(TAG, "*** onResume()");

        super.onResume();

        if (!mBluetoothAdapter.isEnabled() && !mBluetoothAdapter.isEnabled())
            startActivityForResult(new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE), REQUEST_ENABLE_BT);

        mLeDeviceListAdapter = new LeDeviceListAdapter();
        mListView.setAdapter(mLeDeviceListAdapter);
        scanLeDevice(true);
    }
}
