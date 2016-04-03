package com.idragonit.bleexplorer.view;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

import com.idragonit.bleexplorer.AppData;
import com.idragonit.bleexplorer.IReceiveData;
import com.idragonit.bleexplorer.R;
import com.idragonit.bleexplorer.ReadDataActivity;

public class ThresholdView extends View implements IReceiveData {

    private static final int MIN_VALUE = 50;
    private static final int MAX_VALUE = 1800;

    Paint mPaint;

    int mLastX = 0;
    int mLastY = 0;
    int mSelectedIndex = 0;

    Rect[] mStatusRect = new Rect[4];
    int[] mStatusThreshold = new int[4];
    int[] defaultThreshold = new int[] {100, 750, 1500};
    int[] thresholdColors = new int[] {STATUS_IDLE_COLOR, STATUS_NORMAL_CUTTING_COLOR, STATUS_WARNING_COLOR, STATUS_DANGER_COLOR};
    String[] thresholdNames = new String[] {STATUS_IDLE_NAME, STATUS_NORMAL_CUTTING_NAME, STATUS_WARNING_NAME, STATUS_DANGER_NAME};

    int thresholdPadding = 72;
    int thresholdLine = 4;
    int thresholdRadius = 8;
    int thresholdFontsize = 12;

    boolean isInit = false;

    public ThresholdView(Context context) {
        super(context);
        init();
    }

    public ThresholdView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public ThresholdView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    private void init() {
        thresholdPadding = getResources().getDimensionPixelSize(R.dimen.threshold_padding);
        thresholdLine = getResources().getDimensionPixelSize(R.dimen.threshold_line);
        thresholdRadius = getResources().getDimensionPixelSize(R.dimen.threshold_radius);
        thresholdFontsize = getResources().getDimensionPixelSize(R.dimen.threshold_font_size);

        mPaint = new Paint();
        mPaint.setTextSize(thresholdFontsize);
        mPaint.setTextAlign(Paint.Align.CENTER);

        for (int i = 1; i < 4; i++) {
            mStatusThreshold[i] = AppData.getThresholdValue(getContext(), i, defaultThreshold[i - 1]);
        }

        mStatusThreshold[0] = 0;

        for (int i = 0; i < 4; i++) {
            mStatusRect[i] = new Rect();
        }

        isInit = true;

        calcRects();
    }

    private void calcRects() {
        int width = getWidth();
        int height = getHeight();

        if (width == 0 || height == 0 || !isInit)
            return;

        int top = 0;
        int bottom = 0;
        for (int i = 3; i >= 0; i--) {
            bottom = height - height * mStatusThreshold[i] / MAX_VALUE;
            mStatusRect[i].set(thresholdPadding, top, width - thresholdPadding, bottom);
            top = bottom;
        }
    }

    private void calcValues() {
        int height = getHeight();

        for (int i = 1; i < 4; i++) {
            mStatusThreshold[i] = (height - mStatusRect[i].bottom) * MAX_VALUE / height;
            if (mStatusThreshold[i] - mStatusThreshold[i - 1] < MIN_VALUE)
                mStatusThreshold[i] = mStatusThreshold[i - 1] + MIN_VALUE;
        }
    }

