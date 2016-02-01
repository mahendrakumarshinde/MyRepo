package com.idragonit.bleexplorer.fragment;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.idragonit.bleexplorer.IReceiveData;
import com.idragonit.bleexplorer.R;
import com.idragonit.bleexplorer.ReadDataActivity;

public class MonitoringFragment extends Fragment implements IReceiveData {

    View mView;
    TextView mTxtStatus;
    TextView mTxtStatusName;
    TextView mTxtTime;
    TextView mTxtPercentName;
    TextView mTxtPercent;
    TextView mTxtUtilization;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mView = inflater.inflate(R.layout.fragment_monitoring, container, false);

        mTxtStatus = (TextView) mView.findViewById(R.id.txt_status);
        mTxtStatusName = (TextView) mView.findViewById(R.id.txt_status_name);
        mTxtTime = (TextView) mView.findViewById(R.id.txt_time);
        mTxtPercentName = (TextView) mView.findViewById(R.id.txt_status_percent_name);
        mTxtPercent = (TextView) mView.findViewById(R.id.txt_status_percent);
        mTxtUtilization = (TextView) mView.findViewById(R.id.txt_utilization);

        return mView;
    }

    @Override
    public void readData(int status, long readTime) {
        if (status == STATUS_NONE)
            return;

        long duration = ((ReadDataActivity) getActivity()).getStatusDuration(status);
        int percent = ((ReadDataActivity) getActivity()).getStatusPercent(status);

        mTxtPercent.setText(percent + "%");
        mTxtUtilization.setText((100 - ((ReadDataActivity) getActivity()).getStatusPercent(STATUS_IDLE)) + "%");

        if (duration >= 60000L) {
            mTxtTime.setText((duration / 60000L) + "m");
        }
        else {
            mTxtTime.setText((duration / 1000L) + "s");
        }

        if (status == STATUS_IDLE) {
            mView.setBackgroundColor(STATUS_IDLE_COLOR);
            mTxtStatus.setText(STATUS_IDLE_NAME);
            mTxtStatusName.setText(STATUS_IDLE_NAME + " time :");
            mTxtPercentName.setText(STATUS_IDLE_NAME + " % :");
        }
        else if (status == STATUS_NORMAL_CUTTING) {
            mView.setBackgroundColor(STATUS_NORMAL_CUTTING_COLOR);
            mTxtStatus.setText(STATUS_NORMAL_CUTTING_NAME);
            mTxtStatusName.setText(STATUS_NORMAL_CUTTING_NAME + " time :");
            mTxtPercentName.setText(STATUS_NORMAL_CUTTING_NAME + " % :");
        }
        else if (status == STATUS_WARNING) {
            mView.setBackgroundColor(STATUS_WARNING_COLOR);
            mTxtStatus.setText(STATUS_WARNING_NAME);
            mTxtStatusName.setText(STATUS_WARNING_NAME + " time :");
            mTxtPercentName.setText(STATUS_WARNING_NAME + " % :");
        }
        else if (status == STATUS_DANGER) {
            mView.setBackgroundColor(STATUS_DANGER_COLOR);
            mTxtStatus.setText(STATUS_DANGER_NAME);
            mTxtStatusName.setText(STATUS_DANGER_NAME + " time :");
            mTxtPercentName.setText(STATUS_DANGER_NAME + " % :");
        }
    }
}