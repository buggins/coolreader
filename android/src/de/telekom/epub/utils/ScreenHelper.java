/*
 * CoolReader for Android
 * Copyright (C) 2014 fuero <the_master_of_disaster@gmx.at>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package de.telekom.epub.utils;

//import android.graphics.Rect;
import android.util.Log;
//import android.view.View;

public class ScreenHelper {
    private static final String LOG_TAG = ScreenHelper.class.getSimpleName();

    public static final int A2 = 4;
    public static final long DEFAULT_REFRESH_DELAY_TIME = 50L;
    public static final int DU = 1;
    public static final int FB_COLOR_CODE_WHITE = 255;
    public static final int GC16_FULL = 34;
    public static final int GC16_PARTIAL = 2;
    public static final int GLD_F = 39;
    public static final int GLD_P = 7;
    public static final int GLR_FULL = 38;
    public static final int GLR_PARTIAL = 6;
    public static final int NATIVE_UPDATE_MODE_FULL = 1;
    public static final int NATIVE_UPDATE_MODE_PARTIAL = 0;
    public static final int NATIVE_WAVEFORM_MODE_A2 = 4;
    public static final int NATIVE_WAVEFORM_MODE_DU = 1;
    public static final int NATIVE_WAVEFORM_MODE_GC16 = 2;
    public static final int NATIVE_WAVEFORM_MODE_GC4 = 3;
    public static final int NATIVE_WAVEFORM_MODE_INIT = 0;
    public static final int UPDATE_MODE_AUTO = 52;
    public static final int UPDATE_MODE_GC16 = 34;
    public static final int UPDATE_MODE_REAGL = 39;

    static {
        try {
            System.loadLibrary("epd");
        }
        catch (Throwable localThrowable) {
            Log.e(LOG_TAG, "unable to load screen helper library", localThrowable);
        }
    }

    public static native int FullRefresh();
    public static native int PartialRefresh(int left, int top, int right, int bottom);
    public static native int RegionalRefresh(int left, int top, int right, int bottom, int mode, int waveform);
    public static native int SetFBColor(int paramInt1, int paramInt2, int paramInt3, int paramInt4, int paramInt5, int paramInt6);
    public static native int SetFBColorArray(int paramInt1, int paramInt2, int paramInt3, int paramInt4, int[] paramArrayOfInt, int paramInt5);
}
