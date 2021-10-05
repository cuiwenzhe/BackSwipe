package com.example.simplegestureinput;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

public class MultiTouchTraceView extends View {
    private enum STATE {GESTURE, SELECTION};
    private static final int screenWidth = Resources.getSystem().getDisplayMetrics().widthPixels;
    private static final String TAG = MultiTouchTraceView.class.getSimpleName();
    private static final int VELOCITY_UNIT = 10;
    private List<FingerTrace> allFingerTraces = new ArrayList<>();

    private Paint mPaint = new Paint();
    private MultiGestureInterface mMultiGestureInterface;
    private STATE state = STATE.GESTURE;
    private VelocityTracker vTracker;
    private long LastUpdatedTime;
    private Timer gestureTimer;

    public MultiTouchTraceView(Context context) {
        super(context);
        init();
    }


    public MultiTouchTraceView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public void setGestureState(){
        this.state = STATE.GESTURE;
    }

    public void setSelectionState(){
        this.state = STATE.SELECTION;
    }

    private void init() {
        mPaint.setAntiAlias(true);
        mPaint.setStyle(Paint.Style.STROKE);
        mPaint.setStrokeCap(Paint.Cap.ROUND);
        mPaint.setStrokeWidth(10);


    }
    /***
     * Set Timer to check gesture status
     */
    private void beginHearingGestureMovement(){

        LastUpdatedTime = System.currentTimeMillis();
        gestureTimer = new Timer();
        TimerTask task = new TimerTask() {
            @Override
            public void run() {
                if(System.currentTimeMillis() - LastUpdatedTime > 300){
                    mMultiGestureInterface.onGestureStopped(true);
                    gestureTimer.cancel();
                    Log.i(TAG, "GESTURE_STOPED");
                }
            }
        };
        gestureTimer.schedule(task,0,10);

    }

    /***
     * User has to stop timer after each gesture
     */
    private void stopHearingGestureMovement(){
        gestureTimer.cancel();
        Log.i(TAG, "STOP HEARING GESTURE STOP");
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        //printTraces();
        LastUpdatedTime = System.currentTimeMillis();
        switch (event.getActionMasked()){
            case MotionEvent.ACTION_DOWN:
                if(vTracker == null) {
                    // Retrieve a new VelocityTracker object to watch the
                    // velocity of a motion.
                    vTracker = VelocityTracker.obtain();
                }
                else {
                    // Reset the velocity tracker back to its initial state.
                    vTracker.clear();
                }
                mMultiGestureInterface.onGestureStart();
                beginHearingGestureMovement();
                addFingerTrace(event);
                break;
            case MotionEvent.ACTION_POINTER_DOWN:
                vTracker.addMovement(event);
                vTracker.computeCurrentVelocity(VELOCITY_UNIT);
                addFingerTrace(event);
                break;
            case MotionEvent.ACTION_UP:
                stopHearingGestureMovement();
                List<PointT> validPoints = getValidPointList();
                if(validPoints!=null && mMultiGestureInterface != null){
                    if(state == STATE.GESTURE){
                        mMultiGestureInterface.onGestureFinished(validPoints);
                    }
                    else if(state == STATE.SELECTION){
                        int degree = getValidDirection();
                        int relativeDistance = getRelativeDistance();
                        mMultiGestureInterface.onHearingDirection(degree, relativeDistance);
                        mMultiGestureInterface.onDirectionConfirmed(degree, relativeDistance);
                    }
                }
                allFingerTraces.clear();
                vTracker.recycle();
                vTracker = null;
                break;
            case MotionEvent.ACTION_MOVE:
                vTracker.addMovement(event);
                vTracker.computeCurrentVelocity(VELOCITY_UNIT);
                updateAllFingerTraceOnMove(event);
                if(state == STATE.SELECTION){
                    int degree = getValidDirection();
                    int relativeDistance = getRelativeDistance();
                    mMultiGestureInterface.onHearingDirection(degree, relativeDistance);

                }
                break;
            case MotionEvent.ACTION_POINTER_UP:
                removeFingerTrace(event);
                //TODO clear fingers' path information
                break;
            case MotionEvent.ACTION_CANCEL:
                vTracker.recycle();
                vTracker = null;
            default:break;
        }
        invalidate();
        return true;
    }
    public void setMultiGestureInterface(MultiGestureInterface mInterface){
        mMultiGestureInterface = mInterface;
    }

