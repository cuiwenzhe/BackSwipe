package com.example.simplegestureinput.Data;

import android.content.Context;
import android.util.Log;

import com.example.simplegestureinput.Presenter.TestCommandPresenter;
import com.example.simplegestureinput.R;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Random;
import java.util.concurrent.ThreadLocalRandom;



public class TestCommands {
    private static final String TAG = TestCommands.class.getSimpleName();
    private ArrayList<String> testCommands = new ArrayList<>();
    private Context mContext;
    private int currentIndex = -1;
    private static final ArrayList<Integer> PROBABILITIES =
            new ArrayList<>(Arrays.asList(7, 5, 4, 4, 2, 2, 1, 1, 1, 1, 1, 1));

    public TestCommands(Context context, boolean isDemo, int userID){
        this.mContext = context;
        int resource = isDemo ? R.raw.sentence_demo : R.raw.sentence_test;

        generateZipfTestCommandByID(loadCommandsByResource(resource), userID);
    }

    public TestCommands(Context context, int resource, int userID){
        this.mContext = context;
        generateZipfTestCommandByID(loadCommandsByResource(resource), userID);
    }
    private List<String> loadCommandsByResource(int resource){
        ArrayList<String> cmds = new ArrayList<>();
        try (BufferedReader br = new BufferedReader(
                new InputStreamReader(mContext.getResources().openRawResource(resource))
        )
        ) {
            String line;
            while ((line = br.readLine()) != null) {
                String rawSentence = line.toLowerCase();
                cmds.add(rawSentence);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return cmds;
    }
    private void generateZipfTestCommandByID(List<String> cmdPairs, int userID){
        int size = PROBABILITIES.size();
        int alignIndex = userID % size;
        testCommands.clear();
        for(String word: cmdPairs){
            int repeat = PROBABILITIES.get(alignIndex % size);
            for(int i=0; i<repeat; i++){
                testCommands.add(word);
                Log.i(TAG, repeat + ":" + word);
            }
            alignIndex++;
        }
        Collections.shuffle(testCommands, new Random());
    }

    public String getRandomSentence(){
        int length = testCommands.size();
        if(length!=0){
            int randomNum = ThreadLocalRandom.current().nextInt(0, length);
            return testCommands.get(randomNum);
        }
        return "";
    }
    public String getNextSentence(){
        currentIndex++;
        return getCurrentSentence();
    }
    public String getCurrentSentence(){
        String sentence = null;
        if(currentIndex < testCommands.size()) {
            sentence = testCommands.get(currentIndex);
        }

        return sentence;
    }
    public int getCurrentIndex(){
        return currentIndex;
    }
    public int getSentenceSize(){
        return testCommands.size();
    }
}

