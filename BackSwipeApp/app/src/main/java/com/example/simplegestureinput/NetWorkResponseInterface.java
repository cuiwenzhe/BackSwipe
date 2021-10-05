package com.example.simplegestureinput;

import java.util.List;

public interface NetWorkResponseInterface {
    void onResponseReceived(String str);
    void setDecodingResults(List<String> list, List<Double> scores);
}
