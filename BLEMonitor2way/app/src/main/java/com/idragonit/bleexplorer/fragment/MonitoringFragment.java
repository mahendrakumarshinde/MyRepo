package com.idragonit.bleexplorer.fragment;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.idragonit.bleexplorer.IReceiveData;
import com.idragonit.bleexplorer.R;

public class MonitoringFragment extends Fragment implements IReceiveData {

    View mView;
    TextView mTxtStatus;
    TextView mTxtStatusName;
    TextView mTxtTime;
    TextView mTxtUtilization;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mView = inflater.inflate(R.layout.fragment_monitoring, container, false);

        mTxtStatus = (TextView) mView.findViewById(R.id.txt_status);
        mTxtStatusName = (TextView) mView.findViewById(R.id.txt_status_name);
        mTxtTime = (TextView) mView.findViewById(R.id.txt_time);
        mTxtUtilization = (TextView) mView.findViewById(R.id.txt_utilization);

        return mView;
    }

    @Override
    public void readData(String data, long readTime) {
        int status = STATUS_NONE;

        if (data.contains(STATUS_IDLE_NAME)) {
            status = STATUS_IDLE;
            mView.setBackgroundColor(STATUS_IDLE_COLOR);
            mTxtStatus.setText(STATUS_IDLE_NAME);
            mTxtStatusName.setText(STATUS_IDLE_NAME + " time :");
        }
        else if (data.contains("Normal Cutting")) {
            status = STATUS_NORMAL_CUTTING;
            mView.setBackgroundColor(STATUS_NORMAL_CUTTING_COLOR);
            mTxtStatus.setText(STATUS_NORMAL_CUTTING_NAME);
            mTxtStatusName.setText(STATUS_NORMAL_CUTTING_NAME + " time :");
        }
        else if (data.contains("Warning!")) {
            status = STATUS_WARNING;
            mView.setBackgroundColor(STATUS_WARNING_COLOR);
            mTxtStatus.setText(STATUS_WARNING_NAME);
            mTxtStatusName.setText(STATUS_WARNING_NAME + " time :");
        }
        else if (data.contains("Danger!")) {
            status = STATUS_DANGER;
            mView.setBackgroundColor(STATUS_DANGER_COLOR);
            mTxtStatus.setText(STATUS_DANGER_NAME);
            mTxtStatusName.setText(STATUS_DANGER_NAME + " time :");
        }
    }
}