package com.example.simplegestureinput;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.util.Log;

import com.example.simplegestureinput.common.AssetFileAddress;
import com.example.simplegestureinput.common.DecoderResults;
import com.example.simplegestureinput.common.KeyboardParams;

import java.io.IOException;
import java.util.concurrent.atomic.AtomicLong;

import static java.nio.charset.StandardCharsets.UTF_8;

public class DecoderTools {
    static {
        System.loadLibrary("gesture-decoder-lib");
    }
    public static native long createDecoderNative(final byte[] lmName,
                                                  final byte[] filePath, final long offset, final long length);
    public static native void deleteDecoderNative();
    public static native DecoderResults decodeGesture(int[] xs, int[] ys, int[] times, int points_count, final byte[] prevWord);

    static native void setKeyboardLayoutNative(final long ptr, final int keyCount,
                                               final int mostCommonKeyWidth, final int mostCommonKeyHeight, final int keyboardWidth,
                                               final int keyboardHeight, final int[] codes, final int[] xCoordinates,
                                               final int[] yCoordinates, final int[] widths, final int[] heights);

    private final static String TAG = DecoderTools.class.getSimpleName();
    private Context mContext;
    private final AtomicLong mNativeDecoderPtr = new AtomicLong(0);

    DecoderTools(Context context){
        mContext = context;
        AssetFileAddress lmFileAddress = loadDictionary(R.raw.main_en_d3);
        mNativeDecoderPtr.set(createDecoderNative(
                lmFileAddress.mFilename.getBytes(UTF_8) /* lmName */,
                lmFileAddress.mFilename.getBytes(UTF_8),
                lmFileAddress.mOffset, lmFileAddress.mLength));

        setKeyboardLayoutNative(mNativeDecoderPtr.get(), KeyboardParams.keyCount,
                KeyboardParams.mostCommonKeyWidth, KeyboardParams. mostCommonKeyHeight,
                KeyboardParams.keyboardWidth, KeyboardParams.keyboardHeight,
                KeyboardParams.codes,
                KeyboardParams.xCoordinates,
                KeyboardParams.yCoordinates,
                KeyboardParams.widths, KeyboardParams.heights);

    }

    private AssetFileAddress loadDictionary(int rawId){
        int fallbackResId = R.raw.main_en_d3;
        AssetFileAddress lmAddress = null;
        AssetFileDescriptor afd = null;
        try {
            afd = mContext.getResources().openRawResourceFd(fallbackResId);
        } catch (RuntimeException e) {
            Log.e(TAG, "Resource not found: " + fallbackResId + "|" + e.getMessage());
        }
        if (afd == null) {
            Log.e(TAG, "Resource cannot be opened: " + fallbackResId);
        }
        try {
            lmAddress =  AssetFileAddress.makeFromFileNameAndOffset(
                    mContext.getApplicationInfo().sourceDir, afd.getStartOffset(), afd.getLength());
        } finally {
            try {
                afd.close();
            } catch (IOException ignored) {
            }
        }
        return lmAddress;
    }

    public void releaseDecoder(){
        deleteDecoderNative();
    }
}
