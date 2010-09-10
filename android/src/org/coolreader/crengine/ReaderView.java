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

    public enum DocumentFormat {
    	/// lvtinydom.h: source document formats
    	//typedef enum {
    	NONE,// doc_format_none,
    	FB2, // doc_format_fb2,
    	TXT, // doc_format_txt,
    	RTF, // doc_format_rtf,
    	EPUB,// doc_format_epub,
    	HTML,// doc_format_html,
    	TXT_BOOKMARK, // doc_format_txt_bookmark, // coolreader TXT format bookmark
    	CHM; //  doc_format_chm,
  	    // don't forget update getDocFormatName() when changing this enum
    	//} doc_format_t;
    	DocumentFormat byId( int i )
    	{
    		if ( i>=0 && i<=CHM.ordinal() )
    			return values()[i];
    		return null;
    	}
    }
    
    public static class DocumentInfo
    {
    	boolean opened;
    	int width;
    	int height;
    	String fileName;
    	String filePath;
    	int    fileSize;
    	int    arcSize;
    	String arcName;
    	String arcPath;
    	String title;
    	String author;
    	String seriesName;
    	int seriesNumber;
    	DocumentFormat docFormat;
    	int curPage;
    	int pageCount;
    }
    /* implementend by libcr3engine.so */
    // get current page image
    private native void getPageImage(Bitmap bitmap);
    // constructor's native part
    private native void createInternal();
    private native boolean loadDocument( String fileName );
    private native String getSettings();
    private native boolean applySettings( String settings );
    private native boolean readHistory( String filename );
    private native boolean writeHistory( String filename );
    private native void setStylesheet( String stylesheet );
    private native void resize( int dx, int dy );
    private native boolean doCommand( int command );
    private native DocumentInfo getState();
    
    private int mNativeObject;
    
    private final Engine engine;

    public ReaderView(Context context, Engine engine) 
    {
        super(context);
        this.engine = engine;
        createInternal();

        final int W = 200;
        final int H = 200;
        
        mBitmap = Bitmap.createBitmap(W, H, Bitmap.Config.ARGB_8888);
        mBitmap.eraseColor(Color.BLUE);
    }

    @Override protected void onDraw(Canvas canvas) {
    	try {
	        getPageImage(mBitmap);
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
