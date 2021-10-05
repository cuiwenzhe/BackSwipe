package com.example.simplegestureinput.common;

import android.util.Pair;

import java.util.ArrayList;
import java.util.List;

public class DecoderResults {
    private List<Pair<String, Float>> results;
    private float resultConfidence;
    public DecoderResults(){
        results = new ArrayList<>();
        resultConfidence = 0;
    }
    public DecoderResults(List<Pair<String,Float>> _results, float _confidence){
        results = _results;
        resultConfidence = _confidence;
    }

    public void setResults(ArrayList<String> strings, ArrayList<Float> scores){
        results.clear();
        int len = Math.min(strings.size(), scores.size());
        for(int i=0; i < len; i++){
            results.add(new Pair<String, Float>(strings.get(i), scores.get(i)));
        }
    }

    public String toString(){
        StringBuilder s = new StringBuilder();
        for(Pair<String, Float> res: results){
            s.append(res.first).append("|").append(String.valueOf(res.second)).append('\n');
        }
        return s.toString();
    }
}
