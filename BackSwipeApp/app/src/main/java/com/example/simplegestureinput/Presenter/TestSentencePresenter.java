package com.example.simplegestureinput.Presenter;

import android.content.Context;
import android.util.Log;

import com.example.simplegestureinput.Data.TestSentences;
import com.example.simplegestureinput.R;


public class TestSentencePresenter{
    final static private String TAG = TestSentencePresenter.class.getSimpleName();
    private Context context;
    private TestSentences testSentences;
    private final static String MARK_HEAD = "<u>";
    private final static String MARK_TAIL = "</u>";
    public final static String TEST_BREAK_LABEL = "TAKE A BREAK";
    private int testPairCount = 0;



    public TestSentencePresenter(Context _context, String userID, String testType){
        this.context = _context;
        int sentenceResource = R.raw.sentence_demo;
        String decodeType = testType.split("_")[0];
        String sentenceType = testType.split("_")[1];
        if(decodeType.equals("sentence")) {
            if (sentenceType.equals("demo")) {
                sentenceResource = R.raw.sentence_demo;
            } else if (testType.contains("warmup")) {
                sentenceResource = R.raw.sentence_demo;
            } else if (testType.contains("test")) {
                sentenceResource = R.raw.sentence_test;
            }
        }else if(decodeType.equals("command")) {
            if (sentenceType.equals("demo")) {
                sentenceResource = R.raw.dict_80_demo;
            } else if (testType.contains("warmup")) {
                sentenceResource = R.raw.dict_80_demo;
            } else if (testType.contains("test")) {
                sentenceResource = R.raw.dict_80_test;
            }
        }

        Log.i(TAG, String.valueOf(sentenceResource));
        testSentences = new TestSentences(context, sentenceResource);
    }

    public String getNextSentence(){
        testPairCount++;
        return testSentences.getNextSentence();
    }
    public String getCurrentSentence(){
        return testSentences.getCurrentSentence();
    }


    public String getTestStatus(){
        StringBuilder str = new StringBuilder();
        str.append(testSentences.getCurrentIndex()+1);
        str.append('/');
        str.append(testSentences.getSentenceSize());
        return str.toString();
    }
}
