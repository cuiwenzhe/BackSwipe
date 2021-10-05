package com.example.simplegestureinput.util;

import android.util.Log;

import java.util.Arrays;

public class StringMethod {
    static String TAG = StringMethod.class.getSimpleName();

    public static String getPrevTextByText(String text){
        String prev_text = text.trim();
        return  prev_text.toLowerCase();
    }
    public static String getPrevWordByText(String text){
        String prev_text = text.trim();
        String[] split_words = prev_text.split(" ");
        if(split_words.length != 0){
            Log.i(TAG, Arrays.toString(split_words) + "|" + split_words.length);
            String prev_word = split_words[split_words.length-1];
            if(prev_word.length() == 0){
                prev_word = "_HEAD_";
                Log.i(TAG, "Head of the sentence");
            }
            return prev_word;
        }
        return prev_text;
    }
    public static String replaceLastWord(String sentence, String word){
        String prev_text = sentence.trim();
        String[] split_words = prev_text.split(" ");
        int max = split_words.length;
        if(max != 0 && max!=1){
            split_words[max-1] = word;
            StringBuilder builder = new StringBuilder();
            for(String s:split_words){
                builder.append(s).append(" ");
            }
            return builder.toString();
        }
        return word + " ";
    }
}
