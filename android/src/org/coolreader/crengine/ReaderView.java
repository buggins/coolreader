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
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import android.app.ProgressDialog;
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
    private native boolean doCommandInternal( int command, int param );
    private native DocumentInfo getStateInternal();
    
    private int mNativeObject;
    
    private final Engine engine;

	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		super.onSizeChanged(w, h, oldw, oldh);
		Log.d("cr3", "onSizeChanged("+w + ", " + h +")");
		executor.execute(new ResizeTask(w,h));
	}
    
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		Log.d("cr3", "onKeyDown("+keyCode + ", " + event +")");
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

	public void doCommand( final ReaderCommand cmd, final int param )
	{
		Log.d("cr3", "doCommand("+cmd + ", " + param +")");
		executor.execute(new Runnable() {
			public void run() {
				boolean res = doCommandInternal(cmd.nativeId, param);
				if ( res )
					post(new Runnable() {
						public void run() {
							drawPage();
						}
					});
			}
		});
		invalidate();
	}
	
	ExecutorService executor = Executors.newFixedThreadPool(1);
	boolean initialized = false;
	boolean opened = false;
	
	ProgressDialog progressDlg;
	void setProgress( int progress )
	{
		if ( progress==10000 ) {
			if ( progressDlg!=null )
				progressDlg.dismiss();
			progressDlg = null;
		} else if ( progress>=0 && progress<10000 ) {
			if ( progressDlg==null )
				progressDlg = ProgressDialog.show(getContext(), "Please wait", "Initializing engine");
			progressDlg.setProgress(progress);
		}
	}

	void postProgress( final int progress )
	{
		post( new Runnable() {
			public void run() {
				setProgress(progress);
			}
		});
	}
	
	
	class InitializationFinishedEvent implements Runnable
	{
		public void run() {
			Log.d("cr3", "InitializationFinishedEvent");
	        File sddir = Environment.getExternalStorageDirectory();
	        File booksdir = new File( sddir, "books");
	        File exampleFile = new File( booksdir, "example.fb2");
			executor.execute(new LoadDocumentTask(exampleFile.getAbsolutePath()));
			initialized = true;
		}
	}
	
	class LoadFinishedEvent implements Runnable
	{
		boolean success;
		LoadFinishedEvent( boolean success )
		{
			this.success = success;
		}
		public void run() {
			Log.d("cr3", "LoadFinishedEvent");
			drawPage();
		}
	}
	
	class FatalErrorEvent implements Runnable
	{
		String msg;
		public FatalErrorEvent( String msg ) {
			this.msg = msg;
		}
		public void run() {
			Log.e("cr3", "Fatal Error: " + msg);
			// TODO: close application
		}
	}
	
	class InitEngineTask implements Runnable
	{
		public void run() {
			try { 
				engine.init();
				createInternal();
				doCommandInternal(ReaderCommand.DCMD_ZOOM_OUT.nativeId, 5);
				post(new InitializationFinishedEvent());
			} catch ( Exception e ) {
				post(new FatalErrorEvent("Error while initialization of CoolReader engine"));
			}
		}
	}

	private int lastDrawTaskId = 0;
	private class DrawPageTask implements Runnable {
		final int id;
		DrawPageTask()
		{
			this.id = ++lastDrawTaskId;
		}
		public void run() {
			if ( this.id!=lastDrawTaskId ) {
				Log.d("cr3", "skipping duplicate drawPage request");
				return;
			}
			Log.e("cr3", "drawPage.run()");
			if ( internalDX==0 || internalDY==0 ) {
				internalDX=200;
				internalDY=200;
		        resizeInternal(internalDX, internalDY);
			}
			final Bitmap bitmap = Bitmap.createBitmap(internalDX, internalDY, Bitmap.Config.ARGB_8888);
	        bitmap.eraseColor(Color.BLUE);
	        getPageImage(bitmap);
	        post(new Runnable() {
	        	public void run() {
					Log.e("cr3", "drawPage : replacing bitmap");
	        		mBitmap = bitmap;
	        		setProgress(10000);
	        		invalidate();
	        	}
	        });
		}
	}; 
	
	private void drawPage()
	{
		if ( !initialized )
			return;
		executor.execute( new DrawPageTask() );
	}
	
	private int internalDX = 0;
	private int internalDY = 0;
	private int lastResizeTaskId = 0;
	private class ResizeTask implements Runnable
	{
		final int id;
		final int dx;
		final int dy;
		ResizeTask( int dx, int dy )
		{
			this.dx = dx;
			this.dy = dy;
			this.id = ++lastResizeTaskId; 
		}
		public void run() {
			if ( this.id != lastResizeTaskId ) {
				Log.d("cr3", "skipping duplicate resize request");
				return;
			}
	        resizeInternal(dx, dy);
	        internalDX = dx;
	        internalDY = dy;
	        drawPage();
		}
	}
	
	private class LoadDocumentTask implements Runnable
	{
		String filename;
		LoadDocumentTask( String filename )
		{
			this.filename = filename;
		}

		public void run() {
			Log.i("cr3", "Loading document " + filename);
			postProgress(1000);
	        boolean success = loadDocumentInternal(filename);
	        if ( success ) {
				Log.i("cr3", "Document " + filename + " is loaded successfully");
	        	doCommandInternal(ReaderCommand.DCMD_PAGEDOWN.nativeId, 2);
	        } else {
				Log.e("cr3", "Error occured while trying to load document " + filename);
	        }
	        post(new LoadFinishedEvent(success));
			postProgress(5000);
		}
	}
	
    @Override 
    protected void onDraw(Canvas canvas) {
    	try {
    		Log.d("cr3", "onDraw() called");
    		if ( initialized && mBitmap!=null ) {
        		Log.d("cr3", "onDraw() -- drawing page image");
    			canvas.drawBitmap(mBitmap, 0, 0, null);
    		}
    	} catch ( Exception e ) {
    		Log.e("cr3", "exception while drawing", e);
    	}
    }

	public ReaderView(Context context, Engine engine) 
    {
        super(context);
        this.engine = engine;
        setFocusable(true);
        setFocusableInTouchMode(true);
        setProgress(0);
        executor.execute(new InitEngineTask());
    }

}
