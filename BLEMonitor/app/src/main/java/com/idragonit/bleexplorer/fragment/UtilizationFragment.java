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

public class UtilizationFragment extends Fragment implements IReceiveData {
    /** The main series that will include all the data. */
    private CategorySeries mSeries = new CategorySeries("");
    /** The main renderer for the main dataset. */
    private DefaultRenderer mRenderer = new DefaultRenderer();
    /** The chart view that displays the data. */
    private GraphicalView mChartView;

    private View mView;
    HorizontalBarChart mBarChart;

    ArrayList<BarEntry> mBarEntries = new ArrayList<BarEntry>();
    BarData mBarData;
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

        mView = inflater.inflate(R.layout.fragment_utilization, container, false);

        mBarChart = (HorizontalBarChart) mView.findViewById(R.id.bar_chart);

        mRenderer.setZoomButtonsVisible(false);
        mRenderer.setStartAngle(180);
        mRenderer.setDisplayValues(true);
        mRenderer.setZoomEnabled(false);
        mRenderer.setFitLegend(true);
        mRenderer.setPanEnabled(false);
        mRenderer.setShowLegend(false);

        mRenderer.setLabelsColor(0xFF000000);
        mRenderer.setLabelsTextSize(getActivity().getResources().getDimension(R.dimen.label_size));
        mRenderer.setLegendTextSize(getActivity().getResources().getDimension(R.dimen.label_size));

        return mView;
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mChartView == null) {
            LinearLayout layout = (LinearLayout) mView.findViewById(R.id.chart);
            mChartView = ChartFactory.getPieChartView(getActivity(), mSeries, mRenderer);
            mRenderer.setClickEnabled(false);
            layout.addView(mChartView, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.FILL_PARENT,
                    LinearLayout.LayoutParams.FILL_PARENT));

            mSeries.add(STATUS_IDLE_NAME, 25);
            SimpleSeriesRenderer renderer1 = new SimpleSeriesRenderer();
            renderer1.setColor(STATUS_IDLE_COLOR);
            mRenderer.addSeriesRenderer(renderer1);

            mSeries.add(STATUS_NORMAL_CUTTING_NAME, 25);
            SimpleSeriesRenderer renderer2 = new SimpleSeriesRenderer();
            renderer2.setColor(STATUS_NORMAL_CUTTING_COLOR);
            mRenderer.addSeriesRenderer(renderer2);

            mSeries.add(STATUS_WARNING_NAME, 25);
            SimpleSeriesRenderer renderer3 = new SimpleSeriesRenderer();
            renderer3.setColor(STATUS_WARNING_COLOR);
            mRenderer.addSeriesRenderer(renderer3);

            mSeries.add(STATUS_DANGER_NAME, 25);
            SimpleSeriesRenderer renderer4 = new SimpleSeriesRenderer();
            renderer4.setColor(STATUS_DANGER_COLOR);
            mRenderer.addSeriesRenderer(renderer4);

            mBarEntries.add(new BarEntry(0, 0));
            mBarEntries.add(new BarEntry(0, 1));
            mBarEntries.add(new BarEntry(0, 2));
            mBarEntries.add(new BarEntry(0, 3));

            BarDataSet dataset = new BarDataSet(mBarEntries, null);
            ArrayList<String> labels = new ArrayList<String>();
            labels.add(STATUS_DANGER_NAME);
            labels.add(STATUS_WARNING_NAME);
            labels.add(STATUS_NORMAL_CUTTING_NAME);
            labels.add(STATUS_IDLE_NAME);

            mBarData = new BarData(labels, dataset);
            dataset.setColors(new int[] {STATUS_DANGER_COLOR, STATUS_WARNING_COLOR, STATUS_NORMAL_CUTTING_COLOR, STATUS_IDLE_COLOR});
            mBarChart.setData(mBarData);
            mBarChart.getLegend().setEnabled(false);
            mBarChart.setDescription("");

            readData(0, 0);
        }

        mChartView.repaint();
    }

    @Override
    public void readData(int status, long readTime) {
        ReadDataActivity activity = (ReadDataActivity) getActivity();

        mSeries.set(0, STATUS_IDLE_NAME, activity.getStatusPercent(STATUS_IDLE));
        mSeries.set(1, STATUS_NORMAL_CUTTING_NAME, activity.getStatusPercent(STATUS_NORMAL_CUTTING));
        mSeries.set(2, STATUS_WARNING_NAME, activity.getStatusPercent(STATUS_WARNING));
        mSeries.set(3, STATUS_DANGER_NAME, activity.getStatusPercent(STATUS_DANGER));

        mChartView.repaint();

        mBarEntries.clear();
        mBarEntries.add(new BarEntry(activity.getStatusDuration(STATUS_DANGER) / 1000L, 0));
        mBarEntries.add(new BarEntry(activity.getStatusDuration(STATUS_WARNING) / 1000L, 1));
        mBarEntries.add(new BarEntry(activity.getStatusDuration(STATUS_NORMAL_CUTTING) / 1000L, 2));
        mBarEntries.add(new BarEntry(activity.getStatusDuration(STATUS_IDLE) / 1000L, 3));

        BarDataSet dataset = new BarDataSet(mBarEntries, null);
        ArrayList<String> labels = new ArrayList<String>();
        labels.add(STATUS_DANGER_NAME);
        labels.add(STATUS_WARNING_NAME);
        labels.add(STATUS_NORMAL_CUTTING_NAME);
        labels.add(STATUS_IDLE_NAME);

        mBarData = new BarData(labels, dataset);
        dataset.setColors(new int[]{STATUS_DANGER_COLOR, STATUS_WARNING_COLOR, STATUS_NORMAL_CUTTING_COLOR, STATUS_IDLE_COLOR});
        mBarChart.setData(mBarData);
        mBarChart.invalidate();
    }
}