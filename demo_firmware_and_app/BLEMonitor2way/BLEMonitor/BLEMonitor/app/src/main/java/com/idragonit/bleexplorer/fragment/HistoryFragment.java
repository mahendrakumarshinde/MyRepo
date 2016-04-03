package com.idragonit.bleexplorer.fragment;

import android.graphics.Color;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.github.mikephil.charting.charts.CombinedChart;
import com.github.mikephil.charting.components.XAxis;
import com.github.mikephil.charting.components.YAxis;
import com.github.mikephil.charting.data.BarData;
import com.github.mikephil.charting.data.BarDataSet;
import com.github.mikephil.charting.data.BarEntry;
import com.github.mikephil.charting.data.CombinedData;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.idragonit.bleexplorer.AppData;
import com.idragonit.bleexplorer.IReceiveFeatureData;
import com.idragonit.bleexplorer.R;
import com.idragonit.bleexplorer.ReadDataActivity;
import com.idragonit.bleexplorer.dao.BLEFeature;

import java.util.ArrayList;

public class HistoryFragment extends Fragment implements IReceiveFeatureData {

    View mView;
    LinearLayout mLayoutHistory;
    ArrayList<FeatureData> mChartData = new ArrayList<FeatureData>();

    public class FeatureData {
        TextView chartName;
        CombinedChart chartView;
        long startTime = 0;
        String featureId = "";
        ArrayList<BLEFeature> dataList = new ArrayList<BLEFeature>();
        CombinedData combinedData;
        LineData lineData;
        BarData barData;
        float maxValue = 0f;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mView = inflater.inflate(R.layout.fragment_history, container, false);
        mLayoutHistory = (LinearLayout) mView.findViewById(R.id.layout_history);

        ReadDataActivity activity = (ReadDataActivity) getActivity();
        Object[] keys = activity.mFeatureMap.keySet().toArray();
        for (int i = 0; i < keys.length; i++) {
            addChart((String) keys[i]);
        }

        return mView;
    }

    public FeatureData addChart(String id) {
        FeatureData featureData = new FeatureData();
        mChartData.add(featureData);

        View view = getActivity().getLayoutInflater().inflate(R.layout.layout_feature_chart, null);
        mLayoutHistory.addView(view);
        featureData.chartName = (TextView) view.findViewById(R.id.txt_name);
        featureData.chartView = (CombinedChart) view.findViewById(R.id.chart);
        featureData.featureId = id;
        featureData.startTime = AppData.getFeatureStartTime(getActivity(), id);

        featureData.chartName.setText("Feature ID : " + id);

        featureData.chartView.setDescription("");
        featureData.chartView.setBackgroundColor(Color.WHITE);
        featureData.chartView.setDrawGridBackground(false);
        featureData.chartView.setDrawBarShadow(false);

        // draw bars behind lines
        featureData.chartView.setDrawOrder(new CombinedChart.DrawOrder[]{
                CombinedChart.DrawOrder.BAR, CombinedChart.DrawOrder.LINE
        });

        YAxis rightAxis = featureData.chartView.getAxisRight();
        rightAxis.setDrawGridLines(false);
        rightAxis.setAxisMinValue(0f); // this replaces setStartAtZero(true)

        YAxis leftAxis = featureData.chartView.getAxisLeft();
        leftAxis.setDrawGridLines(false);
        leftAxis.setAxisMinValue(0f); // this replaces setStartAtZero(true)

        XAxis xAxis = featureData.chartView.getXAxis();
        xAxis.setPosition(XAxis.XAxisPosition.BOTH_SIDED);

        ReadDataActivity activity = (ReadDataActivity) getActivity();
        refreshFeatureData(id, featureData, activity);

        featureData.chartView.getLegend().setEnabled(false);

        return featureData;
    }

