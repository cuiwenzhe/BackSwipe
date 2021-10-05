package com.example.simplegestureinput.Data;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class CommandResults {
    private ArrayList<String> words;
    private ArrayList<Double> probs;

    public CommandResults() {
        words = new ArrayList<>();
        probs = new ArrayList<>();
    }

    public boolean contains(String str) {
        return words.contains(str);
    }

    public void add(String word, double prob) {
        if (!words.contains(word)) {
            words.add(word);
            probs.add(prob);
        } else {
            int index = words.indexOf(word);
            probs.set(index, probs.get(index) + prob);
        }
    }

    public String getResult(int index) {
        return words.get(index);
    }

    public String getTopResult() {
        if (probs == null || probs.size() == 0) return "";
        return getResult(probs.indexOf(Collections.max(probs)));
    }
    public List<String> getTopNResults(int n) {
        List<String> res = new ArrayList<String>();
        if (probs == null || probs.size() == 0) return null;
        int [] resIndice = indexesOfTopElements(probs, n);
        for(int index: resIndice){
            res.add(getResult(index)    );
        }

        return res;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < words.size(); i++) {
            sb.append(words.get(i)).append(" ").append(probs.get(i)).append(", ");
        }
        return sb.toString();
    }

    private int[] indexesOfTopElements(List<Double> orig, int nummax) {
        Double[] copy = new Double[orig.size()];
        orig.toArray(copy);
        Arrays.sort(copy);
        Double[] honey = Arrays.copyOfRange(copy,copy.length - nummax, copy.length);
        int[] result = new int[nummax];
        int resultPos = 0;
        for(int i = 0; i < orig.size(); i++) {
            double onTrial = orig.get(i);
            int index = Arrays.binarySearch(honey,onTrial);
            if(index < 0) continue;
            result[resultPos++] = i;
        }
        return result;
    }
}