    public void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);

        calcRects();
    }

    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        int width = getWidth();

        for (int i = 0; i < mStatusRect.length; i++) {
            mPaint.setStyle(Paint.Style.FILL_AND_STROKE);
            mPaint.setColor(thresholdColors[i]);
            mPaint.setAlpha(0xFF);

            Rect rc = new Rect(mStatusRect[i]);

            if (i == 0) {
                rc.top = (int) (rc.top + thresholdLine * 1.5f);
            }
            else if (i == 3) {
                rc.bottom = (int) (rc.bottom - thresholdLine * 1.5f);

                canvas.drawRect(mStatusRect[i].left, mStatusRect[i].bottom - thresholdLine / 2, width - thresholdPadding / 2, mStatusRect[i].bottom + thresholdLine / 2, mPaint);
            }
            else {
                rc.top = (int) (rc.top + thresholdLine * 1.5f);
                rc.bottom = (int) (rc.bottom - thresholdLine * 1.5f);

                canvas.drawRect(mStatusRect[i].left, mStatusRect[i].bottom - thresholdLine / 2, width - thresholdPadding / 2, mStatusRect[i].bottom + thresholdLine / 2, mPaint);
            }

            if (i != 0) {
                canvas.drawCircle(width - thresholdPadding / 2, mStatusRect[i].bottom, thresholdRadius, mPaint);
            }

            mPaint.setStyle(Paint.Style.STROKE);
            mPaint.setStrokeWidth(thresholdLine / 2);
            canvas.drawRect(rc.left + thresholdLine / 4, rc.top + thresholdLine / 4, rc.right - thresholdLine / 4, rc.bottom - thresholdLine / 4, mPaint);

            mPaint.setStrokeWidth(1);
            mPaint.setStyle(Paint.Style.FILL);
            mPaint.setAlpha(0x3F);
            canvas.drawRect(rc.left + thresholdLine / 2, rc.top + thresholdLine / 2, rc.right - thresholdLine / 2, rc.bottom - thresholdLine / 2, mPaint);

            mPaint.setStyle(Paint.Style.FILL_AND_STROKE);
            mPaint.setAlpha(0xFF);
            mPaint.setColor(0xFF000000);

            canvas.drawText(thresholdNames[i], mStatusRect[i].centerX(), mStatusRect[i].centerY() + thresholdFontsize / 2, mPaint);

            if (i != 0) {
                canvas.drawText(String.valueOf(mStatusThreshold[i]), thresholdPadding / 2, mStatusRect[i].bottom + thresholdFontsize / 2, mPaint);
            }
        }
    }

    private int getTouchIndex(int x, int y) {
        for (int i = 1; i < mStatusRect.length; i++) {
            if (new Rect(0, mStatusRect[i].bottom - thresholdRadius, getWidth(), mStatusRect[i].bottom + thresholdRadius).contains(x, y))
                return i;
        }

        return 0;
    }

    public boolean onTouchEvent(MotionEvent event) {
        int x = (int) event.getX();
        int y = (int) event.getY();
        int action = event.getAction();

        switch (action) {
            case MotionEvent.ACTION_DOWN:
                mSelectedIndex = getTouchIndex(x, y);
                mLastX = x;
                mLastY = y;
                break;
            case MotionEvent.ACTION_MOVE:
                if (mSelectedIndex != 0) {
                    calcPercent(x, y);
                }
                break;
            case MotionEvent.ACTION_UP:
                if (mSelectedIndex != 0) {
                    calcPercent(x, y);
                    Log.e("test", mStatusThreshold[1] + ":" + mStatusThreshold[2] + ":" + mStatusThreshold[3]);
                    sendData();
                }

                mSelectedIndex = 0;
                break;
        }

        return true;
    }

    private void calcPercent(int x, int y) {
        int step = y - mLastY;
        int curPos = mStatusRect[mSelectedIndex].bottom + step;
        int downLimit = mStatusRect[mSelectedIndex - 1].bottom - MIN_VALUE * getHeight() / MAX_VALUE;
        int upLimit = mStatusRect[mSelectedIndex].top + MIN_VALUE * getHeight() / MAX_VALUE;

        if (step > 0 && curPos > downLimit) {
            curPos = downLimit;
            mStatusRect[mSelectedIndex - 1].top = curPos;
            mStatusRect[mSelectedIndex].bottom = curPos;
        }
        else if (step <= 0 && curPos <= upLimit) {
            curPos = upLimit;
            mStatusRect[mSelectedIndex - 1].top = curPos;
            mStatusRect[mSelectedIndex].bottom = curPos;
        }
        else {
            mStatusRect[mSelectedIndex - 1].top = curPos;
            mStatusRect[mSelectedIndex].bottom = curPos;

            mLastX = x;
            mLastY = y;
        }

        calcValues();

        invalidate();
    }

    @Override
    public void readData(int status, long readTime) {

    }

    private void sendData() {
        ((ReadDataActivity)getContext()).sendData(mStatusThreshold);
    }
}
