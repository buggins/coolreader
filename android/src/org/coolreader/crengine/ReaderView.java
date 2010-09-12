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

import java.io.File;
import java.nio.IntBuffer;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.os.Environment;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;

public class ReaderView extends View {
    private Bitmap mBitmap;

    public enum ReaderCommand
    {
    	//definitions from lvdocview.h
    	DCMD_BEGIN(100),
    	DCMD_LINEUP(101),
    	DCMD_PAGEUP(102),
    	DCMD_PAGEDOWN(103),
    	DCMD_LINEDOWN(104),
    	DCMD_LINK_FORWARD(105),
    	DCMD_LINK_BACK(106),
    	DCMD_LINK_NEXT(107),
    	DCMD_LINK_PREV(108),
    	DCMD_LINK_GO(109),
    	DCMD_END(110),
    	DCMD_GO_POS(111),
    	DCMD_GO_PAGE(112),
    	DCMD_ZOOM_IN(113),
    	DCMD_ZOOM_OUT(114),
    	DCMD_TOGGLE_TEXT_FORMAT(115),
    	DCMD_BOOKMARK_SAVE_N(116),
    	DCMD_BOOKMARK_GO_N(117),
    	DCMD_MOVE_BY_CHAPTER(118),
    	DCMD_GO_SCROLL_POS(119),
    	DCMD_TOGGLE_PAGE_SCROLL_VIEW(120),
    	DCMD_LINK_FIRST(121),
    	DCMD_ROTATE_BY(122),
    	DCMD_ROTATE_SET(123),
    	DCMD_SAVE_HISTORY(124),
    	DCMD_SAVE_TO_CACHE(125),
    	DCMD_TOGGLE_BOLD(126);
    	private final int nativeId;
    	private ReaderCommand( int nativeId )
    	{
    		this.nativeId = nativeId;
    	}
    }
    
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
    private native boolean loadDocumentInternal( String fileName );
    private native String getSettingsInternal();
    private native boolean applySettingsInternal( String settings );
    private native boolean readHistoryInternal( String filename );
    private native boolean writeHistoryInternal( String filename );
    private native void setStylesheetInternal( String stylesheet );
    private native void resizeInternal( int dx, int dy );
    private native boolean doCommandInternal( int command );
    private native DocumentInfo getStateInternal();
    
    private int mNativeObject;
    
    private final Engine engine;

    
    
    @Override
	public boolean dispatchKeyEvent(KeyEvent event) {
		// TODO Auto-generated method stub
		return super.dispatchKeyEvent(event);
	}
	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		super.onSizeChanged(w, h, oldw, oldh);
        resizeInternal(w, h);
        mBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        mBitmap.eraseColor(Color.BLUE);
	}
    
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		switch ( keyCode ) {
		case KeyEvent.KEYCODE_DPAD_DOWN:
			doCommand( ReaderCommand.DCMD_PAGEDOWN, 1);
			break;
		case KeyEvent.KEYCODE_DPAD_UP:
			doCommand( ReaderCommand.DCMD_PAGEUP, 1);
			break;
		case KeyEvent.KEYCODE_DPAD_LEFT:
			doCommand( ReaderCommand.DCMD_PAGEUP, 10);
			break;
		case KeyEvent.KEYCODE_DPAD_RIGHT:
			doCommand( ReaderCommand.DCMD_PAGEDOWN, 10);
			break;
		default:
			return super.onKeyDown(keyCode, event);
		}
		return true;
	}

	public boolean doCommand( ReaderCommand cmd, int param )
	{
		doCommandInternal(cmd.nativeId);
		invalidate();
		return true;
	}
	
	public ReaderView(Context context, Engine engine) 
    {
        super(context);
        this.engine = engine;
        createInternal();
       
        
        File sddir = Environment.getExternalStorageDirectory();
        File booksdir = new File( sddir, "books");
        File exampleFile = new File( booksdir, "example.fb2");
        loadDocumentInternal(exampleFile.getAbsolutePath());
        int W = 200;
        int H = 200;
        resizeInternal(W, H);
        mBitmap = Bitmap.createBitmap(W, H, Bitmap.Config.ARGB_8888);
        mBitmap.eraseColor(Color.BLUE);
        doCommand( ReaderCommand.DCMD_PAGEDOWN, 0 );
        doCommand( ReaderCommand.DCMD_PAGEDOWN, 0 );
        doCommand( ReaderCommand.DCMD_PAGEDOWN, 0 );
        doCommand( ReaderCommand.DCMD_ZOOM_OUT, 1 );
        doCommand( ReaderCommand.DCMD_ZOOM_OUT, 1 );
        doCommand( ReaderCommand.DCMD_ZOOM_OUT, 1 );
        doCommand( ReaderCommand.DCMD_ZOOM_OUT, 1 );
        //doCommand( ReaderCommand.DCMD_PAGEDOWN, 0 );
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
