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
import java.io.IOException;
import java.util.concurrent.Callable;

import org.coolreader.R;

import android.app.Activity;
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
    	//definitions from crengine/include/lvdocview.h
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
    	DCMD_TOGGLE_BOLD(126),

    	// definitions from android/jni/readerview.h
    	DCMD_OPEN_RECENT_BOOK(2000),
    	DCMD_CLOSE_BOOK(2001),
    	DCMD_RESTORE_POSITION(2002),
    	;
    	
    	private final int nativeId;
    	private ReaderCommand( int nativeId )
    	{
    		this.nativeId = nativeId;
    	}
    }
    
    private void execute( Engine.EngineTask task )
    {
    	engine.execute(task, this);
    }
    
    private abstract class Task implements Engine.EngineTask {
    	
    	public void post()
    	{
    		execute(this);
    	}

		public void done() {
			// override to do something useful
		}

		public void fail(Exception e) {
			// do nothing, just log exception
			// override to do custom action
			Log.e("cr3", "Task " + this.getClass().getSimpleName() + " is failed with exception " + e.getMessage(), e);
		}
    }
    
	static class Sync<T> extends Object {
		private T result = null;
		private boolean completed = false;
		public synchronized void set( T res )
		{
			result = res;
			completed = true;
			notify();
		}
		public synchronized T get()
		{
			while ( !completed ) {
    			try {
    				wait();
    			} catch (Exception e) {
    				// ignore
    			}
			}
			return result;
		}
	}

    private <T> T executeSync( final Callable<T> task )
    {
    	//Log.d("cr3", "executeSync called");
    	
    	
    	final Sync<T> sync = new Sync<T>();
    	post( new Runnable() {
    		public void run() {
    			try {
    				sync.set( task.call() );
    			} catch ( Exception e ) {
    			}
    		}
    	});
    	T res = sync.get();
    	//Log.d("cr3", "executeSync done");
    	return res;
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
    private native void destroyInternal();
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
		execute(new ResizeTask(w,h));
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
		execute(new Task() {
			boolean res;
			public void work() {
				res = doCommandInternal(cmd.nativeId, param);
			}
			public void done() {
				if ( res )
					drawPage();
			}
		});
	}
	
	boolean initialized = false;
	boolean opened = false;
	
	//private File historyFile;
	
	class CreateViewTask extends Task
	{
		public void work() throws Exception {
			createInternal();
			File historyDir = new File(Environment.getExternalStorageDirectory(), ".cr3");
			historyDir.mkdirs();
			File historyFile = new File(historyDir, "cr3hist.ini");
			
			//File historyFile = new File(activity.getDir("settings", Context.MODE_PRIVATE), "cr3hist.ini");
			//if ( historyFile.exists() ) {
			Log.d("cr3", "Reading history from file " + historyFile.getAbsolutePath());
			readHistoryInternal(historyFile.getAbsolutePath());
			//}
	        String css = engine.loadResourceUtf8(R.raw.fb2);
	        if ( css!=null && css.length()>0 )
       			setStylesheetInternal(css);
		}
		public void done() {
			Log.d("cr3", "InitializationFinishedEvent");
			initialized = true;
		}
		public void fail( Exception e )
		{
			Log.e("cr3", "CoolReader engine initialization failed. Exiting.", e);
			engine.fatalError("Failed to init CoolReader engine");
		}
	}
	
	public void loadDocument( final String filename )
	{
		execute(new LoadDocumentTask(filename));
	}

	public void loadLastDocument( final Runnable errorHandler )
	{
		execute( new Task() {
			public void work() throws Exception {
				if ( !initialized )
					throw new IllegalStateException("ReaderView is not initialized");
				boolean res = doCommandInternal(ReaderCommand.DCMD_OPEN_RECENT_BOOK.nativeId, 0);
				if ( !res )
					throw new IOException("Cannot open recent book");
			}
			public void done()
			{
		        opened = true;
		        doCommand(ReaderCommand.DCMD_RESTORE_POSITION, 0);
		        drawPage();
			}
			public void fail( Exception e ) {
				errorHandler.run();
			}
		});
	}
	
	private int lastDrawTaskId = 0;
	private class DrawPageTask extends Task {
		final int id;
		Bitmap bitmap;
		DrawPageTask()
		{
			this.id = ++lastDrawTaskId;
		}
		public void work() {
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
			bitmap = Bitmap.createBitmap(internalDX, internalDY, Bitmap.Config.ARGB_8888);
	        bitmap.eraseColor(Color.BLUE);
	        getPageImage(bitmap);
		}
		public void done()
		{
			Log.d("cr3", "drawPage : bitmap is ready, invalidating view to draw new bitmap");
    		mBitmap = bitmap;
    	    engine.hideProgress();
    		invalidate();
		}
	}; 
	
	private void drawPage()
	{
		if ( !initialized || !opened )
			return;
		execute( new DrawPageTask() );
	}
	
	private int internalDX = 0;
	private int internalDY = 0;
	private int lastResizeTaskId = 0;
	private class ResizeTask extends Task
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
		public void work() {
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
	
	private class LoadDocumentTask extends Task
	{
		String filename;
		LoadDocumentTask( String filename )
		{
			this.filename = filename;
	        engine.showProgress( 1000, "Loading..." );
		}

		public void work() {
			Log.i("cr3", "Loading document " + filename);
	        boolean success = loadDocumentInternal(filename);
	        if ( success ) {
				Log.i("cr3", "Document " + filename + " is loaded successfully");
	        } else {
				Log.e("cr3", "Error occured while trying to load document " + filename);
	        }
		}
		public void done()
		{
			Log.d("cr3", "LoadDocumentTask is finished successfully");
	        //showProgress( 5000, 0, "Formatting..." );
	        opened = true;
	        drawPage();
		}
		public void fail( Exception e )
		{
			Log.d("cr3", "LoadDocumentTask is finished with exception " + e.getMessage());
	        opened = true;
			drawPage();
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

    public void close()
    {
    	execute( new Task() {
    		public void work() {
    			if ( opened ) {
					Log.i("cr3", "ReaderView().close() : closing current document");
					doCommandInternal(ReaderCommand.DCMD_CLOSE_BOOK.nativeId, 0);
    			}
    		}
    		public void done() {
    			if ( opened ) {
	    			opened = false;
	    			mBitmap = null;
    			}
    		}
    	});
    }

    public void destroy()
    {
    	if ( initialized ) {
        	close();
        	execute( new Task() {
        		public void work() {
        	    	if ( initialized ) {
        	    		destroyInternal();
        	    		initialized = false;
        	    	}
        		}
        	});
    		engine.waitTasksCompletion();
        	engine.unregisterView(this);
    	}
    }
    
    @Override
	protected void onDetachedFromWindow() {
		// TODO Auto-generated method stub
		super.onDetachedFromWindow();
		Log.d("cr3", "View.onDetachedFromWindow() is called");
	}

	boolean enable_progress_callback = true;
    ReaderCallback readerCallback = new ReaderCallback() {
    
	    public boolean OnExportProgress(int percent) {
	    	Log.d("cr3", "readerCallback.OnExportProgress " + percent);
			return true;
		}
		public void OnExternalLink(String url, String nodeXPath) {
		}
		public void OnFormatEnd() {
	    	Log.d("cr3", "readerCallback.OnFormatEnd");
		}
		public boolean OnFormatProgress(final int percent) {
			if ( enable_progress_callback )
			executeSync( new Callable<Object>() {
				public Object call() {
			    	Log.d("cr3", "readerCallback.OnFormatProgress " + percent);
			    	engine.showProgress( percent*4/10 + 5000, "Formatting...");
			    	return null;
				}
			});
			return true;
		}
		public void OnFormatStart() {
	    	Log.d("cr3", "readerCallback.OnFormatStart");
		}
		public void OnLoadFileEnd() {
	    	Log.d("cr3", "readerCallback.OnLoadFileEnd");
		}
		public void OnLoadFileError(String message) {
	    	Log.d("cr3", "readerCallback.OnLoadFileError(" + message + ")");
		}
		public void OnLoadFileFirstPagesReady() {
	    	Log.d("cr3", "readerCallback.OnLoadFileFirstPagesReady");
		}
		public String OnLoadFileFormatDetected(final DocumentFormat fileFormat) {
			String res = executeSync( new Callable<String>() {
				public String call() {
					Log.i("cr3", "readerCallback.OnLoadFileFormatDetected " + fileFormat);
					if ( fileFormat!=null ) {
						String s = engine.loadResourceUtf8(fileFormat.getResourceId());
						Log.i("cr3", "setting .css for file format " + fileFormat + " from resource " + (fileFormat!=null?fileFormat.getCssName():"[NONE]"));
						return s;
					}
			    	return null;
				}
			});
			return res;
		}
		public boolean OnLoadFileProgress(final int percent) {
			if ( enable_progress_callback )
			executeSync( new Callable<Object>() {
				public Object call() {
			    	Log.d("cr3", "readerCallback.OnLoadFileProgress " + percent);
			    	engine.showProgress( percent*4/10 + 1000, "Loading...");
			    	return null;
				}
			});
			return true;
		}
		public void OnLoadFileStart(String filename) {
	    	Log.d("cr3", "readerCallback.OnLoadFileStart " + filename);
		}
    };

    public void setStyleSheet( final String css )
    {
        if ( css!=null && css.length()>0 ) {
        	execute(new Task() {
        		public void work() {
        			setStylesheetInternal(css);
        		}
        	});
        }
    }
    
    @Override
    public void finalize()
    {
        engine.unregisterView(this);
    	destroyInternal();
    }
    
	Activity activity;
	public ReaderView(Activity activity, Engine engine) 
    {
        super(activity);
        this.activity = activity;
        this.engine = engine;
        engine.registerView(this);
        setFocusable(true);
        setFocusableInTouchMode(true);
        execute(new CreateViewTask());
    }

}
