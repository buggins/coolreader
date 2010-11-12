package org.coolreader.crengine;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Map;
import java.util.concurrent.Callable;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Environment;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

public class ReaderView extends View {
    private Bitmap mBitmap;

    // additional key codes for Nook
    public static final int NOOK_KEY_PREV_LEFT = 96;
    public static final int NOOK_KEY_PREV_RIGHT = 98;
    public static final int NOOK_KEY_NEXT_LEFT = 95;
    public static final int NOOK_KEY_NEXT_RIGHT = 97;    
    public static final int NOOK_KEY_SHIFT_UP = 101;
    public static final int NOOK_KEY_SHIFT_DOWN = 100;
    
    public static final String PROP_NIGHT_MODE              ="crengine.night.mode";
    public static final String PROP_FONT_COLOR_DAY          ="font.color.day";
    public static final String PROP_BACKGROUND_COLOR_DAY    ="background.color.day";
    public static final String PROP_FONT_COLOR_NIGHT        ="font.color.night";
    public static final String PROP_BACKGROUND_COLOR_NIGHT  ="background.color.night";
    public static final String PROP_FONT_COLOR              ="font.color.default";
    public static final String PROP_BACKGROUND_COLOR        ="background.color.default";
    public static final String PROP_FONT_ANTIALIASING       ="font.antialiasing.mode";
    public static final String PROP_FONT_FACE               ="font.face.default";
    public static final String PROP_FONT_WEIGHT_EMBOLDEN    ="font.face.weight.embolden";
    public static final String PROP_TXT_OPTION_PREFORMATTED ="crengine.file.txt.preformatted";
    public static final String PROP_LOG_FILENAME            ="crengine.log.filename";
    public static final String PROP_LOG_LEVEL               ="crengine.log.level";
    public static final String PROP_LOG_AUTOFLUSH           ="crengine.log.autoflush";
    public static final String PROP_FONT_SIZE               ="crengine.font.size";
    public static final String PROP_STATUS_FONT_COLOR       ="crengine.page.header.font.color";
    public static final String PROP_STATUS_FONT_FACE        ="crengine.page.header.font.face";
    public static final String PROP_STATUS_FONT_SIZE        ="crengine.page.header.font.size";
    public static final String PROP_PAGE_MARGIN_TOP         ="crengine.page.margin.top";
    public static final String PROP_PAGE_MARGIN_BOTTOM      ="crengine.page.margin.bottom";
    public static final String PROP_PAGE_MARGIN_LEFT        ="crengine.page.margin.left";
    public static final String PROP_PAGE_MARGIN_RIGHT       ="crengine.page.margin.right";
    public static final String PROP_PAGE_VIEW_MODE          ="crengine.page.view.mode"; // pages/scroll
    public static final String PROP_INTERLINE_SPACE         ="crengine.interline.space";
    public static final String PROP_ROTATE_ANGLE            ="window.rotate.angle";
    public static final String PROP_EMBEDDED_STYLES         ="crengine.doc.embedded.styles.enabled";
    public static final String PROP_DISPLAY_INVERSE         ="crengine.display.inverse";
    public static final String PROP_DISPLAY_FULL_UPDATE_INTERVAL ="crengine.display.full.update.interval";
    public static final String PROP_DISPLAY_TURBO_UPDATE_MODE ="crengine.display.turbo.update";
    public static final String PROP_STATUS_LINE             ="window.status.line";
    public static final String PROP_BOOKMARK_ICONS          ="crengine.bookmarks.icons";
    public static final String PROP_FOOTNOTES               ="crengine.footnotes";
    public static final String PROP_SHOW_TIME               ="window.status.clock";
    public static final String PROP_SHOW_TITLE              ="window.status.title";
    public static final String PROP_SHOW_BATTERY            ="window.status.battery";
    public static final String PROP_SHOW_BATTERY_PERCENT    ="window.status.battery.percent";
    public static final String PROP_FONT_KERNING_ENABLED    ="font.kerning.enabled";
    public static final String PROP_LANDSCAPE_PAGES         ="window.landscape.pages";
    public static final String PROP_HYPHENATION_DICT        ="crengine.hyphenation.directory";
    public static final String PROP_AUTOSAVE_BOOKMARKS      ="crengine.autosave.bookmarks";

    public static final String PROP_MIN_FILE_SIZE_TO_CACHE  ="crengine.cache.filesize.min";
    public static final String PROP_FORCED_MIN_FILE_SIZE_TO_CACHE  ="crengine.cache.forced.filesize.min";
    public static final String PROP_PROGRESS_SHOW_FIRST_PAGE="crengine.progress.show.first.page";

