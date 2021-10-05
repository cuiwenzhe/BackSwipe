package com.example.simplegestureinput;

import java.util.List;

public interface MultiGestureInterface {
    void onGestureFinished(List<MultiTouchTraceView.PointT> pointT);
    void onGestureStart();
    void onHearingDirection(int degree, int relativeDistance);
    void onDirectionConfirmed(int degree, int relativeDistance);
    void onGestureStopped(boolean isStop);
}
