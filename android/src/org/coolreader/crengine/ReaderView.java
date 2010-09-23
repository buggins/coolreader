package org.coolreader.crengine;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.Callable;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.os.Environment;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
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
    	engine.execute(task);
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
    //private native boolean readHistoryInternal( String filename );
    //private native boolean writeHistoryInternal( String filename );
    private native void setStylesheetInternal( String stylesheet );
    private native void resizeInternal( int dx, int dy );
    private native boolean doCommandInternal( int command, int param );
    private native Bookmark getCurrentPageBookmarkInternal();
    private native boolean goToPositionInternal(String xPath);
    private native int getPositionPercentInternal(String xPath);
    private native int getPositionPageInternal(String xPath);
    private native void updateBookInfoInternal( BookInfo info );
    
    private int mNativeObject;
    
    private final Engine engine;
    
    private BookInfo bookInfo;

	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		super.onSizeChanged(w, h, oldw, oldh);
		Log.d("cr3", "onSizeChanged("+w + ", " + h +")");
		init();
		execute(new ResizeTask(w,h));
	}
	
	public boolean isBookLoaded()
	{
		return opened;
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
		case KeyEvent.KEYCODE_DPAD_CENTER:
			activity.showBrowser();
			break;
		case KeyEvent.KEYCODE_MENU:
			activity.openOptionsMenu();
			break;
		default:
			return super.onKeyDown(keyCode, event);
		}
		return true;
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		if ( event.getAction()==MotionEvent.ACTION_DOWN ) {
			int x = (int)event.getX();
			int y = (int)event.getY();
			int dx = getWidth();
			int dy = getHeight();
			boolean fwd = x>dx*3/4 || y>dy*3/4; 
			boolean back = x<dx/4 || y<dy/4;
			boolean center = x>dx/3 && x<dx*2/3 && y>dy/3 && y<dy*2/3;
			if ( fwd && !back ) {
				doCommand( ReaderCommand.DCMD_PAGEDOWN, 1);
				return true;
			} else if ( back ) {
				doCommand( ReaderCommand.DCMD_PAGEUP, 1);
				return true;
			} else if ( center ) {
				activity.openOptionsMenu();
			}
		}
		return super.onTouchEvent(event);
	}

	@Override
	public boolean onTrackballEvent(MotionEvent event) {
		// TODO Auto-generated method stub
		Log.d("cr3", "onTrackballEvent(" + event + ")");
		return super.onTrackballEvent(event);
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
	
	private void updateLoadedBookInfo()
	{
		// get title, authors, etc.
		updateBookInfoInternal( bookInfo );
	}
	
	class CreateViewTask extends Task
	{
		public void work() throws Exception {
			createInternal();
			File historyDir = activity.getDir("settings", Context.MODE_PRIVATE);
			//File historyDir = new File(Environment.getExternalStorageDirectory(), ".cr3");
			historyDir.mkdirs();
			//File historyFile = new File(historyDir, "cr3hist.ini");
			
			//File historyFile = new File(activity.getDir("settings", Context.MODE_PRIVATE), "cr3hist.ini");
			//if ( historyFile.exists() ) {
			//Log.d("cr3", "Reading history from file " + historyFile.getAbsolutePath());
			//readHistoryInternal(historyFile.getAbsolutePath());
			//}
	        String css = engine.loadResourceUtf8(R.raw.fb2);
	        if ( css!=null && css.length()>0 )
       			setStylesheetInternal(css);
			initialized = true;
		}
		public void done() {
			Log.d("cr3", "InitializationFinishedEvent");
		}
		public void fail( Exception e )
		{
			Log.e("cr3", "CoolReader engine initialization failed. Exiting.", e);
			engine.fatalError("Failed to init CoolReader engine");
		}
	}
	
	public void loadDocument( final String filename )
	{
		execute(new LoadDocumentTask(filename, null));
	}

	public boolean loadLastDocument( final Runnable errorHandler )
	{
		Log.i("cr3", "Submitting LastDocumentLoadTask");
		init();
		BookInfo book = activity.getHistory().getLastBook();
		if ( book==null ) {
			errorHandler.run();
			return false;
		}
		execute( new LoadDocumentTask(book.getFileInfo().getPathName(), errorHandler) );
		return true;
	}
	
	
//	class LastDocumentLoadTask extends Task {
//		Runnable errorHandler;
//		LastDocumentLoadTask( Runnable errorHandler )
//		{
//			this.errorHandler = errorHandler;
//		}
//		public void work() throws Exception {
//			if ( !initialized )
//				throw new IllegalStateException("ReaderView is not initialized");
//			Log.i("cr3", "Trying to load last document");
//			boolean res = doCommandInternal(ReaderCommand.DCMD_OPEN_RECENT_BOOK.nativeId, 0);
//			if ( !res )
//				throw new IOException("Cannot open recent book");
//			else
//				Log.i("cr3", "Last document is opened successfully");
//		}
//		public void done()
//		{
//	        opened = true;
//			Log.i("cr3", "Last document is opened. Restoring position...");
//	        doCommand(ReaderCommand.DCMD_RESTORE_POSITION, 0);
//			activity.showReader();
//	        drawPage();
//		}
//		public void fail( Exception e ) {
//			Log.i("cr3", "Last document loading is failed");
//			errorHandler.run();
//		}
//	}
	
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
	        Bookmark bm = getCurrentPageBookmarkInternal();
	        Log.d("cr3", "Current position: " + bm.getPercent() + "% " + bm.getStartPos());
		}
		public void done()
		{
			Log.d("cr3", "drawPage : bitmap is ready, invalidating view to draw new bitmap");
    		mBitmap = bitmap;
    		if (opened)
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
		Runnable errorHandler;
		LoadDocumentTask( String filename, Runnable errorHandler )
		{
			this.filename = filename;
			this.errorHandler = errorHandler;
			FileInfo fileInfo = new FileInfo(filename);
			bookInfo = activity.getHistory().getOrCreateBookInfo( fileInfo );
	        engine.showProgress( 1000, "Loading..." );
	        init();
		}

		public void work() {
			Log.i("cr3", "Loading document " + filename);
	        boolean success = loadDocumentInternal(filename);
	        if ( success ) {
		        //writeHistoryInternal(null);
	        	updateLoadedBookInfo();
				Log.i("cr3", "Document " + filename + " is loaded successfully");
	        } else {
				Log.e("cr3", "Error occured while trying to load document " + filename);
	        }
		}
		public void done()
		{
			Log.d("cr3", "LoadDocumentTask is finished successfully");
	        //showProgress( 5000, 0, "Formatting..." );
	        restorePosition();
	        opened = true;
	        //engine.hideProgress();
	        //doCommand(ReaderCommand.DCMD_RESTORE_POSITION, 0);
			activity.showReader();
	        drawPage();
		}
		public void fail( Exception e )
		{
			activity.getHistory().removeBookInfo( bookInfo );
			bookInfo = null;
			Log.d("cr3", "LoadDocumentTask is finished with exception " + e.getMessage());
	        opened = true;
			drawPage();
			if ( errorHandler!=null ) {
				errorHandler.run();
			}
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

    private void restorePosition()
    {
    	if ( bookInfo!=null && bookInfo.getLastPosition()!=null ) {
    		final String pos = bookInfo.getLastPosition().getStartPos();
    		execute( new Task() {
    			public void work() {
    	    		goToPositionInternal( pos );
    			}
    		});
    		activity.getHistory().updateBookAccess(bookInfo);
    	}
    }
    
    private void savePosition()
    {
    	Bookmark bmk = getCurrentPageBookmarkInternal();
    	if ( bmk!=null && bookInfo!=null ) {
    		bmk.setType(Bookmark.TYPE_LAST_POSITION);
    		bookInfo.setLastPosition(bmk);
    	}
    }
    
    public void close()
    {
    	execute( new Task() {
    		public void work() {
    			if ( opened ) {
					Log.i("cr3", "ReaderView().close() : closing current document");
					savePosition();
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
						String s = engine.loadResourceUtf8(fileFormat.getCSSResourceId());
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
    	destroyInternal();
    }

    private boolean initStarted = false;
    public void init()
    {
    	if ( initStarted )
    		return;
   		execute(new CreateViewTask());
    }
    
	CoolReader activity;
	public ReaderView(CoolReader activity, Engine engine) 
    {
        super(activity);
        this.activity = activity;
        this.engine = engine;
        setFocusable(true);
        setFocusableInTouchMode(true);
    }

}
