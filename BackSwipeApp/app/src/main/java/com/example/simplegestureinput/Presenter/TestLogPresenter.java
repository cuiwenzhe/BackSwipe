package com.example.simplegestureinput.Presenter;

import android.os.SystemClock;
import android.util.Log;

import com.example.simplegestureinput.Data.TestLogger;


public class TestLogPresenter  {
    private TestLogger testLogger;

    public TestLogPresenter(String userID, String testType){
        setTestLogger(userID,testType);
    }

    public void setTestLogger(String userID, String testType){
        testLogger = new TestLogger(userID,testType);
    }

    public void setLogItem(String target, String input, String command, String _operation){
        //target,input,command,operation,timestamp
        if(testLogger==null)
            return;
        String logStr = "";
        String timestamp = String.valueOf(SystemClock.uptimeMillis());
        String operation = _operation;

        logStr = replaceCommaAndEnter(target) + "," + replaceCommaAndEnter(input) + "," + replaceCommaAndEnter(command) + "," + replaceCommaAndEnter(operation) + "," + timestamp;
        testLogger.logString(logStr);
        Log.i("Logging", logStr);
    }

    public void logEnd(){
        if(testLogger!=null)
            testLogger.closeLogFile();
    }

    private String replaceCommaAndEnter(String str){
        String newStr = "\"" + str + "\"";
        return newStr;
    }
}
