package com.idragonit.bleexplorer;

import android.annotation.TargetApi;
import android.app.Service;
import android.bluetooth.*;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;
import android.widget.Toast;
import android.os.*;

import java.util.*;

@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR2)
public class BluetoothLeService extends Service {
    public static final String ACTION_DATA_AVAILABLE = "com.idragonit.bleexplorer.ACTION_DATA_AVAILABLE";
    public static final String ACTION_GATT_CONNECTED = "com.idragonit.bleexplorer.ACTION_GATT_CONNECTED";
    public static final String ACTION_GATT_DISCONNECTED = "com.idragonit.bleexplorer.ACTION_GATT_DISCONNECTED";
    public static final String ACTION_GATT_SERVICES_DISCOVERED = "com.idragonit.bleexplorer.ACTION_GATT_SERVICES_DISCOVERED";
    public static final String DEVICE_DOES_NOT_SUPPORT_UART = "com.nordicsemi.nrfUART.DEVICE_DOES_NOT_SUPPORT_UART";
    public static final String EXTRA_DATA = "com.idragonit.bleexplorer.EXTRA_DATA";
    public static boolean IsDualMode = false;
    public static final String RSSI_DATA_AVAILABLE = "com.idragonit.bleexplorer.RSSI_DATA_AVAILABLE";
    private static final int STATE_CONNECTED = 2;
    private static final int STATE_CONNECTING = 1;
    private static final int STATE_DISCONNECTED = 0;
    private static final String TAG = BluetoothLeService.class.getSimpleName();
    private static final int mDualModeDelay = 5000;
    private static Handler mTimeoutHandler = null;
    private static Runnable mTimeoutRunnable = null;
    static int packetLength = 0;
    static int packetStartPos = 0;
    private boolean IsTimerShouldCancel;
    private final IBinder mBinder = new LocalBinder();
    private BluetoothAdapter mBluetoothAdapter;
    private String mBluetoothDeviceAddress;
    private BluetoothGatt mBluetoothGatt;
    private BluetoothManager mBluetoothManager;
    private int mConnectionState;

    public final BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {

        public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
            broadcastUpdate(ACTION_DATA_AVAILABLE, characteristic);
        }

