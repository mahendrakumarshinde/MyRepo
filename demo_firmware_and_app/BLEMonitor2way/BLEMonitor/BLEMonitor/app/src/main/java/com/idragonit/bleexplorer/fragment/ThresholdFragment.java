package com.idragonit.bleexplorer.fragment;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.idragonit.bleexplorer.IReceiveData;
import com.idragonit.bleexplorer.R;
import com.idragonit.bleexplorer.view.ThresholdView;

public class ThresholdFragment extends Fragment implements IReceiveData {

    View mView;
    ThresholdView mThresholdView;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mView = inflater.inflate(R.layout.fragment_threshold, container, false);

        mThresholdView = (ThresholdView) mView.findViewById(R.id.threshold);

        return mView;
    }

    @Override
    public void readData(int status, long readTime) {
    }
}