    /**
     * get the direction from the first point of valid trace to the last point of the valid trace
     * @return degree of the direction from 0 to 360 degree, return -2 if the result is invalid,
     * return -2 if the valid gesture is to short
     */
    private int getValidDirection(){
        if(allFingerTraces == null || allFingerTraces.isEmpty()){
            return -2;
        }
        for(FingerTrace t:allFingerTraces){
            if(t.isValid){
                PointT firstPoint = t.tracePoints.get(0);
                PointT curPoint = t.tracePoints.get(t.tracePoints.size() - 1 );

                double x_diff = curPoint.x - firstPoint.x;
                double y_diff = curPoint.y - firstPoint.y;
                if(isDirectionClick(x_diff, y_diff)){
                    return -1;
                }
                float angle = (float) Math.toDegrees(Math.atan2(-y_diff, x_diff));
                if(angle < 0){
                    angle += 360;
                }
                return (int)angle;
            }
        }
        return -2;
    }
    /**
     * get the relative x distance from the first point of valid trace to the last point of the valid trace
     * @return positive distance indicates that the cursor moving to the right, negative indicates the
     * cursor moving to the right
     * return -2 if the valid gesture is to short
     */
    private int getRelativeDistance(){
        if(allFingerTraces == null || allFingerTraces.isEmpty()){
            return -2;
        }
        for(FingerTrace t:allFingerTraces){
            if(t.isValid){
                PointT firstPoint = t.tracePoints.get(0);
                PointT curPoint = t.tracePoints.get(t.tracePoints.size() - 1 );

                int screenWidth = Resources.getSystem().getDisplayMetrics().widthPixels;
                double x_diff = curPoint.x - firstPoint.x;
                double y_diff = curPoint.y - firstPoint.y;
                if(isDirectionClick(x_diff, y_diff)){
                    return -1;
                }
                return (int)(x_diff * 1080 / screenWidth);
            }
        }
        return -2;
    }

    private boolean isDirectionClick(double x_diff, double y_diff){
        int THRESHOLD = 10;
        double distance = Math.hypot(x_diff, y_diff);
        return distance < THRESHOLD;
    }
    private List<PointT> getValidPointList(){
        if(allFingerTraces == null || allFingerTraces.isEmpty()){
            return null;
        }
        for(FingerTrace t:allFingerTraces){
            if(t.isValid){
                return new ArrayList<>(t.tracePoints);
            }
        }
        return null;
    }
    private void updateAllFingerTraceOnMove(MotionEvent event){
        int pointerCount = event.getPointerCount();

        for(int i = 0; i < pointerCount; i++){
            int pointerID = event.getPointerId(i);
            for(FingerTrace trace : allFingerTraces){
                if(trace.pointerID == pointerID){
                    int pointerIndex = event.findPointerIndex(pointerID);
                    trace.tracePoints.addAll(getEventPointsByPointerIndex(pointerIndex, event));
                }
                if(trace.isValid){
                    if(isGestureStop(trace)){
                        //mMultiGestureInterface.onGestureStopped(true);
                    }
                }
            }
        }
    }
    private List<PointT> getEventPointsByPointerIndex(int pointerIndex, MotionEvent event){
        List<PointT> allPoints = new ArrayList<>();
        int historySize = event.getHistorySize();
        float velocity = getVelocity(pointerIndex);
        for(int i=0; i < historySize; i++){
            allPoints.add(new PointT(screenWidth - event.getHistoricalX(pointerIndex, i),
                                        event.getHistoricalY(pointerIndex, i),
                                        event.getHistoricalEventTime(i),
                                        event.getHistoricalOrientation(pointerIndex, i), velocity));
        }
        allPoints.add(new PointT(screenWidth - event.getX(pointerIndex),
                                    event.getY(pointerIndex),
                                    event.getEventTime(),
                                    event.getOrientation(pointerIndex), velocity));
        return allPoints;
    }
    private void removeFingerTrace(MotionEvent event){
        int pointerID = event.getPointerId(event.getActionIndex());
        for(FingerTrace trace: allFingerTraces){
            if(trace.pointerID == pointerID){
                trace.pointerID = -1;
            }
        }
    }
    private void addFingerTrace(MotionEvent event){
        int pointerID = event.getPointerId(event.getActionIndex());
        float x = screenWidth - event.getX(event.findPointerIndex(pointerID));
        float y = event.getY(event.findPointerIndex(pointerID));
        long time = event.getEventTime();
        float orientation = event.getOrientation();
        float velocity = getVelocity(pointerID);

        allFingerTraces.add(new FingerTrace(pointerID, x, y, (int)time, orientation, velocity));
    }

