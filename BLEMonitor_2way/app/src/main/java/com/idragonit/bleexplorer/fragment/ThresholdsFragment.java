package com.idragonit.bleexplorer.fragment;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import com.github.mikephil.charting.charts.HorizontalBarChart;
import com.github.mikephil.charting.data.BarData;
import com.github.mikephil.charting.data.BarDataSet;
import com.github.mikephil.charting.data.BarEntry;
import com.idragonit.bleexplorer.IReceiveData;
import com.idragonit.bleexplorer.R;
import com.idragonit.bleexplorer.ReadDataActivity;

import org.achartengine.ChartFactory;
import org.achartengine.GraphicalView;
import org.achartengine.model.CategorySeries;
import org.achartengine.renderer.DefaultRenderer;
import org.achartengine.renderer.SimpleSeriesRenderer;

import java.util.ArrayList;

public class ThresholdsFragment extends Fragment implements IReceiveData {
    /** The main series that will include all the data. */

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void readData(int status, long readTime) {

    }
}