package com.example.simplegestureinput.common;

public class KeyboardParams {
    public final static int keyCount = 26;
    public final static int mostCommonKeyWidth = 108;
    public final static int mostCommonKeyHeight = 165;
    public final static int keyboardWidth = 1080;
    public final static int keyboardHeight = 1326;
    public final static int[] codes = {
            113, 119, 101, 114, 116, 121, 117, 105, 111, 112,
            97, 115, 100, 102, 103, 104, 106, 107, 108, 122,
            120, 99, 118, 98, 110, 109};
    public final static int[] xCoordinates = {
            59, 122, 209, 327, 416, 510, 599, 709, 802, 925,
            40, 168, 269, 373, 451, 553, 634, 718, 860, 176,
            252, 357, 461, 511, 628, 746
    };
    public final static int[] yCoordinates = {
            345, 345, 345, 345, 345, 345, 345, 345, 345, 345,
            510, 510, 510, 510, 510, 510, 510, 510, 510, 675,
            675, 675, 675, 675, 675, 675
    };
    public final static int[] widths = {
            90, 90, 90, 90, 90, 90, 90, 90, 90, 90,
            90, 90, 90, 90, 90, 90, 90, 90, 90, 90,
            90, 90, 90, 90, 90, 90
    };
    public final static int[] heights = {
            125, 125, 125, 125, 125, 125, 125, 125, 125, 125,
            125, 125, 125, 125, 125, 125, 125, 125, 125, 125,
            125, 125, 125, 125, 125, 125
    };

    // Gaussian distribution params: mean_x, mean_y, sd_x, sd_y, covariance (rho).
    // Ordered by position on keyboard.

    public final static double[] MEAN_X_STANDARD = {
            54, 162, 270, 378, 486, 594, 702, 810, 918, 1026,
            108, 216, 324, 432, 540, 648, 756, 864, 972,
            216, 324, 432, 540, 648, 756, 864
    };

    public final static double[] SD_X_STANDARD = {
            28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5,
            28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5,
            28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5
    };

    // key_height = 165
    public final static double[] MEAN_Y_STANDARD = {
            345, 345, 345, 345, 345, 345, 345, 345, 345, 345,
            510, 510, 510, 510, 510, 510, 510, 510, 510,
            675, 675, 675, 675, 675, 675, 675
    };

    public final static double[] SD_Y_STANDARD = {
            53.0, 53.0, 53.0, 53.0, 53.0, 53.0, 53.0, 53.0, 53.0, 53.0,
            53.0, 53.0, 53.0, 53.0, 53.0, 53.0, 53.0, 53.0, 53.0,
            53.0, 53.0, 53.0, 53.0, 53.0, 53.0, 53.0
    };

    public final static double[] RHO_STANDARD = {
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0
    };


}
