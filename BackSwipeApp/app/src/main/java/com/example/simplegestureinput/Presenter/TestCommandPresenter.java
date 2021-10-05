package com.example.simplegestureinput.Presenter;

import android.content.Context;
import android.util.Log;

import com.example.simplegestureinput.Data.CommandResults;
import com.example.simplegestureinput.Data.TestCommands;
import com.example.simplegestureinput.R;

import java.util.HashMap;
import java.util.List;


public class TestCommandPresenter {
    final static private String TAG = TestCommandPresenter.class.getSimpleName();
    private Context context;
    private TestCommands testCommands;
    private HashMap<String, Integer> commandImageHashMap;
    private HashMap<String, String> synonymsCommandHashMap;
    private int testPairCount = 0;



    public TestCommandPresenter(Context _context, String userID, String testType){
        this.context = _context;
        int sentenceResource = R.raw.sentence_demo;
        String decodeType = testType.split("_")[0];
        String sentenceType = testType.split("_")[1];
        if(decodeType.equals("command")) {
            if (sentenceType.equals("demo")) {
                sentenceResource = R.raw.command_demo;
            } else if (testType.contains("warmup")) {
                sentenceResource = R.raw.command_demo;
            } else if (testType.contains("test")) {
                sentenceResource = R.raw.command_test;
            }
        }

        Log.i(TAG, String.valueOf(sentenceResource));
        testCommands = new TestCommands(context, sentenceResource, Integer.valueOf(userID));
        commandImageHashMap = initCommandRes();
        synonymsCommandHashMap = initSynonyms();
    }

    public String getNextCommand(){
        testPairCount++;
        return testCommands.getNextSentence();
    }
    public int getImageByCommand(String command){
        if(commandImageHashMap!=null)
            return commandImageHashMap.get(command);
        return -1;
    }
    public String getCurrentSentence(){
        return testCommands.getCurrentSentence();
    }


    public String getTestStatus(){
        StringBuilder str = new StringBuilder();
        str.append(testCommands.getCurrentIndex()+1);
        str.append('/');
        str.append(testCommands.getSentenceSize());
        return str.toString();
    }

    public List<String> getProbabilisticResults(final List<String> results, final List<Double> scores) {
        CommandResults res = new CommandResults();
        for (int i = 0; i < results.size(); i++) {
            if (synonymsCommandHashMap.containsKey(results.get(i))) {
                String cmd = synonymsCommandHashMap.get(results.get(i));
                res.add(cmd, scores.get(i));
            }
        }
        Log.d(TAG, "command prob: " + res.toString());
        //return res.getTopResult();
        return res.getTopNResults(5);
    }

    public String getCommandByWord(String word){
        if (synonymsCommandHashMap.containsKey(word))
            return synonymsCommandHashMap.get(word);
        return word;
    }

    private HashMap<String, Integer> initCommandRes() {
        HashMap<String, Integer> res = new HashMap<>();
        // 12 targets
        res.put("camera", R.drawable.camera);
        res.put("copy", R.drawable.copy);
        res.put("cut", R.drawable.cut);
        res.put("delete", R.drawable.delete);
        res.put("download", R.drawable.download);
        res.put("edit", R.drawable.edit);
        res.put("file", R.drawable.file);
        res.put("keyboard", R.drawable.keyboard);
        res.put("mail", R.drawable.email);
        res.put("print", R.drawable.printer);
        res.put("rotate", R.drawable.rotate);
        res.put("search", R.drawable.search);

        res.put("network", R.drawable.network);
        res.put("clock", R.drawable.clock);

        res.put("calculator", R.drawable.calculator);
        res.put("help", R.drawable.help);
        res.put("recent", R.drawable.recent);
        res.put("share", R.drawable.share);
        res.put("zoom", R.drawable.zoom);
        res.put("weather", R.drawable.weather);

        //additional
        res.put("contact", R.drawable.contact);
        res.put("date", R.drawable.date);
        res.put("night", R.drawable.night);
        res.put("bluetooth", R.drawable.bluetooth);
        res.put("photo", R.drawable.photo);
        res.put("settings", R.drawable.settings);
        res.put("message", R.drawable.message);
        res.put("flashlight", R.drawable.flashlight);
        res.put("sound", R.drawable.sound);
        res.put("notebook", R.drawable.notebook);



//        res.put("video", R.drawable.video);
        return res;
    }

    // word (synonym) -> command
    private HashMap<String, String> initSynonyms() {
        HashMap<String, String> res = new HashMap<>();
        for (String s : new String[]{"camera", "lens"})
            res.put(s, "camera");

        for (String s : new String[]{"copy", "replicate"})
            res.put(s, "copy");

        for (String s : new String[]{"cut", "trim"})
            res.put(s, "cut");

        for (String s : new String[]{"delete", "remove"})
            res.put(s, "delete");

        for (String s : new String[]{"download", "browse"})
            res.put(s, "download");

        for (String s : new String[]{"edit","revise"})
            res.put(s, "edit");

        for (String s : new String[]{"file", "folder"})
            res.put(s, "file");

        for (String s : new String[]{"keyboard", "typing"})
            res.put(s, "keyboard");

        for (String s : new String[]{"mail", "email" })
            res.put(s, "mail");

        for (String s : new String[]{"print", "publish"})
            res.put(s, "print");

        for (String s : new String[]{"rotate", "spin"})
            res.put(s, "rotate");

        for (String s : new String[]{"search", "find"})
            res.put(s, "search");

        // warm up
        for (String s : new String[]{"network", "internet" })
            res.put(s, "network");

        for (String s : new String[]{"clock", "timer"})
            res.put(s, "clock");

        // distractors
        for (String s : new String[]{"calculator", "computer"})
            res.put(s, "calculator");

        for (String s : new String[]{"help", "assist"})
            res.put(s, "help");

        for (String s : new String[]{"recent", "latest"})
            res.put(s, "recent");

        for (String s : new String[]{"share", "exchange"})
            res.put(s, "share");

        for (String s : new String[]{"weather", "forecast"})
            res.put(s, "weather");

        for (String s : new String[]{"zoom", "enlarge"})
            res.put(s, "zoom");

        for (String s : new String[]{"contact", "association"})
            res.put(s, "contact");
        for (String s : new String[]{"date", "calendar"})
            res.put(s, "date");
        for (String s : new String[]{"night", "dark"})
            res.put(s, "night");
        for (String s : new String[]{"bluetooth", "connection"})
            res.put(s, "bluetooth");
        for (String s : new String[]{"photo", "gallery"})
            res.put(s, "photo");

        for (String s : new String[]{"settings", "ambience"})
            res.put(s, "settings");
        for (String s : new String[]{"message", "letter"})
            res.put(s, "message");
        for (String s : new String[]{"flashlight", "torch"})
            res.put(s, "flashlight");
        for (String s : new String[]{"sound", "voice"})
            res.put(s, "sound");
        for (String s : new String[]{"notebook", "binder"})
            res.put(s, "notebook");
        return res;
    }
}