    private float getVelocity(int pointerID){
        vTracker.computeCurrentVelocity(VELOCITY_UNIT);
        float xVelocity = vTracker.getXVelocity(pointerID);
        float yVelocity = vTracker.getYVelocity(pointerID);
        double velocity = Math.hypot(xVelocity, yVelocity);

        return (float)velocity;
    }
    private boolean isGestureStop(FingerTrace trace){
        int TIME_THRESHOLD = 300;
        int SPEED_THRESHOLD = 1;
        long endTime = trace.tracePoints.get(trace.tracePoints.size()-1).timestamp;

        for(int i = trace.tracePoints.size() - 1; i>=0; i--){
            if(trace.tracePoints.get(i).velocity > SPEED_THRESHOLD){
                return false;
            }
            if(endTime - trace.tracePoints.get(i).timestamp > TIME_THRESHOLD){
                return true;
            }
        }
        return false;
    }
    void printTraces(){
        if(!allFingerTraces.isEmpty()){
            for(FingerTrace trace:allFingerTraces){
                Log.i(TAG, trace.toString());
            }
        }
    }

    private Path getPathFromList(List<PointT> points){
        Path path = new Path();
        if(!points.isEmpty()){
            PointT startPoint = points.get(0);
                path.moveTo(startPoint.x, startPoint.y);
            for(PointT p:points){
                path.lineTo(p.x, p.y);
            }
        }
        return path;
    }
    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (allFingerTraces == null || allFingerTraces.isEmpty()) {
            return;
        }
        for(FingerTrace trace: allFingerTraces){
            if(trace.pointerID != -1){
                Path path = getPathFromList(trace.tracePoints);
                mPaint.setColor(trace.color);
                canvas.drawPath(path, mPaint);
            }
        }
    }
    public static class PointT{
        float x;
        float y;
        long timestamp;
        float orientation;
        float velocity;
        PointT(float _x, float _y, long _timestamp, float _orientation, float _velocity){
            x = _x;
            y = _y;
            timestamp = _timestamp;
            orientation = _orientation;
            velocity = _velocity;
        }
    }

    private static class FingerTrace{
        private int pointerID = -1;
        private boolean isValid = false;
        private int color = 0xffFBBC05;
        List<PointT> tracePoints;
        FingerTrace(int PID, float x, float y, long time, float orientation, float velocity){
            pointerID = PID;
            tracePoints = new ArrayList<>();
            tracePoints.add(new PointT(x, y, time, orientation, velocity));
            isValid = isPointValid(x, y);
            if(isValid){
                color = 0xff18BD8D;
            }
        }

        private boolean isPointValid(float x, float y){
            int screenWidth = Resources.getSystem().getDisplayMetrics().widthPixels;
            int gap = 50;
            if(x < gap || x > screenWidth - gap){
                return false;
            }
            return true;
        }

        @NonNull
        @Override
        public String toString() {
            int length = tracePoints.size();
            if(length == 0)
                return "";
            PointT lastPoint = tracePoints.get(tracePoints.size()-1);
            return "Finger:" + String.valueOf(pointerID) + " Size:" + String.valueOf(length) + " At:("
                    + String.valueOf(lastPoint.x) + " ," + String.valueOf(lastPoint.y) + ")";
        }
    }

}
