package com.example.simplegestureinput.common;

import java.io.File;

/**
 * Immutable class to hold the address of an asset.
 * As opposed to a normal file, an asset is usually represented as a contiguous byte array in
 * the package file. Open it correctly thus requires the name of the package it is in, but
 * also the offset in the file and the length of this data. This class encapsulates these three.
 */
public final class AssetFileAddress {
    public final String mFilename;
    public final long mOffset;
    public final long mLength;

    public AssetFileAddress(final String filename, final long offset, final long length) {
        mFilename = filename;
        mOffset = offset;
        mLength = length;
    }

    /**
     * Makes an AssetFileAddress. This may return null.
     *
     * @param filename the filename.
     * @return the address, or null if the file does not exist or the parameters are not valid.
     */
    public static AssetFileAddress makeFromFileName(final String filename) {
        if (null == filename) return null;
        final File f = new File(filename);
        if (!f.isFile()) return null;
        return new AssetFileAddress(filename, 0l, f.length());
    }

    /**
     * Makes an AssetFileAddress. This may return null.
     *
     * @param filename the filename.
     * @param offset the offset.
     * @param length the length.
     * @return the address, or null if the file does not exist or the parameters are not valid.
     */
    public static AssetFileAddress makeFromFileNameAndOffset(final String filename,
                                                             final long offset, final long length) {
        if (null == filename) return null;
        final File f = new File(filename);
        if (!f.isFile()) return null;
        return new AssetFileAddress(filename, offset, length);
    }
}