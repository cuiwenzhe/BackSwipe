package com.example.simplegestureinput;

import android.content.Intent;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.example.simplegestureinput.Presenter.TestLogPresenter;
import com.example.simplegestureinput.Presenter.TestSentencePresenter;
import com.example.simplegestureinput.util.GridMenu;
import com.example.simplegestureinput.util.StringMethod;
import com.squareup.seismic.ShakeDetector;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class OldCommandActivity extends AppCompatActivity implements MultiGestureInterface, NetWorkResponseInterface, ShakeDetector.Listener, View.OnTouchListener {
    final static String TAG = OldCommandActivity.class.getSimpleName();
    FrameLayout mainFrameLayout;
    MultiTouchTraceView gestureView;
    NetWorkHelper netHelper;
    TextView messageText;
    TextView targetText;
    EditText inputText;
    GridMenu gridMenu;
    GridMenu4 gridMenuLess;
    ImageView keyboardImage;
    List<String> tempResult;
    List<Double> tempScores;
    TestLogPresenter logPresenter;
    TestSentencePresenter testSentencePresenter;
    String decodeMode;

    int undoCount = 0;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        View decorView = getWindow().getDecorView();
        // Hide the status bar.
        int uiOptions = View.SYSTEM_UI_FLAG_FULLSCREEN;
        decorView.setSystemUiVisibility(uiOptions);

        mainFrameLayout = findViewById(R.id.main_frame_layout);
        // Example of a call to a native method
        inputText = findViewById(R.id.input_text);
        messageText = findViewById(R.id.message_text);
        targetText = findViewById(R.id.target_text);
        gestureView = findViewById(R.id.gesture_view);
        keyboardImage = findViewById(R.id.keyboard_view);
        gestureView.setMultiGestureInterface(this);
        netHelper = new NetWorkHelper(this, this);
        gridMenuLess = new GridMenu4(this, findViewById(R.id.suggestion_framelayout));

        gridMenu = gridMenuLess;
        tempResult = new ArrayList<String>();
        tempScores = new ArrayList<Double>();

        SensorManager sensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
        ShakeDetector sd = new ShakeDetector(this);
        sd.setSensitivity(11);
        sd.start(sensorManager);

        Intent intent = getIntent();
        Bundle bundle = intent.getBundleExtra(getString(R.string.menu_bundle));
        String userID = bundle.getString(getString(R.string.user_id));
        String testType = bundle.getString(getString(R.string.test_type));
        String serverHost = bundle.getString(getString(R.string.server_host));
        int serverPort = bundle.getInt(getString(R.string.server_port));
        netHelper.setHostPort(serverHost, serverPort);
        initTest(userID, testType);
    }

    private void initTest(String userID, String testType){
        logPresenter = new TestLogPresenter(userID, testType);
        testSentencePresenter = new TestSentencePresenter(this, userID, testType);
        targetText.setText(testSentencePresenter.getNextSentence());
        decodeMode = "CMD_DECODE";
        logPresenter.setLogItem(testSentencePresenter.getCurrentSentence(),
                "",
                "set_target_text",
                "set_target_text");
    }

    /****
     * pack word point and previous word into JSON format
     * @param xPoints
     * @param yPoints
     * @param timeStamps
     * @param orientations
     * @param preText
     * @return
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
                StringMethod.getPrevWordByText(inputText.getText().toString()),
                StringMethod.getPrevTextByText(inputText.getText().toString()),
                Arrays.toString(tempResult.toArray()),decodeMode);

        netHelper.sendGesture(array);
        //DecoderResults res = decoderTools.decodeGesture(xPoints, yPoints, tPoints, validPoints.size(), prevWord.getBytes(UTF_8));

        logPresenter.setLogItem(testSentencePresenter.getCurrentSentence(),
                Arrays.toString(xPoints) + '|' + Arrays.toString(yPoints),
                "gesture_finished",
                "gesture_finished");

        keyboardImage.setVisibility(View.INVISIBLE);
    }

    @Override
    public void onGestureStart() {
        //todo clean display
        if(gridMenu.isShowing()){
            logPresenter.setLogItem(testSentencePresenter.getCurrentSentence(),
                    inputText.getText().toString(),
                    "suggestion_selecting",
                    "suggestion_selecting");
        }
        else {
            logPresenter.setLogItem(testSentencePresenter.getCurrentSentence(),
                    inputText.getText().toString(),
                    "gesture_start",
                    "gesture_start");
        }
    }

    private void onInputFinished(String input){
        undoCount = 0;
        String target = targetText.getText().toString().toLowerCase().trim();
        String text = input.toLowerCase().trim();

        if(target.split(" ").length == text.split(" ").length){
            gridMenu.clear();
            logPresenter.setLogItem(target,
                    input,
                    "target_finished",
                    "target_finished");
            String nextSentence = testSentencePresenter.getNextSentence();
            if(nextSentence == null){
                Toast.makeText(this,"TestEnd", Toast.LENGTH_SHORT).show();
                finish();
            }else{
                targetText.setText(nextSentence);
                inputText.setText("");
                logPresenter.setLogItem(testSentencePresenter.getCurrentSentence(),
                        "",
                        "set_target_text",
                        "set_target_text");
            }
        }
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
        logPresenter.setLogItem(testSentencePresenter.getCurrentSentence(),
                inputText.getText().toString(),
                "suggestion_confirmed",
                selectedWord);
        String resString = replaceLastWordOnInput(selectedWord);
        gridMenu.clear();
        gestureView.setGestureState();
        netHelper.sendTaskMessage(getWordJsonBytes("CONFIRM",selectedWord));
        tempResult = new ArrayList<>();
        tempScores = new ArrayList<>();
        onInputFinished(resString);
    }

    @Override
    public void onGestureStopped(boolean isStop) {
        if(isStop){
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    if(keyboardImage.getVisibility() != View.VISIBLE)
                        keyboardImage.setVisibility(View.VISIBLE);
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

        String str = inputText.getText().toString() +
                words.get(0) + " ";


        inputText.setText(str);
        inputText.setSelection(inputText.length());

        logPresenter.setLogItem(testSentencePresenter.getCurrentSentence(),
                str,
                "decoding_finished",
                Arrays.toString(tempResult.toArray()) + "&" + Arrays.toString(tempScores.toArray()));

        if(getScoreConfidence(scores) == 4){
            gridMenu.show(words);
            gestureView.setSelectionState();
        }else{
            onInputFinished(str);
        }

    }

    /***
     * decide whether show suggestions by calculating the differences amoung the top 4 word scores
     *
     * @param scores
     * @return default value is 0 (no suggestion), return 1 to show less suggestions,
     * return 4 to show more suggestions
     */
    private int getScoreConfidence(List<Double> scores){
        int VALID_NUM = 4;
        double diff = 0;
        double sum = scores.get(0);
        for(int i = 1; i<VALID_NUM; i++){
            diff += scores.get(i) - scores.get(i-1);
            sum += scores.get(i);
        }
        double score = -(diff/VALID_NUM)/(sum/VALID_NUM);
        Log.i(TAG, String.valueOf(score));
        return score < 0.06 ? 4 : 0;
    }

    private String replaceLastWordOnInput(String word){
        String newString = StringMethod.replaceLastWord(inputText.getText().toString(),word).trim() + " ";
        if(newString.equals(" ")){
            newString = "";
        }
        inputText.setText(newString);
        inputText.setSelection(inputText.length());
        return newString;
    }

    /****
     * hearing the shake from user, delete the last word on editing area
     */
    @Override
    public void hearShake() {
        //TODO: clear menu view
    }

    private void autoFill(){
        int input_count = inputText.getText().toString().trim().split(" ").length;
        String[] targets = targetText.getText().toString().split(" ");
        StringBuilder newString = new StringBuilder();
        for(int i = 0; i <input_count+1; i++){
            String s = targets[i];
            newString.append(s);
            newString.append(" ");
        }
        inputText.setText(newString);
        inputText.setSelection(inputText.length());
    }

    @Override
    protected void onResume(){
        super.onResume();
    }
    @Override
    protected void onDestroy() {
        netHelper.finish();
        super.onDestroy();
    }

    @Override
    public void onResponseReceived(String str) {
        if(str == null){
            messageText.setText("Empty MSG");
            return;
        }
        messageText.setText(str);
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        return false;
    }
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
            Toast.makeText(this, "undo input", Toast.LENGTH_SHORT).show();
            String resStr = replaceLastWordOnInput("");
            gridMenu.clear();
            gestureView.setGestureState();
            netHelper.sendTaskMessage(getWordJsonBytes("UNDO",Arrays.toString(tempResult.toArray())));
            undoCount++;
            if(undoCount == 3){
                //  autoFill();
                undoCount = 0;
            }
            logPresenter.setLogItem(testSentencePresenter.getCurrentSentence(),
                    resStr,
                    "undo",
                    Arrays.toString(tempResult.toArray()) + "&" + Arrays.toString(tempScores.toArray()));
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }
}
