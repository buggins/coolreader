package org.coolreader.crengine;

import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.Callable;

import org.coolreader.CoolReader;
import org.coolreader.R;
import org.coolreader.crengine.Engine.HyphDict;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.text.ClipboardManager;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class ReaderView extends SurfaceView implements android.view.SurfaceHolder.Callback {

	public static final Logger log = L.create("rv");
	
    // additional key codes for Nook
    public static final int NOOK_KEY_PREV_LEFT = 96;
    public static final int NOOK_KEY_PREV_RIGHT = 98;
    public static final int NOOK_KEY_NEXT_LEFT = 95;
    public static final int NOOK_KEY_NEXT_RIGHT = 97;    
    public static final int NOOK_KEY_SHIFT_UP = 101;
    public static final int NOOK_KEY_SHIFT_DOWN = 100;
    // Nook touch buttons
    public static final int KEYCODE_PAGE_BOTTOMLEFT = 0x5d; // fwd
    public static final int KEYCODE_PAGE_BOTTOMRIGHT = 0x5f; // fwd
    public static final int KEYCODE_PAGE_TOPLEFT = 0x5c; // back
    public static final int KEYCODE_PAGE_TOPRIGHT = 0x5e; // back
    
    public static final String PROP_PAGE_BACKGROUND_IMAGE       ="background.image";
    public static final String PROP_PAGE_BACKGROUND_IMAGE_DAY   ="background.image.day";
    public static final String PROP_PAGE_BACKGROUND_IMAGE_NIGHT ="background.image.night";
    public static final String PROP_NIGHT_MODE              ="crengine.night.mode";
    public static final String PROP_FONT_COLOR_DAY          ="font.color.day";
    public static final String PROP_BACKGROUND_COLOR_DAY    ="background.color.day";
    public static final String PROP_FONT_COLOR_NIGHT        ="font.color.night";
    public static final String PROP_BACKGROUND_COLOR_NIGHT  ="background.color.night";
    public static final String PROP_FONT_COLOR              ="font.color.default";
    public static final String PROP_BACKGROUND_COLOR        ="background.color.default";
    public static final String PROP_FONT_ANTIALIASING       ="font.antialiasing.mode";
    public static final String PROP_FONT_FACE               ="font.face.default";
    public static final String PROP_FONT_GAMMA              ="font.gamma";
    public static final String PROP_FONT_WEIGHT_EMBOLDEN    ="font.face.weight.embolden";
    public static final String PROP_TXT_OPTION_PREFORMATTED ="crengine.file.txt.preformatted";
    public static final String PROP_LOG_FILENAME            ="crengine.log.filename";
    public static final String PROP_LOG_LEVEL               ="crengine.log.level";
    public static final String PROP_LOG_AUTOFLUSH           ="crengine.log.autoflush";
    public static final String PROP_FONT_SIZE               ="crengine.font.size";
    public static final String PROP_FALLBACK_FONT_FACE      ="crengine.font.fallback.face";
    public static final String PROP_STATUS_FONT_COLOR       ="crengine.page.header.font.color";
    public static final String PROP_STATUS_FONT_COLOR_DAY   ="crengine.page.header.font.color.day";
    public static final String PROP_STATUS_FONT_COLOR_NIGHT ="crengine.page.header.font.color.night";
    public static final String PROP_STATUS_FONT_FACE        ="crengine.page.header.font.face";
    public static final String PROP_STATUS_FONT_SIZE        ="crengine.page.header.font.size";
    public static final String PROP_STATUS_CHAPTER_MARKS    ="crengine.page.header.chapter.marks";
    public static final String PROP_PAGE_MARGIN_TOP         ="crengine.page.margin.top";
    public static final String PROP_PAGE_MARGIN_BOTTOM      ="crengine.page.margin.bottom";
    public static final String PROP_PAGE_MARGIN_LEFT        ="crengine.page.margin.left";
    public static final String PROP_PAGE_MARGIN_RIGHT       ="crengine.page.margin.right";
    public static final String PROP_PAGE_VIEW_MODE          ="crengine.page.view.mode"; // pages/scroll
    public static final String PROP_PAGE_ANIMATION          ="crengine.page.animation";
    public static final String PROP_INTERLINE_SPACE         ="crengine.interline.space";
    public static final String PROP_ROTATE_ANGLE            ="window.rotate.angle";
    public static final String PROP_EMBEDDED_STYLES         ="crengine.doc.embedded.styles.enabled";
    public static final String PROP_DISPLAY_INVERSE         ="crengine.display.inverse";
//    public static final String PROP_DISPLAY_FULL_UPDATE_INTERVAL ="crengine.display.full.update.interval";
//    public static final String PROP_DISPLAY_TURBO_UPDATE_MODE ="crengine.display.turbo.update";
    public static final String PROP_STATUS_LINE             ="window.status.line";
    public static final String PROP_BOOKMARK_ICONS          ="crengine.bookmarks.icons";
    public static final String PROP_FOOTNOTES               ="crengine.footnotes";
    public static final String PROP_SHOW_TIME               ="window.status.clock";
    public static final String PROP_SHOW_TITLE              ="window.status.title";
    public static final String PROP_SHOW_BATTERY            ="window.status.battery";
    public static final String PROP_SHOW_BATTERY_PERCENT    ="window.status.battery.percent";
    public static final String PROP_SHOW_POS_PERCENT        ="window.status.pos.percent";
    public static final String PROP_SHOW_PAGE_COUNT         ="window.status.pos.page.count";
    public static final String PROP_SHOW_PAGE_NUMBER        ="window.status.pos.page.number";
    public static final String PROP_FONT_KERNING_ENABLED    ="font.kerning.enabled";
    public static final String PROP_FLOATING_PUNCTUATION    ="crengine.style.floating.punctuation.enabled";
    public static final String PROP_LANDSCAPE_PAGES         ="window.landscape.pages";
    public static final String PROP_HYPHENATION_DICT        ="crengine.hyphenation.dictionary.code"; // non-crengine
    public static final String PROP_AUTOSAVE_BOOKMARKS      ="crengine.autosave.bookmarks";

	 // image scaling settings
	 // mode: 0=disabled, 1=integer scaling factors, 2=free scaling
	 // scale: 0=auto based on font size, 1=no zoom, 2=scale up to *2, 3=scale up to *3
    public static final String PROP_IMG_SCALING_ZOOMIN_INLINE_MODE = "crengine.image.scaling.zoomin.inline.mode";
    public static final String PROP_IMG_SCALING_ZOOMIN_INLINE_SCALE = "crengine.image.scaling.zoomin.inline.scale";
    public static final String PROP_IMG_SCALING_ZOOMOUT_INLINE_MODE = "crengine.image.scaling.zoomout.inline.mode";
    public static final String PROP_IMG_SCALING_ZOOMOUT_INLINE_SCALE = "crengine.image.scaling.zoomout.inline.scale";
    public static final String PROP_IMG_SCALING_ZOOMIN_BLOCK_MODE = "crengine.image.scaling.zoomin.block.mode";
    public static final String PROP_IMG_SCALING_ZOOMIN_BLOCK_SCALE = "crengine.image.scaling.zoomin.block.scale";
    public static final String PROP_IMG_SCALING_ZOOMOUT_BLOCK_MODE = "crengine.image.scaling.zoomout.block.mode";
    public static final String PROP_IMG_SCALING_ZOOMOUT_BLOCK_SCALE = "crengine.image.scaling.zoomout.block.scale";
    
    public static final String PROP_MIN_FILE_SIZE_TO_CACHE  ="crengine.cache.filesize.min";
    public static final String PROP_FORCED_MIN_FILE_SIZE_TO_CACHE  ="crengine.cache.forced.filesize.min";
    public static final String PROP_PROGRESS_SHOW_FIRST_PAGE="crengine.progress.show.first.page";

    public static final String PROP_CONTROLS_ENABLE_VOLUME_KEYS ="app.controls.volume.keys.enabled";
    
    public static final String PROP_APP_FULLSCREEN          ="app.fullscreen";
    public static final String PROP_APP_BOOK_PROPERTY_SCAN_ENABLED ="app.browser.fileprops.scan.enabled";
    public static final String PROP_APP_SHOW_COVERPAGES     ="app.browser.coverpages";
    public static final String PROP_APP_SCREEN_ORIENTATION  ="app.screen.orientation";
    public static final String PROP_APP_SCREEN_BACKLIGHT    ="app.screen.backlight";
    public static final String PROP_APP_SCREEN_BACKLIGHT_DAY   ="app.screen.backlight.day";
    public static final String PROP_APP_SCREEN_BACKLIGHT_NIGHT ="app.screen.backlight.night";
    public static final String PROP_APP_DOUBLE_TAP_SELECTION     ="app.controls.doubletap.selection";
    public static final String PROP_APP_TAP_ZONE_ACTIONS_TAP     ="app.tapzone.action.tap";
    public static final String PROP_APP_KEY_ACTIONS_PRESS     ="app.key.action.press";
    public static final String PROP_APP_TRACKBALL_DISABLED    ="app.trackball.disabled";
    public static final String PROP_APP_SCREEN_BACKLIGHT_LOCK    ="app.screen.backlight.lock.enabled";
    public static final String PROP_APP_TAP_ZONE_HILIGHT     ="app.tapzone.hilight";
    public static final String PROP_APP_FLICK_BACKLIGHT_CONTROL = "app.screen.backlight.control.flick";
    public static final String PROP_APP_BOOK_SORT_ORDER = "app.browser.sort.order";
    public static final String PROP_APP_DICTIONARY = "app.dictionary.current";
    public static final String PROP_APP_SELECTION_ACTION = "app.selection.action";
    public static final String PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS = "app.browser.hide.empty.folders";
    public static final String PROP_APP_FILE_BROWSER_SIMPLE_MODE = "app.browser.simple.mode";

    public static final String PROP_APP_SCREEN_UPDATE_MODE  ="app.screen.update.mode";
    public static final String PROP_APP_SCREEN_UPDATE_INTERVAL  ="app.screen.update.interval";
    
    public static final int PAGE_ANIMATION_NONE = 0;
    public static final int PAGE_ANIMATION_PAPER = 1;
    public static final int PAGE_ANIMATION_SLIDE = 2;
    public static final int PAGE_ANIMATION_SLIDE2 = 3;
    public static final int PAGE_ANIMATION_MAX = 3;
    
    public static final int SELECTION_ACTION_TOOLBAR = 0;
    public static final int SELECTION_ACTION_COPY = 1;
    public static final int SELECTION_ACTION_DICTIONARY = 2;
    public static final int SELECTION_ACTION_BOOKMARK = 3;
    
    public static final int SEL_CMD_SELECT_FIRST_SENTENCE_ON_PAGE = 1;
    public static final int SEL_CMD_NEXT_SENTENCE = 2;
    public static final int SEL_CMD_PREV_SENTENCE = 3;
    
    
    public enum ViewMode
    {
    	PAGES,
    	SCROLL
    }
    
    private ViewMode viewMode = ViewMode.PAGES;
    
    public enum ReaderCommand
    {
    	DCMD_NONE(0),
    	DCMD_REPEAT(1), // repeat last action
    	
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
    	DCMD_SCROLL_BY(127),
    	DCMD_REQUEST_RENDER(128),
    	DCMD_GO_PAGE_DONT_SAVE_HISTORY(129),
    	DCMD_SET_INTERNAL_STYLES(130),
    	
        DCMD_SELECT_FIRST_SENTENCE(131), // select first sentence on page
        DCMD_SELECT_NEXT_SENTENCE(132), // move selection to next sentence
        DCMD_SELECT_PREV_SENTENCE(133), // move selection to next sentence
        DCMD_SELECT_MOVE_LEFT_BOUND_BY_WORDS(134), // move selection start by words 
        DCMD_SELECT_MOVE_RIGHT_BOUND_BY_WORDS(135), // move selection end by words 
    	
    	// definitions from android/jni/readerview.h
    	DCMD_OPEN_RECENT_BOOK(2000),
    	DCMD_CLOSE_BOOK(2001),
    	DCMD_RESTORE_POSITION(2002),

    	// application actions
    	DCMD_RECENT_BOOKS_LIST(2003),
    	DCMD_SEARCH(2004),
    	DCMD_EXIT(2005),
    	DCMD_BOOKMARKS(2005),
    	DCMD_GO_PERCENT_DIALOG(2006),
    	DCMD_GO_PAGE_DIALOG(2007),
    	DCMD_TOC_DIALOG(2008),
    	DCMD_FILE_BROWSER(2009),
    	DCMD_OPTIONS_DIALOG(2010),
    	DCMD_TOGGLE_DAY_NIGHT_MODE(2011),
    	DCMD_READER_MENU(2012),
    	DCMD_TOGGLE_TOUCH_SCREEN_LOCK(2013),
    	DCMD_TOGGLE_SELECTION_MODE(2014),
    	DCMD_TOGGLE_ORIENTATION(2015),
    	DCMD_TOGGLE_FULLSCREEN(2016),
    	DCMD_SHOW_HOME_SCREEN(2017), // home screen activity
    	DCMD_TOGGLE_DOCUMENT_STYLES(2018),
    	DCMD_ABOUT(2019),
    	DCMD_BOOK_INFO(2020),
    	DCMD_TTS_PLAY(2021),
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
    
    private void post( Engine.EngineTask task )
    {
    	mEngine.post(task);
    }
    
    private abstract class Task implements Engine.EngineTask {
    	
		public void done() {
			// override to do something useful
		}

		public void fail(Exception e) {
			// do nothing, just log exception
			// override to do custom action
			log.e("Task " + this.getClass().getSimpleName() + " is failed with exception " + e.getMessage(), e);
		}
    }
    
	static class Sync<T> extends Object {
		private volatile T result = null;
		private volatile boolean completed = false;
		public void set( T res )
		{
			log.d("sync.set() called from " + Thread.currentThread().getName());
			result = res;
			completed = true;
			synchronized(this) {
				notify();
			}
			log.d("sync.set() returned from notify " + Thread.currentThread().getName());
		}
		public T get()
		{
			log.d("sync.get() called from " + Thread.currentThread().getName());
			while ( !completed ) {
    			try {
    				log.d("sync.get() before wait " + Thread.currentThread().getName());
    				synchronized(this) {
    					if ( !completed )
    						wait();
    				}
    				log.d("sync.get() after wait wait " + Thread.currentThread().getName());
    			} catch (InterruptedException e) {
    				log.d("sync.get() exception", e);
    				// ignore
    			} catch (Exception e) {
    				log.d("sync.get() exception", e);
    				// ignore
    			}
			}
			log.d("sync.get() returning " + Thread.currentThread().getName());
			return result;
		}
	}

    private <T> T executeSync( final Callable<T> task )
    {
    	//log.d("executeSync called");
    	
    	
    	final Sync<T> sync = new Sync<T>();
    	post( new Runnable() {
    		public void run() {
    			log.d("executeSync " + Thread.currentThread().getName());
    			try {
    				sync.set( task.call() );
    			} catch ( Exception e ) {
    			}
    		}
    	});
    	T res = sync.get();
    	//log.d("executeSync done");
    	return res;
    }
    
    // Native functions
    /* implementend by libcr3engine.so */
    
    // get current page image
    private native void getPageImageInternal(Bitmap bitmap);
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
    private native byte[] getCoverPageDataInternal();
    private native void setPageBackgroundTextureInternal( byte[] imageBytes, int tileFlags );
    private native void updateSelectionInternal( Selection sel );
    private native boolean moveSelectionInternal( Selection sel, int moveCmd, int params );
    private native String checkLinkInternal( int x, int y, int delta );
    private native int goLinkInternal( String link );

    public static final int SWAP_DONE=0;
    public static final int SWAP_TIMEOUT=1;
    public static final int SWAP_ERROR=2;
    /// returns either SWAP_DONE, SWAP_TIMEOUT or SWAP_ERROR 
    private native int swapToCacheInternal();
    
    
    protected int mNativeObject; // used from JNI
    
	private final CoolReader mActivity;
    private final Engine mEngine;
    private final BackgroundThread mBackThread;
    
    private BookInfo mBookInfo;
    
    private Properties mSettings = new Properties();

    public Engine getEngine()
    {
    	return mEngine;
    }
    
    public CoolReader getActivity()
    {
    	return mActivity;
    }
    
	private int lastResizeTaskId = 0;
	@Override
	protected void onSizeChanged(final int w, final int h, int oldw, int oldh) {
		log.i("onSizeChanged("+w + ", " + h +")");
		super.onSizeChanged(w, h, oldw, oldh);
		final int thisId = ++lastResizeTaskId;
	    if ( w<h && mActivity.isLandscape() ) {
	    	log.i("ignoring size change to portrait since landscape is set");
	    	return;
	    }
//		if ( mActivity.isPaused() ) {
//			log.i("ignoring size change since activity is paused");
//			return;
//		}
		// update size with delay: chance to avoid extra unnecessary resizing
		
	    Runnable task = new Runnable() {
	    	public void run() {
	    		if ( thisId != lastResizeTaskId ) {
					log.d("skipping duplicate resize request in GUI thread");
	    			return;
	    		}
	    		mActivity.getHistory().updateCoverPageSize(w, h);
	    		post(new Task() {
	    			public void work() {
	    				BackgroundThread.ensureBackground();
	    				if ( thisId != lastResizeTaskId ) {
	    					log.d("skipping duplicate resize request");
	    					return;
	    				}
	    		        internalDX = w;
	    		        internalDY = h;
	    				log.d("ResizeTask: resizeInternal("+w+","+h+")");
	    		        resizeInternal(w, h);
//	    		        if ( mOpened ) {
//	    					log.d("ResizeTask: done, drawing page");
//	    			        drawPage();
//	    		        }
	    			}
	    			public void done() {
	    				clearImageCache();
	    				invalidate();
	    			}
	    		});
	    	}
	    };
	    if ( mOpened ) {
	    	log.d("scheduling delayed resize task id="+thisId);
	    	BackgroundThread.instance().postGUI( task, 1500);
	    } else {
	    	log.d("executing resize without delay");
	    	task.run();
	    }
	    
	}
	
	public boolean isBookLoaded()
	{
		BackgroundThread.ensureGUI();
		return mOpened;
	}
	
	public int getOrientation()
	{
		int angle = mSettings.getInt(PROP_APP_SCREEN_ORIENTATION, 0);
		if ( angle==4 )
			angle = mActivity.getOrientationFromSensor();
		return angle;
	}

	private int overrideKey( int keyCode )
	{
		return keyCode;
/*		
		
		int angle = getOrientation();
		int[] subst = new int[] {
			1, 	KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_DPAD_LEFT,
			1, 	KeyEvent.KEYCODE_DPAD_DOWN, KeyEvent.KEYCODE_DPAD_RIGHT,
			1, 	KeyEvent.KEYCODE_DPAD_LEFT, KeyEvent.KEYCODE_DPAD_DOWN,
			1, 	KeyEvent.KEYCODE_DPAD_RIGHT, KeyEvent.KEYCODE_DPAD_UP,
			1, 	KeyEvent.KEYCODE_VOLUME_UP, KeyEvent.KEYCODE_VOLUME_DOWN,
			1, 	KeyEvent.KEYCODE_VOLUME_DOWN, KeyEvent.KEYCODE_VOLUME_UP,
//			2, 	KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_DPAD_DOWN,
//			2, 	KeyEvent.KEYCODE_DPAD_DOWN, KeyEvent.KEYCODE_DPAD_UP,
//			2, 	KeyEvent.KEYCODE_DPAD_LEFT, KeyEvent.KEYCODE_DPAD_RIGHT,
//			2, 	KeyEvent.KEYCODE_DPAD_RIGHT, KeyEvent.KEYCODE_DPAD_LEFT,
//			2, 	KeyEvent.KEYCODE_VOLUME_UP, KeyEvent.KEYCODE_VOLUME_DOWN,
//			2, 	KeyEvent.KEYCODE_VOLUME_DOWN, KeyEvent.KEYCODE_VOLUME_UP,
//			3, 	KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_DPAD_RIGHT,
//			3, 	KeyEvent.KEYCODE_DPAD_DOWN, KeyEvent.KEYCODE_DPAD_LEFT,
//			3, 	KeyEvent.KEYCODE_DPAD_LEFT, KeyEvent.KEYCODE_DPAD_UP,
//			3, 	KeyEvent.KEYCODE_DPAD_RIGHT, KeyEvent.KEYCODE_DPAD_DOWN,
		};
		for ( int i=0; i<subst.length; i+=3 ) {
			if ( angle==subst[i] && keyCode==subst[i+1] )
				return subst[i+2];
		}
		return keyCode;
*/
	}
	
	public int getTapZone( int x, int y, int dx, int dy )
	{
		int x1 = dx / 3;
		int x2 = dx * 2 / 3;
		int y1 = dy / 3;
		int y2 = dy * 2 / 3;
		int zone = 0;
		if ( y<y1 ) {
			if ( x<x1 )
				zone = 1;
			else if ( x<x2 )
				zone = 2;
			else
				zone = 3;
		} else if ( y<y2 ) {
			if ( x<x1 )
				zone = 4;
			else if ( x<x2 )
				zone = 5;
			else
				zone = 6;
		} else {
			if ( x<x1 )
				zone = 7;
			else if ( x<x2 )
				zone = 8;
			else
				zone = 9;
		}
		return zone;
	}
	
	public void onTapZone( int zone, boolean isLongPress )
	{
		ReaderAction action;
		if ( !isLongPress )
			action = ReaderAction.findForTap(zone, mSettings);
		else
			action = ReaderAction.findForLongTap(zone, mSettings);
		if ( action.isNone() )
			return;
		log.d("onTapZone : action " + action.id + " is found for tap zone " + zone + (isLongPress ? " (long)":""));
		onAction( action );
	}
	
	public FileInfo getOpenedFileInfo()
	{
		if ( isBookLoaded() && mBookInfo!=null )
			return mBookInfo.getFileInfo();
		return null;
	}
	
	public final int LONG_KEYPRESS_TIME = 900;
	public final int AUTOREPEAT_KEYPRESS_TIME = 700;
	public final int DOUBLE_CLICK_INTERVAL = 400;
	private ReaderAction currentDoubleClickAction = null;
	private ReaderAction currentSingleClickAction = null;
	private long currentDoubleClickActionStart = 0;
	private int currentDoubleClickActionKeyCode = 0;
	@Override
	public boolean onKeyUp(int keyCode, final KeyEvent event) {
		if ( keyCode==KeyEvent.KEYCODE_VOLUME_DOWN || keyCode==KeyEvent.KEYCODE_VOLUME_UP )
			if ( !enableVolumeKeys )
				return super.onKeyUp(keyCode, event);
		if ( keyCode==KeyEvent.KEYCODE_POWER || keyCode==KeyEvent.KEYCODE_ENDCALL ) {
			mActivity.releaseBacklightControl();
			return false;
		}
		boolean tracked = isTracked(event);
		if ( keyCode!=KeyEvent.KEYCODE_BACK )
			backKeyDownHere = false;
		mActivity.onUserActivity();

		if ( keyCode==KeyEvent.KEYCODE_BACK && !tracked )
			return true;
		backKeyDownHere = false;
		
		// apply orientation
		keyCode = overrideKey( keyCode );
		boolean isLongPress = false;
		Long keyDownTs = keyDownTimestampMap.get(keyCode);
		if ( keyDownTs!=null && System.currentTimeMillis()-keyDownTs>=LONG_KEYPRESS_TIME )
			isLongPress = true;
		ReaderAction action = ReaderAction.findForKey( keyCode, mSettings );
		ReaderAction longAction = ReaderAction.findForLongKey( keyCode, mSettings );
		ReaderAction dblAction = ReaderAction.findForDoubleKey( keyCode, mSettings );
		stopTracking();

		if ( keyCode>=KeyEvent.KEYCODE_0 && keyCode<=KeyEvent.KEYCODE_9 && tracked ) {
			// goto/set shortcut bookmark
			int shortcut = keyCode - KeyEvent.KEYCODE_0;
			if ( shortcut==0 )
				shortcut = 10;
			if ( isLongPress )
				addBookmark(shortcut);
			else
				goToBookmark(shortcut);
			return true;
		}
		if ( action.isNone() || !tracked ) {
			return super.onKeyUp(keyCode, event);
		}
		if ( !action.isNone() && action.canRepeat() && longAction.isRepeat() ) {
			// already processed by onKeyDown()
			return true;
		}
		
		if ( isLongPress ) {
			action = longAction;
		} else {
			if ( !dblAction.isNone() ) {
				// wait for possible double click
				currentDoubleClickActionStart = android.os.SystemClock.uptimeMillis();
				currentDoubleClickAction = dblAction;
				currentSingleClickAction = action;
				currentDoubleClickActionKeyCode = keyCode;
				final int myKeyCode = keyCode;
				BackgroundThread.instance().postGUI(new Runnable() {
					public void run() {
						if ( currentSingleClickAction!=null && currentDoubleClickActionKeyCode==myKeyCode ) {
							log.d("onKeyUp: single click action " + currentSingleClickAction.id + " found for key " + myKeyCode + " single click");
							onAction( currentSingleClickAction );
						}
						currentDoubleClickActionStart = 0;
						currentDoubleClickActionKeyCode = 0;
						currentDoubleClickAction = null;
						currentSingleClickAction = null;
					}
				}, DOUBLE_CLICK_INTERVAL);
				// posted
				return true;
			}
		}
		if ( !action.isNone() ) {
			log.d("onKeyUp: action " + action.id + " found for key " + keyCode + (isLongPress?" (long)" : "") );
			onAction( action );
			return true;
		}
		

		// not processed
		return super.onKeyUp(keyCode, event);
	}

	boolean VOLUME_KEYS_ZOOM = false;
	
	private boolean backKeyDownHere = false;

	
	
	@Override
	protected void onFocusChanged(boolean gainFocus, int direction,
			Rect previouslyFocusedRect) {
		stopTracking();
		super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
	}
	
	private boolean startTrackingKey( KeyEvent event ) {
		if ( event.getRepeatCount()==0 ) {
			stopTracking();
			trackedKeyEvent = event;
			return true;
		}
		return false;
	}
	
	private void stopTracking() {
		trackedKeyEvent = null;
		actionToRepeat = null;
		repeatActionActive = false;
	}

	private boolean isTracked( KeyEvent event ) {
        if ( trackedKeyEvent!=null) {
            int tkeKc = trackedKeyEvent.getKeyCode();
            int eKc = event.getKeyCode();
            // check if tracked key and current key are the same
            if (tkeKc == eKc) {
                long tkeDt = trackedKeyEvent.getDownTime();
                long eDt = event.getDownTime();
                // empirical value (could be changed or moved to constant)
                long delta = 300l;
                // time difference between tracked and current event
                long diff = eDt - tkeDt;
                // needed for correct function on HTC Desire for CENTER_KEY
                if (delta > diff)
                    return true;
            }
            else {
                log.v("isTracked( trackedKeyEvent=" + trackedKeyEvent + ", event=" + event + " )");
            }
        }
		stopTracking();
		return false;
	}

	@Override
	public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event) {
		log.v("onKeyMultiple( keyCode=" + keyCode + ", repeatCount=" + repeatCount + ", event=" + event);
		return super.onKeyMultiple(keyCode, repeatCount, event);
	}


	private KeyEvent trackedKeyEvent = null; 
	private ReaderAction actionToRepeat = null;
	private boolean repeatActionActive = false;
	private Map<Integer, Long> keyDownTimestampMap = new HashMap<Integer, Long>();
	
	@Override
	public boolean onKeyDown(int keyCode, final KeyEvent event) {
		backKeyDownHere = false;
		if ( event.getRepeatCount()==0 ) {
			log.v("onKeyDown("+keyCode + ", " + event +")");
			keyDownTimestampMap.put(keyCode, System.currentTimeMillis());
		}
		if ( keyCode==KeyEvent.KEYCODE_POWER || keyCode==KeyEvent.KEYCODE_ENDCALL ) {
			mActivity.releaseBacklightControl();
			boolean res = super.onKeyDown(keyCode, event);
			mActivity.onUserActivity();
			return res;
		}

    	if ( keyCode==KeyEvent.KEYCODE_VOLUME_UP || keyCode==KeyEvent.KEYCODE_VOLUME_DOWN )
    		if (!enableVolumeKeys) {
    			boolean res = super.onKeyDown(keyCode, event);
    			mActivity.onUserActivity();
    			return res;
    		}
    	
		mActivity.onUserActivity();
		keyCode = overrideKey( keyCode );
		ReaderAction action = ReaderAction.findForKey( keyCode, mSettings );
		ReaderAction longAction = ReaderAction.findForLongKey( keyCode, mSettings );
		//ReaderAction dblAction = ReaderAction.findForDoubleKey( keyCode, mSettings );

		if ( event.getRepeatCount()==0 ) {
			if ( keyCode==currentDoubleClickActionKeyCode && currentDoubleClickActionStart + DOUBLE_CLICK_INTERVAL > android.os.SystemClock.uptimeMillis() ) {
				if ( currentDoubleClickAction!=null ) {
					log.d("executing doubleclick action " + currentDoubleClickAction);
					onAction(currentDoubleClickAction);
				}
				currentDoubleClickActionStart = 0;
				currentDoubleClickActionKeyCode = 0;
				currentDoubleClickAction = null;
				currentSingleClickAction = null;
				return true;
			} else {
				if ( currentSingleClickAction!=null ) {
					onAction(currentSingleClickAction);
				}
				currentDoubleClickActionStart = 0;
				currentDoubleClickActionKeyCode = 0;
				currentDoubleClickAction = null;
				currentSingleClickAction = null;
			}
			
		}
		
		
    	if ( event.getRepeatCount()>0 ) {
    		if ( !isTracked(event) )
    			return true; // ignore
    		// repeating key down
    		boolean isLongPress = (event.getEventTime()-event.getDownTime())>=AUTOREPEAT_KEYPRESS_TIME;
    		if ( isLongPress ) {
	    		if ( actionToRepeat!=null ) {
	    			if ( !repeatActionActive ) {
		    			log.v("autorepeating action : " + actionToRepeat );
		    			repeatActionActive = true;
		    			onAction(actionToRepeat, new Runnable() {
		    				public void run() {
		    					if ( trackedKeyEvent!=null && trackedKeyEvent.getDownTime()==event.getDownTime() ) {
		    						log.v("action is completed : " + actionToRepeat );
		    						repeatActionActive = false;
		    					}
		    				}
		    			});
	    			}
	    		} else {
	    			stopTracking();
	    			log.v("executing action on long press : " + longAction );
	    			onAction(longAction);
	    		}
    		}
    		return true;
    	}
		
		if ( !action.isNone() && action.canRepeat() && longAction.isRepeat() ) {
			// start tracking repeat
			startTrackingKey(event);
			actionToRepeat = action;
			log.v("running action with scheduled autorepeat : " + actionToRepeat );
			repeatActionActive = true;
			onAction(actionToRepeat, new Runnable() {
				public void run() {
					if ( trackedKeyEvent==event ) {
						log.v("action is completed : " + actionToRepeat );
						repeatActionActive = false;
					}
				}
			});
			return true;
		} else {
			actionToRepeat = null;
		}
		
		if ( keyCode>=KeyEvent.KEYCODE_0 && keyCode<=KeyEvent.KEYCODE_9 ) {
			// will process in keyup handler
			startTrackingKey(event);
			return true;
		}
		if ( action.isNone() && longAction.isNone() )
			return super.onKeyDown(keyCode, event);
		startTrackingKey(event);
		return true;
	}
	
	private int nextUpdateId = 0;
	private void updateSelection(int startX, int startY, int endX, int endY, final boolean isUpdateEnd ) {
		final Selection sel = new Selection();
		final int myId = ++nextUpdateId;
		sel.startX = startX;
		sel.startY = startY;
		sel.endX = endX;
		sel.endY = endY;
		mEngine.execute(new Task() {
			@Override
			public void work() throws Exception {
				if ( myId != nextUpdateId && !isUpdateEnd )
					return;
				updateSelectionInternal(sel);
				if ( !sel.isEmpty() ) {
					invalidImages = true;
					BitmapInfo bi = preparePageImage(0);
					if ( bi!=null ) {
						draw();
					}
				}
			}

			@Override
			public void done() {
				if ( isUpdateEnd ) {
					String text = sel.text;
					if ( text!=null && text.length()>0 ) {
						onSelectionComplete( sel );
					} else {
						clearSelection();
					}
				}
			}
		});
	}

	private int mSelectionAction = SELECTION_ACTION_TOOLBAR;
	private void onSelectionComplete( Selection sel ) {
		switch ( mSelectionAction ) {
		case SELECTION_ACTION_TOOLBAR:
			SelectionToolbarDlg.showDialog(mActivity, ReaderView.this, sel);
			break;
		case SELECTION_ACTION_COPY:
			copyToClipboard(sel.text);
			clearSelection();
			break;
		case SELECTION_ACTION_DICTIONARY:
			mActivity.findInDictionary( sel.text );
			clearSelection();
			break;
		case SELECTION_ACTION_BOOKMARK:
			clearSelection();
			showNewBookmarkDialog( sel );
			break;
		default:
			clearSelection();
			break;
		}
		
	}
	
	public void showNewBookmarkDialog( Selection sel ) {
		if ( mBookInfo==null )
			return;
		Bookmark bmk = new Bookmark();
		bmk.setType(Bookmark.TYPE_COMMENT);
		bmk.setPosText(sel.text);
		bmk.setStartPos(sel.startPos);
		bmk.setEndPos(sel.endPos);
		bmk.setPercent(sel.percent);
		bmk.setTitleText(sel.chapter);
		BookmarkEditDialog dlg = new BookmarkEditDialog(mActivity, this, mBookInfo, bmk, true);
		dlg.show();
	}
	
	public void sendQuotationInEmail( Selection sel ) {
        final Intent emailIntent = new Intent(android.content.Intent.ACTION_SEND);
        emailIntent.setType("plain/text");
        emailIntent.putExtra(android.content.Intent.EXTRA_EMAIL, new String[]{ });
        StringBuilder buf = new StringBuilder();
        if (mBookInfo.getFileInfo().authors!=null)
        	buf.append(mBookInfo.getFileInfo().authors + "\n");
        if (mBookInfo.getFileInfo().title!=null)
        	buf.append(mBookInfo.getFileInfo().title + "\n");
        if (sel.chapter!=null && sel.chapter.length()>0)
        	buf.append(sel.chapter + "\n");
    	buf.append(sel.text + "\n");
    	emailIntent.putExtra(android.content.Intent.EXTRA_SUBJECT, mBookInfo.getFileInfo().authors + " " + mBookInfo.getFileInfo().title);
        emailIntent.putExtra(android.content.Intent.EXTRA_TEXT, buf.toString());
		mActivity.startActivity(Intent.createChooser(emailIntent, "Send quotation..."));	
	}
	
	public void copyToClipboard( String text ) {
		if ( text!=null && text.length()>0 ) {
			ClipboardManager cm = mActivity.getClipboardmanager();
			cm.setText(text);
			log.i("Setting clipboard text: " + text);
			mActivity.showToast("Selection text copied to clipboard");
		}
	}
	
	private void cancelSelection() {
		//
		selectionInProgress = false;
		clearSelection();
	}

	private int isBacklightControlFlick = 1;
	private boolean isTouchScreenEnabled = true;
	private boolean isManualScrollActive = false;
	private boolean isBrightnessControlActive = false;
	private int manualScrollStartPosX = -1;
	private int manualScrollStartPosY = -1;
	volatile private boolean touchEventIgnoreNextUp = false;
	volatile private int longTouchId = 0;
	volatile private long currentDoubleTapActionStart = 0;
	private boolean selectionInProgress = false;
	private int selectionStartX = 0;
	private int selectionStartY = 0;
	private int selectionEndX = 0;
	private int selectionEndY = 0;
	private boolean doubleTapSelectionEnabled = false;
	private boolean selectionModeActive = false;
	
	public void toggleSelectionMode() {
		selectionModeActive = !selectionModeActive;
		mActivity.showToast( selectionModeActive ? R.string.action_toggle_selection_mode_on : R.string.action_toggle_selection_mode_off);
	}
	
	public void onLongTap( final int x, final int y, final int zone ) {
		mEngine.execute(new Task() {
			String link;
			public void work() {
				link = checkLinkInternal(x, y, mActivity.getPalmTipPixels() / 2 );
				if ( link!=null ) {
					if ( link.startsWith("#") ) {
						log.d("go to " + link);
						goLinkInternal(link);
						drawPage();
					}
				}
			}
			public void done() {
				if ( link==null )
					onTapZone( zone, true );
				else if (!link.startsWith("#")) {
					mActivity.showToast("External links are not yet supported");
				}
			}
		});
	}
	
	@Override
	public boolean onTouchEvent(MotionEvent event) {
		
		if ( !isTouchScreenEnabled ) {
			return true;
		}
		
		int x = (int)event.getX();
		int y = (int)event.getY();
		int dx = getWidth();
		int dy = getHeight();
		int START_DRAG_THRESHOLD = mActivity.getPalmTipPixels();
		final int zone = getTapZone(x, y, dx, dy);
		
		if ( event.getAction()==MotionEvent.ACTION_UP ) {
			longTouchId++;
			if ( selectionInProgress ) {
				log.v("touch ACTION_UP: selection finished");
				selectionEndX = x;
				selectionEndY = y;
				updateSelection( selectionStartX, selectionStartY, selectionEndX, selectionEndY, true );
				selectionInProgress = false;
				selectionModeActive = false; // TODO: multiple selection mode
				return true;
			}
			if ( touchEventIgnoreNextUp )
				return true;
			mActivity.onUserActivity();
			unhiliteTapZone(); 
			boolean isLongPress = (event.getEventTime()-event.getDownTime())>LONG_KEYPRESS_TIME;
			stopAnimation(x, y);
			stopBrightnessControl(x, y);
			if ( isManualScrollActive || isBrightnessControlActive ) {
				isManualScrollActive = false;
				isBrightnessControlActive = false;
				manualScrollStartPosX = manualScrollStartPosY = -1;
				return true;
			}
			if ( isLongPress || !doubleTapSelectionEnabled ) {
				currentDoubleTapActionStart = 0;
				if ( !isLongPress )
					onTapZone( zone, isLongPress );
				else {
					onLongTap( x, y, zone );
				}
			} else {
				currentDoubleTapActionStart = android.os.SystemClock.uptimeMillis();
				final long myStart = currentDoubleTapActionStart;
				BackgroundThread.instance().postGUI(new Runnable() {
					@Override
					public void run() {
						if ( currentDoubleTapActionStart == myStart ) {
							onTapZone( zone, false );
						}
						currentDoubleTapActionStart = 0;
					}
				}, DOUBLE_CLICK_INTERVAL);
			}
			return true;
		} else if ( event.getAction()==MotionEvent.ACTION_DOWN ) {
			touchEventIgnoreNextUp = false;
			if ( selectionModeActive || currentDoubleTapActionStart + DOUBLE_CLICK_INTERVAL > android.os.SystemClock.uptimeMillis() ) {
				log.v("touch ACTION_DOWN: double tap: starting selection");
				// double tap started
				selectionInProgress = true;
				longTouchId++;
				selectionStartX = x;
				selectionStartY = y;
				selectionEndX = x;
				selectionEndY = y;
				currentDoubleTapActionStart = 0;
				updateSelection( selectionStartX, selectionStartY, selectionEndX, selectionEndY, false );
				return true;
			}
			currentDoubleTapActionStart = 0;
			selectionInProgress = false;
			manualScrollStartPosX = x;
			manualScrollStartPosY = y;
			currentDoubleTapActionStart = 0;
			if ( hiliteTapZoneOnTap ) {
				hiliteTapZone( true, x, y, dx, dy );
				scheduleUnhilite( LONG_KEYPRESS_TIME );
			}
			final int myId = ++longTouchId;
			mBackThread.postGUI( new Runnable() {
				@Override
				public void run() {
					log.v("onTouchEvent: long tap delayed event myId=" + myId + ", currentId=" + longTouchId);
					if ( myId==longTouchId ) {
						touchEventIgnoreNextUp = true;
						isBrightnessControlActive = false;
						isManualScrollActive = false;
						onLongTap( manualScrollStartPosX, manualScrollStartPosY, zone );
						manualScrollStartPosX = manualScrollStartPosY = -1;
					}
				}
				
			}, LONG_KEYPRESS_TIME);
			return true;
		} else if ( event.getAction()==MotionEvent.ACTION_MOVE) {
			if ( selectionInProgress ) {
				log.v("touch ACTION_MOVE: updating selection");
				selectionEndX = x;
				selectionEndY = y;
				updateSelection( selectionStartX, selectionStartY, selectionEndX, selectionEndY, false );
				return true;
			}
			if ( touchEventIgnoreNextUp )
				return true;
			if ( !isManualScrollActive && !isBrightnessControlActive && manualScrollStartPosX>=0 && manualScrollStartPosY>=0 ) {
				int movex = manualScrollStartPosX - x;
				int deltay = manualScrollStartPosY - y;
				int deltax = movex < 0 ? -movex : movex;
				deltay = deltay < 0 ? -deltay : deltay;
				if ( deltax + deltay > START_DRAG_THRESHOLD ) {
					log.v("onTouchEvent: move threshold reached");
					longTouchId++;
					if ( manualScrollStartPosX < START_DRAG_THRESHOLD * 170 / 100 && deltay>deltax && isBacklightControlFlick==1 ) {
						// brightness
						isBrightnessControlActive = true;
						startBrightnessControl(x, y);
						return true;
					} else if ( manualScrollStartPosX > dx - START_DRAG_THRESHOLD * 170 / 100 && deltay>deltax && isBacklightControlFlick==2 ) {
							// brightness
							isBrightnessControlActive = true;
							startBrightnessControl(x, y);
							return true;
					} else {
						//pageFlipAnimationSpeedMs
						// scroll
						boolean isPageMode = mSettings.getInt(PROP_PAGE_VIEW_MODE, 1)==1;
						boolean startScrollEnabled = true;
						if ( isPageMode ) {
							if ( deltax < START_DRAG_THRESHOLD ) // check only horizontal distance
								startScrollEnabled = false;
							if ( movex>0 && x>dx*2/3 )
								startScrollEnabled = false;
							if ( movex<0 && x<dx/3 )
								startScrollEnabled = false;
						}
						if ( startScrollEnabled ) {
							if ( pageFlipAnimationSpeedMs!=0 ) {
								isManualScrollActive = true;
								startAnimation(manualScrollStartPosX, manualScrollStartPosY, dx, dy);
								int nx = x;
								int ny = y;
								if ( isPageMode )
									nx = movex < 0 ? (x + dx) / 2 : x / 2;
								else
									ny = (manualScrollStartPosY + y) / 2;
								updateAnimation(nx, ny);
								updateAnimation(x, y);
								return true;
							} else {
								touchEventIgnoreNextUp = true;
								if ( movex<0 ) {
									// back
									onCommand(ReaderCommand.DCMD_PAGEUP, 1);
								} else {
									// forward
									onCommand(ReaderCommand.DCMD_PAGEDOWN, 1);
								}
								return true;
							}
						}
					}
				}
			}
			if ( isManualScrollActive )
				updateAnimation(x, y);
			else if ( isBrightnessControlActive ) 
				updateBrightnessControl(x, y);
			return true;
		} else if ( event.getAction()==MotionEvent.ACTION_OUTSIDE ) {
			if ( selectionInProgress ) {
				// cancel selection
				cancelSelection();
			}
			isManualScrollActive = false;
			isBrightnessControlActive = false;
			selectionModeActive = false;
			currentDoubleTapActionStart = 0;
			longTouchId++;
			stopAnimation(-1, -1);
			stopBrightnessControl(-1, -1);
			hiliteTapZone( false, x, y, dx, dy ); 
		}
		return true;
		//return super.onTouchEvent(event);
	}

	@Override
	public boolean onTrackballEvent(MotionEvent event) {
		log.d("onTrackballEvent(" + event + ")");
		if ( mSettings.getBool(PROP_APP_TRACKBALL_DISABLED, false) ) {
			log.d("trackball is disabled in settings");
			return true;
		}
		return super.onTrackballEvent(event);
	}
	
	public void showTOC()
	{
		BackgroundThread.ensureGUI();
		final ReaderView view = this; 
		mEngine.post(new Task() {
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
				FindNextDlg.showDialog( mActivity, view, pattern, caseInsensitive );
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
		mEngine.post(new Task() {
			public void work() throws Exception {
				BackgroundThread.ensureBackground();
				clearSelectionInternal();
				invalidImages = true;
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
					if ( shortcut==0 )
						mBookInfo.addBookmark(bm);
					else
						mBookInfo.setShortcutBookmark(shortcut, bm);
					mActivity.getDB().save(mBookInfo);
					String s;
					if ( shortcut==0 )
						s = mActivity.getString(R.string.toast_position_bookmark_is_set);
					else {
						s = mActivity.getString(R.string.toast_shortcut_bookmark_is_set);
						s.replace("$1", String.valueOf(shortcut));
					}
					mActivity.showToast(s);
				}
			}
		});
	}
	
	public boolean onMenuItem( final int itemId )
	{
		BackgroundThread.ensureGUI();
		ReaderAction action = ReaderAction.findByMenuId(itemId);
		if ( action.isNone() )
			return false;
		onAction(action);
		return true;
	}
	
	public void onAction( final ReaderAction action )
	{
		onAction(action, null);
	}
	public void onAction( final ReaderAction action, final Runnable onFinishHandler )
	{
		BackgroundThread.ensureGUI();
		if ( action.cmd!=ReaderCommand.DCMD_NONE )
			onCommand( action.cmd, action.param, onFinishHandler );
	}
	
	public void toggleDayNightMode()
	{
		Properties settings = getSettings();
		OptionsDialog.toggleDayNightMode(settings);
		setSettings(settings, null);
		invalidImages = true;
	}
	
	public boolean isNightMode() {
		return mSettings.getBool(PROP_NIGHT_MODE, false);
	}

	public String getSetting( String name ) {
		return mSettings.getProperty(name);
	}

	public void setSetting( String name, String value ) {
		Properties settings = getSettings();
		settings.put(name, value);
		setSettings(settings, null, false);
		invalidImages = true;
	}
	
	public void saveSetting( String name, String value ) {
		mSettings.setProperty(name, value);
		mActivity.saveSettings(mSettings);
	}
	
	public void toggleScreenOrientation()
	{
		int orientation = mActivity.getScreenOrientation();
		orientation = ( orientation==0 )? 1 : 0;
		saveSetting(PROP_APP_SCREEN_ORIENTATION, String.valueOf(orientation));
		mActivity.setScreenOrientation(orientation);
	}
	
	public void toggleFullscreen()
	{
		boolean newBool = !mActivity.isFullscreen();
		String newValue = newBool ? "1" : "0";
		saveSetting(PROP_APP_FULLSCREEN, newValue);
		mActivity.setFullscreen(newBool);
	}
	
	public void toggleDocumentStyles()
	{
		if ( mOpened && mBookInfo!=null ) {
			log.d("toggleDocumentStyles()");
			boolean flg = !mBookInfo.getFileInfo().getFlag(FileInfo.DONT_USE_DOCUMENT_STYLES_FLAG);
			mBookInfo.getFileInfo().setFlag(FileInfo.DONT_USE_DOCUMENT_STYLES_FLAG, flg);
            doEngineCommand( ReaderCommand.DCMD_SET_INTERNAL_STYLES, flg ? 0 : 1);
            doEngineCommand( ReaderCommand.DCMD_REQUEST_RENDER, 1);
		}
	}
	
	public boolean getDocumentStylesEnabled() {
		if ( mOpened && mBookInfo!=null ) {
			boolean flg = !mBookInfo.getFileInfo().getFlag(FileInfo.DONT_USE_DOCUMENT_STYLES_FLAG);
			return flg;
		}
		return true;
	}
	
	static private SimpleDateFormat timeFormat = new SimpleDateFormat("HH:mm", Locale.getDefault());
	public void showBookInfo() {
		final ArrayList<String> items = new ArrayList<String>();
		items.add("section=section.system");
		items.add("system.version=Cool Reader " + mActivity.getVersion());
		items.add("system.battery=" + mBatteryState + "%");
		items.add("system.time=" + timeFormat.format(new Date()));
		final BookInfo bi = mBookInfo;
		if ( bi!=null ) {
			FileInfo fi = bi.getFileInfo();
			items.add("section=section.file");
			String fname = new File(fi.pathname).getName();
			items.add("file.name=" + fname);
			if ( new File(fi.pathname).getParent()!=null )
				items.add("file.path=" + new File(fi.pathname).getParent());
			items.add("file.size=" + fi.size);
			if ( fi.arcname!=null ) {
				items.add("file.arcname=" + new File(fi.arcname).getName());
				if ( new File(fi.arcname).getParent()!=null )
					items.add("file.arcpath=" + new File(fi.arcname).getParent());
				items.add("file.arcsize=" + fi.arcsize);
			}
			items.add("file.format=" + fi.format.name());
		}
		execute( new Task() {
			Bookmark bm;
			@Override
			public void work() {
				bm = getCurrentPageBookmarkInternal();
				if ( bm!=null ) {
					PositionProperties prop = getPositionPropsInternal(bm.getStartPos());
					items.add("section=section.position");
					if ( prop.pageMode!=0 ) {
						items.add("position.page=" + (prop.pageNumber+1) + " / " + prop.pageCount);
					}
					int percent = (int)(10000 * (long)prop.y / prop.fullHeight);
					items.add("position.percent=" + (percent/100) + "." + (percent%100) + "%" );
					String chapter = bm.getTitleText();
					if ( chapter!=null && chapter.length()>100 )
						chapter = chapter.substring(0, 100) + "...";
					items.add("position.chapter=" + chapter);
				}
			}
			public void done() {
				FileInfo fi = bi.getFileInfo();
				items.add("section=section.book");
				if ( fi.authors!=null || fi.title!=null || fi.series!=null) { 
					items.add("book.authors=" + fi.authors);
					items.add("book.title=" + fi.title);
					if ( fi.series!=null ) {
						String s = fi.series;
						if ( fi.seriesNumber>0 )
							s = s + " #" + fi.seriesNumber; 
						items.add("book.series=" + s);
					}
				}
				BookInfoDialog dlg = new BookInfoDialog(mActivity, items);
				dlg.show();
			}
		});
	}
	
	public void onCommand( final ReaderCommand cmd, final int param )
	{
		onCommand( cmd, param, null );
	}
	
	public void onCommand( final ReaderCommand cmd, final int param, final Runnable onFinishHandler )
	{
		BackgroundThread.ensureGUI();
		log.i("On command " + cmd + (param!=0?" ("+param+")":" "));
		switch ( cmd ) {
		case DCMD_ABOUT:
			mActivity.showAboutDialog();
			break;
		case DCMD_BOOK_INFO:
			showBookInfo();
			break;
		case DCMD_TTS_PLAY:
			{
				log.i("DCMD_TTS_PLAY: initializing TTS");
				if ( !mActivity.initTTS(new TTS.OnTTSCreatedListener() {
					@Override
					public void onCreated(TTS tts) {
						log.i("TTS created: opening TTS toolbar");
						TTSToolbarDlg.showDialog(mActivity, ReaderView.this, tts);
					}
				}) ) {
					log.e("Cannot initilize TTS");
				}
			}
			break;
		case DCMD_TOGGLE_DOCUMENT_STYLES:
			toggleDocumentStyles();
			break;
		case DCMD_SHOW_HOME_SCREEN:
			mActivity.showHomeScreen();
			break;
		case DCMD_TOGGLE_ORIENTATION:
			toggleScreenOrientation();
			break;
		case DCMD_TOGGLE_FULLSCREEN:
			toggleFullscreen();
			break;
		case DCMD_TOGGLE_SELECTION_MODE:
			toggleSelectionMode();
			break;
		case DCMD_TOGGLE_TOUCH_SCREEN_LOCK:
			isTouchScreenEnabled = !isTouchScreenEnabled;
			if ( isTouchScreenEnabled )
				mActivity.showToast(R.string.action_touch_screen_enabled_toast);
			else
				mActivity.showToast(R.string.action_touch_screen_disabled_toast);
			break;
		case DCMD_LINK_BACK:
		case DCMD_LINK_FORWARD:
            doEngineCommand( cmd, 0);
            drawPage();
            break;
		case DCMD_ZOOM_OUT:
            doEngineCommand( ReaderCommand.DCMD_ZOOM_OUT, param);
            syncViewSettings(getSettings(), true);
            break;
		case DCMD_ZOOM_IN:
            doEngineCommand( ReaderCommand.DCMD_ZOOM_IN, param);
            syncViewSettings(getSettings(), true);
            break;
		case DCMD_PAGEDOWN:
			if ( param==1 )
				animatePageFlip(1, onFinishHandler);
			else
				doEngineCommand(cmd, param, onFinishHandler);
			break;
		case DCMD_PAGEUP:
			if ( param==1 )
				animatePageFlip(-1, onFinishHandler);
			else
				doEngineCommand(cmd, param, onFinishHandler);
			break;
		case DCMD_BEGIN:
		case DCMD_END:
			doEngineCommand(cmd, param);
			break;
		case DCMD_RECENT_BOOKS_LIST:
			mActivity.showBrowserRecentBooks();
			break;
		case DCMD_SEARCH:
			showSearchDialog();
			break;
		case DCMD_EXIT:
			mActivity.finish();
			break;
		case DCMD_BOOKMARKS:
			mActivity.showBookmarksDialog();
			break;
		case DCMD_GO_PERCENT_DIALOG:
			mActivity.showGoToPercentDialog();
			break;
		case DCMD_GO_PAGE_DIALOG:
			mActivity.showGoToPageDialog();
			break;
		case DCMD_TOC_DIALOG:
			showTOC();
			break;
		case DCMD_FILE_BROWSER:
			mActivity.showBrowser(getOpenedFileInfo());
			break;
		case DCMD_OPTIONS_DIALOG:
			mActivity.showOptionsDialog();
			break;
		case DCMD_READER_MENU:
			mActivity.openOptionsMenu();
			break;
		case DCMD_TOGGLE_DAY_NIGHT_MODE:
			toggleDayNightMode();
			break;
		}
	}
	
	public void doEngineCommand( final ReaderCommand cmd, final int param )
	{
		doEngineCommand( cmd, param, null );
	}
	public void doEngineCommand( final ReaderCommand cmd, final int param, final Runnable doneHandler )
	{
		BackgroundThread.ensureGUI();
		log.d("doCommand("+cmd + ", " + param +")");
		post(new Task() {
			boolean res;
			public void work() {
				BackgroundThread.ensureBackground();
				res = doCommandInternal(cmd.nativeId, param);
			}
			public void done() {
				if ( res )
					drawPage( doneHandler );
			}
		});
	}
	
	public void doCommandFromBackgroundThread( final ReaderCommand cmd, final int param )
	{
		log.d("doCommandFromBackgroundThread("+cmd + ", " + param +")");
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
	
	volatile private boolean mInitialized = false;
	volatile private boolean mOpened = false;
	
	//private File historyFile;
	
	private void updateLoadedBookInfo()
	{
		BackgroundThread.ensureBackground();
		// get title, authors, etc.
		updateBookInfoInternal( mBookInfo );
	}
	
	private void applySettings( Properties props, boolean save )
	{
		BackgroundThread.ensureBackground();
		log.v("applySettings() " + props);
		boolean isFullScreen = props.getBool(PROP_APP_FULLSCREEN, false );
		props.setBool(PROP_SHOW_BATTERY, isFullScreen); 
		props.setBool(PROP_SHOW_TIME, isFullScreen);
		String backgroundImageId = props.getProperty(PROP_PAGE_BACKGROUND_IMAGE);
		if ( backgroundImageId!=null )
			setBackgroundTexture(backgroundImageId);
		props.remove(PROP_EMBEDDED_STYLES);
        applySettingsInternal(props);
        syncViewSettings(props, save);
        drawPage();
	}
	
	public static boolean eq(Object obj1, Object obj2)
	{
		if ( obj1==null && obj2==null )
			return true;
		if ( obj1==null || obj2==null )
			return false;
		return obj1.equals(obj2);
	}

	public void saveSettings( Properties settings )
	{
		mActivity.saveSettings(settings);
	}
	
	/**
	 * Read JNI view settings, update and save if changed 
	 */
	private void syncViewSettings( final Properties currSettings, final boolean save )
	{
		post( new Task() {
			Properties props;
			public void work() {
				BackgroundThread.ensureBackground();
				java.util.Properties internalProps = getSettingsInternal(); 
				props = new Properties(internalProps);
				props.remove(PROP_EMBEDDED_STYLES);
			}
			public void done() {
				Properties changedSettings = props.diff(currSettings);
		        for ( Map.Entry<Object, Object> entry : changedSettings.entrySet() ) {
	        		currSettings.setProperty((String)entry.getKey(), (String)entry.getValue());
		        }
	        	mSettings = currSettings;
	        	if ( save )
	        		saveSettings(currSettings);
			}
		});
	}
	
	public Properties getSettings()
	{
		return new Properties(mSettings);
	}
	
	static public int stringToInt( String value, int defValue ) {
		if ( value==null )
			return defValue;
		try {
			return Integer.valueOf(value);
		} catch ( NumberFormatException e ) {
			return defValue;
		}
	}

	private boolean hiliteTapZoneOnTap = false;
	private boolean enableVolumeKeys = true; 
	static private final int DEF_PAGE_FLIP_MS = 700; 
	public void applyAppSetting( String key, String value )
	{
		boolean flg = "1".equals(value);
        if ( key.equals(PROP_APP_FULLSCREEN) ) {
			this.mActivity.setFullscreen( "1".equals(value) );
        } else if ( key.equals(PROP_APP_SHOW_COVERPAGES) ) {
			mActivity.getHistory().setCoverPagesEnabled(flg);
        } else if ( key.equals(PROP_APP_BOOK_PROPERTY_SCAN_ENABLED) ) {
			mActivity.getScanner().setDirScanEnabled(flg);
        } else if ( key.equals(PROP_APP_SCREEN_BACKLIGHT_LOCK) ) {
			mActivity.setWakeLockEnabled(flg);
        } else if ( key.equals(PROP_APP_FILE_BROWSER_SIMPLE_MODE) ) {
        	if ( mActivity.getBrowser()!=null )
        		mActivity.getBrowser().setSimpleViewMode(flg);
        } else if ( key.equals(PROP_NIGHT_MODE) ) {
			mActivity.setNightMode(flg);
        } else if ( key.equals(PROP_APP_SCREEN_UPDATE_MODE) ) {
			mActivity.setScreenUpdateMode(stringToInt(value, 0));
        } else if ( key.equals(PROP_APP_SCREEN_UPDATE_INTERVAL) ) {
			mActivity.setScreenUpdateInterval(stringToInt(value, 10));
        } else if ( key.equals(PROP_APP_TAP_ZONE_HILIGHT) ) {
        	hiliteTapZoneOnTap = flg;
        } else if ( key.equals(PROP_APP_DICTIONARY) ) {
        	mActivity.setDict(value);
        } else if ( key.equals(PROP_APP_DOUBLE_TAP_SELECTION) ) {
        	doubleTapSelectionEnabled = flg;
        } else if ( key.equals(PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS) ) {
        	mActivity.getScanner().setHideEmptyDirs(flg);
        } else if ( key.equals(PROP_APP_FLICK_BACKLIGHT_CONTROL) ) {
        	isBacklightControlFlick = "1".equals(value) ? 1 : ("2".equals(value) ? 2 : 0);
        } else if ( key.equals(PROP_APP_SCREEN_ORIENTATION) ) {
			int orientation = "1".equals(value) ? 1 : ("4".equals(value) ? 4 : 0);
        	mActivity.setScreenOrientation(orientation);
        } else if ( PROP_PAGE_ANIMATION.equals(key) ) {
        	try {
        		int n = Integer.valueOf(value);
        		if ( n<0 || n>PAGE_ANIMATION_MAX )
        			n = PAGE_ANIMATION_SLIDE2;
        		pageFlipAnimationMode = n;
        	} catch ( Exception e ) {
        		// ignore
        	}
			pageFlipAnimationSpeedMs = pageFlipAnimationMode!=PAGE_ANIMATION_NONE ? DEF_PAGE_FLIP_MS : 0; 
        } else if ( PROP_CONTROLS_ENABLE_VOLUME_KEYS.equals(key) ) {
        	enableVolumeKeys = flg;
        } else if ( PROP_APP_SCREEN_BACKLIGHT.equals(key) ) {
        	try {
        		final int n = Integer.valueOf(value);
        		// delay before setting brightness
        		mBackThread.postGUI(new Runnable() {
        			public void run() {
        				execute( new Task() {

							@Override
							public void work() throws Exception {
								// do nothing
							}

							@Override
							public void done() {
				        		mActivity.setScreenBacklightLevel(n);
								super.done();
							}
        					
        				});
        			}
        		}, 100);
        	} catch ( Exception e ) {
        		// ignore
        	}
        } else if ( PROP_APP_SELECTION_ACTION.equals(key) ) {
        	try {
        		int n = Integer.valueOf(value);
        		mSelectionAction = n;
        	} catch ( Exception e ) {
        		// ignore
        	}
        }
        //
	}
	
	public void setAppSettings( Properties newSettings, Properties oldSettings )
	{
		log.v("setAppSettings() " + newSettings.toString());
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
    		} else if ( PROP_PAGE_VIEW_MODE.equals(key) ) {
    			boolean flg = "1".equals(value);
    			viewMode = flg ? ViewMode.PAGES : ViewMode.SCROLL;
    		} else if ( PROP_APP_SCREEN_ORIENTATION.equals(key) || PROP_PAGE_ANIMATION.equals(key)
    				|| PROP_CONTROLS_ENABLE_VOLUME_KEYS.equals(key) || PROP_APP_SHOW_COVERPAGES.equals(key) 
    				|| PROP_APP_SCREEN_BACKLIGHT.equals(key) 
    				|| PROP_APP_BOOK_PROPERTY_SCAN_ENABLED.equals(key)
    				|| PROP_APP_SCREEN_BACKLIGHT_LOCK.equals(key)
    				|| PROP_APP_TAP_ZONE_HILIGHT.equals(key)
    				|| PROP_APP_DICTIONARY.equals(key)
    				|| PROP_APP_DOUBLE_TAP_SELECTION.equals(key)
    				|| PROP_APP_FLICK_BACKLIGHT_CONTROL.equals(key)
    				|| PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS.equals(key)
    				|| PROP_APP_SELECTION_ACTION.equals(key)
    				|| PROP_APP_FILE_BROWSER_SIMPLE_MODE.equals(key)
    				// TODO: redesign all this mess!
    				) {
    			newSettings.setProperty(key, value);
    		} else if ( PROP_HYPHENATION_DICT.equals(key) ) {
    			Engine.HyphDict dict = HyphDict.byCode(value);
    			//mEngine.setHyphenationDictionary();
    			if ( mEngine.setHyphenationDictionary(dict) ) {
    				if ( isBookLoaded() ) {
    					doEngineCommand( ReaderCommand.DCMD_REQUEST_RENDER, 0);
    					//drawPage();
    				}
    			}
    			newSettings.setProperty(key, value);
    		}
        }
	}
	
	public ViewMode getViewMode()
	{
		return viewMode;
	}
	
	/**
     * Change settings.
	 * @param newSettings are new settings
	 * @param oldSettings are old settings, null to use mSettings
	 */
	public void setSettings(Properties newSettings, Properties oldSettings)
	{
		setSettings(newSettings, oldSettings, true);
	}

	/**
     * Change settings.
	 * @param newSettings are new settings
	 * @param oldSettings are old settings, null to use mSettings
	 * @param save is true to save settings to file, false to skip saving
	 */
	public void setSettings(Properties newSettings, Properties oldSettings, final boolean save)
	{
		log.v("setSettings() " + newSettings.toString());
		BackgroundThread.ensureGUI();
		if ( oldSettings==null )
			oldSettings = mSettings;
		final Properties currSettings = new Properties(oldSettings);
		setAppSettings( newSettings, currSettings );
		Properties changedSettings = newSettings.diff(currSettings);
		currSettings.setAll(changedSettings);
    	mBackThread.executeBackground(new Runnable() {
    		public void run() {
    			applySettings(currSettings, save);
    		}
    	});
//        }
	}

	private void setBackgroundTexture( String textureId ) {
		BackgroundTextureInfo[] textures = mEngine.getAvailableTextures();
		for ( BackgroundTextureInfo item : textures ) {
			if ( item.id.equals(textureId) ) {
				setBackgroundTexture( item );
				return;
			}
		}
		setBackgroundTexture( Engine.NO_TEXTURE );
	}

	private void setBackgroundTexture( BackgroundTextureInfo texture ) {
		if ( !currentBackgroundTexture.equals(texture) ) {
		log.d("setBackgroundTexture( " + texture + " )");
			currentBackgroundTexture = texture;
			byte[] data = mEngine.getImageData(currentBackgroundTexture);
			setPageBackgroundTextureInternal(data, texture.tiled ? 1 : 0);
		}
	}
	
	BackgroundTextureInfo currentBackgroundTexture = Engine.NO_TEXTURE;
	class CreateViewTask extends Task
	{
        Properties props = new Properties();
        public CreateViewTask( Properties props ) {
       		this.props = props;
       		Properties oldSettings = new Properties(); // may be changed by setAppSettings 
   			setAppSettings(props, oldSettings);
   			props.setAll(oldSettings);
       		mSettings = props;
        }
		public void work() throws Exception {
			BackgroundThread.ensureBackground();
			log.d("CreateViewTask - in background thread");
//			BackgroundTextureInfo[] textures = mEngine.getAvailableTextures();
//			byte[] data = mEngine.getImageData(textures[3]);
			byte[] data = mEngine.getImageData(currentBackgroundTexture);
			setPageBackgroundTextureInternal(data, currentBackgroundTexture.tiled?1:0);
			
			//File historyDir = activity.getDir("settings", Context.MODE_PRIVATE);
			//File historyDir = new File(Environment.getExternalStorageDirectory(), ".cr3");
			//historyDir.mkdirs();
			//File historyFile = new File(historyDir, "cr3hist.ini");
			
			//File historyFile = new File(activity.getDir("settings", Context.MODE_PRIVATE), "cr3hist.ini");
			//if ( historyFile.exists() ) {
			//log.d("Reading history from file " + historyFile.getAbsolutePath());
			//readHistoryInternal(historyFile.getAbsolutePath());
			//}
	        String css = mEngine.loadResourceUtf8(R.raw.fb2);
	        if ( css!=null && css.length()>0 )
       			setStylesheetInternal(css);
   			applySettings(props, false);
   			mInitialized = true;
		}
		public void done() {
			log.d("InitializationFinishedEvent");
			BackgroundThread.ensureGUI();
	        //setSettings(props, new Properties());
		}
		public void fail( Exception e )
		{
			log.e("CoolReader engine initialization failed. Exiting.", e);
			mEngine.fatalError("Failed to init CoolReader engine");
		}
	}

	public void closeIfOpened( final FileInfo fileInfo )
	{
		if ( this.mBookInfo!=null && this.mBookInfo.getFileInfo().pathname.equals(fileInfo.pathname) && mOpened ) {
			close();
		}
	}
	
	public void loadDocument( final FileInfo fileInfo )
	{
		if ( this.mBookInfo!=null && this.mBookInfo.getFileInfo().pathname.equals(fileInfo.pathname) && mOpened ) {
			log.d("trying to load already opened document");
			mActivity.showReader();
			drawPage();
			return;
		}
		post(new LoadDocumentTask(fileInfo, null));
	}

	public boolean loadLastDocument( final Runnable errorHandler )
	{
		BackgroundThread.ensureGUI();
		//BookInfo book = mActivity.getHistory().getLastBook();
		String lastBookName = mActivity.getLastSuccessfullyOpenedBook();
		log.i("loadLastDocument() is called, lastBookName = " + lastBookName);
		return loadDocument( lastBookName, errorHandler );
	}
	
	public boolean loadDocument( String fileName, final Runnable errorHandler )
	{
		BackgroundThread.ensureGUI();
		log.i("loadDocument(" + fileName + ")");
		if ( fileName==null ) {
			log.v("loadDocument() : no filename specified");
			errorHandler.run();
			return false;
		}
		BookInfo book = fileName!=null ? mActivity.getHistory().getBookInfo(fileName) : null;
		if ( book!=null )
			log.v("loadDocument() : found book in history : " + book);
		FileInfo fi = null;
		if ( book==null ) {
			log.v("loadDocument() : book not found in history, looking for location directory");
			FileInfo dir = mActivity.getScanner().findParent(new FileInfo(fileName), mActivity.getScanner().getRoot());
			if ( dir!=null ) {
				log.v("loadDocument() : document location found : " + dir);
				fi = dir.findItemByPathName(fileName);
				log.v("loadDocument() : item inside location : " + fi);
			}
			if ( fi==null ) {
				log.v("loadDocument() : no file item " + fileName + " found inside " + dir);
				errorHandler.run();
				return false;
			}
			if ( fi.isDirectory ) {
				log.v("loadDocument() : is a directory, opening browser");
				mActivity.showBrowser(fi);
				return true;
			}
		} else {
			fi = book.getFileInfo();
			log.v("loadDocument() : item from history : " + fi);
		}
		post( new LoadDocumentTask(fi, errorHandler) );
		log.v("loadDocument: LoadDocumentTask(" + fi + ") is posted");
		return true;
	}
	
	public BookInfo getBookInfo() {
		BackgroundThread.ensureGUI();
		return mBookInfo;
	}
	
	
	private int mBatteryState = 100;
	public void setBatteryState( int state ) {
		if ( state!=mBatteryState ) {
			log.i("Battery state changed: " + state);
			mBatteryState = state;
			drawPage();
		}
	}
	
	public int getBatteryState() {
		return mBatteryState;
	}
	
	private static class BitmapFactory {
		public static final int MAX_FREE_LIST_SIZE=2;
		ArrayList<Bitmap> freeList = new ArrayList<Bitmap>(); 
		ArrayList<Bitmap> usedList = new ArrayList<Bitmap>(); 
		public synchronized Bitmap get( int dx, int dy ) {
			for ( int i=0; i<freeList.size(); i++ ) {
				Bitmap bmp = freeList.get(i);
				if ( bmp.getWidth()==dx && bmp.getHeight()==dy ) {
					// found bitmap of proper size
					freeList.remove(i);
					usedList.add(bmp);
					//log.d("BitmapFactory: reused free bitmap, used list = " + usedList.size() + ", free list=" + freeList.size());
					return bmp;
				}
			}
			for ( int i=freeList.size()-1; i>=0; i-- ) {
				Bitmap bmp = freeList.remove(i);
				//log.d("Recycling free bitmap "+bmp.getWidth()+"x"+bmp.getHeight());
				//bmp.recycle(); //20110109 
			}
			Bitmap bmp = Bitmap.createBitmap(dx, dy, Bitmap.Config.RGB_565);
			//bmp.setDensity(0);
			usedList.add(bmp);
			//log.d("Created new bitmap "+dx+"x"+dy+". New bitmap list size = " + usedList.size());
			return bmp;
		}
		public synchronized void compact() {
			while ( freeList.size()>0 ) {
				//freeList.get(0).recycle();//20110109
				freeList.remove(0);
			}
		}
		public synchronized void release( Bitmap bmp ) {
			for ( int i=0; i<usedList.size(); i++ ) {
				if ( usedList.get(i)==bmp ) {
					freeList.add(bmp);
					usedList.remove(i);
					while ( freeList.size()>MAX_FREE_LIST_SIZE ) {
						//freeList.get(0).recycle(); //20110109
						freeList.remove(0);
					}
					log.d("BitmapFactory: bitmap released, used size = " + usedList.size() + ", free size=" + freeList.size());
					return;
				}
			}
			// unknown bitmap, just recycle
			//bmp.recycle();//20110109
		}
	};
	BitmapFactory factory = new BitmapFactory(); 
	
	class BitmapInfo {
		Bitmap bitmap;
		PositionProperties position;
		void recycle()
		{
			factory.release(bitmap);
			bitmap = null;
			position = null;
		}
		@Override
		public String toString() {
			return "BitmapInfo [position=" + position + "]";
		}
		
	}
	
    private BitmapInfo mCurrentPageInfo;
    private BitmapInfo mNextPageInfo;
	/**
	 * Prepare and cache page image.
	 * Cache is represented by two slots: mCurrentPageInfo and mNextPageInfo.  
	 * If page already exists in cache, returns it (if current page requested, 
	 *  ensures that it became stored as mCurrentPageInfo; if another page requested, 
	 *  no mCurrentPageInfo/mNextPageInfo reordering made).
	 * @param offset is kind of page: 0==current, -1=previous, 1=next page
	 * @return page image and properties, null if requested page is unavailable (e.g. requested next/prev page is out of document range)
	 */
	private BitmapInfo preparePageImage( int offset )
	{
		BackgroundThread.ensureBackground();
		log.v("preparePageImage( "+offset+")");
		if ( invalidImages ) {
			if ( mCurrentPageInfo!=null )
				mCurrentPageInfo.recycle();
			mCurrentPageInfo = null;
			if ( mNextPageInfo!=null )
				mNextPageInfo.recycle();
			mNextPageInfo = null;
			invalidImages = false;
		}

		if ( internalDX==0 || internalDY==0 ) {
			internalDX=200;
			internalDY=300;
	        resizeInternal(internalDX, internalDY);
		}
		
		PositionProperties currpos = getPositionPropsInternal(null);
		
		boolean isPageView = currpos.pageMode!=0;
		
		BitmapInfo currposBitmap = null;
		if ( mCurrentPageInfo!=null && mCurrentPageInfo.position.equals(currpos) )
			currposBitmap = mCurrentPageInfo;
		else if ( mNextPageInfo!=null && mNextPageInfo.position.equals(currpos) )
			currposBitmap = mNextPageInfo;
		if ( offset==0 ) {
			// Current page requested
			if ( currposBitmap!=null ) {
				if ( mNextPageInfo==currposBitmap ) {
					// reorder pages
					BitmapInfo tmp = mNextPageInfo;
					mNextPageInfo = mCurrentPageInfo;
					mCurrentPageInfo = tmp;
				}
				// found ready page image
				return mCurrentPageInfo;
			}
			if ( mCurrentPageInfo!=null ) {
				mCurrentPageInfo.recycle();
				mCurrentPageInfo = null;
			}
			BitmapInfo bi = new BitmapInfo();
	        bi.position = currpos;
			bi.bitmap = factory.get(internalDX, internalDY);
	        setBatteryStateInternal(mBatteryState);
	        getPageImageInternal(bi.bitmap);
	        mCurrentPageInfo = bi;
	        //log.v("Prepared new current page image " + mCurrentPageInfo);
	        return mCurrentPageInfo;
		}
		if ( isPageView ) {
			// PAGES: one of next or prev pages requested, offset is specified as param 
			int cmd1 = offset > 0 ? ReaderCommand.DCMD_PAGEDOWN.nativeId : ReaderCommand.DCMD_PAGEUP.nativeId;
			int cmd2 = offset > 0 ? ReaderCommand.DCMD_PAGEUP.nativeId : ReaderCommand.DCMD_PAGEDOWN.nativeId;
			if ( offset<0 )
				offset = -offset;
			if ( doCommandInternal(cmd1, offset) ) {
				// can move to next page
				PositionProperties nextpos = getPositionPropsInternal(null);
				BitmapInfo nextposBitmap = null;
				if ( mCurrentPageInfo!=null && mCurrentPageInfo.position.equals(nextpos) )
					nextposBitmap = mCurrentPageInfo;
				else if ( mNextPageInfo!=null && mNextPageInfo.position.equals(nextpos) )
					nextposBitmap = mNextPageInfo;
				if ( nextposBitmap==null ) {
					// existing image not found in cache, overriding mNextPageInfo
					if ( mNextPageInfo!=null )
						mNextPageInfo.recycle();
					mNextPageInfo = null;
					BitmapInfo bi = new BitmapInfo();
			        bi.position = nextpos;
					bi.bitmap = factory.get(internalDX, internalDY);
			        setBatteryStateInternal(mBatteryState);
			        getPageImageInternal(bi.bitmap);
			        mNextPageInfo = bi;
			        nextposBitmap = bi;
			        //log.v("Prepared new current page image " + mNextPageInfo);
				}
				// return back to previous page
				doCommandInternal(cmd2, offset);
				return nextposBitmap;
			} else {
				// cannot move to page: out of document range
				return null;
			}
		} else {
			// SCROLL next or prev page requested, with pixel offset specified
			int y = currpos.y + offset;
			if ( doCommandInternal(ReaderCommand.DCMD_GO_POS.nativeId, y) ) {
				PositionProperties nextpos = getPositionPropsInternal(null);
				BitmapInfo nextposBitmap = null;
				if ( mCurrentPageInfo!=null && mCurrentPageInfo.position.equals(nextpos) )
					nextposBitmap = mCurrentPageInfo;
				else if ( mNextPageInfo!=null && mNextPageInfo.position.equals(nextpos) )
					nextposBitmap = mNextPageInfo;
				if ( nextposBitmap==null ) {
					// existing image not found in cache, overriding mNextPageInfo
					if ( mNextPageInfo!=null )
						mNextPageInfo.recycle();
					mNextPageInfo = null;
					BitmapInfo bi = new BitmapInfo();
			        bi.position = nextpos;
					bi.bitmap = factory.get(internalDX, internalDY);
			        setBatteryStateInternal(mBatteryState);
			        getPageImageInternal(bi.bitmap);
			        mNextPageInfo = bi;
			        nextposBitmap = bi;
				}
				// return back to prev position
				doCommandInternal(ReaderCommand.DCMD_GO_POS.nativeId, currpos.y);
				return nextposBitmap;
			} else {
				return null;
			}
		}
		
	}
	
	private int lastDrawTaskId = 0;
	private class DrawPageTask extends Task {
		final int id;
		BitmapInfo bi;
		Runnable doneHandler;
		DrawPageTask(Runnable doneHandler)
		{
			this.id = ++lastDrawTaskId;
			this.doneHandler = doneHandler;
		}
		public void work() {
			BackgroundThread.ensureBackground();
			if ( this.id!=lastDrawTaskId ) {
				log.d("skipping duplicate drawPage request");
				return;
			}
			nextHiliteId++;
			if ( currentAnimation!=null ) {
				log.d("skipping drawPage request while scroll animation is in progress");
				return;
			}
			log.e("DrawPageTask.work("+internalDX+","+internalDY+")");
			bi = preparePageImage(0);
			if ( bi!=null ) {
				draw();
			}
		}
		@Override
		public void done()
		{
			BackgroundThread.ensureGUI();
//			log.d("drawPage : bitmap is ready, invalidating view to draw new bitmap");
//			if ( bi!=null ) {
//				setBitmap( bi.bitmap );
//				invalidate();
//			}
//    		if (mOpened)
   			mEngine.hideProgress();
   			if ( doneHandler!=null )
   				doneHandler.run();
		}
		@Override
		public void fail(Exception e) {
   			mEngine.hideProgress();
		}
	};
	
	class ReaderSurfaceView extends SurfaceView {
		public ReaderSurfaceView( Context context )
		{
			super(context);
		}
	}
	
	// SurfaceView callbacks
	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width,
			int height) {
		log.i("surfaceChanged(" + width + ", " + height + ")");
		drawPage();
	}

	boolean mSurfaceCreated = false;
	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		log.i("surfaceCreated()");
		mSurfaceCreated = true;
		drawPage();
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		log.i("surfaceDestroyed()");
		mSurfaceCreated = false;
	}
	
	enum AnimationType {
		SCROLL, // for scroll mode
		PAGE_SHIFT, // for simple page shift
	}

	
	
	private ViewAnimationControl currentAnimation = null;

	private int pageFlipAnimationSpeedMs = DEF_PAGE_FLIP_MS; // if 0 : no animation
	private int pageFlipAnimationMode = PAGE_ANIMATION_SLIDE2; //PAGE_ANIMATION_PAPER; // if 0 : no animation
	private void animatePageFlip( final int dir ) {
		animatePageFlip(dir, null);
	}
	private void animatePageFlip( final int dir, final Runnable onFinishHandler )
	{
		BackgroundThread.backgroundExecutor.execute(new Runnable() {
			@Override
			public void run() {
				BackgroundThread.ensureBackground();
				if ( currentAnimation==null ) {
					PositionProperties currPos = getPositionPropsInternal(null);
					if ( currPos==null )
						return;
					int w = currPos.pageWidth;
					int h = currPos.pageHeight;
					int dir2 = dir;
//					if ( currPos.pageMode==2 )
//						if ( dir2==1 )
//							dir2 = 2;
//						else if ( dir2==-1 ) 
//							dir2 = -2;
					int speed = pageFlipAnimationSpeedMs;
					if ( onFinishHandler!=null )
						speed = pageFlipAnimationSpeedMs / 2;
					if ( currPos.pageMode!=0 ) {
						int fromX = dir2>0 ? w : 0;
						int toX = dir2>0 ? 0 : w;
						new PageViewAnimation(fromX, w, dir2);
						if ( currentAnimation!=null ) {
							if ( currentAnimation!=null ) {
								nextHiliteId++;
								hiliteRect = null;
							}
							currentAnimation.update(toX, h/2);
							currentAnimation.move(speed, true);
							currentAnimation.stop(-1, -1);
							if ( onFinishHandler!=null )
								BackgroundThread.guiExecutor.execute(onFinishHandler);
						}
					} else {
						//new ScrollViewAnimation(startY, maxY);
						int fromY = dir>0 ? h*7/8 : 0;
						int toY = dir>0 ? 0 : h*7/8;
						new ScrollViewAnimation(fromY, h);
						if ( currentAnimation!=null ) {
							if ( currentAnimation!=null ) {
								nextHiliteId++;
								hiliteRect = null;
								
							}
							currentAnimation.update(w/2, toY);
							currentAnimation.move(speed, true);
							currentAnimation.stop(-1, -1);
							if ( onFinishHandler!=null )
								BackgroundThread.guiExecutor.execute(onFinishHandler);
						}
					}
				}
			}
		});
	}
	
	static private Rect tapZoneBounds( int startX, int startY, int maxX, int maxY ) {
		if ( startX<0 )
			startX=0;
		if ( startY<0 )
			startY = 0;
		if ( startX>maxX )
			startX = maxX;
		if ( startY>maxY)
			startY = maxY;
		int dx = (maxX + 2) / 3;
		int dy = (maxY + 2) / 3;
		int x0 = startX / dx * dx; 
		int y0 = startY / dy * dy;
		return new Rect(x0, y0, x0+dx, y0+dy);
	}
	
	volatile private int nextHiliteId = 0;
	private final static int HILITE_RECT_ALPHA = 32;
	private Rect hiliteRect = null;
	private void unhiliteTapZone() {
		hiliteTapZone( false, 0, 0, getWidth(), getHeight() );
	}
	private void hiliteTapZone( final boolean hilite, final int startX, final int startY, final int maxX, final int maxY )
	{
		if (DEBUG_ANIMATION) log.d("highliteTapZone("+startX + ", " + startY+")");
		final int myHiliteId = ++nextHiliteId;
		int txcolor = mSettings.getColor(PROP_FONT_COLOR, Color.BLACK);
		final int color = (txcolor & 0xFFFFFF) | (HILITE_RECT_ALPHA<<24);
		BackgroundThread.backgroundExecutor.execute(new Runnable() {
			@Override
			public void run() {
				if ( myHiliteId != nextHiliteId || (!hilite && hiliteRect==null) )
					return;
				BackgroundThread.ensureBackground();
				final BitmapInfo pageImage = preparePageImage(0);
				if ( pageImage!=null && pageImage.bitmap!=null && pageImage.position!=null ) {
					//PositionProperties currPos = pageImage.position;
					final Rect rc = hilite ? tapZoneBounds( startX, startY, maxX, maxY ) : hiliteRect;
					if ( hilite )
						hiliteRect = rc;
					else
						hiliteRect = null;
					if ( rc!=null )
					drawCallback( new DrawCanvasCallback() {
						@Override
						public void drawTo(Canvas canvas) {
				    		if ( mInitialized && mCurrentPageInfo!=null ) {
				        		log.d("onDraw() -- drawing page image");
				        		drawDimmedBitmap(canvas, mCurrentPageInfo.bitmap, rc, rc);
				    			if ( hilite ) {
					    			Paint p = new Paint();
					    			p.setColor(color);
					    			if ( true ) {
					    				canvas.drawRect(new Rect(rc.left, rc.top, rc.right-2, rc.top+2), p);
					    				canvas.drawRect(new Rect(rc.left, rc.top+2, rc.left+2, rc.bottom-2), p);
					    				canvas.drawRect(new Rect(rc.right-2-2, rc.top+2, rc.right-2, rc.bottom-2), p);
					    				canvas.drawRect(new Rect(rc.left+2, rc.bottom-2-2, rc.right-2-2, rc.bottom-2), p);
					    			} else {
					    				canvas.drawRect(rc, p);
					    			}
				    			}
				    		}
						}
						
					}, rc);
				}
			}
			
		});
	}
	private void scheduleUnhilite( int delay ) {
		final int myHiliteId = nextHiliteId;
		mBackThread.postGUI(new Runnable() {
			@Override
			public void run() {
				if ( myHiliteId == nextHiliteId && hiliteRect!=null )
					unhiliteTapZone(); 
			}
		}, delay);
	}
	
	int currentBrightnessValueIndex = -1;
	private void startBrightnessControl(final int startX, final int startY)
	{
		currentBrightnessValueIndex = -1;
		updateBrightnessControl(startX, startY);
	}
	private void updateBrightnessControl(final int x, final int y) {
		int n = OptionsDialog.mBacklightLevels.length;
		int index = n - 1 - y * n / getHeight();
		if ( index<0 )
			index = 0;
		else if ( index>=n )
			index = n-1;
		if ( index != currentBrightnessValueIndex ) {
			currentBrightnessValueIndex = index;
			int newValue = OptionsDialog.mBacklightLevels[currentBrightnessValueIndex]; 
			mActivity.setScreenBacklightLevel(newValue);
		}
		
	}
	private void stopBrightnessControl(final int x, final int y) {
		if ( currentBrightnessValueIndex>=0 ) {
			if ( x>=0 && y>=0 ) {
				updateBrightnessControl(x, y);
			}
			mSettings.setInt(PROP_APP_SCREEN_BACKLIGHT, OptionsDialog.mBacklightLevels[currentBrightnessValueIndex]);
			OptionsDialog.mBacklightLevelsTitles[0] = mActivity.getString(R.string.options_app_backlight_screen_default);
			String s = OptionsDialog.mBacklightLevelsTitles[currentBrightnessValueIndex];
			if ( showBrightnessFlickToast )
				mActivity.showToast(s);
			mActivity.saveSettings(mSettings);
			currentBrightnessValueIndex = -1;
		}
	}
	private static final boolean showBrightnessFlickToast = false;


	private void startAnimation( final int startX, final int startY, final int maxX, final int maxY )
	{
		if (DEBUG_ANIMATION) log.d("startAnimation("+startX + ", " + startY+")");
		BackgroundThread.backgroundExecutor.execute(new Runnable() {
			@Override
			public void run() {
				BackgroundThread.ensureBackground();
				PositionProperties currPos = getPositionPropsInternal(null);
				if ( currPos!=null && currPos.pageMode!=0 ) {
					//int dir = startX > maxX/2 ? currPos.pageMode : -currPos.pageMode;
					int dir = startX > maxX/2 ? 1 : -1;
					int sx = startX;
					if ( dir<0 )
						sx = 0;
					new PageViewAnimation(sx, maxX, dir);
				} else {
					new ScrollViewAnimation(startY, maxY);
				}
				if ( currentAnimation!=null ) {
					nextHiliteId++;
					hiliteRect = null;
				}
			}
			
		});
	}

	
	private final static boolean DEBUG_ANIMATION = false;
	private volatile int updateSerialNumber = 0;
	private void updateAnimation( final int x, final int y )
	{
		if (DEBUG_ANIMATION) log.d("updateAnimation("+x + ", " + y+")");
		final int serial = ++updateSerialNumber;
		BackgroundThread.backgroundExecutor.execute(new Runnable() {
			@Override
			public void run() {
				if ( currentAnimation!=null ) {
					currentAnimation.update(x, y);
					if ( serial==updateSerialNumber ) //|| serial==updateSerialNumber-1 
						currentAnimation.animate();
				}
			}
		});
		try {
			// give a chance to background thread to process event faster
			Thread.sleep(0);
		} catch ( InterruptedException e ) {
			// ignore
		}
	}
	
	private void stopAnimation( final int x, final int y )
	{
		if (DEBUG_ANIMATION) log.d("stopAnimation("+x+", "+y+")");
		BackgroundThread.backgroundExecutor.execute(new Runnable() {
			@Override
			public void run() {
				if ( currentAnimation!=null ) {
					currentAnimation.stop(x, y);
				}
			}
			
		});
	}

	private int animationSerialNumber = 0;
	private void scheduleAnimation()
	{
		final int serial = ++animationSerialNumber; 
		BackgroundThread.backgroundExecutor.execute(new Runnable() {
			@Override
			public void run() {
				if ( serial!=animationSerialNumber )
					return;
				if ( currentAnimation!=null ) {
					currentAnimation.animate();
				}
			}
		});
	}
	
	interface ViewAnimationControl
	{
		public void update( int x, int y );
		public void stop( int x, int y );
		public void animate();
		public void move( int duration, boolean accelerated );
		public boolean isStarted();
	}

	private Object surfaceLock = new Object(); 

	private static final int[] accelerationShape = new int[] {
		0, 6, 24, 54, 95, 146, 206, 273, 345, 421, 500, 578, 654, 726, 793, 853, 904, 945, 975, 993, 1000  
	};
	static public int accelerate( int x0, int x1, int x )
	{
		if ( x<x0 )
			x = x0;
		if (x>x1)
			x = x1;
		int intervals = accelerationShape.length - 1;
		int pos = 100 * intervals * (x - x0) / (x1-x0);
		int interval = pos / 100;
		int part = pos % 100;
		if ( interval<0 )
			interval = 0;
		else if ( interval>intervals )
			interval = intervals;
		int y = interval==intervals ? 100000 : accelerationShape[interval]*100 + (accelerationShape[interval+1]-accelerationShape[interval]) * part;
		return x0 + (x1 - x0) * y / 100000;
	}

	private interface DrawCanvasCallback {
		public void drawTo( Canvas c );
	}
	private void drawCallback( DrawCanvasCallback callback, Rect rc )
	{
		if ( !mSurfaceCreated )
			return;
		//synchronized(surfaceLock) { }
		//log.v("draw() - in thread " + Thread.currentThread().getName());
		final SurfaceHolder holder = getHolder();
		//log.v("before synchronized(surfaceLock)");
		if ( holder!=null )
		//synchronized(surfaceLock) 
		{
			Canvas canvas = null;
			long startTs = android.os.SystemClock.uptimeMillis();
			try {
				canvas = holder.lockCanvas(rc);
				//log.v("before draw(canvas)");
				if (DeviceInfo.EINK_SCREEN) {
					EinkScreen.PrepareController();
				}
				if ( canvas!=null ) {
					callback.drawTo(canvas);
				}
			} finally {
				//log.v("exiting finally");
				if ( canvas!=null && getHolder()!=null ) {
					//log.v("before unlockCanvasAndPost");
					if ( canvas!=null && holder!=null ) {
						holder.unlockCanvasAndPost(canvas);
						//if ( rc==null ) {
							long endTs = android.os.SystemClock.uptimeMillis();
							updateAnimationDurationStats(endTs - startTs);
						//}
					}
					//log.v("after unlockCanvasAndPost");
				}
			}
		}
		//log.v("exiting draw()");
	}
	
	abstract class ViewAnimationBase implements ViewAnimationControl {
		long startTimeStamp;
		boolean started;
		public boolean isStarted()
		{
			return started;
		}
		ViewAnimationBase()
		{
			startTimeStamp = android.os.SystemClock.uptimeMillis();
		}
		public void close()
		{
			currentAnimation = null;
		}

		

		public void draw()
		{
			drawCallback( new DrawCanvasCallback() {
				@Override
				public void drawTo(Canvas c) {
					//long startTs = android.os.SystemClock.uptimeMillis();
					draw(c);
					//long endTs = android.os.SystemClock.uptimeMillis();
					//updateAnimationDurationStats(endTs - startTs);
				}
				
			}, null);
		}
		abstract void draw( Canvas canvas );
	}
	
	//private static final int PAGE_ANIMATION_DURATION = 3000;
	class ScrollViewAnimation extends ViewAnimationBase {
		int startY;
		int maxY;
		int pointerStartPos;
		int pointerDestPos;
		int pointerCurrPos;
		ScrollViewAnimation( int startY, int maxY )
		{
			super();
			this.startY = startY;
			this.maxY = maxY;
			long start = android.os.SystemClock.uptimeMillis();
			log.v("ScrollViewAnimation -- creating: drawing two pages to buffer");
			PositionProperties currPos = getPositionPropsInternal(null);
			int pos = currPos.y;
			int pos0 = pos - (maxY - startY);
			if ( pos0<0 )
				pos0 = 0;
			pointerStartPos = pos;
			pointerCurrPos = pos;
			pointerDestPos = startY;
			BitmapInfo image1;
			BitmapInfo image2;
			doCommandInternal(ReaderCommand.DCMD_GO_POS.nativeId, pos0);
			image1 = preparePageImage(0);
			image2 = preparePageImage(image1.position.pageHeight);
//			doCommandInternal(ReaderCommand.DCMD_GO_POS.nativeId, pos0 + image1.position.pageHeight);
//			image2 = preparePageImage(0);
			doCommandInternal(ReaderCommand.DCMD_GO_POS.nativeId, pos);
			if ( image1==null || image2==null ) {
				log.v("ScrollViewAnimation -- not started: image is null");
				return;
			}
			long duration = android.os.SystemClock.uptimeMillis() - start;
			log.v("ScrollViewAnimation -- created in " + duration + " millis");
			currentAnimation = this;
		}
		
		@Override
		public void stop(int x, int y) {
			//if ( started ) {
				if ( y!=-1 ) {
					int delta = startY - y;
					pointerCurrPos = pointerStartPos + delta;
				}
				pointerDestPos = pointerCurrPos;
				draw();
				doCommandInternal(ReaderCommand.DCMD_GO_POS.nativeId, pointerDestPos);
			//}
			close();
		}

		@Override
		public void move( int duration, boolean accelerated  ) {
			if ( duration>0  && pageFlipAnimationSpeedMs!=0 ) {
				int steps = (int)(duration / getAvgAnimationDrawDuration()) + 2;
				int x0 = pointerCurrPos;
				int x1 = pointerDestPos;
				if ( (x0-x1)<10 && (x0-x1)>-10 )
					steps = 2;
				for ( int i=1; i<steps; i++ ) {
					int x = x0 + (x1-x0) * i / steps;
					pointerCurrPos = accelerated ? accelerate( x0, x1, x ) : x; 
					draw();
				}
			}
			pointerCurrPos = pointerDestPos; 
			draw();
		}

		@Override
		public void update(int x, int y) {
			int delta = startY - y;
			pointerDestPos = pointerStartPos + delta;
		}

		public void animate()
		{
			//log.d("animate() is called");
			if ( pointerDestPos != pointerCurrPos ) {
				if ( !started )
					started = true;
				if ( pageFlipAnimationSpeedMs==0 )
					pointerCurrPos = pointerDestPos;
				else {
					int delta = pointerCurrPos-pointerDestPos;
					if ( delta<0 )
						delta = -delta;
					long avgDraw = getAvgAnimationDrawDuration();
					//int maxStep = (int)(maxY * PAGE_ANIMATION_DURATION / avgDraw);
					int maxStep = pageFlipAnimationSpeedMs > 0 ? (int)(maxY * 1000 / avgDraw / pageFlipAnimationSpeedMs) : maxY;
					int step;
					if ( delta > maxStep * 2 )
						step = maxStep;
					else
						step = (delta + 3) / 4;
					//int step = delta<3 ? 1 : (delta<5 ? 2 : (delta<10 ? 3 : (delta<15 ? 6 : (delta<25 ? 10 : (delta<50 ? 15 : 30))))); 
					if ( pointerCurrPos<pointerDestPos )
						pointerCurrPos+=step;
					else if ( pointerCurrPos>pointerDestPos )
						pointerCurrPos-=step;
					log.d("animate("+pointerCurrPos + " => " + pointerDestPos + "  step=" + step + ")");
				}
				//pointerCurrPos = pointerDestPos;
				draw();
				if ( pointerDestPos != pointerCurrPos )
					scheduleAnimation();
			}
		}

		public void draw(Canvas canvas)
		{
			BitmapInfo image1 = mCurrentPageInfo;
			BitmapInfo image2 = mNextPageInfo;
			int h = image1.position.pageHeight;
			int rowsFromImg1 = image1.position.y + h - pointerCurrPos;
			int rowsFromImg2 = h - rowsFromImg1;
    		Rect src1 = new Rect(0, h-rowsFromImg1, mCurrentPageInfo.bitmap.getWidth(), h);
    		Rect dst1 = new Rect(0, 0, mCurrentPageInfo.bitmap.getWidth(), rowsFromImg1);
    		drawDimmedBitmap(canvas, image1.bitmap, src1, dst1);
			if (image2!=null) {
	    		Rect src2 = new Rect(0, 0, mCurrentPageInfo.bitmap.getWidth(), rowsFromImg2);
	    		Rect dst2 = new Rect(0, rowsFromImg1, mCurrentPageInfo.bitmap.getWidth(), h);
	    		drawDimmedBitmap(canvas, image2.bitmap, src2, dst2);
			}
			//log.v("anim.drawScroll( pos=" + pointerCurrPos + ", " + src1 + "=>" + dst1 + ", " + src2 + "=>" + dst2 + " )");
		}
	}

	private final static int SIN_TABLE_SIZE = 1024;
	private final static int SIN_TABLE_SCALE = 0x10000;
	private final static int PI_DIV_2 = (int)(Math.PI / 2 * SIN_TABLE_SCALE);
	/// sin table, for 0..PI/2
	private static int[] SIN_TABLE = new int[SIN_TABLE_SIZE+1];
	private static int[] ASIN_TABLE = new int[SIN_TABLE_SIZE+1];
	// mapping of 0..1 shift to angle
	private static int[] SRC_TABLE = new int[SIN_TABLE_SIZE+1];
	// mapping of 0..1 shift to sin(angle)
	private static int[] DST_TABLE = new int[SIN_TABLE_SIZE+1];
	// for dx=0..1 find such alpha (0..pi/2) that alpha - sin(alpha) = dx  
	private static double shiftfn( double dx ) {
		double a = 0;
		double b = Math.PI/2;
		double c = 0;
		for ( int i=0; i<15; i++ ) {
			c = (a + b) / 2;
			double cq = c - Math.sin(c);
			if ( cq < dx )
				a = c;
			else
				b = c;
		}
		return c;
	}
	static {
		for ( int i=0; i<=SIN_TABLE_SIZE; i++ ) {
			double angle = Math.PI / 2 * i / SIN_TABLE_SIZE;
			int s = (int)Math.round(Math.sin(angle) * SIN_TABLE_SCALE);
			SIN_TABLE[i] = s;
			double x = (double)i / SIN_TABLE_SIZE;
			s = (int)Math.round(Math.asin(x) * SIN_TABLE_SCALE);
			ASIN_TABLE[i] = s;
			
			double dx = i * (Math.PI/2 - 1.0) / SIN_TABLE_SIZE;
			angle = shiftfn( dx );
			SRC_TABLE[i] = (int)Math.round(angle * SIN_TABLE_SCALE);
			DST_TABLE[i] = (int)Math.round(Math.sin(angle) * SIN_TABLE_SCALE);
		}
	}
	
	class PageViewAnimation extends ViewAnimationBase {
		int startX;
		int maxX;
		int page1;
		int page2;
		int direction;
		int currShift;
		int destShift;
		int pageCount;
		Paint divPaint;
		Paint[] shadePaints;
		Paint[] hilitePaints;
		private final boolean naturalPageFlip; 
		private final boolean flipTwoPages; 
		PageViewAnimation( int startX, int maxX, int direction )
		{
			super();
			this.startX = startX;
			this.maxX = maxX;
			this.direction = direction;
			this.currShift = 0;
			this.destShift = 0;
			this.naturalPageFlip = (pageFlipAnimationMode==PAGE_ANIMATION_PAPER);
			this.flipTwoPages = (pageFlipAnimationMode==PAGE_ANIMATION_SLIDE2);
			
			long start = android.os.SystemClock.uptimeMillis();
			log.v("PageViewAnimation -- creating: drawing two pages to buffer");
			
			PositionProperties currPos = mCurrentPageInfo.position;
			if ( currPos==null )
				currPos = getPositionPropsInternal(null);
			page1 = currPos.pageNumber;
			page2 = currPos.pageNumber + direction;
			if ( page2<0 || page2>=currPos.pageCount) {
				currentAnimation = null;
				return;
			}
			this.pageCount = currPos.pageMode;
			BitmapInfo image1 = preparePageImage(0);
			BitmapInfo image2 = preparePageImage(direction);
			if ( image1==null || image2==null ) {
				log.v("PageViewAnimation -- cannot start animation: page image is null");
				return;
			}
			if ( page1==page2 ) {
				log.v("PageViewAnimation -- cannot start animation: not moved");
				return;
			}
			page2 = image2.position.pageNumber;
			currentAnimation = this;
			divPaint = new Paint();
			divPaint.setStyle(Paint.Style.FILL);
			divPaint.setColor(Color.argb(128, 128, 128, 128));
			final int numPaints = 16;
			shadePaints = new Paint[numPaints];
			hilitePaints = new Paint[numPaints];
			for ( int i=0; i<numPaints; i++ ) {
				shadePaints[i] = new Paint();
				hilitePaints[i] = new Paint();
				hilitePaints[i].setStyle(Paint.Style.FILL);
				shadePaints[i].setStyle(Paint.Style.FILL);
				if ( mActivity.isNightMode() ) {
					shadePaints[i].setColor(Color.argb((i+1)*96 / numPaints, 0, 0, 0));
					hilitePaints[i].setColor(Color.argb((i+1)*96 / numPaints, 128, 128, 128));
				} else {
					shadePaints[i].setColor(Color.argb((i+1)*96 / numPaints, 0, 0, 0));
					hilitePaints[i].setColor(Color.argb((i+1)*96 / numPaints, 255, 255, 255));
				}
			}

			
			long duration = android.os.SystemClock.uptimeMillis() - start;
			log.d("PageViewAnimation -- created in " + duration + " millis");
		}
		
		private void drawGradient( Canvas canvas, Rect rc, Paint[] paints, int startIndex, int endIndex ) {
			int n = (startIndex<endIndex) ? endIndex-startIndex+1 : startIndex-endIndex + 1;
			int dir = (startIndex<endIndex) ? 1 : -1;
			int dx = rc.right - rc.left;
			Rect rect = new Rect(rc);
			for ( int i=0; i<n; i++ ) {
				int index = startIndex + i*dir;
				int x1 = rc.left + dx*i/n;
				int x2 = rc.left + dx*(i+1)/n;
				if ( x2>rc.right )
					x2 = rc.right;
				rect.left = x1;
				rect.right = x2;
				if ( x2>x1 ) {
					canvas.drawRect(rect, paints[index]);
				}
			}
		}
		
		private void drawShadow( Canvas canvas, Rect rc ) {
			drawGradient(canvas, rc, shadePaints, shadePaints.length/2, shadePaints.length/10);
		}
		
		private final int DISTORT_PART_PERCENT = 30;
		private void drawDistorted( Canvas canvas, Bitmap bmp, Rect src, Rect dst, int dir) {
			int srcdx = src.width();
			int dstdx = dst.width();
			int dx = srcdx - dstdx;
			int maxdistortdx = srcdx * DISTORT_PART_PERCENT / 100;
			int maxdx = maxdistortdx * (PI_DIV_2 - SIN_TABLE_SCALE) / SIN_TABLE_SCALE;
			int maxdistortsrc = maxdistortdx * PI_DIV_2 / SIN_TABLE_SCALE;
			
			int distortdx = dx < maxdistortdx ? dx : maxdistortdx;
			int distortsrcstart = -1;
			int distortsrcend = -1;
			int distortdststart = -1;
			int distortdstend = -1;
			int distortanglestart = -1;
			int distortangleend = -1;
			int normalsrcstart = -1;
			int normalsrcend = -1;
			int normaldststart = -1;
			int normaldstend = -1;
			
			if ( dx < maxdx ) {
				// start
				int index = dx>=0 ? dx * SIN_TABLE_SIZE / maxdx : 0;
				int dstv = DST_TABLE[index] * maxdistortdx / SIN_TABLE_SCALE;
				distortdststart = distortsrcstart = dstdx - dstv;
				distortsrcend = srcdx;
				distortdstend = dstdx;
				normalsrcstart = normaldststart = 0;
				normalsrcend = distortsrcstart;
				normaldstend = distortdststart;
				distortanglestart = 0;
				distortangleend = SRC_TABLE[index];
				distortdx = maxdistortdx;
			} else if (dstdx>maxdistortdx) {
				// middle
				distortdststart = distortsrcstart = dstdx - maxdistortdx;
				distortsrcend = distortsrcstart + maxdistortsrc;
				distortdstend = dstdx;
				normalsrcstart = normaldststart = 0;
				normalsrcend = distortsrcstart;
				normaldstend = distortdststart;
				distortanglestart = 0;
				distortangleend = PI_DIV_2;
			} else {
				// end
				normalsrcstart = normaldststart = normalsrcend = normaldstend = -1;
				distortdx = dstdx;
				distortsrcstart = 0;
				int n = maxdistortdx >= dstdx ? maxdistortdx - dstdx : 0;
				distortsrcend = ASIN_TABLE[SIN_TABLE_SIZE * n/maxdistortdx ] * maxdistortsrc / SIN_TABLE_SCALE;
				distortdststart = 0;
				distortdstend = dstdx;
				distortangleend = PI_DIV_2; 
				n = maxdistortdx >= distortdx ? maxdistortdx - distortdx : 0;
				distortanglestart = ASIN_TABLE[SIN_TABLE_SIZE * (maxdistortdx - distortdx)/maxdistortdx ];
			}
			
			Rect srcrc = new Rect(src);
			Rect dstrc = new Rect(dst);
			if ( normalsrcstart<normalsrcend ) {
				if ( dir>0 ) {
					srcrc.left = src.left + normalsrcstart;
					srcrc.right = src.left + normalsrcend;
					dstrc.left = dst.left + normaldststart;
					dstrc.right = dst.left + normaldstend;
				} else {
					srcrc.right = src.right - normalsrcstart;
					srcrc.left = src.right - normalsrcend;
					dstrc.right = dst.right - normaldststart;
					dstrc.left = dst.right - normaldstend;
				}
				drawDimmedBitmap(canvas, bmp, srcrc, dstrc);
			}
			if ( distortdststart<distortdstend ) {
				int n = distortdx / 5 + 1;
				int dst0 = SIN_TABLE[distortanglestart * SIN_TABLE_SIZE / PI_DIV_2] * maxdistortdx / SIN_TABLE_SCALE; 
				int src0 = distortanglestart * maxdistortdx / SIN_TABLE_SCALE;
				for ( int i=0; i<n; i++ ) {
					int angledelta = distortangleend - distortanglestart;
					int startangle = distortanglestart + i * angledelta / n;
					int endangle = distortanglestart + (i+1) * angledelta / n;
					int src1 = startangle * maxdistortdx / SIN_TABLE_SCALE - src0;
					int src2 = endangle * maxdistortdx / SIN_TABLE_SCALE - src0;
					int dst1 = SIN_TABLE[startangle * SIN_TABLE_SIZE / PI_DIV_2] * maxdistortdx / SIN_TABLE_SCALE - dst0;
					int dst2 = SIN_TABLE[endangle * SIN_TABLE_SIZE / PI_DIV_2] * maxdistortdx / SIN_TABLE_SCALE - dst0;
					int hiliteIndex = startangle * hilitePaints.length / PI_DIV_2;
					Paint[] paints;
					if ( dir>0 ) {
						dstrc.left = dst.left + distortdststart + dst1; 
						dstrc.right = dst.left + distortdststart + dst2;
						srcrc.left = src.left + distortsrcstart + src1;
						srcrc.right = src.left + distortsrcstart + src2;
						paints = hilitePaints;
					} else {
						dstrc.right = dst.right - distortdststart - dst1; 
						dstrc.left = dst.right - distortdststart - dst2;
						srcrc.right = src.right - distortsrcstart - src1;
						srcrc.left = src.right - distortsrcstart - src2;
						paints = shadePaints;
					}
					drawDimmedBitmap(canvas, bmp, srcrc, dstrc);
					canvas.drawRect(dstrc, paints[hiliteIndex]);
				}
			}
		}
		
		@Override
		public void move( int duration, boolean accelerated ) {
			if ( duration > 0 && pageFlipAnimationSpeedMs!=0 ) {
				int steps = (int)(duration / getAvgAnimationDrawDuration()) + 2;
				int x0 = currShift;
				int x1 = destShift;
				if ( (x0-x1)<10 && (x0-x1)>-10 )
					steps = 2;
				for ( int i=1; i<steps; i++ ) {
					int x = x0 + (x1-x0) * i / steps;
					currShift = accelerated ? accelerate( x0, x1, x ) : x;
					draw();
				}
			}
			currShift = destShift;
			draw();
		}

		@Override
		public void stop(int x, int y) {
			if (DEBUG_ANIMATION) log.v("PageViewAnimation.stop(" + x + ", " + y + ")");
			//if ( started ) {
				boolean moved = false;
				if ( x!=-1 ) {
					int threshold = mActivity.getPalmTipPixels() * 7/8;
					if ( direction>0 ) {
						// |  <=====  |
						int dx = startX - x; 
						if ( dx>threshold )
							moved = true;
					} else {
						// |  =====>  |
						int dx = x - startX; 
						if ( dx>threshold )
							moved = true;
					}
					int duration;
					if ( moved ) {
						destShift = maxX;
						duration = 300; // 500 ms forward
					} else {
						destShift = 0;
						duration = 200; // 200 ms cancel
					}
					move( duration, false );
				} else {
					moved = true;
				}
				doCommandInternal(ReaderCommand.DCMD_GO_PAGE_DONT_SAVE_HISTORY.nativeId, moved ? page2 : page1);
			//}
			close();
			// preparing images for next page flip
			preparePageImage(0);
			preparePageImage(direction);
			//if ( started )
			//	drawPage();
		}

		@Override
		public void update(int x, int y) {
			if (DEBUG_ANIMATION) log.v("PageViewAnimation.update(" + x + ", " + y + ")");
			int delta = direction>0 ? startX - x : x - startX;
			if ( delta<=0 )
				destShift = 0;
			else if ( delta<maxX )
				destShift = delta;
			else
				destShift = maxX;
		}

		public void animate()
		{
			if (DEBUG_ANIMATION) log.v("PageViewAnimation.animate("+currShift + " => " + destShift + ") speed=" + pageFlipAnimationSpeedMs);
			//log.d("animate() is called");
			if ( currShift != destShift ) {
				started = true;
				if ( pageFlipAnimationSpeedMs==0 )
					currShift = destShift;
				else {
					int delta = currShift - destShift;
					if ( delta<0 )
						delta = -delta;
					long avgDraw = getAvgAnimationDrawDuration();
					int maxStep = pageFlipAnimationSpeedMs > 0 ? (int)(maxX * 1000 / avgDraw / pageFlipAnimationSpeedMs) : maxX;
					int step;
					if ( delta > maxStep * 2 )
						step = maxStep;
					else
						step = (delta + 3) / 4;
					//int step = delta<3 ? 1 : (delta<5 ? 2 : (delta<10 ? 3 : (delta<15 ? 6 : (delta<25 ? 10 : (delta<50 ? 15 : 30))))); 
					if ( currShift < destShift )
						currShift+=step;
					else if ( currShift > destShift )
						currShift-=step;
					if (DEBUG_ANIMATION) log.v("PageViewAnimation.animate("+currShift + " => " + destShift + "  step=" + step + ")");
				}
				//pointerCurrPos = pointerDestPos;
				draw();
				if ( currShift != destShift )
					scheduleAnimation();
			}
		}

		public void draw(Canvas canvas)
		{
			if (DEBUG_ANIMATION) log.v("PageViewAnimation.draw("+currShift + ")");
			BitmapInfo image1 = mCurrentPageInfo;
			BitmapInfo image2 = mNextPageInfo;
			int w = image1.bitmap.getWidth(); 
			int h = image1.bitmap.getHeight();
			int div;
			if ( direction > 0 ) {
				// FORWARD
				div = w-currShift;
				Rect shadowRect = new Rect(div, 0, div+w/10, h);
				if ( naturalPageFlip ) {
					if ( this.pageCount==2 ) {
						int w2 = w/2;
						if ( div<w2 ) {
							// left - part of old page
				    		Rect src1 = new Rect(0, 0, div, h);
				    		Rect dst1 = new Rect(0, 0, div, h);
				    		drawDimmedBitmap(canvas, image1.bitmap, src1, dst1);
							// left, resized part of new page
				    		Rect src2 = new Rect(0, 0, w2, h);
				    		Rect dst2 = new Rect(div, 0, w2, h);
				    		//canvas.drawBitmap(image2.bitmap, src2, dst2, null);
							drawDistorted(canvas, image2.bitmap, src2, dst2, -1);
							// right, new page
				    		Rect src3 = new Rect(w2, 0, w, h);
				    		Rect dst3 = new Rect(w2, 0, w, h);
				    		drawDimmedBitmap(canvas, image2.bitmap, src3, dst3);

						} else {
							// left - old page
				    		Rect src1 = new Rect(0, 0, w2, h);
				    		Rect dst1 = new Rect(0, 0, w2, h);
				    		drawDimmedBitmap(canvas, image1.bitmap, src1, dst1);
							// right, resized old page
				    		Rect src2 = new Rect(w2, 0, w, h);
				    		Rect dst2 = new Rect(w2, 0, div, h);
				    		//canvas.drawBitmap(image1.bitmap, src2, dst2, null);
							drawDistorted(canvas, image1.bitmap, src2, dst2, 1);
							// right, new page
				    		Rect src3 = new Rect(div, 0, w, h);
				    		Rect dst3 = new Rect(div, 0, w, h);
				    		drawDimmedBitmap(canvas, image2.bitmap, src3, dst3);

							if ( div>0 && div<w )
								drawShadow( canvas, shadowRect );
						}
					} else {
			    		Rect src1 = new Rect(0, 0, w, h);
			    		Rect dst1 = new Rect(0, 0, w-currShift, h);
			    		//log.v("drawing " + image1);
						//canvas.drawBitmap(image1.bitmap, src1, dst1, null);
						drawDistorted(canvas, image1.bitmap, src1, dst1, 1);
			    		Rect src2 = new Rect(w-currShift, 0, w, h);
			    		Rect dst2 = new Rect(w-currShift, 0, w, h);
			    		//log.v("drawing " + image1);
			    		drawDimmedBitmap(canvas, image2.bitmap, src2, dst2);

						if ( div>0 && div<w )
							drawShadow( canvas, shadowRect );
					}
				} else {
					if ( flipTwoPages ) {
			    		Rect src1 = new Rect(currShift, 0, w, h);
			    		Rect dst1 = new Rect(0, 0, w-currShift, h);
			    		//log.v("drawing " + image1);
			    		drawDimmedBitmap(canvas, image1.bitmap, src1, dst1);
			    		Rect src2 = new Rect(0, 0, currShift, h);
			    		Rect dst2 = new Rect(w-currShift, 0, w, h);
			    		//log.v("drawing " + image1);
			    		drawDimmedBitmap(canvas, image2.bitmap, src2, dst2);
					} else {
			    		Rect src1 = new Rect(currShift, 0, w, h);
			    		Rect dst1 = new Rect(0, 0, w-currShift, h);
			    		//log.v("drawing " + image1);
			    		drawDimmedBitmap(canvas, image1.bitmap, src1, dst1);
			    		Rect src2 = new Rect(w-currShift, 0, w, h);
			    		Rect dst2 = new Rect(w-currShift, 0, w, h);
			    		//log.v("drawing " + image1);
			    		drawDimmedBitmap(canvas, image2.bitmap, src2, dst2);
					}
				}
			} else {
				// BACK
				div = currShift;
				Rect shadowRect = new Rect(div, 0, div+10, h);
				if ( naturalPageFlip ) {
					if ( this.pageCount==2 ) {
						int w2 = w/2;
						if ( div<w2 ) {
							// left - part of old page
				    		Rect src1 = new Rect(0, 0, div, h);
				    		Rect dst1 = new Rect(0, 0, div, h);
				    		drawDimmedBitmap(canvas, image2.bitmap, src1, dst1);
							// left, resized part of new page
				    		Rect src2 = new Rect(0, 0, w2, h);
				    		Rect dst2 = new Rect(div, 0, w2, h);
				    		//canvas.drawBitmap(image1.bitmap, src2, dst2, null);
							drawDistorted(canvas, image1.bitmap, src2, dst2, -1);
							// right, new page
				    		Rect src3 = new Rect(w2, 0, w, h);
				    		Rect dst3 = new Rect(w2, 0, w, h);
				    		drawDimmedBitmap(canvas, image1.bitmap, src3, dst3);
						} else {
							// left - old page
				    		Rect src1 = new Rect(0, 0, w2, h);
				    		Rect dst1 = new Rect(0, 0, w2, h);
				    		drawDimmedBitmap(canvas, image2.bitmap, src1, dst1);
							// right, resized old page
				    		Rect src2 = new Rect(w2, 0, w, h);
				    		Rect dst2 = new Rect(w2, 0, div, h);
				    		//canvas.drawBitmap(image2.bitmap, src2, dst2, null);
							drawDistorted(canvas, image2.bitmap, src2, dst2, 1);
							// right, new page
				    		Rect src3 = new Rect(div, 0, w, h);
				    		Rect dst3 = new Rect(div, 0, w, h);
				    		drawDimmedBitmap(canvas, image1.bitmap, src3, dst3);

							if ( div>0 && div<w )
								drawShadow( canvas, shadowRect );
						}
					} else {
			    		Rect src1 = new Rect(currShift, 0, w, h);
			    		Rect dst1 = new Rect(currShift, 0, w, h);
			    		drawDimmedBitmap(canvas, image1.bitmap, src1, dst1);
			    		Rect src2 = new Rect(0, 0, w, h);
			    		Rect dst2 = new Rect(0, 0, currShift, h);
						//canvas.drawBitmap(image2.bitmap, src2, dst2, null);
						drawDistorted(canvas, image2.bitmap, src2, dst2, 1);

						if ( div>0 && div<w )
							drawShadow( canvas, shadowRect );
					}
				} else {
					if ( flipTwoPages ) {
			    		Rect src1 = new Rect(0, 0, w-currShift, h);
			    		Rect dst1 = new Rect(currShift, 0, w, h);
			    		drawDimmedBitmap(canvas, image1.bitmap, src1, dst1);
			    		Rect src2 = new Rect(w-currShift, 0, w, h);
			    		Rect dst2 = new Rect(0, 0, currShift, h);
			    		drawDimmedBitmap(canvas, image2.bitmap, src2, dst2);
					} else {
			    		Rect src1 = new Rect(currShift, 0, w, h);
			    		Rect dst1 = new Rect(currShift, 0, w, h);
			    		drawDimmedBitmap(canvas, image1.bitmap, src1, dst1);
			    		Rect src2 = new Rect(w-currShift, 0, w, h);
			    		Rect dst2 = new Rect(0, 0, currShift, h);
		        		drawDimmedBitmap(canvas, image2.bitmap, src2, dst2);
					}
				}
			}
			if ( div>0 && div<w ) {
				canvas.drawLine(div, 0, div, h, divPaint);
			}
		}
	}

	private long sumAnimationDrawDuration = 500;
	private int drawAnimationCount = 10;
	private long getAvgAnimationDrawDuration()
	{
		return sumAnimationDrawDuration / drawAnimationCount; 
//		return sumAnimationDrawDuration;// / drawAnimationCount; 
	}
	
	private void updateAnimationDurationStats( long duration )
	{
		if ( duration<=0 )
			duration = 1;
		else if ( duration>1000 )
			return;
//		sumAnimationDrawDuration = (sumAnimationDrawDuration*7 + duration * 1)/8;
		sumAnimationDrawDuration += duration;
		if ( ++drawAnimationCount>20 ) {
			drawAnimationCount /= 2;
			sumAnimationDrawDuration /= 2;
		}
	}
	
	private void drawPage()
	{
		drawPage(null);
	}
	private void drawPage( Runnable doneHandler )
	{
		if ( !mInitialized || !mOpened )
			return;
		log.v("drawPage() : submitting DrawPageTask");
		post( new DrawPageTask(doneHandler) );
	}
	
	private int internalDX = 0;
	private int internalDY = 0;

	private byte[] coverPageBytes = null;
	private BitmapDrawable coverPageDrawable = null;
	private void findCoverPage()
	{
    	log.d("document is loaded succesfull, checking coverpage data");
    	if ( mActivity.getHistory().getCoverPagesEnabled() ) {
	    	byte[] coverpageBytes = getCoverPageDataInternal();
	    	if ( coverpageBytes!=null ) {
	    		log.d("Found cover page data: " + coverpageBytes.length + " bytes");
	    		BitmapDrawable drawable = mActivity.getHistory().decodeCoverPage(coverpageBytes);
	    		if ( drawable!=null ) {
	    			coverPageBytes = coverpageBytes;
	    			coverPageDrawable = drawable;
	    		}
	    	}
    	}
	}
	
	private class LoadDocumentTask extends Task
	{
		String filename;
		Runnable errorHandler;
		String pos;
		LoadDocumentTask( FileInfo fileInfo, Runnable errorHandler )
		{
			log.v("LoadDocumentTask for " + fileInfo);
			BackgroundThread.ensureGUI();
			this.filename = fileInfo.getPathName();
			this.errorHandler = errorHandler;
			//FileInfo fileInfo = new FileInfo(filename);
			mBookInfo = mActivity.getHistory().getOrCreateBookInfo( fileInfo );
	    	if ( mBookInfo!=null && mBookInfo.getLastPosition()!=null )
	    		pos = mBookInfo.getLastPosition().getStartPos();
			log.v("LoadDocumentTask : book info " + mBookInfo);
			log.v("LoadDocumentTask : last position = " + pos);
    		//mBitmap = null;
	        mEngine.showProgress( 1000, R.string.progress_loading );
	        //init();
		}

		public void work() throws IOException {
			BackgroundThread.ensureBackground();
			coverPageBytes = null;
			coverPageDrawable = null;
			log.i("Loading document " + filename);
	        boolean success = loadDocumentInternal(filename);
	        if ( success ) {
				log.v("loadDocumentInternal completed successfully");
	        	findCoverPage();
				log.v("requesting page image, to render");
	        	preparePageImage(0);
				log.v("updating loaded book info");
	        	updateLoadedBookInfo();
				log.i("Document " + filename + " is loaded successfully");
				if ( pos!=null ) {
					log.i("Restoring position : " + pos);
					restorePositionBackground(pos);
				}
				CoolReader.dumpHeapAllocation();
	        } else {
				log.e("Error occured while trying to load document " + filename);
				throw new IOException("Cannot read document");
	        }
		}
		public void done()
		{
			BackgroundThread.ensureGUI();
			log.d("LoadDocumentTask, GUI thread is finished successfully");
			if ( mActivity.getHistory()!=null ) {
	    		mActivity.getHistory().updateBookAccess(mBookInfo);
	    		mActivity.getHistory().saveToDB();
		        if ( coverPageBytes!=null && coverPageDrawable!=null && mBookInfo!=null && mBookInfo.getFileInfo()!=null ) {
		        	mActivity.getHistory().setBookCoverpageData( mBookInfo.getFileInfo().id, coverPageBytes );
		        	//mEngine.setProgressDrawable(coverPageDrawable);
		        }
		        mOpened = true;
		        drawPage();
		        mBackThread.postGUI(new Runnable() {
		        	public void run() {
		    			mActivity.showReader();
		        	}
		        });
		        mActivity.setLastSuccessfullyOpenedBook(filename);
			}
		}
		public void fail( Exception e )
		{
			BackgroundThread.ensureGUI();
			log.e("LoadDocumentTask failed for " + mBookInfo);
			mActivity.getHistory().removeBookInfo( mBookInfo.getFileInfo(), true, false );
			mBookInfo = null;
			log.d("LoadDocumentTask is finished with exception " + e.getMessage());
	        mOpened = false;
			drawPage();
			mEngine.hideProgress();
			mActivity.showToast("Error while loading document");
			if ( errorHandler!=null ) {
				log.e("LoadDocumentTask: Calling error handler");
				errorHandler.run();
			}
		}
	}

	private final static boolean dontStretchWhileDrawing = true;
	private final static boolean centerPageInsteadOfResizing = true;
	
	private void dimRect( Canvas canvas, Rect dst ) {
		int alpha = dimmingAlpha;
		if ( alpha!=255 ) {
			Paint p = new Paint();
			p.setColor((255-alpha)<<24);
			canvas.drawRect(dst, p);
		}
	}
	
	private void drawDimmedBitmap( Canvas canvas, Bitmap bmp, Rect src, Rect dst ) {
		canvas.drawBitmap(bmp, src, dst, null);
		dimRect( canvas, dst );
	}
	
	protected void doDraw(Canvas canvas)
	{
       	try {
    		log.d("doDraw() called");
    		if ( mInitialized && mCurrentPageInfo!=null ) {
        		log.d("onDraw() -- drawing page image");
        		
        		Rect dst = new Rect(0, 0, canvas.getWidth(), canvas.getHeight());
        		Rect src = new Rect(0, 0, mCurrentPageInfo.bitmap.getWidth(), mCurrentPageInfo.bitmap.getHeight());
        		if ( dontStretchWhileDrawing ) {
	        		if ( dst.right>src.right )
	        			dst.right = src.right;
	        		if ( dst.bottom>src.bottom )
	        			dst.bottom = src.bottom;
	        		if ( src.right>dst.right )
	        			src.right = dst.right;
	        		if ( src.bottom>dst.bottom )
	        			src.bottom = dst.bottom;
	        		if ( centerPageInsteadOfResizing ) {
		        		int ddx = (canvas.getWidth() - dst.width()) / 2;
		        		int ddy = (canvas.getHeight() - dst.height()) / 2;
		        		dst.left += ddx; 
		        		dst.right += ddx; 
		        		dst.top += ddy; 
		        		dst.bottom += ddy; 
	        		}
        		}
        		if ( dst.width()!=canvas.getWidth() || dst.height()!=canvas.getHeight() )
        			canvas.drawColor(Color.rgb(32, 32, 32));
        		drawDimmedBitmap(canvas, mCurrentPageInfo.bitmap, src, dst);
    		} else {
        		log.d("onDraw() -- drawing empty screen");
    			canvas.drawColor(Color.rgb(64, 64, 64));
    		}
    	} catch ( Exception e ) {
    		log.e("exception while drawing", e);
    	}
	}
	
	protected void draw()
	{
		drawCallback(new DrawCanvasCallback() {
			@Override
			public void drawTo(Canvas c) {
				doDraw(c);
			}
		}, null);
	}
	
    @Override 
    protected void onDraw(Canvas canvas) {
    	try {
    		log.d("onDraw() called");
    		draw();
//    		if ( mInitialized && mBitmap!=null ) {
//        		log.d("onDraw() -- drawing page image");
//        		Rect rc = new Rect(0, 0, mBitmap.getWidth(), mBitmap.getHeight());
//    			canvas.drawBitmap(mBitmap, rc, rc, null);
//    		} else {
//        		log.d("onDraw() -- drawing empty screen");
//    			canvas.drawColor(Color.rgb(192, 192, 192));
//    		}
    	} catch ( Exception e ) {
    		log.e("exception while drawing", e);
    	}
    }
    
    private int dimmingAlpha = 255; // no dimming
    public void setDimmingAlpha( int alpha ) {
    	if ( alpha>255 )
    		alpha = 255;
    	if ( alpha<32 )
    		alpha = 32;
    	if ( dimmingAlpha!=alpha ) {
    		dimmingAlpha = alpha;
    		mEngine.execute(new Task() {
				@Override
				public void work() throws Exception {
		    		draw();
				}
    			
    		});
    	}
    }

    private void restorePositionBackground( String pos )
    {
		BackgroundThread.ensureBackground();
    	if ( pos!=null ) {
			BackgroundThread.ensureBackground();
    		goToPositionInternal( pos );
    		preparePageImage(0);
    	}
    }
    
