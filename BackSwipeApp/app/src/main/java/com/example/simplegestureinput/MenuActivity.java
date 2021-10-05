package com.example.simplegestureinput;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;

import androidx.core.app.ActivityCompat;

import java.util.List;

public class MenuActivity extends Activity implements NetWorkResponseInterface{
    static final String TAG = MenuActivity.class.getSimpleName();
    RadioGroup radioGroup;
    EditText userID;
    Button testButton;
    Button demoButton;
    Button warmUpButton;
    NetWorkHelper netHelper;
    TextView messageText;
    String serverURL = "";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.menu_activity_layout);
        userID = findViewById(R.id.user_id_edit);
        radioGroup = findViewById(R.id.radio_group);
        testButton = findViewById(R.id.test_button);
        demoButton = findViewById(R.id.demo_button);
        warmUpButton = findViewById(R.id.warm_up_button);
        messageText = findViewById(R.id.menuMessageText);
        netHelper = new NetWorkHelper(this,this);


        checkPermission();


        warmUpButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startTest("warmup");
            }
        });

        demoButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startTest("demo");
            }
        });

        testButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startTest("test");
            }
        });

        netHelper.sniffHost();
    }
    private void startTest(String testType){
        if(serverURL.equals("")){
            Toast.makeText(this,"Waiting for server",Toast.LENGTH_SHORT).show();
            return;
        }
        String test_type = getTestType();
        test_type += "_" + testType;

        String user = userID.getText().toString();
        String Host = serverURL.split(":")[0];
        int Port = Integer.valueOf(serverURL.split(":")[1]);

        Intent intent;
        if(test_type.contains("warmup")){
            intent = new Intent(getApplicationContext(), DisplayActivity.class);
        }
        else if(test_type.contains("command")){
            intent = new Intent(getApplicationContext(), CommandActivity.class);
        }else{
            intent = new Intent(getApplicationContext(), MainActivity.class);
        }


        if(user.length() * test_type.length() == 0){
            Toast.makeText(getApplicationContext(),"Fill in the blank", Toast.LENGTH_SHORT).show();
        }
        else{
            Log.i(TAG,"user: " + user + " Type: " + test_type);
            Bundle bundle = new Bundle();
            bundle.putString(getString(R.string.user_id), user);
            bundle.putString(getString(R.string.test_type), test_type);
            bundle.putString(getString(R.string.server_host), Host);
            bundle.putInt(getString(R.string.server_port), Port);
            intent.putExtra(getString(R.string.menu_bundle) ,bundle);
            startActivity(intent);
        }
    }
    private String getTestType(){
        int selected_type_radio_id = radioGroup.getCheckedRadioButtonId();
        String testType = "";
        if(selected_type_radio_id!=-1){
            switch(selected_type_radio_id){
                case R.id.sentence_test_radio:
                    testType = "sentence";
                    break;

                case R.id.command_test_radio:
                    testType = "command";
                    break;



                default:break;
            }
        }
        return testType;
    }

    private static final int REQUEST_AUDIO_WRITE_EXTERNAL_STORAGE = 100;
    private static final int REQUEST_CAMERA = 200;
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
                                            int[] grantResults) {
        switch (requestCode) {
            case REQUEST_AUDIO_WRITE_EXTERNAL_STORAGE:
                if (grantResults.length > 0) {
                    if (grantResults[0] != PackageManager.PERMISSION_GRANTED) {
                        AlertDialog.Builder builder = new AlertDialog.Builder(this);
                        builder.setMessage("Please grant permissions to store activity logs")
                                .setCancelable(false)
                                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog, int id) {
                                        requestPermissions(new String[]{
                                                        Manifest.permission.WRITE_EXTERNAL_STORAGE},
                                                REQUEST_AUDIO_WRITE_EXTERNAL_STORAGE);
                                    }
                                });
                        AlertDialog alert = builder.create();
                        alert.show();
                    } else {
                        //TODO:Load data
                    }
                }
                break;
        }
    }

    private void checkPermission(){
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                == PackageManager.PERMISSION_GRANTED &&
                ActivityCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
                        == PackageManager.PERMISSION_GRANTED) {
            //TODO:Load data
        } else {
            ActivityCompat.requestPermissions(this, new String[]{
                            Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    REQUEST_AUDIO_WRITE_EXTERNAL_STORAGE);
        }
    }

    @Override
    public void onResponseReceived(String str) {
        if(str == null){
            messageText.setText("Empty MSG");
            return;
        }
        messageText.setText(str);
        serverURL = str;
    }

    @Override
    public void setDecodingResults(List<String> list, List<Double> scores) {

    }

}