    public void refreshFeatureData(String id, FeatureData featureData, ReadDataActivity activity) {
        ArrayList<BLEFeature> featureList = activity.mFeatureMap.get(id);
        ArrayList<String> timeList = new ArrayList<>();
        long time = 0;

        timeList.add("0s");

        for (int i = 0; i < featureList.size(); i++) {
            time += featureList.get(i).getDuration();

            if (time < 60000)
                timeList.add((time / 1000) + "s");
            else if (time < 3600000)
                timeList.add((time / 60000) + "m" + ((time % 60000) / 1000) + "s");
            else
                timeList.add((time / 3600000) + "h" + ((time % 3600000) / 60000) + "m");

            if (featureData.maxValue < featureList.get(i).getFeatureValue())
                featureData.maxValue = featureList.get(i).getFeatureValue();
        }

        featureData.combinedData = new CombinedData(timeList);
        featureData.lineData = generateLineData(id, featureList);
        featureData.barData = generateBarData(id, featureList, featureData.maxValue);

        featureData.combinedData.setData(featureData.lineData);
        //featureData.combinedData.setData(featureData.barData);

        featureData.chartView.setData(featureData.combinedData);
        featureData.chartView.invalidate();
    }

    @Override
    public void readData(int status, String id, float value, long readTime, boolean isNew) {
        if (getActivity() == null)
            return;

        ReadDataActivity activity = (ReadDataActivity) getActivity();
        FeatureData featureData = null;

        for (int i = 0; i < mChartData.size(); i++) {
            if (TextUtils.equals(mChartData.get(i).featureId, id)) {
                featureData = mChartData.get(i);
                break;
            }
        }

        if (featureData == null) {
            addChart(id);
        }
        else {
            refreshFeatureData(id, featureData, activity);
        }

    }

    private LineData generateLineData(String id, ArrayList<BLEFeature> featureList) {

        LineData d = new LineData();

        ArrayList<Entry> entries = new ArrayList<Entry>();
        int[] colors = new int[featureList.size()];

//        if (featureList.size() > 0)
//            entries.add(new Entry(featureList.get(0).getFeatureValue(), 0));
        entries.add(new Entry(0f, 0));

        for (int index = 0; index < featureList.size(); index++) {
            BLEFeature feature = featureList.get(index);
            entries.add(new Entry(feature.getFeatureValue(), index+1));
            if (feature.getStatus() == STATUS_IDLE)
                colors[index] = STATUS_IDLE_COLOR;
            else if (feature.getStatus() == STATUS_NORMAL_CUTTING)
                colors[index] = STATUS_NORMAL_CUTTING_COLOR;
            else if (feature.getStatus() == STATUS_WARNING)
                colors[index] = STATUS_WARNING_COLOR;
            else if (feature.getStatus() == STATUS_DANGER)
                colors[index] = STATUS_DANGER_COLOR;
        }

        LineDataSet set = new LineDataSet(entries, id);
        set.setColors(colors);
        set.setLineWidth(1.5f);
        set.setCircleColor(Color.rgb(0, 0, 0));
        set.setCircleRadius(2.5f);
        set.setFillColor(Color.rgb(240, 238, 70));
        set.setDrawCubic(false);
        set.setDrawValues(true);
        set.setValueTextSize(10f);
        set.setValueTextColor(Color.rgb(0, 0, 0));

        set.setAxisDependency(YAxis.AxisDependency.LEFT);

        d.addDataSet(set);

        return d;
    }

    private BarData generateBarData(String id, ArrayList<BLEFeature> featureList, float maxValue) {

        BarData d = new BarData();

        ArrayList<BarEntry> entries = new ArrayList<BarEntry>();

        int[] colors = new int[featureList.size()];

        for (int index = 0; index < featureList.size(); index++) {
            BLEFeature feature = featureList.get(index);
            entries.add(new BarEntry(maxValue, index));

            if (feature.getStatus() == STATUS_IDLE)
                colors[index] = STATUS_IDLE_COLOR & 0x3FFFFFFF;
            else if (feature.getStatus() == STATUS_NORMAL_CUTTING)
                colors[index] = STATUS_NORMAL_CUTTING_COLOR & 0x3FFFFFFF;
            else if (feature.getStatus() == STATUS_WARNING)
                colors[index] = STATUS_WARNING_COLOR & 0x3FFFFFFF;
            else if (feature.getStatus() == STATUS_DANGER)
                colors[index] = STATUS_DANGER_COLOR & 0x3FFFFFFF;
        }

        BarDataSet set = new BarDataSet(entries, id);
        set.setColors(colors);
        set.setDrawValues(false);
        d.addDataSet(set);

        set.setAxisDependency(YAxis.AxisDependency.LEFT);

        return d;
    }

}