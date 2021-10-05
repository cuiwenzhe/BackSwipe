package com.example.simplegestureinput.util;

import android.util.Log;

import java.util.List;

public interface GridMenu {
    void show(List<String> list);
    void highlightSuggestionByDegree(int degree, int relativeDistance);
    String getHighLightWord();
    void clear();
    boolean isShowing();
    int getResultNumber();
}
