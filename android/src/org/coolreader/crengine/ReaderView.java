/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.coolreader.crengine;

import java.nio.IntBuffer;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.util.Log;
import android.view.View;

public class ReaderView extends View {
    private Bitmap mBitmap;

    /* implementend by libcr3engine.so */
    private static native void getPageImage(Bitmap bitmap);

    public ReaderView(Context context) {
        super(context);

        final int W = 200;
        final int H = 200;

        
        mBitmap = Bitmap.createBitmap(W, H, Bitmap.Config.ARGB_8888);
        mBitmap.eraseColor(Color.BLUE);
    }

    @Override protected void onDraw(Canvas canvas) {
    	try {
	        getPageImage(mBitmap);
	        int[] pixels = new int[mBitmap.getRowBytes()*mBitmap.getHeight()/4];
	        for ( int i=0; i<pixels.length; i++ ) {
	        	pixels[i] = 0xFF000000 + i;
	        }
	        //{Color.RED, Color.CYAN};
	        IntBuffer testBuf = IntBuffer.wrap(pixels);
	        mBitmap.copyPixelsFromBuffer(testBuf);
	        canvas.drawBitmap(mBitmap, 0, 0, null);
    	} catch ( Exception e ) {
    		Log.e("cr3", "exception while drawing", e);
    	}

        //invalidate();
    }
    /* load our native library */
    static {
        //System.loadLibrary("cr3engine");
    }
}
