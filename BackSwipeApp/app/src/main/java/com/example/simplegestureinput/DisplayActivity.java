package com.example.simplegestureinput;

import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;

import android.graphics.Color;
import android.hardware.Camera;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.example.simplegestureinput.Presenter.TestCommandPresenter;
import com.example.simplegestureinput.Presenter.TestLogPresenter;
import com.example.simplegestureinput.util.GridMenu;
import com.example.simplegestureinput.util.StringMethod;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class DisplayActivity extends AppCompatActivity implements MultiGestureInterface, NetWorkResponseInterface,View.OnTouchListener {
    final static String TAG = DisplayActivity.class.getSimpleName();
    FrameLayout mainFrameLayout;
    MultiTouchTraceView gestureView;
    NetWorkHelper netHelper;
    TextView targetText;
    TextView inputText;
    TextView infoText;
    GridMenu gridMenu;
    GridMenu4 gridMenuLess;
    ImageView keyboardImage;
    List<String> tempResult;
    List<Double> tempScores;
    String decodeMode;

    Handler handler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_display);
        View decorView = getWindow().getDecorView();
        // Hide the status bar.
        int uiOptions = View.SYSTEM_UI_FLAG_FULLSCREEN;
        decorView.setSystemUiVisibility(uiOptions);

        mainFrameLayout = findViewById(R.id.main_frame_layout);
        // Example of a call to a native method

        targetText = findViewById(R.id.target_command);
        inputText = findViewById(R.id.input_command);
        infoText = findViewById(R.id.message_text);
        gestureView = findViewById(R.id.gesture_view);
        gestureView.setMultiGestureInterface(this);
        keyboardImage = findViewById(R.id.keyboard_view);


        netHelper = new NetWorkHelper(this, this);
        gridMenuLess = new GridMenu4(this, findViewById(R.id.suggestion_framelayout));

        gridMenu = gridMenuLess;
        tempResult = new ArrayList<String>();
        tempScores = new ArrayList<Double>();

        handler = new Handler();

        Intent intent = getIntent();
        Bundle bundle = intent.getBundleExtra(getString(R.string.menu_bundle));
        if(bundle!=null){
            String userID = bundle.getString(getString(R.string.user_id));
            String testType = bundle.getString(getString(R.string.test_type));
            String serverHost = bundle.getString(getString(R.string.server_host));
            int serverPort = bundle.getInt(getString(R.string.server_port));
            netHelper.setHostPort(serverHost, serverPort);
            initTest(userID, testType);
        }

    }

    private void initTest(String userID, String testType){
        targetText.setText("");
        decodeMode = "CMD_DECODE";
    }

    /****
     * pack word point and previous word into JSON format
     */
    private byte[] getDecodeJSONBytes(int[] xPoints, int[] yPoints,
                                      long[] timeStamps, float[] orientations, float[] velocities,
                                      String prevWord, String preText, String bannedWords, String task){
        byte[] sendBytes = {};
        JSONObject obj = new JSONObject();
        try {
            obj.put("TASK",task);
            obj.put("XPOINTS", Arrays.toString(xPoints));
            obj.put("YPOINTS", Arrays.toString(yPoints));
            obj.put("TIMESTAMPS", Arrays.toString(timeStamps));
            obj.put("ORIENTATIONS", Arrays.toString(orientations));
            obj.put("VELOCITIES", Arrays.toString(velocities));
            obj.put("PREV_TEXT", preText);
            obj.put("PREV_WORD", prevWord);
            obj.put("PORT", "9528");
            obj.put("UNDO_WORDS", bannedWords);
            sendBytes = obj.toString().getBytes("utf-8");

        } catch (JSONException | UnsupportedEncodingException e) {
            e.printStackTrace();
        }

        return sendBytes;
    }

    private byte[] getWordJsonBytes(String task, String selectedWord){
        byte[] sendBytes = {};
        JSONObject obj = new JSONObject();
        try {
            obj.put("TASK", task);
            obj.put("CUR_WORD", selectedWord);
            sendBytes = obj.toString().getBytes("utf-8");
        } catch (JSONException | UnsupportedEncodingException e) {
            e.printStackTrace();
        }

        return sendBytes;
    }

    /***
     * Handle gesture input on backscreen
     * send the point information to decoder
     * @param validPoints
     */
    @Override
    public void onGestureFinished(List<MultiTouchTraceView.PointT> validPoints) {
        if(validPoints.size() == 1){
            Log.i(TAG,"invalid one point");
            return;
        }
        int[] xPoints = new int[validPoints.size()];
        int[] yPoints = new int[validPoints.size()];
        long[] timeStamps = new long[validPoints.size()];
        float[] orintations = new float[validPoints.size()];
        float[] velocities = new float[validPoints.size()];
        int size = validPoints.size();

        for (int i = 0; i < size; i++) {
            xPoints[i] = (int) validPoints.get(i).x;
            yPoints[i] = (int) validPoints.get(i).y;
            timeStamps[i] = validPoints.get(i).timestamp;
            orintations[i] = validPoints.get(i).orientation;
            velocities[i] = validPoints.get(i).velocity;
        }

        //

        byte[] array = getDecodeJSONBytes(xPoints, yPoints, timeStamps, orintations, velocities,
                StringMethod.getPrevWordByText(""),
                StringMethod.getPrevTextByText(""),
                "",decodeMode);

        netHelper.sendGesture(array);
        //DecoderResults res = decoderTools.decodeGesture(xPoints, yPoints, tPoints, validPoints.size(), prevWord.getBytes(UTF_8));

        keyboardImage.setVisibility(View.INVISIBLE);
    }

    @Override
    public void onGestureStart() {
    }

    private void onInputFinished(String input){
        String target = targetText.getText().toString().toLowerCase().trim();
        String text = input.toLowerCase().trim();
        inputText.setText(input);
        Handler handler = new Handler();
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                triggerFunction(text);
            }
        },1000);



    }
    private void triggerFunction(String text){
        switch (text) {
            case "torch": {
                CameraManager camManager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
                String cameraId = null;
                try {
                    cameraId = camManager.getCameraIdList()[0];
                    camManager.setTorchMode(cameraId, true);   //Turn ON
                } catch (CameraAccessException e) {
                    e.printStackTrace();
                }
                break;
            }
            case "email":
                try {
                    Intent intent = getPackageManager().getLaunchIntentForPackage("com.google.android.gm");
                    startActivity(intent);
                } catch (ActivityNotFoundException e) {
                    Toast.makeText(this, e.toString(), Toast.LENGTH_SHORT).show();
                }

                break;
            case "photo": {
                try {
                    Intent intent = getPackageManager().getLaunchIntentForPackage("com.google.android.apps.photos");
                    startActivity(intent);
                } catch (ActivityNotFoundException e) {
                    Toast.makeText(this, e.toString(), Toast.LENGTH_SHORT).show();
                }
                break;
            }
            case "date": {
                try {
                    Intent intent = getPackageManager().getLaunchIntentForPackage("com.android.calendar");
                    startActivity(intent);
                } catch (ActivityNotFoundException e) {
                    Toast.makeText(this, e.toString(), Toast.LENGTH_SHORT).show();
                }
                break;
            }
            case "camera": {
                try {
                    Intent intent = getPackageManager().getLaunchIntentForPackage("com.zte.camera");
                    startActivity(intent);
                } catch (ActivityNotFoundException e) {
                    Toast.makeText(this, e.toString(), Toast.LENGTH_SHORT).show();
                }
                break;
            }
            case "message": {
                try {
                    Intent intent = getPackageManager().getLaunchIntentForPackage("com.android.messaging");
                    startActivity(intent);
                } catch (ActivityNotFoundException e) {
                    Toast.makeText(this, e.toString(), Toast.LENGTH_SHORT).show();
                }
                break;
            }
            case "": {
                CameraManager camManager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
                String cameraId = null;
                try {
                    cameraId = camManager.getCameraIdList()[0];
                    camManager.setTorchMode(cameraId, false);   //Turn ON
                } catch (CameraAccessException e) {
                    e.printStackTrace();
                }
                break;
            }
        }
        inputText.setText("");
    }

    /***
     * Hearing gesture direction on word selection
     * after receiving decoding result
     * @param degree
     */
    @Override
    public void onHearingDirection(int degree, int relativeDistance) {
        gridMenu.highlightSuggestionByDegree(degree, relativeDistance);
    }
    @Override
    public void onDirectionConfirmed(int degree, int relativeDistance) {
        gridMenu.highlightSuggestionByDegree(degree, relativeDistance);
        String selectedWord = gridMenu.getHighLightWord();
        Log.i(TAG, selectedWord);

        inputText.setText(selectedWord);
        gridMenu.clear();
        gestureView.setGestureState();
        netHelper.sendTaskMessage(getWordJsonBytes("CONFIRM",selectedWord));
        tempResult = new ArrayList<>();
        tempScores = new ArrayList<>();
        onInputFinished(selectedWord);
    }

    @Override
    public void onGestureStopped(boolean isStop) {
        if(isStop){
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
//                    if(keyboardImage.getVisibility() != View.VISIBLE)
//                        keyboardImage.setVisibility(View.VISIBLE);
                }
            });
        }
    }

    /***
     * append the first word in the sorted list to the end of editing text,
     * and put this word in the center of selection menu
     * then using the rest 8 words fill up the other 8 grid in the menu starting
     * from the left of the center grid anticlockwise
     * @param words
     */
    @Override
    public void setDecodingResults(List<String> words, List<Double> scores) {
        int length = Math.min(gridMenu.getResultNumber(),words.size());
        tempResult = words.subList(0,length);
        tempScores = scores.subList(0,length);

        Log.i(TAG, Arrays.toString(words.toArray()));
        Log.i(TAG, Arrays.toString(scores.toArray()));

        String topWord = words.get(0);

        inputText.setText(topWord);

        onInputFinished(topWord);
//        if(getScoreConfidence(scores) == 0){
//            onInputFinished(topWord);
//        }else{
//            gridMenu.show(words);
//            gestureView.setSelectionState();
//        }
    }

    /***
     * decide whether show suggestions by calculating the differences amoung the top 4 word scores
     *
     * @param scores
     * @return default value is 0 (no suggestion), return 1 to show less suggestions,
     * return 4 to show more suggestions
     */
    private int getScoreConfidence(List<Double> scores){
        int VALID_NUM = 2;
        double diff = 0;
        double sum = scores.get(0);
        if(scores.size() < 2){
            return 0;
        }
        for(int i = 1; i<VALID_NUM; i++){
            diff += scores.get(i) - scores.get(i-1);
            sum += scores.get(i);
        }
        double score = -(diff/VALID_NUM)/(sum/VALID_NUM);
        Log.i(TAG, String.valueOf(score));
        return score < 0.06 ? 4 : 0;
    }

    @Override
    protected void onResume(){
        super.onResume();
        View decorView = getWindow().getDecorView();
        // Hide the status bar.
        int uiOptions = View.SYSTEM_UI_FLAG_FULLSCREEN;
        decorView.setSystemUiVisibility(uiOptions);
    }
    @Override
    protected void onDestroy() {
        netHelper.finish();
        super.onDestroy();
    }

    @Override
    public void onResponseReceived(String str) { }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        return false;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
            gridMenu.clear();
            gestureView.setGestureState();

            inputText.setText("");
            onInputFinished("");

            return true;
        }
        return super.onKeyDown(keyCode, event);
    }
}