    public static final String PROP_APP_FULLSCREEN          ="app.fullscreen";
    
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
    	mEngine.execute(task);
    }
    
    private abstract class Task implements Engine.EngineTask {
    	
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
    
    // Native functions
    /* implementend by libcr3engine.so */
    
    // get current page image
    private native void getPageImage(Bitmap bitmap);
    // constructor's native part
    private native void createInternal();
    private native void destroyInternal();
    private native boolean loadDocumentInternal( String fileName );
    private native java.util.Properties getSettingsInternal();
    private native boolean applySettingsInternal( java.util.Properties settings );
    private native void setStylesheetInternal( String stylesheet );
    private native void resizeInternal( int dx, int dy );
    private native boolean doCommandInternal( int command, int param );
    private native Bookmark getCurrentPageBookmarkInternal();
    private native boolean goToPositionInternal(String xPath);
    private native PositionProperties getPositionPropsInternal(String xPath);
    private native void updateBookInfoInternal( BookInfo info );
    private native TOCItem getTOCInternal();
    private native void clearSelectionInternal();
    private native boolean findTextInternal( String pattern, int origin, int reverse, int caseInsensitive );
    private native void setBatteryStateInternal( int state );
    
    
    protected int mNativeObject; // used from JNI
    
	private final CoolReader mActivity;
    private final Engine mEngine;
    private final BackgroundThread mBackThread;
    
    private BookInfo mBookInfo;
    
    private Properties mSettings = new Properties();

	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		Log.d("cr3", "onSizeChanged("+w + ", " + h +")");
		super.onSizeChanged(w, h, oldw, oldh);
		init();
		execute(new ResizeTask(w,h));
	}
	
	public boolean isBookLoaded()
	{
		BackgroundThread.ensureGUI();
		return mOpened;
	}
    
	public final int LONG_KEYPRESS_TIME = 1200;
	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		boolean isLongPress = (event.getEventTime()-event.getDownTime())>=LONG_KEYPRESS_TIME;
		if ( keyCode>=KeyEvent.KEYCODE_0 && keyCode<=KeyEvent.KEYCODE_9 ) {
			// goto/set shortcut bookmark
			int shortcut = keyCode - KeyEvent.KEYCODE_0;
			if ( shortcut==0 )
				shortcut = 10;
			if ( isLongPress )
				addBookmark(shortcut);
			else
				goToBookmark(shortcut);
			return true;
		} else if ( keyCode==KeyEvent.KEYCODE_VOLUME_DOWN || keyCode==KeyEvent.KEYCODE_VOLUME_UP )
			return true;

		if ( keyCode>=KeyEvent.KEYCODE_0 && keyCode<=KeyEvent.KEYCODE_9 ) {
			// will process in keyup handler
			return true;
		} else
		switch ( keyCode ) {
		case NOOK_KEY_NEXT_LEFT:
		case NOOK_KEY_NEXT_RIGHT:    
		case NOOK_KEY_SHIFT_DOWN:
		case KeyEvent.KEYCODE_DPAD_DOWN:
			doCommand( ReaderCommand.DCMD_PAGEDOWN, isLongPress ? 10 : 1);
			break;
		case NOOK_KEY_PREV_LEFT:
		case NOOK_KEY_PREV_RIGHT:
		case NOOK_KEY_SHIFT_UP:
		case KeyEvent.KEYCODE_DPAD_UP:
			doCommand( ReaderCommand.DCMD_PAGEUP, isLongPress ? 10 : 1);
			break;
		case KeyEvent.KEYCODE_DPAD_LEFT:
			doCommand( ReaderCommand.DCMD_PAGEUP, 10);
			break;
		case KeyEvent.KEYCODE_DPAD_RIGHT:
			doCommand( ReaderCommand.DCMD_PAGEDOWN, 10);
			break;
		case KeyEvent.KEYCODE_DPAD_CENTER:
			mActivity.showBrowser();
			break;
		case KeyEvent.KEYCODE_SEARCH:
			showSearchDialog();
			return true;
		case KeyEvent.KEYCODE_MENU:
			if ( isLongPress ) {
				mActivity.showOptionsDialog();
			} else {
				mActivity.openOptionsMenu();
			}
			break;
		case KeyEvent.KEYCODE_HOME:
			mActivity.showBrowser();
			break;
		case KeyEvent.KEYCODE_BACK:
			//saveSettings();
			return super.onKeyUp(keyCode, event);
		default:
			return super.onKeyUp(keyCode, event);
		}
		return true;
		//return super.onKeyUp(keyCode, event);
	}

	boolean VOLUME_KEYS_ZOOM = false;
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		Log.d("cr3", "onKeyDown("+keyCode + ", " + event +")");
		switch ( keyCode ) {
		case KeyEvent.KEYCODE_VOLUME_UP:
			if ( VOLUME_KEYS_ZOOM ) {
				doCommand( ReaderCommand.DCMD_ZOOM_IN, 1);
				syncViewSettings(getSettings());
			} else
				doCommand( ReaderCommand.DCMD_PAGEUP, 1);
			return true;
		case KeyEvent.KEYCODE_VOLUME_DOWN:
			if ( VOLUME_KEYS_ZOOM ) {
				doCommand( ReaderCommand.DCMD_ZOOM_OUT, 1);
				syncViewSettings(getSettings());
			} else
				doCommand( ReaderCommand.DCMD_PAGEDOWN, 1);
			return true;
		}
		if ( keyCode==KeyEvent.KEYCODE_BACK )
			return super.onKeyDown(keyCode, event);
		return true;
		//return super.onKeyDown(keyCode, event);
		//return true;
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		if ( event.getAction()==MotionEvent.ACTION_UP ) {
			boolean isLongPress = (event.getEventTime()-event.getDownTime())>LONG_KEYPRESS_TIME;
			int x = (int)event.getX();
			int y = (int)event.getY();
			int dx = getWidth();
			int dy = getHeight();
			boolean fwd = x>dx*3/4 || y>dy*3/4; 
			boolean back = x<dx/4 || y<dy/4;
			boolean center = x>dx/3 && x<dx*2/3 && y>dy/3 && y<dy*2/3;
			if ( fwd && !back ) {
				doCommand( ReaderCommand.DCMD_PAGEDOWN, isLongPress ? 10 : 1);
				return true;
			} else if ( back ) {
				doCommand( ReaderCommand.DCMD_PAGEUP, isLongPress ? 10 : 1);
				return true;
			} else if ( center ) {
				if ( isLongPress )
					mActivity.showOptionsDialog();
				else
					mActivity.openOptionsMenu();
			}
			return true;
		}
		return true;
		//return super.onTouchEvent(event);
	}

	@Override
	public boolean onTrackballEvent(MotionEvent event) {
		// TODO Auto-generated method stub
		Log.d("cr3", "onTrackballEvent(" + event + ")");
		return super.onTrackballEvent(event);
	}
	
	public void showTOC()
	{
		BackgroundThread.ensureGUI();
		final ReaderView view = this; 
		mEngine.execute(new Task() {
			TOCItem toc;
			PositionProperties pos;
			public void work() {
				BackgroundThread.ensureBackground();
				toc = getTOCInternal();
				pos = getPositionPropsInternal(null);
			}
			public void done() {
				BackgroundThread.ensureGUI();
				if ( toc!=null && pos!=null ) {
					TOCDlg dlg = new TOCDlg(mActivity, view, toc, pos.pageNumber);
					dlg.show();
				} else {
					mActivity.showToast("No Table of Contents found");
				}
			}
		});
	}
	
	public void showSearchDialog()
	{
		BackgroundThread.ensureGUI();
		SearchDlg dlg = new SearchDlg( mActivity, this );
		dlg.show();
	}

    public void findText( final String pattern, final boolean reverse, final boolean caseInsensitive )
    {
		BackgroundThread.ensureGUI();
		final ReaderView view = this; 
		mEngine.execute(new Task() {
			public void work() throws Exception {
				BackgroundThread.ensureBackground();
				boolean res = findTextInternal( pattern, 1, reverse?1:0, caseInsensitive?1:0);
				if ( !res )
					res = findTextInternal( pattern, -1, reverse?1:0, caseInsensitive?1:0);
				if ( !res ) {
					clearSelectionInternal();
					throw new Exception("pattern not found");
				}
			}
			public void done() {
				BackgroundThread.ensureGUI();
				drawPage();
				FindNextDlg dlg = new FindNextDlg( mActivity, view, pattern, caseInsensitive );
				// TODO: remove hardcoded position
				dlg.showAtLocation(view, Gravity.NO_GRAVITY, 0, 30);
			}
			public void fail(Exception e) {
				BackgroundThread.ensureGUI();
				mActivity.showToast("Pattern not found");
			}
			
		});
    }
    
    public void findNext( final String pattern, final boolean reverse, final boolean caseInsensitive )
    {
		BackgroundThread.ensureGUI();
		mEngine.execute(new Task() {
			public void work() throws Exception {
				BackgroundThread.ensureBackground();
				boolean res = findTextInternal( pattern, 1, reverse?1:0, caseInsensitive?1:0);
				if ( !res )
					res = findTextInternal( pattern, -1, reverse?1:0, caseInsensitive?1:0);
				if ( !res ) {
					clearSelectionInternal();
					throw new Exception("pattern not found");
				}
			}
			public void done() {
				BackgroundThread.ensureGUI();
				drawPage();
			}
		});
    }
    
    public void clearSelection()
    {
		BackgroundThread.ensureGUI();
		mEngine.execute(new Task() {
			public void work() throws Exception {
				BackgroundThread.ensureBackground();
				clearSelectionInternal();
			}
			public void done() {
				BackgroundThread.ensureGUI();
				drawPage();
			}
		});
    }

    public void goToBookmark( Bookmark bm )
	{
		BackgroundThread.ensureGUI();
		final String pos = bm.getStartPos();
		mEngine.execute(new Task() {
			public void work() {
				BackgroundThread.ensureBackground();
				goToPositionInternal(pos);
			}
			public void done() {
				BackgroundThread.ensureGUI();
				drawPage();
			}
		});
	}
	
	public boolean goToBookmark( final int shortcut )
	{
		BackgroundThread.ensureGUI();
		if ( mBookInfo!=null ) {
			Bookmark bm = mBookInfo.findShortcutBookmark(shortcut);
			if ( bm==null ) {
				addBookmark(shortcut);
				return true;
			} else {
				// go to bookmark
				goToBookmark( bm );
				return false;
			}
		}
		return false;
	}
	
	public void addBookmark( final int shortcut )
	{
		BackgroundThread.ensureGUI();
		// set bookmark instead
		mEngine.execute(new Task() {
			Bookmark bm;
			public void work() {
				BackgroundThread.ensureBackground();
				if ( mBookInfo!=null ) {
					bm = getCurrentPageBookmarkInternal();
					bm.setShortcut(shortcut);
				}
			}
			public void done() {
				if ( mBookInfo!=null && bm!=null ) {
					mBookInfo.setShortcutBookmark(shortcut, bm);
					mActivity.getDB().save(mBookInfo);
					mActivity.showToast("Bookmark " + (shortcut==10?0:shortcut) + " is set.");
				}
			}
		});
	}
	
	public void doCommand( final ReaderCommand cmd, final int param )
	{
		BackgroundThread.ensureGUI();
		Log.d("cr3", "doCommand("+cmd + ", " + param +")");
		execute(new Task() {
			boolean res;
			public void work() {
				BackgroundThread.ensureBackground();
				res = doCommandInternal(cmd.nativeId, param);
			}
			public void done() {
				if ( res )
					drawPage();
			}
		});
	}
	
	public void doCommandFromBackgroundThread( final ReaderCommand cmd, final int param )
	{
		Log.d("cr3", "doCommandFromBackgroundThread("+cmd + ", " + param +")");
		BackgroundThread.ensureBackground();
		boolean res = doCommandInternal(cmd.nativeId, param);
		if ( res ) {
			BackgroundThread.guiExecutor.execute(new Runnable() {
				public void run() {
					drawPage();
				}
			});
		}
	}
	
	private boolean mInitialized = false;
	private boolean mOpened = false;
	
	//private File historyFile;
	
	private void updateLoadedBookInfo()
	{
		BackgroundThread.ensureBackground();
		// get title, authors, etc.
		updateBookInfoInternal( mBookInfo );
	}
	
	private void applySettings( Properties props )
	{
		BackgroundThread.ensureBackground();
		Log.v("cr3", "applySettings() " + props);
        applySettingsInternal(props);
        syncViewSettings(props);
        drawPage();
	}
	
	File propsFile;
	private void saveSettings( Properties settings )
	{
		try {
			Log.v("cr3", "saveSettings() " + settings);
    		FileOutputStream os = new FileOutputStream(propsFile);
    		settings.store(os, "Cool Reader 3 settings");
			Log.i("cr3", "Settings successfully saved to file " + propsFile.getAbsolutePath());
		} catch ( Exception e ) {
			Log.e("cr3", "exception while saving settings", e);
		}
	}

	public static boolean eq(Object obj1, Object obj2)
	{
		if ( obj1==null && obj2==null )
			return true;
		if ( obj1==null || obj2==null )
			return false;
		return obj1.equals(obj2);
	}

	/**
	 * Read JNI view settings, update and save if changed 
	 */
	private void syncViewSettings( final Properties currSettings )
	{
		execute( new Task() {
			Properties props;
			public void work() {
				BackgroundThread.ensureBackground();
				java.util.Properties internalProps = getSettingsInternal(); 
				props = new Properties(internalProps);
			}
			public void done() {
				Properties changedSettings = props.diff(currSettings);
		        for ( Map.Entry<Object, Object> entry : changedSettings.entrySet() ) {
	        		currSettings.setProperty((String)entry.getKey(), (String)entry.getValue());
		        }
	        	mSettings = currSettings;
	        	saveSettings(currSettings);
			}
		});
	}
	
	public Properties getSettings()
	{
		return new Properties(mSettings);
	}
	
	public void applyAppSetting( String key, String value )
	{
        if ( key.equals(PROP_APP_FULLSCREEN) ) {
			this.mActivity.setFullscreen( "1".equals(value) );
        }
	}
	
	public void setAppSettings( Properties newSettings, Properties oldSettings )
	{
		Log.v("cr3", "setAppSettings() " + newSettings.toString());
		BackgroundThread.ensureGUI();
		if ( oldSettings==null )
			oldSettings = mSettings;
		Properties changedSettings = newSettings.diff(oldSettings);
        for ( Map.Entry<Object, Object> entry : changedSettings.entrySet() ) {
    		String key = (String)entry.getKey();
    		String value = (String)entry.getValue();
    		applyAppSetting( key, value );
    		if ( PROP_APP_FULLSCREEN.equals(key) ) {
    			boolean flg = mSettings.getBool(PROP_APP_FULLSCREEN, false);
    			newSettings.setBool(PROP_SHOW_BATTERY, flg); 
    			newSettings.setBool(PROP_SHOW_TIME, flg); 
    		}
        }
	}
	
	/**
     * Change settings.
	 * @param newSettings are new settings
	 * @param oldSettings are old settings, null to use mSettings
	 */
	public void setSettings(Properties newSettings, Properties oldSettings)
	{
		Log.v("cr3", "setSettings() " + newSettings.toString());
		BackgroundThread.ensureGUI();
		if ( oldSettings==null )
			oldSettings = mSettings;
		final Properties currSettings = new Properties(oldSettings);
		setAppSettings( newSettings, currSettings );
		Properties changedSettings = newSettings.diff(currSettings);
		currSettings.setAll(changedSettings);
    	mBackThread.executeBackground(new Runnable() {
    		public void run() {
    			applySettings(currSettings);
    		}
    	});
//        }
	}

	private Properties loadSettings()
	{
        Properties props = new Properties();
		File propsDir = mActivity.getDir("settings", Context.MODE_PRIVATE);
		propsDir.mkdirs();
		propsFile = new File( propsDir, "cr3.ini");
        if ( propsFile.exists() && !DEBUG_RESET_OPTIONS ) {
        	try {
        		FileInputStream is = new FileInputStream(propsFile);
        		props.load(is);
        		Log.v("cr3", "" + props.size() + " settings items loaded from file " + propsFile.getAbsolutePath() );
        	} catch ( Exception e ) {
        		Log.e("cr3", "error while reading settings");
        	}
        }
        props.applyDefault(PROP_FONT_SIZE, "1208");
        props.applyDefault(PROP_FONT_FACE, "Droid Sans");
        props.setProperty(PROP_STATUS_FONT_FACE, "Droid Sans");
        props.setProperty(PROP_STATUS_FONT_SIZE, "14");
        props.applyDefault(PROP_APP_FULLSCREEN, "0");
		props.applyDefault(PROP_APP_FULLSCREEN, "0");
		props.applyDefault(PROP_SHOW_BATTERY, "0"); 
		props.applyDefault(PROP_SHOW_TIME, "0");
		props.applyDefault(PROP_FONT_ANTIALIASING, "2");
		props.setProperty(PROP_MIN_FILE_SIZE_TO_CACHE, "100000");
		props.setProperty(PROP_FORCED_MIN_FILE_SIZE_TO_CACHE, "32768");
		return props;
	}
	
	private static boolean DEBUG_RESET_OPTIONS = false;
	class CreateViewTask extends Task
	{
        Properties props = new Properties();
        public CreateViewTask() {
       		props = loadSettings();
       		Properties oldSettings = new Properties(); // may be changed by setAppSettings 
   			setAppSettings(props, oldSettings);
   			props.setAll(oldSettings);
       		mSettings = props;
        }
		public void work() throws Exception {
			BackgroundThread.ensureBackground();
			Log.d("cr3", "CreateViewTask - in background thread");
			createInternal();
			//File historyDir = activity.getDir("settings", Context.MODE_PRIVATE);
			//File historyDir = new File(Environment.getExternalStorageDirectory(), ".cr3");
			//historyDir.mkdirs();
			//File historyFile = new File(historyDir, "cr3hist.ini");
			
			//File historyFile = new File(activity.getDir("settings", Context.MODE_PRIVATE), "cr3hist.ini");
			//if ( historyFile.exists() ) {
			//Log.d("cr3", "Reading history from file " + historyFile.getAbsolutePath());
			//readHistoryInternal(historyFile.getAbsolutePath());
			//}
	        String css = mEngine.loadResourceUtf8(R.raw.fb2);
	        if ( css!=null && css.length()>0 )
       			setStylesheetInternal(css);
   			applySettings(props);
   			mInitialized = true;
		}
		public void done() {
			Log.d("cr3", "InitializationFinishedEvent");
			BackgroundThread.ensureGUI();
	        setSettings(props, new Properties());
		}
		public void fail( Exception e )
		{
			Log.e("cr3", "CoolReader engine initialization failed. Exiting.", e);
			mEngine.fatalError("Failed to init CoolReader engine");
		}
	}
	
	public void loadDocument( final FileInfo fileInfo )
	{
		if ( this.mBookInfo!=null && this.mBookInfo.getFileInfo().pathname.equals(fileInfo.pathname) && mOpened ) {
			Log.d("cr3", "trying to load already opened document");
			mActivity.showReader();
			return;
		}
		execute(new LoadDocumentTask(fileInfo, null));
	}

	public boolean loadLastDocument( final Runnable errorHandler )
	{
		BackgroundThread.ensureGUI();
		Log.i("cr3", "Submitting LastDocumentLoadTask");
		init();
		BookInfo book = mActivity.getHistory().getLastBook();
		if ( book==null ) {
			errorHandler.run();
			return false;
		}
		execute( new LoadDocumentTask(book.getFileInfo(), errorHandler) );
		return true;
	}
	
	public BookInfo getBookInfo() {
		BackgroundThread.ensureGUI();
		return mBookInfo;
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
	private int mBatteryState = 100;
	public void setBatteryState( int state ) {
		mBatteryState = state;
		drawPage();
	}
	
	private static class BitmapFactory {
		public static final int MAX_FREE_LIST_SIZE=1;
		ArrayList<Bitmap> freeList = new ArrayList<Bitmap>(); 
		ArrayList<Bitmap> usedList = new ArrayList<Bitmap>(); 
		public synchronized Bitmap get( int dx, int dy ) {
			for ( int i=0; i<freeList.size(); i++ ) {
				Bitmap bmp = freeList.get(i);
				if ( bmp.getWidth()==dx && bmp.getWidth()==dy ) {
					// found bitmap of proper size
					freeList.remove(i);
					usedList.add(bmp);
					return bmp;
				}
			}
			for ( int i=freeList.size()-1; i>=0; i-- ) {
				Bitmap bmp = freeList.remove(i);
				bmp.recycle(); 
			}
			Bitmap bmp = Bitmap.createBitmap(dx, dy, Bitmap.Config.ARGB_8888);
			usedList.add(bmp);
			return bmp;
		}
		public synchronized void compact() {
			while ( freeList.size()>0 ) {
				freeList.get(0).recycle();
				freeList.remove(0);
			}
		}
		public synchronized void release( Bitmap bmp ) {
			for ( int i=0; i<usedList.size(); i++ ) {
				if ( usedList.get(i)==bmp ) {
					freeList.add(bmp);
					usedList.remove(i);
					while ( freeList.size()>MAX_FREE_LIST_SIZE ) {
						freeList.get(0).recycle();
						freeList.remove(0);
					}
					return;
				}
			}
			// unknown bitmap, just recycle
			bmp.recycle();
		}
	};
	BitmapFactory factory = new BitmapFactory(); 
	
	private static final boolean GC_PAGE_IMAGE = true;
	private Bitmap preparePageImage()
	{
		BackgroundThread.ensureBackground();
		Bitmap bitmap;
		if ( internalDX==0 || internalDY==0 ) {
			internalDX=200;
			internalDY=300;
	        resizeInternal(internalDX, internalDY);
		}
		bitmap = factory.get(internalDX, internalDY);
        setBatteryStateInternal(mBatteryState);
        getPageImage(bitmap);
        if ( GC_PAGE_IMAGE )
        	System.gc();
        return bitmap;
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
			BackgroundThread.ensureBackground();
			if ( this.id!=lastDrawTaskId ) {
				Log.d("cr3", "skipping duplicate drawPage request");
				return;
			}
			Log.e("cr3", "DrawPageTask.work("+internalDX+","+internalDY+")");
			bitmap = preparePageImage();
	        mEngine.hideProgress();
		}
		public void done()
		{
			BackgroundThread.ensureGUI();
			Log.d("cr3", "drawPage : bitmap is ready, invalidating view to draw new bitmap");
			setBitmap( bitmap );
//    		if (mOpened)
//    			mEngine.hideProgress();
    		invalidate();
		}
	};
	
	private void setBitmap(Bitmap bmp)
	{
		if ( mBitmap!=null )
			factory.release(mBitmap);
		mBitmap = bmp;
	} 
	
	private void drawPage()
	{
		if ( !mInitialized || !mOpened )
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
			BackgroundThread.ensureBackground();
			if ( this.id != lastResizeTaskId ) {
				Log.d("cr3", "skipping duplicate resize request");
				return;
			}
	        internalDX = dx;
	        internalDY = dy;
			Log.d("cr3", "ResizeTask: resizeInternal("+dx+","+dy+")");
	        resizeInternal(dx, dy);
	        drawPage();
		}
	}
	
	private class LoadDocumentTask extends Task
	{
		String filename;
		Runnable errorHandler;
		LoadDocumentTask( FileInfo fileInfo, Runnable errorHandler )
		{
			BackgroundThread.ensureGUI();
			this.filename = fileInfo.pathname;
			this.errorHandler = errorHandler;
			//FileInfo fileInfo = new FileInfo(filename);
			mBookInfo = mActivity.getHistory().getOrCreateBookInfo( fileInfo );
    		//mBitmap = null;
	        mEngine.showProgress( 1000, R.string.progress_loading );
	        //init();
		}

		public void work() throws IOException {
			BackgroundThread.ensureBackground();
			Log.i("cr3", "Loading document " + filename);
	        boolean success = loadDocumentInternal(filename);
	        if ( success ) {
	        	preparePageImage();
	        	updateLoadedBookInfo();
				Log.i("cr3", "Document " + filename + " is loaded successfully");
	        } else {
				Log.e("cr3", "Error occured while trying to load document " + filename);
				throw new IOException("Cannot read document");
	        }
		}
		public void done()
		{
			BackgroundThread.ensureGUI();
			Log.d("cr3", "LoadDocumentTask is finished successfully");
	        restorePosition();
	        mOpened = true;
			mActivity.showReader();
	        drawPage();
		}
		public void fail( Exception e )
		{
			BackgroundThread.ensureGUI();
			mActivity.getHistory().removeBookInfo( mBookInfo );
			mBookInfo = null;
			Log.d("cr3", "LoadDocumentTask is finished with exception " + e.getMessage());
	        mOpened = true;
			drawPage();
			mEngine.hideProgress();
			mActivity.showToast("Error while loading document");
			if ( errorHandler!=null ) {
				errorHandler.run();
			}
		}
	}
	
    @Override 
    protected void onDraw(Canvas canvas) {
    	try {
    		Log.d("cr3", "onDraw() called");
    		if ( mInitialized && mBitmap!=null ) {
        		Log.d("cr3", "onDraw() -- drawing page image");
        		Rect rc = new Rect(0, 0, mBitmap.getWidth(), mBitmap.getHeight());
    			canvas.drawBitmap(mBitmap, rc, rc, null);
    		} else {
        		Log.d("cr3", "onDraw() -- drawing empty screen");
    			canvas.drawColor(Color.rgb(192, 192, 192));
    		}
    	} catch ( Exception e ) {
    		Log.e("cr3", "exception while drawing", e);
    	}
    }

    private void restorePosition()
    {
		BackgroundThread.ensureGUI();
    	if ( mBookInfo!=null && mBookInfo.getLastPosition()!=null ) {
    		final String pos = mBookInfo.getLastPosition().getStartPos();
    		execute( new Task() {
    			public void work() {
    				BackgroundThread.ensureBackground();
    	    		goToPositionInternal( pos );
    			}
    		});
    		mActivity.getHistory().updateBookAccess(mBookInfo);
    		mActivity.getHistory().saveToDB();
    	}
    }
    