        public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS)
                broadcastUpdate(ACTION_DATA_AVAILABLE, characteristic);
        }

        public void onCharacteristicWrite(BluetoothGatt bluetoothgatt, BluetoothGattCharacteristic bluetoothgattcharacteristic, int i) {
        }

        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                mConnectionState = STATE_CONNECTED;
                broadcastUpdate(ACTION_GATT_CONNECTED);

                Log.i(BluetoothLeService.TAG, "Connected to GATT server.");
                Log.i(BluetoothLeService.TAG, "Attempting to start service discovery:" + mBluetoothGatt.discoverServices());

                IsTimerShouldCancel = false;

                TimerTask timertask = new TimerTask() {

                    public void run() {
                        if (IsTimerShouldCancel) {
                            mRssiTimer.cancel();
                        } else {
                            mBluetoothGatt.readRemoteRssi();
                        }
                    }
                };

                mRssiTimer = new Timer();
                mRssiTimer.schedule(timertask, 1000L, 1000L);
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                mConnectionState = STATE_DISCONNECTED;

                Log.i(BluetoothLeService.TAG, "Disconnected from GATT server.");

                broadcastUpdate(ACTION_GATT_DISCONNECTED);
                IsTimerShouldCancel = true;
            }
        }

        public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status) {
            //Log.d("onReadRemoteRss()", String.format("rssi=%d, status=%d", rssi, status));

            super.onReadRemoteRssi(gatt, rssi, status);

            //broadcastRSSIUpdate(RSSI_DATA_AVAILABLE, rssi);
        }

        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                if (BluetoothLeService.IsDualMode) {
                    Log.d(BluetoothLeService.TAG, "Dual Mode Services Discovered");
                    if (BluetoothLeService.mTimeoutHandler != null) {
                        BluetoothLeService.mTimeoutHandler.removeCallbacks(BluetoothLeService.mTimeoutRunnable);
                        BluetoothLeService.mTimeoutRunnable = null;
                        BluetoothLeService.mTimeoutHandler = null; //had the issue  now

                    }
                }
                broadcastUpdate(ACTION_GATT_SERVICES_DISCOVERED);
            } else {
                Log.w(BluetoothLeService.TAG, "onServicesDiscovered received: " + status);
            }
        }
    };

    private Timer mRssiTimer;
    byte packetPayload[];

    static {
        IsDualMode = false;
    }

    public class LocalBinder extends Binder {
        BluetoothLeService getService() {
            return BluetoothLeService.this;
        }
    }

    public BluetoothLeService() {
        mConnectionState = STATE_DISCONNECTED;
    }

    private void broadcastRSSIUpdate(String action, int value) {
        Log.d(TAG, String.format("broadcastRSSIUpdate Received Value=%d", value));

        Intent intent = new Intent(action);
        intent.putExtra(EXTRA_DATA, String.valueOf(value));

        sendBroadcast(intent);
    }

    private void broadcastUpdate(String action) {
        sendBroadcast(new Intent(action));
    }

    private void broadcastUpdate(String action, BluetoothGattCharacteristic characteristic) {
        Intent intent = new Intent(action);

        if (GattAttributes.IsUartTxCharacteristic(characteristic.getUuid()) < 0) {
            byte data[] = characteristic.getValue();
            if (data != null && data.length > 0)
                intent.putExtra(EXTRA_DATA, data);
        } else {
            intent.putExtra(EXTRA_DATA, characteristic.getValue());
        }

        sendBroadcast(intent);
    }

    void CreateTimeoutHandler() {
        mTimeoutHandler = new Handler();
        mTimeoutRunnable = new Runnable() {

            public void run() {
                Toast.makeText(getApplication(), "Try Connecting BLE", Toast.LENGTH_LONG).show();
                mBluetoothGatt.disconnect();
                Log.d(BluetoothLeService.TAG, "run Dual Mode disconnect");
                mBluetoothGatt.connect();
                Log.d(BluetoothLeService.TAG, "run Dual Mode connect");
            }
        };

        mTimeoutHandler.postDelayed(mTimeoutRunnable, mDualModeDelay);
    }

    public void close() {
        Log.w(TAG, "BluetoothLeService.close()");

        if (mBluetoothGatt != null) {
            mBluetoothGatt.disconnect();
            mBluetoothGatt.close();
            mBluetoothGatt = null;
            IsTimerShouldCancel = true;
        }
    }

    public boolean connect(String address) {
        if (mBluetoothAdapter == null || address == null) {
            Log.w(TAG, "BluetoothAdapter not initialized or unspecified address.");
            return false;
        }

        if (mBluetoothDeviceAddress != null && address.equals(mBluetoothDeviceAddress) && mBluetoothGatt != null) {
            Log.d(TAG, "Trying to use an existing mBluetoothGatt for connection.");
            if (mBluetoothGatt.connect()) {
                mConnectionState = STATE_CONNECTING;
                return true;
            } else {
                return false;
            }
        }

        BluetoothDevice bluetoothdevice = mBluetoothAdapter.getRemoteDevice(address);

        if (bluetoothdevice == null) {
            Log.w(TAG, "Device not found.  Unable to connect.");
            return false;
        }

        if (IsDualMode) {
            Log.d(TAG, "try connect Dual Mode");
            mBluetoothGatt = bluetoothdevice.connectGatt(this, true, mGattCallback);
            CreateTimeoutHandler();
        } else {
            mBluetoothGatt = bluetoothdevice.connectGatt(this, false, mGattCallback);
        }

        Log.d(TAG, "Trying to create a new connection.");
        mBluetoothDeviceAddress = address;
        mConnectionState = STATE_CONNECTING;

        return true;
    }

    public void disconnect() {
        Log.w(TAG, "BluetoothLeService.disconnect()");

        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
        } else {
            mBluetoothGatt.disconnect();
            IsTimerShouldCancel = true;
        }
    }

    public void enableTXNotification() {
        Log.d(TAG, String.format("enableTXNotification():uartIndex=%d", ReadDataActivity.uartIndex));
        BluetoothGattService bluetoothgattservice = mBluetoothGatt.getService(UUID.fromString(((UartChar) GattAttributes.uarts.get(ReadDataActivity.uartIndex)).Service));
        if (bluetoothgattservice == null) {
            Log.d(TAG, "UART service not found!");
            broadcastUpdate(DEVICE_DOES_NOT_SUPPORT_UART);
            return;
        }

        BluetoothGattCharacteristic characteristic = bluetoothgattservice.getCharacteristic(UUID.fromString(((UartChar) GattAttributes.uarts.get(ReadDataActivity.uartIndex)).TxChar));
        if (characteristic == null) {
            Log.d(TAG, "Tx charateristic not found!");
            broadcastUpdate(DEVICE_DOES_NOT_SUPPORT_UART);
        } else {
            mBluetoothGatt.setCharacteristicNotification(characteristic, true);
            BluetoothGattDescriptor descriptor = characteristic.getDescriptor(GattAttributes.UUID_CLIENT_CHARACTERISTIC_CONFIG);
            descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
            mBluetoothGatt.writeDescriptor(descriptor);
        }
    }

    public List getSupportedGattServices() {
        Log.w(TAG, "BluetoothLeService.getSupportedGattServices()");

        if (mBluetoothGatt == null)
            return null;

        return mBluetoothGatt.getServices();
    }

    public boolean initialize() {
        if (mBluetoothManager == null) {
            mBluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
            if (mBluetoothManager == null) {
                Log.e(TAG, "Unable to initialize BluetoothManager.");
                return false;
            }
        }

        mBluetoothAdapter = mBluetoothManager.getAdapter();
        if (mBluetoothAdapter == null) {
            Log.e(TAG, "Unable to obtain a BluetoothAdapter.");
            return false;
        }

        return true;
    }

    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    public boolean onUnbind(Intent intent) {
        close();
        return super.onUnbind(intent);
    }

    public void readCharacteristic(BluetoothGattCharacteristic characteristic) {
        Log.w(TAG, "BluetoothLeService.readCharacteristic()");

        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
        } else {
            mBluetoothGatt.readCharacteristic(characteristic);
        }
    }

    public void setCharacteristicIndication(BluetoothGattCharacteristic characteristic, boolean enable) {
        Log.w(TAG, "BluetoothLeService.setCharacteristicIndication()");

        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
        } else {
            mBluetoothGatt.setCharacteristicNotification(characteristic, enable);

            BluetoothGattDescriptor bluetoothgattdescriptor = characteristic.getDescriptor(GattAttributes.UUID_CLIENT_CHARACTERISTIC_CONFIG);

            Log.d(TAG, String.format("enable indication value : %s ", (new DataManager()).byteArrayToHex(BluetoothGattDescriptor.ENABLE_INDICATION_VALUE)));

            bluetoothgattdescriptor.setValue(BluetoothGattDescriptor.ENABLE_INDICATION_VALUE);
            mBluetoothGatt.writeDescriptor(bluetoothgattdescriptor);
        }
    }

    public void setCharacteristicNotification(BluetoothGattCharacteristic characteristic, boolean enable) {
        Log.w(TAG, "BluetoothLeService.setCharacteristicNotification()");
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
        } else {
            mBluetoothGatt.setCharacteristicNotification(characteristic, enable);
            BluetoothGattDescriptor bluetoothgattdescriptor = characteristic.getDescriptor(GattAttributes.UUID_CLIENT_CHARACTERISTIC_CONFIG);

            Log.d(TAG, String.format("enable notify value : %s ", (new DataManager()).byteArrayToHex(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)));

            bluetoothgattdescriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
            mBluetoothGatt.writeDescriptor(bluetoothgattdescriptor);
        }
    }

