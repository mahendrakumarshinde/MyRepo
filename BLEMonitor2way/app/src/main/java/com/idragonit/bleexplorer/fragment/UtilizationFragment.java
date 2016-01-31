package com.idragonit.bleexplorer.fragment;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.idragonit.bleexplorer.IReceiveData;
import com.idragonit.bleexplorer.R;

public class UtilizationFragment extends Fragment implements IReceiveData {

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_utilization, container, false);
    }

    @Override
    public void readData(String data, long readTime) {

    }
}