//    private void savePosition()
//    {
//		BackgroundThread.ensureBackground();
//    	if ( !mOpened )
//    		return;
//    	Bookmark bmk = getCurrentPageBookmarkInternal();
//    	if ( bmk!=null )
//    		Log.d("cr3", "saving position, bmk=" + bmk.getStartPos());
//    	else
//    		Log.d("cr3", "saving position: no current page bookmark obtained");
//    	if ( bmk!=null && mBookInfo!=null ) {
//        	bmk.setTimeStamp(System.currentTimeMillis());
//    		bmk.setType(Bookmark.TYPE_LAST_POSITION);
//    		mBookInfo.setLastPosition(bmk);
//    		mActivity.getHistory().updateRecentDir();
//    		mActivity.getHistory().saveToDB();
//    		saveSettings();
//    	}
//    }
    
    private class SavePositionTask extends Task {

    	Bookmark bmk;
    	
		@Override
		public void done() {
	    	if ( bmk!=null && mBookInfo!=null ) {
	        	bmk.setTimeStamp(System.currentTimeMillis());
	    		bmk.setType(Bookmark.TYPE_LAST_POSITION);
	    		mBookInfo.setLastPosition(bmk);
	    		mActivity.getHistory().updateRecentDir();
	    		mActivity.getHistory().saveToDB();
	    	}
		}

		public void work() throws Exception {
			BackgroundThread.ensureBackground();
	    	if ( !mOpened )
	    		return;
	    	bmk = getCurrentPageBookmarkInternal();
	    	if ( bmk!=null )
	    		Log.d("cr3", "saving position, bmk=" + bmk.getStartPos());
	    	else
	    		Log.d("cr3", "saving position: no current page bookmark obtained");
		}
    	
    }

    public void save()
    {
		BackgroundThread.ensureGUI();
    	execute( new SavePositionTask() );
    }
    
    public void close()
    {
		BackgroundThread.ensureGUI();
    	Log.i("cr3", "ReaderView.close() is called");
    	if ( !mOpened )
    		return;
		//save();
    	execute( new Task() {
    		public void work() {
    			BackgroundThread.ensureBackground();
    			if ( mOpened ) {
	    			mOpened = false;
					Log.i("cr3", "ReaderView().close() : closing current document");
					doCommandInternal(ReaderCommand.DCMD_CLOSE_BOOK.nativeId, 0);
    			}
    		}
    		public void done() {
    			BackgroundThread.ensureGUI();
    			setBitmap(null);
    			factory.compact();
    			mBitmap = null;
    		}
    	});
    }

    public void destroy()
    {
    	Log.i("cr3", "ReaderView.destroy() is called");
		BackgroundThread.ensureGUI();
    	if ( mInitialized ) {
        	//close();
        	BackgroundThread.backgroundExecutor.execute( new Runnable() {
        		public void run() {
        			BackgroundThread.ensureBackground();
        	    	if ( mInitialized ) {
        	    		destroyInternal();
        	    		mInitialized = false;
        	    	}
        		}
        	});
    		//engine.waitTasksCompletion();
    	}
    }
    
    @Override
	protected void onDetachedFromWindow() {
		// TODO Auto-generated method stub
		super.onDetachedFromWindow();
		Log.d("cr3", "View.onDetachedFromWindow() is called");
	}

	private String getCSSForFormat( DocumentFormat fileFormat )
	{
		if ( fileFormat==null )
			fileFormat = DocumentFormat.FB2;
		File[] dataDirs = Engine.getDataDirectories(null, false);
		for ( File dir : dataDirs ) {
			File file = new File( dir, fileFormat.getCssName() );
			if ( file.exists() ) {
				String css = mEngine.loadFileUtf8(file);
				if ( css!=null )
					return css;
			} 
		}
		String s = mEngine.loadResourceUtf8(fileFormat.getCSSResourceId());
		return s;
	} 

	boolean enable_progress_callback = true;
    ReaderCallback readerCallback = new ReaderCallback() {
    
	    public boolean OnExportProgress(int percent) {
			BackgroundThread.ensureBackground();
	    	Log.d("cr3", "readerCallback.OnExportProgress " + percent);
			return true;
		}
		public void OnExternalLink(String url, String nodeXPath) {
			BackgroundThread.ensureBackground();
		}
		public void OnFormatEnd() {
			BackgroundThread.ensureBackground();
	    	Log.d("cr3", "readerCallback.OnFormatEnd");
		}
		public boolean OnFormatProgress(final int percent) {
			BackgroundThread.ensureBackground();
			if ( enable_progress_callback )
			executeSync( new Callable<Object>() {
				public Object call() {
					BackgroundThread.ensureGUI();
			    	Log.d("cr3", "readerCallback.OnFormatProgress " + percent);
			    	mEngine.showProgress( percent*4/10 + 5000, R.string.progress_formatting);
			    	return null;
				}
			});
			return true;
		}
		public void OnFormatStart() {
			BackgroundThread.ensureBackground();
	    	Log.d("cr3", "readerCallback.OnFormatStart");
		}
		public void OnLoadFileEnd() {
			BackgroundThread.ensureBackground();
	    	Log.d("cr3", "readerCallback.OnLoadFileEnd");
		}
		public void OnLoadFileError(String message) {
			BackgroundThread.ensureBackground();
	    	Log.d("cr3", "readerCallback.OnLoadFileError(" + message + ")");
		}
		public void OnLoadFileFirstPagesReady() {
			BackgroundThread.ensureBackground();
	    	Log.d("cr3", "readerCallback.OnLoadFileFirstPagesReady");
		}
		public String OnLoadFileFormatDetected(final DocumentFormat fileFormat) {
			BackgroundThread.ensureBackground();
			String res = executeSync( new Callable<String>() {
				public String call() {
					BackgroundThread.ensureGUI();
					Log.i("cr3", "readerCallback.OnLoadFileFormatDetected " + fileFormat);
					if ( fileFormat!=null ) {
						String s = getCSSForFormat(fileFormat);
						Log.i("cr3", "setting .css for file format " + fileFormat + " from resource " + (fileFormat!=null?fileFormat.getCssName():"[NONE]"));
						return s;
					}
			    	return null;
				}
			});
			return res;
		}
		public boolean OnLoadFileProgress(final int percent) {
			BackgroundThread.ensureBackground();
			if ( enable_progress_callback )
			executeSync( new Callable<Object>() {
				public Object call() {
					BackgroundThread.ensureGUI();
			    	Log.d("cr3", "readerCallback.OnLoadFileProgress " + percent);
			    	mEngine.showProgress( percent*4/10 + 1000, R.string.progress_loading);
			    	return null;
				}
			});
			return true;
		}
		public void OnLoadFileStart(String filename) {
			BackgroundThread.ensureBackground();
	    	Log.d("cr3", "readerCallback.OnLoadFileStart " + filename);
		}
    };

    public void setStyleSheet( final String css )
    {
		BackgroundThread.ensureGUI();
        if ( css!=null && css.length()>0 ) {
        	execute(new Task() {
        		public void work() {
        			setStylesheetInternal(css);
        		}
        	});
        }
    }
    
    public void goToPage( int pageNumber )
    {
		BackgroundThread.ensureGUI();
		doCommand(ReaderView.ReaderCommand.DCMD_GO_PAGE, pageNumber-1);
    }
    
    public void goToPercent( final int percent )
    {
		BackgroundThread.ensureGUI();
    	if ( percent>=0 && percent<=100 )
	    	execute( new Task() {
	    		public void work() {
	    			PositionProperties pos = getPositionPropsInternal(null);
	    			if ( pos!=null && pos.pageCount>0) {
	    				int pageNumber = pos.pageCount * percent / 100; 
						doCommandFromBackgroundThread(ReaderView.ReaderCommand.DCMD_GO_PAGE, pageNumber);
	    			}
	    		}
	    	});
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
    	initStarted = true;
   		execute(new CreateViewTask());
    }
    
	public ReaderView(CoolReader activity, Engine engine, BackgroundThread backThread) 
    {
        super(activity);
		BackgroundThread.ensureGUI();
        this.mActivity = activity;
        this.mEngine = engine;
        this.mBackThread = backThread;
        setFocusable(true);
        setFocusableInTouchMode(true);
    }

}