//    public void writeCharacteristic(BluetoothGattCharacteristic characteristic, byte data[]) {
//        int len = 20;
//        packetPayload = Arrays.copyOf(data, data.length);
//        packetStartPos = 0;
//        if (data.length - packetStartPos <= len)
//            len = data.length - packetStartPos;
//        packetLength = len;
//        Log.d(TAG, "Total Length = " + data.length + ", Packet Length = " + packetLength);
//        byte[] value = Arrays.copyOfRange(data, packetStartPos, packetLength);
//        DeviceControlActivity.mTargetCharacteristic.setValue(value);
//        boolean status = mBluetoothGatt.writeCharacteristic(characteristic);
//        Log.d(TAG, "write status=" + status);
//    }
//
//    public void writeRXCharacteristic(byte data[]) {
//        BluetoothGattService bluetoothgattservice = mBluetoothGatt.getService(UUID.fromString(((UartChar) GattAttributes.uarts.get(CharacteristicUartActivity.uartIndex)).Service));
//        Log.d(TAG, "writeRXCharacteristic");
//
//        if (bluetoothgattservice == null) {
//            Log.d(TAG, "Rx service not found!");
//            broadcastUpdate(DEVICE_DOES_NOT_SUPPORT_UART);
//            return;
//        }
//
//        BluetoothGattCharacteristic characteristic = bluetoothgattservice.getCharacteristic(UUID.fromString(((UartChar) GattAttributes.uarts.get(CharacteristicUartActivity.uartIndex)).RxChar));
//        if (characteristic == null) {
//            Log.d(TAG, "Rx charateristic not found!");
//            broadcastUpdate(DEVICE_DOES_NOT_SUPPORT_UART);
//            return;
//        }
//
//        packetPayload = Arrays.copyOf(data, data.length);
//        packetStartPos = 0;
//
//        int len;
//        if (data.length - packetStartPos > 20)
//            len = 20;
//        else
//            len = data.length - packetStartPos;
//        packetLength = len;
//
//        Log.d(TAG, "Total Length = " + data.length + ", Packet Length = " + packetLength);
//
//        characteristic.setValue(Arrays.copyOfRange(data, packetStartPos, packetLength));
//
//        boolean status = mBluetoothGatt.writeCharacteristic(characteristic);
//        Log.d(TAG, "write Nordic RXchar - status=" + status);
//    }
}