//    private void restorePosition()
//    {
//		BackgroundThread.ensureGUI();
//    	if ( mBookInfo!=null ) {
//    		if ( mBookInfo.getLastPosition()!=null ) {
//	    		final String pos = mBookInfo.getLastPosition().getStartPos();
//	    		post( new Task() {
//	    			public void work() {
//	    				BackgroundThread.ensureBackground();
//	    	    		goToPositionInternal( pos );
//	    	    		preparePageImage(0);
//	    			}
//	    		});
//	    		mActivity.getHistory().updateBookAccess(mBookInfo);
//    		}
//    		mActivity.getHistory().saveToDB();
//    	}
//    }
    
//    private void savePosition()
//    {
//		BackgroundThread.ensureBackground();
//    	if ( !mOpened )
//    		return;
//    	Bookmark bmk = getCurrentPageBookmarkInternal();
//    	if ( bmk!=null )
//    		log.d("saving position, bmk=" + bmk.getStartPos());
//    	else
//    		log.d("saving position: no current page bookmark obtained");
//    	if ( bmk!=null && mBookInfo!=null ) {
//        	bmk.setTimeStamp(System.currentTimeMillis());
//    		bmk.setType(Bookmark.TYPE_LAST_POSITION);
//    		mBookInfo.setLastPosition(bmk);
//    		mActivity.getHistory().updateRecentDir();
//    		mActivity.getHistory().saveToDB();
//    		saveSettings();
//    	}
//    }
    
    public Bookmark saveCurrentPositionBookmarkSync( boolean saveToDB ) {
        Bookmark bmk = mBackThread.callBackground(new Callable<Bookmark>() {
            @Override
            public Bookmark call() throws Exception {
                if ( !mOpened )
                    return null;
                return getCurrentPageBookmarkInternal();
            }
        });
        if ( bmk!=null ) {
            bmk.setTimeStamp(System.currentTimeMillis());
            bmk.setType(Bookmark.TYPE_LAST_POSITION);
            if ( mBookInfo!=null )
                mBookInfo.setLastPosition(bmk);
            if ( saveToDB ) {
                mActivity.getHistory().updateRecentDir();
                mActivity.getHistory().saveToDB();
                mActivity.getDB().flush();
            }
        }
        return bmk;
    }
    
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
                log.i("SavePositionTask.done()");
	    	}
		}

		public void work() throws Exception {
			BackgroundThread.ensureBackground();
	    	if ( !mOpened )
	    		return;
	    	bmk = getCurrentPageBookmarkInternal();
	    	if ( bmk!=null )
	    		log.d("saving position, bmk=" + bmk.getStartPos());
	    	else
	    		log.d("saving position: no current page bookmark obtained");
		}
    	
    }

    public void save()
    {
		BackgroundThread.ensureGUI();
    	post( new SavePositionTask() );
    }
    
    public void close()
    {
		BackgroundThread.ensureGUI();
    	log.i("ReaderView.close() is called");
    	if ( !mOpened )
    		return;
		cancelSwapTask();
		//save();
    	post( new Task() {
    		public void work() {
    			BackgroundThread.ensureBackground();
    			if ( mOpened ) {
	    			mOpened = false;
					log.i("ReaderView().close() : closing current document");
					doCommandInternal(ReaderCommand.DCMD_CLOSE_BOOK.nativeId, 0);
    			}
    		}
    		public void done() {
    			BackgroundThread.ensureGUI();
    			if ( currentAnimation==null ) {
	    			if (  mCurrentPageInfo!=null ) {
	    				mCurrentPageInfo.recycle();
	    				mCurrentPageInfo = null;
	    			}
	    			if (  mNextPageInfo!=null ) {
	    				mNextPageInfo.recycle();
	    				mNextPageInfo = null;
	    			}
    			} else
        	    	invalidImages = true;
    			factory.compact();
    			mCurrentPageInfo = null;
    		}
    	});
    }

    public void destroy()
    {
    	log.i("ReaderView.destroy() is called");
		BackgroundThread.ensureGUI();
    	if ( mInitialized ) {
        	//close();
        	BackgroundThread.backgroundExecutor.execute( new Runnable() {
        		public void run() {
        			BackgroundThread.ensureBackground();
        	    	if ( mInitialized ) {
        	        	log.i("ReaderView.destroyInternal() calling");
        	    		destroyInternal();
        	    		mInitialized = false;
        	    		currentBackgroundTexture = Engine.NO_TEXTURE;
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
		log.d("View.onDetachedFromWindow() is called");
	}

	private String getCSSForFormat( DocumentFormat fileFormat )
	{
		if ( fileFormat==null )
			fileFormat = DocumentFormat.FB2;
		File[] dataDirs = Engine.getDataDirectories(null, false, false);
		String defaultCss = mEngine.loadResourceUtf8(fileFormat.getCSSResourceId());
		for ( File dir : dataDirs ) {
			File file = new File( dir, fileFormat.getCssName() );
			if ( file.exists() ) {
				String css = mEngine.loadFileUtf8(file);
				if ( css!=null ) {
					int p1 = css.indexOf("@import");
					if ( p1<0 )
						p1 = css.indexOf("@include");
					int p2 = css.indexOf("\";");
					if (p1 >= 0 && p2 >= 0 && p1 < p2 ) {
						css = css.substring(0, p1) + "\n" + defaultCss + "\n" + css.substring(p2+2);
					}
					return css;
				}
			} 
		}
		return defaultCss;
	} 

	boolean enable_progress_callback = true;
    ReaderCallback readerCallback = new ReaderCallback() {
    
	    public boolean OnExportProgress(int percent) {
			BackgroundThread.ensureBackground();
	    	log.d("readerCallback.OnExportProgress " + percent);
			return true;
		}
		public void OnExternalLink(String url, String nodeXPath) {
			BackgroundThread.ensureBackground();
		}
		public void OnFormatEnd() {
			BackgroundThread.ensureBackground();
	    	log.d("readerCallback.OnFormatEnd");
			//mEngine.hideProgress();
			drawPage();
			scheduleSwapTask();
		}
		public boolean OnFormatProgress(final int percent) {
			BackgroundThread.ensureBackground();
			if ( enable_progress_callback ) {
		    	log.d("readerCallback.OnFormatProgress " + percent);
		    	mEngine.showProgress( percent*4/10 + 5000, R.string.progress_formatting);
			}
//			executeSync( new Callable<Object>() {
//				public Object call() {
//					BackgroundThread.ensureGUI();
//			    	log.d("readerCallback.OnFormatProgress " + percent);
//			    	mEngine.showProgress( percent*4/10 + 5000, R.string.progress_formatting);
//			    	return null;
//				}
//			});
			return true;
		}
		public void OnFormatStart() {
			BackgroundThread.ensureBackground();
	    	log.d("readerCallback.OnFormatStart");
		}
		public void OnLoadFileEnd() {
			BackgroundThread.ensureBackground();
	    	log.d("readerCallback.OnLoadFileEnd");
		}
		public void OnLoadFileError(String message) {
			BackgroundThread.ensureBackground();
	    	log.d("readerCallback.OnLoadFileError(" + message + ")");
		}
		public void OnLoadFileFirstPagesReady() {
			BackgroundThread.ensureBackground();
	    	log.d("readerCallback.OnLoadFileFirstPagesReady");
		}
		public String OnLoadFileFormatDetected(final DocumentFormat fileFormat) {
			BackgroundThread.ensureBackground();
			String res = executeSync( new Callable<String>() {
				public String call() {
					BackgroundThread.ensureGUI();
					log.i("readerCallback.OnLoadFileFormatDetected " + fileFormat);
					if ( fileFormat!=null ) {
						String s = getCSSForFormat(fileFormat);
						log.i("setting .css for file format " + fileFormat + " from resource " + (fileFormat!=null?fileFormat.getCssName():"[NONE]"));
						return s;
					}
			    	return null;
				}
			});
			int internalStyles = mBookInfo.getFileInfo().getFlag(FileInfo.DONT_USE_DOCUMENT_STYLES_FLAG)? 0 : 1; 
			log.d("internalStyles: " + internalStyles);
			doCommandInternal(ReaderCommand.DCMD_SET_INTERNAL_STYLES.nativeId, internalStyles);
			return res;
		}
		public boolean OnLoadFileProgress(final int percent) {
			BackgroundThread.ensureBackground();
			if ( enable_progress_callback ) {
		    	log.d("readerCallback.OnLoadFileProgress " + percent);
		    	mEngine.showProgress( percent*4/10 + 1000, R.string.progress_loading);
			}
//			executeSync( new Callable<Object>() {
//				public Object call() {
//					BackgroundThread.ensureGUI();
//			    	log.d("readerCallback.OnLoadFileProgress " + percent);
//			    	mEngine.showProgress( percent*4/10 + 1000, R.string.progress_loading);
//			    	return null;
//				}
//			});
			return true;
		}
		public void OnLoadFileStart(String filename) {
			cancelSwapTask();
			BackgroundThread.ensureBackground();
	    	log.d("readerCallback.OnLoadFileStart " + filename);
		}
	    /// Override to handle external links
	    public void OnImageCacheClear() {
	    	//log.d("readerCallback.OnImageCacheClear");
	    	clearImageCache();
	    }
    };
    
    private volatile SwapToCacheTask currentSwapTask;
	private void scheduleSwapTask() {
		currentSwapTask = new SwapToCacheTask();
		currentSwapTask.reschedule();
	}
	private void cancelSwapTask() {
		currentSwapTask = null;
	}
    private class SwapToCacheTask extends Task {
    	boolean isTimeout;
    	long startTime;
    	public SwapToCacheTask() {
    		startTime = System.currentTimeMillis();
    	}
    	public void reschedule() {
    		if ( this!=currentSwapTask )
    			return;
			BackgroundThread.instance().postGUI( new Runnable() {
				@Override
				public void run() {
					post(SwapToCacheTask.this);
				}
			}, 2000);
    	}
		@Override
		public void work() throws Exception {
    		if ( this!=currentSwapTask )
    			return;
			int res = swapToCacheInternal();
			isTimeout = res==SWAP_TIMEOUT;
			long duration = System.currentTimeMillis() - startTime;
			if ( !isTimeout ) {
				log.i("swapToCacheInternal is finished with result " + res + " in " + duration + " ms");
			} else {
				log.d("swapToCacheInternal exited by TIMEOUT in " + duration + " ms: rescheduling");
			}
		}
		@Override
		public void done() {
			if ( isTimeout )
				reschedule();
		}
		
    }
    
    private boolean invalidImages = true;
    private void clearImageCache()
    {
    	BackgroundThread.instance().postBackground( new Runnable() {
    		public void run() {
    	    	invalidImages = true;
    		}
    	});
    }

    public void setStyleSheet( final String css )
    {
		BackgroundThread.ensureGUI();
        if ( css!=null && css.length()>0 ) {
        	post(new Task() {
        		public void work() {
        			setStylesheetInternal(css);
        		}
        	});
        }
    }
    
    public void goToPosition( int position )
    {
		BackgroundThread.ensureGUI();
		doEngineCommand(ReaderView.ReaderCommand.DCMD_GO_POS, position);
    }
    
    public void moveBy( final int delta )
    {
		BackgroundThread.ensureGUI();
		log.d("moveBy(" + delta + ")");
		post(new Task() {
			public void work() {
				BackgroundThread.ensureBackground();
				doCommandInternal(ReaderCommand.DCMD_SCROLL_BY.nativeId, delta);
			}
			public void done() {
				drawPage();
			}
		});
    }
    
    public void goToPage( int pageNumber )
    {
		BackgroundThread.ensureGUI();
		doEngineCommand(ReaderView.ReaderCommand.DCMD_GO_PAGE, pageNumber-1);
    }
    
    public void goToPercent( final int percent )
    {
		BackgroundThread.ensureGUI();
    	if ( percent>=0 && percent<=100 )
	    	post( new Task() {
	    		public void work() {
	    			PositionProperties pos = getPositionPropsInternal(null);
	    			if ( pos!=null && pos.pageCount>0) {
	    				int pageNumber = pos.pageCount * percent / 100; 
						doCommandFromBackgroundThread(ReaderView.ReaderCommand.DCMD_GO_PAGE, pageNumber);
	    			}
	    		}
	    	});
    }

    public interface MoveSelectionCallback {
    	// selection is changed
    	public void onNewSelection( Selection selection );
    	// cannot move selection
    	public void onFail();
    }
    
    public void moveSelection( final ReaderCommand command, final int param, final MoveSelectionCallback callback ) {
    	post( new Task() {
    		private boolean res;
    		private Selection selection = new Selection();
			@Override
			public void work() throws Exception {
				res = moveSelectionInternal(selection, command.nativeId, param);
			}

			@Override
			public void done() {
				if ( callback!=null ) {
					clearImageCache();
					invalidate();
					drawPage();
					if ( res )
						callback.onNewSelection(selection);
					else
						callback.onFail();
				}
			}

			@Override
			public void fail(Exception e) {
				if ( callback!=null )
					callback.onFail();
			}
			
			
    		
    	});
    }
    
    @Override
    public void finalize()
    {
    	log.w("ReaderView.finalize() is called");
    	//destroyInternal();
    }

	public ReaderView(CoolReader activity, Engine engine, BackgroundThread backThread, Properties props ) 
    {
        super(activity);
        SurfaceHolder holder = getHolder();
        holder.addCallback(this);
        
		BackgroundThread.ensureGUI();
        this.mActivity = activity;
        this.mEngine = engine;
        this.mBackThread = backThread;
        setFocusable(true);
        setFocusableInTouchMode(true);
        
        mBackThread.postBackground(new Runnable() {

			@Override
			public void run() {
				log.d("ReaderView - in background thread: calling createInternal()");
				createInternal();
				mInitialized = true;
			}
        	
        });

        post(new CreateViewTask( props ));

    }

}
