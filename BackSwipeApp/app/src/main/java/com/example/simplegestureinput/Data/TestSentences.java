package com.example.simplegestureinput.Data;

import android.content.Context;

import com.example.simplegestureinput.R;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Random;
import java.util.concurrent.ThreadLocalRandom;



public class TestSentences {
    private ArrayList<String> sentencePairs = new ArrayList<>();
    private Context mContext;
    private int currentIndex = -1;
    public TestSentences(Context context, boolean isDemo){
        this.mContext = context;
        int resource = isDemo ? R.raw.sentence_demo : R.raw.sentence_test;
        loadTestSentenceByResource(resource);
    }

    public TestSentences(Context context, int resource){
        this.mContext = context;
        loadTestSentenceByResource(resource);
    }
    private void loadTestSentenceByResource(int source){

        try (BufferedReader br = new BufferedReader(
                new InputStreamReader(mContext.getResources().openRawResource(source))
            )
        ) {
            String line;
            while ((line = br.readLine()) != null) {
                String rawSentence = line.toLowerCase();
                sentencePairs.add(rawSentence);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        Collections.shuffle(sentencePairs, new Random());
    }

    public String getRandomSentence(){
        int length = sentencePairs.size();
        if(length!=0){
            int randomNum = ThreadLocalRandom.current().nextInt(0, length);
            return sentencePairs.get(randomNum);
        }
        return "";
    }
    public String getNextSentence(){
        currentIndex++;
        return getCurrentSentence();
    }
    public String getCurrentSentence(){
        String sentence = null;
        if(currentIndex < sentencePairs.size()) {
            sentence = sentencePairs.get(currentIndex);
        }

        return sentence;
    }
    public int getCurrentIndex(){
        return currentIndex;
    }
    public int getSentenceSize(){
        return sentencePairs.size();
    }
}
