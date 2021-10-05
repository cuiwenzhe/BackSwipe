package com.example.simplegestureinput.Data;

import android.os.Environment;
import android.util.Log;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

public class TestLogger {
    private static final String TAG = TestLogger.class.getSimpleName();
    private static final String firstLine = "target,input,command,operation,timestamp";
    // TODO(suwen): Change target directory name.
    private static final String INPUT_TEST_DIR = "BackScreenInputTest";
    private static final String WARM_UP_DIR = "BackScreenWarmUp";

    private PrintWriter mLogWriter;
    private List<String> logList = new ArrayList<>();

    private String userID;
    private String testType;


    public TestLogger(final String _userID, final String _testType) {
        this.userID = _userID;
        this.testType = _testType;
        String path = "";
        if(_testType.toLowerCase().contains("warmup")){
            path = WARM_UP_DIR;
        }else if(_testType.toLowerCase().contains("test")){
            path = INPUT_TEST_DIR;
        }
        final File dir =
                new File(Environment.getExternalStorageDirectory()
                        + File.separator + path);
        dir.mkdirs();

        SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMdd_HHmmss");
        String currentDateandTime = sdf.format(new Date());
        final String loggerFileName = userID + "_" + testType + "_" + currentDateandTime + ".csv";
        final File logFile = new File(dir, loggerFileName);
        Log.d(TAG, "logFile=" + logFile.getPath());
        try {
            if(!_testType.toLowerCase().contains("demo")){
                mLogWriter = new PrintWriter(
                        new BufferedWriter(new FileWriter(logFile)));
                logString(firstLine);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void logString(final String stringToLog) {
        if (mLogWriter == null)
            return;
        mLogWriter.println(stringToLog);
        mLogWriter.flush();
        logList.add(stringToLog);

        Log.i("LOG_STRING",stringToLog);
    }

    public void closeLogFile() {
        if (mLogWriter == null)
            return;
        logList.clear();
        mLogWriter.flush();
        mLogWriter.close();

    }

}
