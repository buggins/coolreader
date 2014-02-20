package org.coolreader.crengine;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.Callable;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import org.coolreader.CoolReader;
import org.coolreader.R;
import org.coolreader.crengine.Engine.HyphDict;
import org.coolreader.crengine.InputDialog.InputHandler;
import org.coolreader.sync.ChangeInfo;
import org.coolreader.sync.SyncServiceAccessor;
import org.koekak.android.ebookdownloader.SonyBookSelector;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.opengl.GLSurfaceView;
import android.opengl.GLU;
import android.text.ClipboardManager;
import android.util.Log;
import android.util.SparseArray;
import android.view.GestureDetector;
import android.view.GestureDetector.SimpleOnGestureListener;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.view.View.OnKeyListener;
import android.view.View.OnTouchListener;

public class ReaderView implements android.view.SurfaceHolder.Callback, Settings, OnKeyListener, OnTouchListener, OnFocusChangeListener {

	public static final Logger log = L.create("rv", Log.VERBOSE);
	public static final Logger alog = L.create("ra", Log.WARN);
	
	private final SurfaceView surface;
	private final BookView bookView;
	public SurfaceView getSurface() { return surface; }
	
	public interface BookView {
		void draw();
		void draw(boolean isPartially);
		void invalidate();
		void onPause();
		void onResume();
	}
	
	public class ReaderSurface extends SurfaceView implements BookView {

		public ReaderSurface(Context context) {
			super(context);
			// TODO Auto-generated constructor stub
		}
		@Override 
		public void onPause() {
			
		}
		@Override 
		public void onResume() {
			
		}
		@Override 
	    protected void onDraw(Canvas canvas) {
	    	try {
	    		log.d("onDraw() called");
	    		draw();
	    	} catch ( Exception e ) {
	    		log.e("exception while drawing", e);
	    	}
	    }
	    
	    @Override
		protected void onDetachedFromWindow() {
			super.onDetachedFromWindow();
			log.d("View.onDetachedFromWindow() is called");
		}

		@Override
		public boolean onTrackballEvent(MotionEvent event) {
			log.d("onTrackballEvent(" + event + ")");
			if ( mSettings.getBool(PROP_APP_TRACKBALL_DISABLED, false) ) {
				log.d("trackball is disabled in settings");
				return true;
			}
			mActivity.onUserActivity();
			return super.onTrackballEvent(event);
		}
	
		@Override
		protected void onSizeChanged(final int w, final int h, int oldw, int oldh) {
			log.i("onSizeChanged("+w + ", " + h +")");
			super.onSizeChanged(w, h, oldw, oldh);
			requestResize(w, h);
		}

		@Override
		public void onWindowVisibilityChanged(int visibility) {
			if (visibility == VISIBLE)
				startStats();
			else
				stopStats();
			super.onWindowVisibilityChanged(visibility);
		}
		 	
		@Override
		public void onWindowFocusChanged(boolean hasWindowFocus) {
			if (hasWindowFocus)
				startStats();
			else
				stopStats();
			super.onWindowFocusChanged(hasWindowFocus);
		}
		
		protected void doDraw(Canvas canvas)
		{
	       	try {
	    		log.d("doDraw() called");
	    		if (isProgressActive()) {
	        		log.d("onDraw() -- drawing progress " + (currentProgressPosition / 100));
	        		drawPageBackground(canvas);
	        		doDrawProgress(canvas, currentProgressPosition, currentProgressTitle);
	    		} else if (mInitialized && mCurrentPageInfo != null && mCurrentPageInfo.bitmap != null) {
	        		log.d("onDraw() -- drawing page image");

	        		if (currentAutoScrollAnimation != null) {
	        			currentAutoScrollAnimation.draw(canvas);
	        			return;
	        		}
	        		
	        		if (currentAnimation != null) {
	        			currentAnimation.draw(canvas);
	        			return;
	        		}

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
	        		drawPageBackground(canvas);
	    		}
	    	} catch ( Exception e ) {
	    		log.e("exception while drawing", e);
	    	}
		}
		@Override
		public void draw()
		{
			draw(false);
		}
		@Override
		public void draw(boolean isPartially)
		{
			drawCallback(new DrawCanvasCallback() {
				@Override
				public void drawTo(Canvas c) {
					doDraw(c);
				}
			}, null, isPartially);
		}

		@Override
		public void invalidate() {
			super.invalidate();
		}

	}
	
	private DocView doc;
	
    // additional key codes for Nook
    public static final int NOOK_KEY_PREV_LEFT = 96;
    public static final int NOOK_KEY_PREV_RIGHT = 98;
    public static final int NOOK_KEY_NEXT_RIGHT = 97;    
    public static final int NOOK_KEY_SHIFT_UP = 101;
    public static final int NOOK_KEY_SHIFT_DOWN = 100;

    // nook 1 & 2
    public static final int NOOK_12_KEY_NEXT_LEFT = 95;
   
    // Nook touch buttons
    public static final int KEYCODE_PAGE_BOTTOMLEFT = 0x5d; // fwd = 93 (
    //    public static final int KEYCODE_PAGE_BOTTOMRIGHT = 158; // 0x5f; // fwd = 95
    public static final int KEYCODE_PAGE_TOPLEFT = 0x5c; // back = 92
    public static final int KEYCODE_PAGE_TOPRIGHT = 0x5e; // back = 94
    
    public static final int SONY_DPAD_UP_SCANCODE = 105;
    public static final int SONY_DPAD_DOWN_SCANCODE = 106;
    public static final int SONY_DPAD_LEFT_SCANCODE = 125;
    public static final int SONY_DPAD_RIGHT_SCANCODE = 126;
    
    public static final int KEYCODE_ESCAPE = 111; // KeyEvent constant since API 11

    //    public static final int SONY_MENU_SCANCODE = 357;
//    public static final int SONY_BACK_SCANCODE = 158;
//    public static final int SONY_HOME_SCANCODE = 102;
    
    public static final int PAGE_ANIMATION_NONE = 0;
    public static final int PAGE_ANIMATION_PAPER = 1;
    public static final int PAGE_ANIMATION_SLIDE = 2;
    public static final int PAGE_ANIMATION_SLIDE2 = 3;
    public static final int PAGE_ANIMATION_MAX = 3;
    
    public static final int SEL_CMD_SELECT_FIRST_SENTENCE_ON_PAGE = 1;
    public static final int SEL_CMD_NEXT_SENTENCE = 2;
    public static final int SEL_CMD_PREV_SENTENCE = 3;
    
    // Double tap selections within this radius are are assumed to be attempts to select a single point 
    public static final int DOUBLE_TAP_RADIUS = 60;
    
    private ViewMode viewMode = ViewMode.PAGES;
    
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

	private final CoolReader mActivity;
    private final Engine mEngine;
    
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
	
	public ReaderAction findTapZoneAction(int zone, int tapActionType) {
		ReaderAction action = ReaderAction.NONE;
		boolean isSecondaryAction = (secondaryTapActionType == tapActionType);
		if (tapActionType == TAP_ACTION_TYPE_SHORT) {
			action = ReaderAction.findForTap(zone, mSettings);
		} else {
			if (isSecondaryAction)
				action = ReaderAction.findForLongTap(zone, mSettings);
			else if (doubleTapSelectionEnabled || tapActionType == TAP_ACTION_TYPE_LONGPRESS)
				action = ReaderAction.START_SELECTION;
		}
		return action;
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
//	boolean VOLUME_KEYS_ZOOM = false;
	
	//private boolean backKeyDownHere = false;


	private long statStartTime;
	private long statTimeElapsed;

	public void startStats() {
		if (statStartTime == 0) {
			statStartTime = android.os.SystemClock.uptimeMillis();
			log.d("stats: started reading");
		}
	}

	public void stopStats() {
		if (statStartTime > 0) {
			statTimeElapsed += android.os.SystemClock.uptimeMillis() - statStartTime;
			statStartTime = 0;
			log.d("stats: stopped reading");
		}
	}

	public long getTimeElapsed() {
		if (statStartTime > 0)
			return statTimeElapsed + android.os.SystemClock.uptimeMillis() - statStartTime;
		else
			return statTimeElapsed++;
	}

	public void setTimeElapsed(long timeElapsed) {
		statTimeElapsed = timeElapsed;
	}

	public void onAppPause() {
		stopTracking();
		if (currentAutoScrollAnimation != null)
			stopAutoScroll();
		saveCurrentPositionBookmarkSync(true);
		log.i("calling bookView.onPause()");
		bookView.onPause();
	}

	public void onAppResume() {
		log.i("calling bookView.onResume()");
		bookView.onResume();
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
		if (currentTapHandler != null)
			currentTapHandler.cancel();
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


	private KeyEvent trackedKeyEvent = null; 
	private ReaderAction actionToRepeat = null;
	private boolean repeatActionActive = false;
	private SparseArray<Long> keyDownTimestampMap = new SparseArray<Long>();
	
	private int translateKeyCode(int keyCode) {
		if (DeviceInfo.REVERT_LANDSCAPE_VOLUME_KEYS && (mActivity.getScreenOrientation() & 1) != 0) {
			if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN)
				return KeyEvent.KEYCODE_VOLUME_UP;
			if (keyCode == KeyEvent.KEYCODE_VOLUME_UP)
				return KeyEvent.KEYCODE_VOLUME_DOWN;
		}
		return keyCode;
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
				doc.updateSelection(sel);
				if ( !sel.isEmpty() ) {
					invalidImages = true;
					BitmapInfo bi = preparePageImage(0);
					if ( bi!=null ) {
						bookView.draw(true);
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

	public static boolean isMultiSelection(Selection sel){
		String str = sel.text;
	    if(str != null){
	        for(int i = 0; i < str.length(); i++){
	            if(Character.isWhitespace(str.charAt(i))){
	                return true;
	            }
	        }
	    }
	    return false;
	}	
	
	private int mSelectionAction = SELECTION_ACTION_TOOLBAR;
	private int mMultiSelectionAction = SELECTION_ACTION_TOOLBAR;
	private void onSelectionComplete( Selection sel ) {
		int iSelectionAction;
		iSelectionAction = isMultiSelection(sel) ? mMultiSelectionAction : mSelectionAction;
		
		switch ( iSelectionAction ) {
		case SELECTION_ACTION_TOOLBAR:
			SelectionToolbarDlg.showDialog(mActivity, ReaderView.this, sel);
			break;
		case SELECTION_ACTION_COPY:
			copyToClipboard(sel.text);
			clearSelection();
			break;
		case SELECTION_ACTION_DICTIONARY:
			mActivity.findInDictionary( sel.text );
			if (!getSettings().getBool(PROP_APP_SELECTION_PERSIST, false))
				clearSelection();
			break;
		case SELECTION_ACTION_BOOKMARK:
			clearSelection();
			showNewBookmarkDialog( sel );
			break;
		case SELECTION_ACTION_FIND:
			clearSelection();
			showSearchDialog(sel.text);
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
		BookmarkEditDialog dlg = new BookmarkEditDialog(mActivity, this, bmk, true);
		dlg.show();
	}
	
	public void sendQuotationInEmail( Selection sel ) {
        StringBuilder buf = new StringBuilder();
        if (mBookInfo.getFileInfo().authors!=null)
        	buf.append("|" + mBookInfo.getFileInfo().authors + "\n");
        if (mBookInfo.getFileInfo().title!=null)
        	buf.append("|" + mBookInfo.getFileInfo().title + "\n");
        if (sel.chapter!=null && sel.chapter.length()>0)
        	buf.append("|" + sel.chapter + "\n");
    	buf.append(sel.text + "\n");
    	mActivity.sendBookFragment(mBookInfo, buf.toString());
	}
	
	public void copyToClipboard( String text ) {
		if ( text!=null && text.length()>0 ) {
			ClipboardManager cm = mActivity.getClipboardmanager();
			cm.setText(text);
			log.i("Setting clipboard text: " + text);
			mActivity.showToast("Selection text copied to clipboard");
		}
	}
	
//	private void cancelSelection() {
//		//
//		selectionInProgress = false;
//		clearSelection();
//	}

	private int isBacklightControlFlick = 1;
	private boolean isTouchScreenEnabled = true;
//	private boolean isManualScrollActive = false;
//	private boolean isBrightnessControlActive = false;
//	private int manualScrollStartPosX = -1;
//	private int manualScrollStartPosY = -1;
//	volatile private boolean touchEventIgnoreNextUp = false;
//	volatile private int longTouchId = 0;
//	volatile private long currentDoubleTapActionStart = 0;
//	private boolean selectionInProgress = false;
//	private int selectionStartX = 0;
//	private int selectionStartY = 0;
//	private int selectionEndX = 0;
//	private int selectionEndY = 0;
	private boolean doubleTapSelectionEnabled = false;
	private boolean gesturePageFlippingEnabled = true;
	private int secondaryTapActionType = TAP_ACTION_TYPE_LONGPRESS;
	private boolean selectionModeActive = false;
	
	public void toggleSelectionMode() {
		selectionModeActive = !selectionModeActive;
		mActivity.showToast( selectionModeActive ? R.string.action_toggle_selection_mode_on : R.string.action_toggle_selection_mode_off);
	}

	private ImageViewer currentImageViewer;
	private class ImageViewer extends SimpleOnGestureListener {
		private ImageInfo currentImage;
		final GestureDetector detector;
		int oldOrientation;
		public ImageViewer(ImageInfo image) {
			lockOrientation();
			detector = new GestureDetector(this);
			if (image.bufHeight / image.height >= 2 && image.bufWidth / image.width >= 2) {
				image.scaledHeight *= 2;
				image.scaledWidth *= 2;
			}
			centerIfLessThanScreen(image);
			currentImage = image;
		}
		
		private void lockOrientation() {
			oldOrientation = mActivity.getScreenOrientation();
			if (oldOrientation == 4)
				mActivity.setScreenOrientation(mActivity.getOrientationFromSensor());
		}

		private void unlockOrientation() {
			if (oldOrientation == 4)
				mActivity.setScreenOrientation(oldOrientation);
		}

		private void centerIfLessThanScreen(ImageInfo image) {
			if (image.scaledHeight < image.bufHeight)
				image.y = (image.bufHeight - image.scaledHeight) / 2;
			if (image.scaledWidth < image.bufWidth)
				image.x = (image.bufWidth - image.scaledWidth) / 2;
		}
		
		private void fixScreenBounds(ImageInfo image) {
			if (image.scaledHeight > image.bufHeight) {
				if (image.y < image.bufHeight - image.scaledHeight)
					image.y = image.bufHeight - image.scaledHeight;
				if (image.y > 0)
					image.y = 0;
			}
			if (image.scaledWidth > image.bufWidth) {
				if (image.x < image.bufWidth - image.scaledWidth)
					image.x = image.bufWidth - image.scaledWidth;
				if (image.x > 0)
					image.x = 0;
			}
		}
		
		private void updateImage(ImageInfo image) {
			centerIfLessThanScreen(image);
			fixScreenBounds(image);
			if (!currentImage.equals(image)) {
				currentImage = image;
				drawPage();
			}
		}
		
		public void zoomIn() {
			ImageInfo image = new ImageInfo(currentImage);
			if (image.scaledHeight >= image.height) {
				int scale = image.scaledHeight / image.height;
				if (scale < 4)
					scale++;
				image.scaledHeight = image.height * scale;
				image.scaledWidth = image.width * scale;
			} else {
				int scale = image.height / image.scaledHeight;
				if (scale > 1)
					scale--;
				image.scaledHeight = image.height / scale;
				image.scaledWidth = image.width / scale;
			}
			updateImage(image);
		}
		
		public void zoomOut() {
			ImageInfo image = new ImageInfo(currentImage);
			if (image.scaledHeight > image.height) {
				int scale = image.scaledHeight / image.height;
				if (scale > 1)
					scale--;
				image.scaledHeight = image.height * scale;
				image.scaledWidth = image.width * scale;
			} else {
				int scale = image.height / image.scaledHeight;
				if (image.scaledHeight > image.bufHeight || image.scaledWidth > image.bufWidth)
					scale++;
				image.scaledHeight = image.height / scale;
				image.scaledWidth = image.width / scale;
			}
			updateImage(image);
		}
		
		public int getStep() {
			ImageInfo image = currentImage;
			int max = image.bufHeight;
			if (max < image.bufWidth)
				max = image.bufWidth;
			return max / 10;
		}
		
		public void moveBy(int dx, int dy) {
			ImageInfo image = new ImageInfo(currentImage);
			image.x += dx;
			image.y += dy;
			updateImage(image);
		}
		
		public boolean onKeyDown(int keyCode, final KeyEvent event) {
			if (keyCode == 0)
				keyCode = event.getScanCode();
			switch (keyCode) {
			case KeyEvent.KEYCODE_VOLUME_UP:
				zoomIn();
				return true;
			case KeyEvent.KEYCODE_VOLUME_DOWN:
				zoomOut();
				return true;
			case KeyEvent.KEYCODE_DPAD_CENTER:
			case KeyEvent.KEYCODE_BACK:
			case KeyEvent.KEYCODE_ENDCALL:
				close();
				return true;
			case KeyEvent.KEYCODE_DPAD_LEFT:
				moveBy(getStep(), 0);
				return true;
			case KeyEvent.KEYCODE_DPAD_RIGHT:
				moveBy(-getStep(), 0);
				return true;
			case KeyEvent.KEYCODE_DPAD_UP:
				moveBy(0, getStep());
				return true;
			case KeyEvent.KEYCODE_DPAD_DOWN:
				moveBy(0, -getStep());
				return true;
			}
			return false;
		}

		public boolean onKeyUp(int keyCode, final KeyEvent event) {
			if (keyCode == 0)
				keyCode = event.getScanCode();
			switch (keyCode) {
			case KeyEvent.KEYCODE_BACK:
			case KeyEvent.KEYCODE_ENDCALL:
				close();
				return true;
			}
			return false;
		}

		public boolean onTouchEvent(MotionEvent event) {
//			int aindex = event.getActionIndex();
//			if (event.getAction() == MotionEvent.ACTION_POINTER_DOWN) {
//				log.v("ACTION_POINTER_DOWN");
//			}
			return detector.onTouchEvent(event);
		}
		
		
		
		@Override
		public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX,
				float velocityY) {
			log.v("onFling()");
			return true;
		}

		@Override
		public boolean onScroll(MotionEvent e1, MotionEvent e2,
				float distanceX, float distanceY) {
			log.v("onScroll() " + distanceX + ", " + distanceY);
			int dx = (int)distanceX;
			int dy = (int)distanceY;
			moveBy(-dx, -dy);
			return true;
		}

		@Override
		public boolean onSingleTapConfirmed(MotionEvent e) {
			log.v("onSingleTapConfirmed()");
			ImageInfo image = new ImageInfo(currentImage);
			
			int x = (int)e.getX();
			int y = (int)e.getY();

			int zone = 0;
			int zw = mActivity.getDensityDpi() / 2;
			int w = image.bufWidth;
			int h = image.bufHeight;
			if (image.rotation == 0) {
				if (x < zw && y > h - zw)
					zone = 1;
				if (x > w - zw && y > h - zw)
					zone = 2;
			} else {
				if (x < zw && y < zw)
					zone = 1;
				if (x < zw && y > h - zw)
					zone = 2;
			}
			if (zone != 0) {
				if (zone == 1)
					zoomIn();
				else
					zoomOut();
				return true;
			}
			
			close();
			return super.onSingleTapConfirmed(e);
		}
		
		@Override
		public boolean onDown(MotionEvent e) {
			return true;
		}

		public void close() {
			if (currentImageViewer == null)
				return;
			currentImageViewer = null;
			unlockOrientation();
			BackgroundThread.instance().postBackground(new Runnable() {
				@Override
				public void run() {
					doc.closeImage();
				}
			});
			drawPage();
		}
		
		public BitmapInfo prepareImage() {
			// called from background thread
			ImageInfo img = currentImage;
			img.bufWidth = internalDX;
			img.bufHeight = internalDY;
			if (mCurrentPageInfo != null) {
				if (img.equals(mCurrentPageInfo.imageInfo))
					return mCurrentPageInfo;
				mCurrentPageInfo.recycle();
				mCurrentPageInfo = null;
			}
			PositionProperties currpos = doc.getPositionProps(null);
			BitmapInfo bi = new BitmapInfo();
	        bi.imageInfo = new ImageInfo(img);
			bi.bitmap = factory.get(internalDX, internalDY);
			bi.position = currpos;
			doc.drawImage(bi.bitmap, bi.imageInfo);
	        mCurrentPageInfo = bi;
	        return mCurrentPageInfo;
		}
		
	}

	private void startImageViewer(ImageInfo image) {
		currentImageViewer = new ImageViewer(image);
		drawPage();
	}

	private boolean isImageViewMode() {
		return currentImageViewer != null;
	}

	private void stopImageViewer() {
		if (currentImageViewer != null)
			currentImageViewer.close();
	}

	private TapHandler currentTapHandler = null;
	public class TapHandler {

		private final static int STATE_INITIAL = 0; // no events yet
		private final static int STATE_DOWN_1 = 1; // down first time
		private final static int STATE_SELECTION = 3; // selection is started
		private final static int STATE_FLIPPING = 4; // flipping is in progress
		private final static int STATE_WAIT_FOR_DOUBLE_CLICK = 5; // flipping is in progress
		private final static int STATE_DONE = 6; // done: no more tracking
		private final static int STATE_BRIGHTNESS = 7; // brightness change in progress
		
		private final static int EXPIRATION_TIME_MS = 180000;
		
		int state = STATE_INITIAL;
		
		int start_x = 0;
		int start_y = 0;
		int width = 0;
		int height = 0;
		ReaderAction shortTapAction = ReaderAction.NONE;
		ReaderAction longTapAction = ReaderAction.NONE;
		ReaderAction doubleTapAction = ReaderAction.NONE;
		long firstDown;
		
		/// handle unexpected event for state: stop tracking
		private boolean unexpectedEvent() {
			cancel();
			return true; // ignore
		}
		
		public boolean isInitialState() {
			return state == STATE_INITIAL;
		}
		public void checkExpiration() {
			if (state != STATE_INITIAL && Utils.timeInterval(firstDown) > EXPIRATION_TIME_MS)
				cancel();
		}
		
		/// cancel current action and reset touch tracking state
		private boolean cancel() {
			if (state == STATE_INITIAL)
				return true;
			switch (state) {
			case STATE_DOWN_1:
			case STATE_SELECTION:
				clearSelection();
				break;
			case STATE_FLIPPING:
				stopAnimation(-1, -1);
				break;
			case STATE_WAIT_FOR_DOUBLE_CLICK:
			case STATE_DONE:
			case STATE_BRIGHTNESS:
				stopBrightnessControl(-1, -1);
				break;
			}
			state = STATE_DONE;
			unhiliteTapZone(); 
			currentTapHandler = new TapHandler();
			return true;
		}

		/// perform action and reset touch tracking state
		private boolean performAction(final ReaderAction action, boolean checkForLinks) {
			log.d("performAction on touch: " + action);
			state = STATE_DONE;

			currentTapHandler = new TapHandler();

			if (!checkForLinks) {
				onAction(action);
				return true;
			}
			
			// check link before executing action
			mEngine.execute(new Task() {
				String link;
				ImageInfo image;
				Bookmark bookmark;
				public void work() {
					image = new ImageInfo();
					image.bufWidth = internalDX;
					image.bufHeight = internalDY;
					image.bufDpi = mActivity.getDensityDpi();
					if (doc.checkImage(start_x, start_y, image)) {
						return;
					}
					image = null;
					link = doc.checkLink(start_x, start_y, mActivity.getPalmTipPixels() / 2 );
					if ( link!=null ) {
						if ( link.startsWith("#") ) {
							log.d("go to " + link);
							doc.goLink(link);
							drawPage();
						}
						return;
					} 
					bookmark = doc.checkBookmark(start_x, start_y);
					if (bookmark != null && bookmark.getType() == Bookmark.TYPE_POSITION)
						bookmark = null;
				}
				public void done() {
					if (bookmark != null)
						bookmark = mBookInfo.findBookmark(bookmark);
					if (link == null && image == null && bookmark == null) {
						onAction(action);
					} else if (image != null) {
						startImageViewer(image);
					} else if (bookmark != null) {
						BookmarkEditDialog dlg = new BookmarkEditDialog(mActivity, ReaderView.this, bookmark, false);
						dlg.show();
					} else if (!link.startsWith("#")) {
						log.d("external link " + link);
						if (link.startsWith("http://")||link.startsWith("https://")) {
							mActivity.openURL(link);
						} else {
							// absolute path to file
							FileInfo fi = new FileInfo(link);
							if (fi.exists()) {
								mActivity.loadDocument(fi);
								return;
							}
							File baseDir = null;
							if (mBookInfo!=null && mBookInfo.getFileInfo()!=null) {
								if (!mBookInfo.getFileInfo().isArchive) {
									// relatively to base directory
									File f = new File(mBookInfo.getFileInfo().getBasePath());
									baseDir = f.getParentFile();
									String url = link;
									while (baseDir!=null && url!=null && url.startsWith("../")) {
										baseDir = baseDir.getParentFile();
										url = url.substring(3);
									}
									if (baseDir!=null && url!=null && url.length()>0) {
										fi = new FileInfo(baseDir.getAbsolutePath()+"/"+url);
										if (fi.exists()) {
											mActivity.loadDocument(fi);
											return;
										}
									}
								} else {
									// from archive
									fi = new FileInfo(mBookInfo.getFileInfo().getArchiveName() + FileInfo.ARC_SEPARATOR + link);
									if (fi.exists()) {
										mActivity.loadDocument(fi);
										return;
									}
								}
							}
							mActivity.showToast("Cannot open link " + link);
						}
					}
				}
			});
			return true;
		}
		
		private boolean startSelection() {
			state = STATE_SELECTION;
			// check link before executing action
			mEngine.execute(new Task() {
				ImageInfo image;
				Bookmark bookmark;
				public void work() {
					image = new ImageInfo();
					image.bufWidth = internalDX;
					image.bufHeight = internalDY;
					image.bufDpi = mActivity.getDensityDpi();
					if (!doc.checkImage(start_x, start_y, image))
						image = null;
					bookmark = doc.checkBookmark(start_x, start_y);
					if (bookmark != null && bookmark.getType() == Bookmark.TYPE_POSITION)
						bookmark = null;
				}
				public void done() {
					if (bookmark != null)
						bookmark = mBookInfo.findBookmark(bookmark);
					if (image != null) {
						cancel();
						startImageViewer(image);
					} else if (bookmark != null) {
						cancel();
						BookmarkEditDialog dlg = new BookmarkEditDialog(mActivity, ReaderView.this, bookmark, false);
						dlg.show();
					} else {
						updateSelection( start_x, start_y, start_x, start_y, false );
					}
				}
			});
			return true;
		}
		
		private boolean trackDoubleTap() {
			state = STATE_WAIT_FOR_DOUBLE_CLICK;
			BackgroundThread.instance().postGUI(new Runnable() {
				@Override
				public void run() {
					if (currentTapHandler == TapHandler.this && state == STATE_WAIT_FOR_DOUBLE_CLICK)
						performAction(shortTapAction, false);
				}
			}, DOUBLE_CLICK_INTERVAL);
			return true;
		}
		
		private boolean trackLongTap() {
			BackgroundThread.instance().postGUI(new Runnable() {
				@Override
				public void run() {
					if (currentTapHandler == TapHandler.this && state == STATE_DOWN_1) {
						if (longTapAction == ReaderAction.START_SELECTION)
							startSelection();
						else
							performAction(longTapAction, true);
					}
				}
			}, LONG_KEYPRESS_TIME);
			return true;
		}
		
		public boolean onTouchEvent(MotionEvent event) {
			int x = (int)event.getX();
			int y = (int)event.getY();
			if ((DeviceInfo.getSDKLevel() >= 19) && mActivity.isFullscreen() && (event.getAction() == MotionEvent.ACTION_DOWN)) {
				if ((y < 30) || (y > (getSurface().getHeight() - 30))) 
					return unexpectedEvent();
			}			

			if (state == STATE_INITIAL && event.getAction() != MotionEvent.ACTION_DOWN)
				return unexpectedEvent(); // ignore unexpected event
			
			if (event.getAction() == MotionEvent.ACTION_UP) {
				long duration = Utils.timeInterval(firstDown);
				switch (state) {
				case STATE_DOWN_1:
					if ( hiliteTapZoneOnTap ) {
						hiliteTapZone( true, x, y, width, height );
						scheduleUnhilite( LONG_KEYPRESS_TIME );
					}
					if (duration > LONG_KEYPRESS_TIME) {
						if (longTapAction == ReaderAction.START_SELECTION)
							return startSelection();
						return performAction(longTapAction, true);
					}
					if (doubleTapAction.isNone())
						return performAction(shortTapAction, false);
					// start possible double tap tracking
					return trackDoubleTap();
				case STATE_FLIPPING:
					stopAnimation(x, y);
					state = STATE_DONE;
					return cancel();
				case STATE_BRIGHTNESS:
					stopBrightnessControl(x, y);
					state = STATE_DONE;
					return cancel();
				case STATE_SELECTION:
					// If the second tap is within a radius of the first tap point, assume the user is trying to double tap on the same point 
					if ( start_x-x <= DOUBLE_TAP_RADIUS && x-start_x <= DOUBLE_TAP_RADIUS && y-start_y <= DOUBLE_TAP_RADIUS && start_y-y <= DOUBLE_TAP_RADIUS )
						updateSelection( start_x, start_y, start_x, start_y, true );
					else
						updateSelection( start_x, start_y, x, y, true );
					selectionModeActive = false;
					state = STATE_DONE;
					return cancel();
				}
			} else if (event.getAction() == MotionEvent.ACTION_DOWN) {
				switch (state) {
				case STATE_INITIAL:
					start_x = x;
					start_y = y;
					width = surface.getWidth();
					height = surface.getHeight();
					int zone = getTapZone(x, y, width, height);
					shortTapAction = findTapZoneAction(zone, TAP_ACTION_TYPE_SHORT);
					longTapAction = findTapZoneAction(zone, TAP_ACTION_TYPE_LONGPRESS);
					doubleTapAction = findTapZoneAction(zone, TAP_ACTION_TYPE_DOUBLE);
					firstDown = Utils.timeStamp();
					if (selectionModeActive) {
						startSelection();
					} else {
						state = STATE_DOWN_1;
						trackLongTap();
					}
					return true;
				case STATE_DOWN_1:
				case STATE_BRIGHTNESS:
				case STATE_FLIPPING:
				case STATE_SELECTION:
					return unexpectedEvent();
				case STATE_WAIT_FOR_DOUBLE_CLICK:
					if (doubleTapAction == ReaderAction.START_SELECTION)
						return startSelection();
					return performAction(doubleTapAction, true);
				}
			} else if (event.getAction() == MotionEvent.ACTION_MOVE) {
				int dx = x - start_x;
				int dy = y - start_y;
				int adx = dx > 0 ? dx : -dx;
				int ady = dy > 0 ? dy : -dy;
				int distance = adx + ady;
				int dragThreshold = mActivity.getPalmTipPixels();
				switch (state) {
				case STATE_DOWN_1:
					if (distance < dragThreshold)
						return true;
					if (!DeviceInfo.EINK_SCREEN && isBacklightControlFlick != BACKLIGHT_CONTROL_FLICK_NONE && ady > adx) {
						// backlight control enabled
						if (start_x < dragThreshold * 170 / 100 && isBacklightControlFlick == 1
								|| start_x > width - dragThreshold * 170 / 100 && isBacklightControlFlick == 2) {
							// brightness
							state = STATE_BRIGHTNESS;
							startBrightnessControl(start_x, start_y);
							return true;
						}
					}
					boolean isPageMode = mSettings.getInt(PROP_PAGE_VIEW_MODE, 1) == 1;
					int dir = isPageMode ? x - start_x : y - start_y;
					if (gesturePageFlippingEnabled) {
						if (pageFlipAnimationSpeedMs == 0 || DeviceInfo.EINK_SCREEN) {
							// no animation
							return performAction(dir < 0 ? ReaderAction.PAGE_DOWN : ReaderAction.PAGE_UP, false);
						}
						startAnimation(start_x, start_y, width, height, x, y);
						updateAnimation(x, y);
						state = STATE_FLIPPING;
					}
					return true;
				case STATE_FLIPPING:
					updateAnimation(x, y);
					return true;
				case STATE_BRIGHTNESS:
					updateBrightnessControl(x, y);
					return true;
				case STATE_WAIT_FOR_DOUBLE_CLICK:
					return true;
				case STATE_SELECTION:
					updateSelection( start_x, start_y, x, y, false );
					break;
				}
				
			} else if (event.getAction() == MotionEvent.ACTION_OUTSIDE) {
				return unexpectedEvent();
			}
			return true;
		}
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
				toc = doc.getTOC();
				pos = doc.getPositionProps(null);
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
	
	public void showSearchDialog(String initialText)
	{
		if (initialText != null && initialText.length() > 40)
			initialText = initialText.substring(0, 40);
		BackgroundThread.ensureGUI();
		SearchDlg dlg = new SearchDlg( mActivity, this, initialText);
		dlg.show();
	}

    public void findText( final String pattern, final boolean reverse, final boolean caseInsensitive )
    {
		BackgroundThread.ensureGUI();
		final ReaderView view = this; 
		mEngine.execute(new Task() {
			public void work() throws Exception {
				BackgroundThread.ensureBackground();
				boolean res = doc.findText( pattern, 1, reverse?1:0, caseInsensitive?1:0);
				if ( !res )
					res = doc.findText( pattern, -1, reverse?1:0, caseInsensitive?1:0);
				if ( !res ) {
					doc.clearSelection();
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
				boolean res = doc.findText( pattern, 1, reverse?1:0, caseInsensitive?1:0);
				if ( !res )
					res = doc.findText( pattern, -1, reverse?1:0, caseInsensitive?1:0);
				if ( !res ) {
					doc.clearSelection();
					throw new Exception("pattern not found");
				}
			}
			public void done() {
				BackgroundThread.ensureGUI();
//				drawPage();
				drawPage(true);
			}
		});
    }
    
    private boolean flgHighlightBookmarks = false;
    public void clearSelection()
    {
		BackgroundThread.ensureGUI();
    	if (mBookInfo == null || !isBookLoaded())
    		return;
		mEngine.post(new Task() {
			public void work() throws Exception {
				doc.clearSelection();
				invalidImages = true;
			}
			public void done() {
				if (surface.isShown())
					drawPage(true);
			}
		});
    }
    
    public void highlightBookmarks() {
		BackgroundThread.ensureGUI();
    	if (mBookInfo == null || !isBookLoaded())
    		return;
    	int count = mBookInfo.getBookmarkCount();
    	final Bookmark[] list = (count > 0 && flgHighlightBookmarks) ? new Bookmark[count] : null; 
    	for (int i=0; i<count && flgHighlightBookmarks; i++)
    		list[i] = mBookInfo.getBookmark(i);
		mEngine.post(new Task() {
			public void work() throws Exception {
		    	doc.hilightBookmarks(list);
				invalidImages = true;
			}
			public void done() {
				if (surface.isShown())
					drawPage(true);
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
				doc.goToPosition(pos, true);
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
	
	public SyncServiceAccessor getSyncService() {
		return mActivity.getSyncService();
	}
	
	public Bookmark removeBookmark(final Bookmark bookmark) {
		Bookmark removed = mBookInfo.removeBookmark(bookmark);
		if (removed != null) {
            getSyncService().removeBookmark(mBookInfo.getFileInfo().getPathName(), removed);
			if ( removed.getId()!=null ) {
				mActivity.getDB().deleteBookmark(removed);
			}
			highlightBookmarks();
		}
		return removed;
	}

	public Bookmark updateBookmark(final Bookmark bookmark) {
		Bookmark bm = mBookInfo.updateBookmark(bookmark);
		if (bm != null) {
	        scheduleSaveCurrentPositionBookmark(DEF_SAVE_POSITION_INTERVAL);
	        highlightBookmarks();
		}
		return bm;
	}
	
	public void addBookmark(final Bookmark bookmark) {
		mBookInfo.addBookmark(bookmark);
		getSyncService().saveBookmark(mBookInfo.getFileInfo().getPathName(), bookmark, false);
        highlightBookmarks();
        scheduleSaveCurrentPositionBookmark(DEF_SAVE_POSITION_INTERVAL);
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
					bm = doc.getCurrentPageBookmark();
					bm.setShortcut(shortcut);
				}
			}
			public void done() {
				if ( mBookInfo!=null && bm!=null ) {
					if ( shortcut==0 )
						mBookInfo.addBookmark(bm);
					else
						mBookInfo.setShortcutBookmark(shortcut, bm);
					getSyncService().saveBookmark(mBookInfo.getFileInfo().getPathName(), bm, false);
					mActivity.getDB().saveBookInfo(mBookInfo);
					String s;
					if ( shortcut==0 )
						s = mActivity.getString(R.string.toast_position_bookmark_is_set);
					else {
						s = mActivity.getString(R.string.toast_shortcut_bookmark_is_set);
						s.replace("$1", String.valueOf(shortcut));
					}
			        highlightBookmarks();
					mActivity.showToast(s);
			        scheduleSaveCurrentPositionBookmark(DEF_SAVE_POSITION_INTERVAL);
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
		//setSettings(settings, mActivity.settings());
		mActivity.setSettings(settings, 60000, true);
		invalidImages = true;
	}
	
	public boolean isNightMode() {
		return mSettings.getBool(PROP_NIGHT_MODE, false);
	}

	public String getSetting( String name ) {
		return mSettings.getProperty(name);
	}

	public void setSetting(String name, String value, boolean invalidateImages, boolean save, boolean apply) {
		mActivity.setSetting(name, value, apply);
		invalidImages = true;
	}
	
	public void setSetting( String name, String value ) {
		setSetting(name, value, true, false, true);
	}
	
	public void saveSetting( String name, String value ) {
		setSetting(name, value, true, true, true);
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
	
	public void showReadingPositionPopup()
	{
		if (mBookInfo==null)
			return;
		final StringBuilder buf = new StringBuilder();
//		if (mActivity.isFullscreen()) {
		buf.append( Utils.formatTime(mActivity, System.currentTimeMillis()) +  " ");
		if (mBatteryState>=0)
 			buf.append(" [" + mBatteryState + "%]\n");
//		}
		execute( new Task() {
			Bookmark bm;
			@Override
			public void work() {
				bm = doc.getCurrentPageBookmark();
				if ( bm!=null ) {
					PositionProperties prop = doc.getPositionProps(bm.getStartPos());
					if ( prop.pageMode!=0 ) {
						buf.append("" + (prop.pageNumber+1) + " / " + prop.pageCount + "   ");
					}
					int percent = (int)(10000 * (long)prop.y / prop.fullHeight);
					buf.append("" + (percent/100) + "." + (percent%100) + "%" );

 					// Show chapter details if book has more than one chapter
 					TOCItem toc = doc.getTOC();
 					if ( toc!=null && toc.getChildCount() > 1) {
 						TOCItem chapter = toc.getChapterAtPage(prop.pageNumber);
 
 						String chapterName = chapter.getName();
 						if (chapterName!=null && chapterName.length()>30)
 							chapterName = chapterName.substring(0, 30) + "...";
 
 						TOCItem nextChapter = chapter.getNextChapter();
 						int iChapterEnd = (nextChapter != null) ? nextChapter.getPage() : prop.pageCount;
 						
 						String chapterPos = null;
 						if ( prop.pageMode!=0 ) {
 							int iChapterStart = chapter.getPage();
 							int iChapterLen = iChapterEnd - iChapterStart;
 							int iChapterPage = prop.pageNumber - iChapterStart + 1;
 
 							chapterPos = "  (" + iChapterPage + " / " + iChapterLen + ")";
 						}
 						
 						if (chapterName != null && chapterName.length() > 0)
 				 			buf.append("\n" + chapterName);
 						if (chapterPos != null && chapterPos.length() > 0)
 							buf.append(chapterPos);
 					}
				}
			}
			public void done() {
				mActivity.showToast(buf.toString());
			}
		});
	}

	public void toggleTitlebar()
	{
		boolean newBool = "1".equals(getSetting(PROP_STATUS_LINE));
		String newValue = !newBool ? "1" : "0";
		mActivity.setSetting(PROP_STATUS_LINE, newValue, true);
	}
	
	public void toggleDocumentStyles() {
		if ( mOpened && mBookInfo!=null ) {
			log.d("toggleDocumentStyles()");
			boolean disableInternalStyles = mBookInfo.getFileInfo().getFlag(FileInfo.DONT_USE_DOCUMENT_STYLES_FLAG);
			disableInternalStyles = !disableInternalStyles;
			mBookInfo.getFileInfo().setFlag(FileInfo.DONT_USE_DOCUMENT_STYLES_FLAG, disableInternalStyles);
            doEngineCommand(ReaderCommand.DCMD_SET_INTERNAL_STYLES, disableInternalStyles ? 0 : 1);
            doEngineCommand(ReaderCommand.DCMD_REQUEST_RENDER, 1);
            mActivity.getDB().saveBookInfo(mBookInfo);
		}
	}
	
	public void toggleEmbeddedFonts() {
		if ( mOpened && mBookInfo!=null ) {
			log.d("toggleEmbeddedFonts()");
			boolean enableInternalFonts = mBookInfo.getFileInfo().getFlag(FileInfo.USE_DOCUMENT_FONTS_FLAG);
			enableInternalFonts = !enableInternalFonts;
			mBookInfo.getFileInfo().setFlag(FileInfo.USE_DOCUMENT_FONTS_FLAG, enableInternalFonts);
            doEngineCommand( ReaderCommand.DCMD_SET_DOC_FONTS, enableInternalFonts ? 1 : 0);
            doEngineCommand( ReaderCommand.DCMD_REQUEST_RENDER, 1);
            mActivity.getDB().saveBookInfo(mBookInfo);
		}
	}
	
	public boolean isTextAutoformatEnabled() {
		if ( mOpened && mBookInfo!=null ) {
			boolean disableTextReflow = mBookInfo.getFileInfo().getFlag(FileInfo.DONT_REFLOW_TXT_FILES_FLAG);
			return !disableTextReflow;
		}
		return true;
	}
	
	public boolean isTextFormat() {
		if ( mOpened && mBookInfo!=null ) {
			DocumentFormat fmt = mBookInfo.getFileInfo().format;
			return fmt == DocumentFormat.TXT || fmt == DocumentFormat.HTML || fmt == DocumentFormat.PDB;
		}
		return false;
	}

	public boolean isFormatWithEmbeddedFonts() {
		if ( mOpened && mBookInfo!=null ) {
			DocumentFormat fmt = mBookInfo.getFileInfo().format;
			return fmt == DocumentFormat.EPUB;
		}
		return false;
	}

	public void toggleTextFormat() {
		if ( mOpened && mBookInfo!=null ) {
			log.d("toggleDocumentStyles()");
			if (!isTextFormat())
				return;
			boolean disableTextReflow = mBookInfo.getFileInfo().getFlag(FileInfo.DONT_REFLOW_TXT_FILES_FLAG);
			disableTextReflow = !disableTextReflow;
			mBookInfo.getFileInfo().setFlag(FileInfo.DONT_REFLOW_TXT_FILES_FLAG, disableTextReflow);
			mActivity.getDB().saveBookInfo(mBookInfo);
			reloadDocument();
		}
	}
	
	public boolean getDocumentStylesEnabled() {
		if ( mOpened && mBookInfo!=null ) {
			boolean flg = !mBookInfo.getFileInfo().getFlag(FileInfo.DONT_USE_DOCUMENT_STYLES_FLAG);
			return flg;
		}
		return true;
	}
	
	public boolean getDocumentFontsEnabled() {
		if ( mOpened && mBookInfo!=null ) {
			boolean flg = mBookInfo.getFileInfo().getFlag(FileInfo.USE_DOCUMENT_FONTS_FLAG);
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
		items.add("system.time=" + Utils.formatTime(mActivity, System.currentTimeMillis()));
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
				bm = doc.getCurrentPageBookmark();
				if ( bm!=null ) {
					PositionProperties prop = doc.getPositionProps(bm.getStartPos());
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
				if ( fi.language != null) {
					items.add("book.language=" + fi.language);
				}
				BookInfoDialog dlg = new BookInfoDialog(mActivity, items);
				dlg.show();
			}
		});
	}

	private int autoScrollSpeed = 1500; // chars / minute
	private int autoScrollNotificationId = 0;
	private AutoScrollAnimation currentAutoScrollAnimation = null;
	
	private boolean isAutoScrollActive() {
		return currentAutoScrollAnimation != null;
	}
	
	private void stopAutoScroll() {
		if (!isAutoScrollActive())
			return;
		log.d("stopAutoScroll()");
		//notifyAutoscroll("Autoscroll is stopped");
		currentAutoScrollAnimation.stop();
	}
	
	public static final int AUTOSCROLL_START_ANIMATION_PERCENT = 5; 
	
	private void startAutoScroll() {
		if (isAutoScrollActive())
			return;
		log.d("startAutoScroll()");
		currentAutoScrollAnimation = new AutoScrollAnimation(AUTOSCROLL_START_ANIMATION_PERCENT * 100);
		nextHiliteId++;
		hiliteRect = null;
	}
	
	private void toggleAutoScroll() {
		if (isAutoScrollActive())
			stopAutoScroll();
		else
			startAutoScroll();
	}
	
	private final static boolean AUTOSCROLL_SPEED_NOTIFICATION_ENABLED = false;
	private void notifyAutoscroll(final String msg) {
		if (DeviceInfo.EINK_SCREEN)
			return; // disable toast for eink
		if (AUTOSCROLL_SPEED_NOTIFICATION_ENABLED) {
			final int myId = ++autoScrollNotificationId;
			BackgroundThread.instance().postGUI(new Runnable() {
				@Override
				public void run() {
					if (myId == autoScrollNotificationId)
						mActivity.showToast(msg);
				}}, 1000);
		}
	}
	
	private void notifyAutoscrollSpeed() {
		final String msg = mActivity.getString(R.string.lbl_autoscroll_speed).replace("$1", String.valueOf(autoScrollSpeed));
		notifyAutoscroll(msg);
	}
	
	private void changeAutoScrollSpeed(int delta) {
		if (autoScrollSpeed<300)
			delta *= 10;
		else if (autoScrollSpeed<500)
			delta *= 20;
		else if (autoScrollSpeed<1000)
			delta *= 40;
		else if (autoScrollSpeed<2000)
			delta *= 80;
		else if (autoScrollSpeed<5000)
			delta *= 200;
		else
			delta *= 300;
		autoScrollSpeed += delta;
		if (autoScrollSpeed < 200)
			autoScrollSpeed = 200;
		if (autoScrollSpeed > 10000)
			autoScrollSpeed = 10000;
		setSetting(PROP_APP_VIEW_AUTOSCROLL_SPEED, String.valueOf(autoScrollSpeed), false, true, false);
		notifyAutoscrollSpeed();
	}
	
	class AutoScrollAnimation {

		boolean isScrollView;
		BitmapInfo image1;
		BitmapInfo image2;
		PositionProperties currPos;
		int progress;
		int pageCount;
		int charCount;
		int timerInterval;
		long pageTurnStart;
		int nextPos;
		
		Paint[] shadePaints;
		Paint[] hilitePaints;
		
		final int startAnimationProgress;
		
		public static final int MAX_PROGRESS = 10000;
		public final static int ANIMATION_INTERVAL_NORMAL = 30;
		public final static int ANIMATION_INTERVAL_EINK = 5000;
		
		public AutoScrollAnimation(final int startProgress) {
			progress = startProgress;
			startAnimationProgress = AUTOSCROLL_START_ANIMATION_PERCENT * 100;
			currentAutoScrollAnimation = this;

			final int numPaints = 32;
			shadePaints = new Paint[numPaints];
			hilitePaints = new Paint[numPaints];
			for ( int i=0; i<numPaints; i++ ) {
				shadePaints[i] = new Paint();
				hilitePaints[i] = new Paint();
				hilitePaints[i].setStyle(Paint.Style.FILL);
				shadePaints[i].setStyle(Paint.Style.FILL);
				if ( mActivity.isNightMode() ) {
					shadePaints[i].setColor(Color.argb((i+1)*128 / numPaints, 0, 0, 0));
					hilitePaints[i].setColor(Color.argb((i+1)*128 / numPaints, 128, 128, 128));
				} else {
					shadePaints[i].setColor(Color.argb((i+1)*128 / numPaints, 0, 0, 0));
					hilitePaints[i].setColor(Color.argb((i+1)*128 / numPaints, 255, 255, 255));
				}
			}
			
			BackgroundThread.instance().postBackground(new Runnable() {
				@Override
				public void run() {
					if (initPageTurn(startProgress)) {
						log.d("AutoScrollAnimation: starting autoscroll timer");
						timerInterval = DeviceInfo.EINK_SCREEN ? ANIMATION_INTERVAL_EINK : ANIMATION_INTERVAL_NORMAL;
						startTimer(timerInterval);
					} else {
						currentAutoScrollAnimation = null;
					}
				}
			});
		}
		
		private int calcProgressPercent() {
			long duration = Utils.timeInterval(pageTurnStart);
			long estimatedFullDuration = 60000 * charCount / autoScrollSpeed; 
			int percent = (int)(10000 * duration / estimatedFullDuration);
//			if (duration > estimatedFullDuration - timerInterval / 3)
//				percent = 10000;
			if (percent > 10000)
				percent = 10000;
			if (percent < 0)
				percent = 0;
			return percent;
		}
		
		private boolean onTimer() {
			int newProgress = calcProgressPercent();
			alog.v("onTimer(progress = " + newProgress + ")");
			mActivity.onUserActivity();
			progress = newProgress;
			if (progress == 0 || progress >= startAnimationProgress) {
				if (image1 != null && image2 != null) {
					if (image1.isReleased() || image2.isReleased()) {
						log.d("Images lost! Recreating images...");
						initPageTurn(progress);
					}
					draw();
				}
			}
			if (progress >= 10000) {
				if (!donePageTurn(true)) {
					stop();
					return false;
				}
				initPageTurn(0);
			}
			return true;
		}
		
		class AutoscrollTimerTask implements Runnable {
			final long interval;
			public AutoscrollTimerTask(long interval) {
				this.interval = interval;
				mActivity.onUserActivity();
				BackgroundThread.instance().postGUI(this, interval);
			}
			@Override
			public void run() {
				if (currentAutoScrollAnimation != AutoScrollAnimation.this) {
					log.v("timer is cancelled - GUI");
					return;
				}
				BackgroundThread.instance().postBackground(new Runnable() {
					@Override
					public void run() {
						if (currentAutoScrollAnimation != AutoScrollAnimation.this) {
							log.v("timer is cancelled - BackgroundThread");
							return;
						}
						if (onTimer())
							BackgroundThread.instance().postGUI(AutoscrollTimerTask.this, interval);
						else
							log.v("timer is cancelled - onTimer returned false");
					}
				});
			}
		}
		
		private void startTimer(final int interval) {
			new AutoscrollTimerTask(interval);
		}
		
		private boolean initPageTurn(int startProgress) {
			cancelGc();
			log.v("initPageTurn(startProgress = " + startProgress + ")");
			pageTurnStart = Utils.timeStamp();
			progress = startProgress;
			currPos = doc.getPositionProps(null);
			charCount = currPos.charCount;
			pageCount = currPos.pageMode;
			if (charCount < 150)
				charCount = 150;
			isScrollView = currPos.pageMode == 0;
			log.v("initPageTurn(charCount = " + charCount + ")");
			if (isScrollView) {
				image1 = preparePageImage(0);
				if (image1 == null) {
					log.v("ScrollViewAnimation -- not started: image is null");
					return false;
				}
				int pos0 = image1.position.y;
				int pos1 = pos0 + image1.position.pageHeight * 9/10;
				if (pos1 > image1.position.fullHeight - image1.position.pageHeight)
					pos1 = image1.position.fullHeight - image1.position.pageHeight;
				if (pos1 < 0)
					pos1 = 0;
				nextPos = pos1; 
				image2 = preparePageImage(pos1 - pos0);
				if (image2 == null) {
					log.v("ScrollViewAnimation -- not started: image is null");
					return false;
				}
			} else {
				int page1 = currPos.pageNumber;
				int page2 = currPos.pageNumber + 1;
				if ( page2<0 || page2>=currPos.pageCount) {
					currentAnimation = null;
					return false;
				}
				image1 = preparePageImage(0);
				image2 = preparePageImage(1);
				if ( page1==page2 ) {
					log.v("PageViewAnimation -- cannot start animation: not moved");
					return false;
				}
				if ( image1==null || image2==null ) {
					log.v("PageViewAnimation -- cannot start animation: page image is null");
					return false;
				}
				
			}
			long duration = android.os.SystemClock.uptimeMillis() - pageTurnStart;
			log.v("AutoScrollAnimation -- page turn initialized in " + duration + " millis");
			currentAutoScrollAnimation = this;
			draw();
			return true;
		}
		
		
		private boolean donePageTurn(boolean turnPage) {
			log.v("donePageTurn()");
			if (turnPage) {
				if (isScrollView)
					doc.doCommand(ReaderCommand.DCMD_GO_POS.nativeId, nextPos);
				else
					doc.doCommand(ReaderCommand.DCMD_PAGEDOWN.nativeId, 1);
			}
			progress = 0;
			//draw();
			return currPos.canMoveToNextPage();
		}

		public void draw()
		{
			draw(true);
		}

		public void draw(boolean isPartially)
		{
			drawCallback( new DrawCanvasCallback() {
				@Override
				public void drawTo(Canvas c) {
				//	long startTs = android.os.SystemClock.uptimeMillis();
					draw(c);
				}
			}, null, isPartially);
		}
		
		public void stop() {
			currentAutoScrollAnimation = null;
			BackgroundThread.instance().executeBackground(new Runnable() {
				@Override
				public void run() {
					donePageTurn(wantPageTurn());
					//redraw();
					drawPage(null, false);
					scheduleSaveCurrentPositionBookmark(DEF_SAVE_POSITION_INTERVAL);
				}
			});
			scheduleGc();
		}
		
	    private boolean wantPageTurn() {
	    	return (progress > (startAnimationProgress + MAX_PROGRESS) / 2);
	    }
		
		private void drawGradient( Canvas canvas, Rect rc, Paint[] paints, int startIndex, int endIndex ) {
			//log.v("drawShadow");
			int n = (startIndex<endIndex) ? endIndex-startIndex+1 : startIndex-endIndex + 1;
			int dir = (startIndex<endIndex) ? 1 : -1;
			int dx = rc.bottom - rc.top;
			Rect rect = new Rect(rc);
			for ( int i=0; i<n; i++ ) {
				int index = startIndex + i*dir;
				int x1 = rc.top + dx*i/n;
				int x2 = rc.top + dx*(i+1)/n;
				if (x1 < 0)
					x1 = 0;
				if (x2 > canvas.getHeight())
					x2 = canvas.getHeight();
				rect.top = x1;
				rect.bottom = x2;
				if ( x2>x1 ) {
					//log.v("drawShadow : " + x1 + ", " + x2 + ", " + index);
					canvas.drawRect(rect, paints[index]);
				}
			}
		}
		
		private void drawShadow( Canvas canvas, Rect rc ) {
			drawGradient(canvas, rc, shadePaints, shadePaints.length * 3 / 4, 0);
		}
		
	    void drawPageProgress(Canvas canvas, int scrollPercent, Rect dst, Rect src) {
	    	int shadowHeight = 32;
			int h = dst.height();
			int div = (h + shadowHeight) * scrollPercent / 10000 - shadowHeight;
			//log.v("drawPageProgress() div = " + div + ", percent = " + scrollPercent);
			int d = div >= 0 ? div : 0; 
			if (d > 0) {
	    		Rect src1 = new Rect(src.left, src.top, src.right, src.top + d);
	    		Rect dst1 = new Rect(dst.left, dst.top, dst.right, dst.top + d);
	    		drawDimmedBitmap(canvas, image2.bitmap, src1, dst1);
			}
			if (d < h) {
	    		Rect src2 = new Rect(src.left, src.top + d, src.right, src.bottom);
	    		Rect dst2 = new Rect(dst.left, dst.top + d, dst.right, dst.bottom);
	    		drawDimmedBitmap(canvas, image1.bitmap, src2, dst2);
			}
    		if (scrollPercent > 0 && scrollPercent < 10000) {
				Rect shadowRect = new Rect(src.left, src.top + div, src.right, src.top + div + shadowHeight);
				drawShadow(canvas, shadowRect);
    		}
	    }
	    
		public void draw(Canvas canvas) {
			if (currentAutoScrollAnimation != this)
				return;
			alog.v("AutoScrollAnimation.draw(" + progress + ")");
			if (progress!=0 && progress<startAnimationProgress)
				return; // don't draw page w/o started animation
			int scrollPercent = 10000 * (progress - startAnimationProgress) / (MAX_PROGRESS - startAnimationProgress);
			if (scrollPercent < 0)
				scrollPercent = 0;
			int w = image1.bitmap.getWidth(); 
			int h = image1.bitmap.getHeight();
			if (isScrollView) {
				// scroll
				drawPageProgress(canvas, scrollPercent, new Rect(0, 0, w, h), new Rect(0, 0, w, h));
			} else {
				if (image1.isReleased() || image2.isReleased())
					return;
				if (pageCount==2) {
					if (scrollPercent<5000) {
						// < 50%
						scrollPercent = scrollPercent * 2; 
						drawPageProgress(canvas, scrollPercent, new Rect(0, 0, w/2, h), new Rect(0, 0, w/2, h));
						drawPageProgress(canvas, 0, new Rect(w/2, 0, w, h), new Rect(w/2, 0, w, h));
					} else {
						// >=50%
						scrollPercent = (scrollPercent - 5000) * 2; 
						drawPageProgress(canvas, 10000, new Rect(0, 0, w/2, h), new Rect(0, 0, w/2, h));
						drawPageProgress(canvas, scrollPercent, new Rect(w/2, 0, w, h), new Rect(w/2, 0, w, h));
					}
				} else {
					drawPageProgress(canvas, scrollPercent, new Rect(0, 0, w, h), new Rect(0, 0, w, h));
				}
			}
			
		}
		
	}

	public void onCommand( final ReaderCommand cmd, final int param )
	{
		onCommand( cmd, param, null );
	}
	
	private void navigateByHistory(final ReaderCommand cmd) {
		BackgroundThread.instance().postBackground(new Runnable() {
			@Override
			public void run() {
				final boolean res = doc.doCommand(cmd.nativeId, 0);
				BackgroundThread.instance().postGUI(new Runnable() {
					@Override
					public void run() {
						if (res) {
							// successful
							drawPage();
						} else {
							// cannot navigate - no data on stack
							if (cmd == ReaderCommand.DCMD_LINK_BACK) {
								// TODO: exit from activity in some cases?
								if (mActivity.isPreviousFrameHome())
									mActivity.showRootWindow();
								else
									mActivity.showBrowser(!mActivity.isBrowserCreated() ? getOpenedFileInfo() : null);
							}
						}
					}
				});
			}
		});
	}
	
	public void onCommand( final ReaderCommand cmd, final int param, final Runnable onFinishHandler )
	{
		BackgroundThread.ensureGUI();
		log.i("On command " + cmd + (param!=0?" ("+param+")":" "));
		switch ( cmd ) {
		case DCMD_FILE_BROWSER_ROOT:
			mActivity.showRootWindow();
			break;
		case DCMD_ABOUT:
			mActivity.showAboutDialog();
			break;
		case DCMD_SWITCH_PROFILE:
			showSwitchProfileDialog();
			break;
		case DCMD_TOGGLE_AUTOSCROLL:
			toggleAutoScroll();
			break;
		case DCMD_AUTOSCROLL_SPEED_INCREASE:
			changeAutoScrollSpeed(1);
			break;
		case DCMD_AUTOSCROLL_SPEED_DECREASE:
			changeAutoScrollSpeed(-1);
			break;
		case DCMD_SHOW_DICTIONARY:
			mActivity.showDictionary();
			break;
		case DCMD_OPEN_PREVIOUS_BOOK:
			loadPreviousDocument(new Runnable() {
				@Override
				public void run() {
					// do nothing
				}
			});
			break;
		case DCMD_BOOK_INFO:
			showBookInfo();
			break;
		case DCMD_USER_MANUAL:
			showManual();
			break;
		case DCMD_TTS_PLAY:
			{
				log.i("DCMD_TTS_PLAY: initializing TTS");
				if ( !mActivity.initTTS(new TTS.OnTTSCreatedListener() {
					@Override
					public void onCreated(TTS tts) {
						log.i("TTS created: opening TTS toolbar");
						ttsToolbar = TTSToolbarDlg.showDialog(mActivity, ReaderView.this, tts);
						ttsToolbar.setOnCloseListener(new Runnable() {
							@Override
							public void run() {
								ttsToolbar = null;
							}
						});
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
		case DCMD_TOGGLE_TITLEBAR:
			toggleTitlebar();
			break;
		case DCMD_SHOW_POSITION_INFO_POPUP:
			showReadingPositionPopup();
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
			navigateByHistory(cmd);
            break;
		case DCMD_ZOOM_OUT:
            doEngineCommand( ReaderCommand.DCMD_ZOOM_OUT, param);
            syncViewSettings(getSettings(), true, true);
            break;
		case DCMD_ZOOM_IN:
            doEngineCommand( ReaderCommand.DCMD_ZOOM_IN, param);
            syncViewSettings(getSettings(), true, true);
            break;
		case DCMD_FONT_NEXT:
			switchFontFace(1);
            break;
		case DCMD_FONT_PREVIOUS:
			switchFontFace(-1);
            break;
		case DCMD_MOVE_BY_CHAPTER:
			doEngineCommand(cmd, param, onFinishHandler);
            drawPage();
			break;
		case DCMD_PAGEDOWN:
			if ( param==1 && !DeviceInfo.EINK_SCREEN)
				animatePageFlip(1, onFinishHandler);
			else
				doEngineCommand(cmd, param, onFinishHandler);
			break;
		case DCMD_PAGEUP:
			if ( param==1 && !DeviceInfo.EINK_SCREEN)
				animatePageFlip(-1, onFinishHandler);
			else
				doEngineCommand(cmd, param, onFinishHandler);
			break;
		case DCMD_BEGIN:
		case DCMD_END:
			doEngineCommand(cmd, param);
			break;
		case DCMD_RECENT_BOOKS_LIST:
			mActivity.showRecentBooks();
			break;
		case DCMD_SEARCH:
			showSearchDialog(null);
			break;
		case DCMD_EXIT:
			mActivity.finish();
			break;
		case DCMD_BOOKMARKS:
			mActivity.showBookmarksDialog();
			break;
		case DCMD_GO_PERCENT_DIALOG:
			showGoToPercentDialog();
			break;
		case DCMD_GO_PAGE_DIALOG:
			showGoToPageDialog();
			break;
		case DCMD_TOC_DIALOG:
			showTOC();
			break;
		case DCMD_FILE_BROWSER:
			mActivity.showBrowser(!mActivity.isBrowserCreated() ? getOpenedFileInfo() : null);
			break;
		case DCMD_CURRENT_BOOK_DIRECTORY:
			mActivity.showBrowser(getOpenedFileInfo());
			break;
		case DCMD_OPTIONS_DIALOG:
			mActivity.showOptionsDialog(OptionsDialog.Mode.READER);
			break;
		case DCMD_READER_MENU:
			mActivity.showReaderMenu();
			break;
		case DCMD_TOGGLE_DAY_NIGHT_MODE:
			toggleDayNightMode();
			break;
		}
	}
	boolean firstShowBrowserCall = true;
	
	
	private TTSToolbarDlg ttsToolbar;
	public void stopTTS() {
		if (ttsToolbar != null)
			ttsToolbar.pause();
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
			boolean isMoveCommand;
			public void work() {
				BackgroundThread.ensureBackground();
				res = doc.doCommand(cmd.nativeId, param);
				switch (cmd) {
					case DCMD_BEGIN:
					case DCMD_LINEUP:
					case DCMD_PAGEUP:
					case DCMD_PAGEDOWN:
					case DCMD_LINEDOWN:
					case DCMD_LINK_FORWARD:
					case DCMD_LINK_BACK:
					case DCMD_LINK_NEXT:
					case DCMD_LINK_PREV:
					case DCMD_LINK_GO:
					case DCMD_END:
					case DCMD_GO_POS:
					case DCMD_GO_PAGE:
					case DCMD_MOVE_BY_CHAPTER:
					case DCMD_GO_SCROLL_POS:
					case DCMD_LINK_FIRST:
					case DCMD_SCROLL_BY:
			    		isMoveCommand = true;
						break;
				}
				if (isMoveCommand)
					updateCurrentPositionStatus();
			}
			public void done() {
				if (res) {
					invalidImages = true;
					drawPage( doneHandler, false );
				}
				if (isMoveCommand)
			    	scheduleSaveCurrentPositionBookmark(DEF_SAVE_POSITION_INTERVAL);
			}
		});
	}
	
	// update book and position info in status bar
	private void updateCurrentPositionStatus() {
		if (mBookInfo == null)
			return;
		// in background thread
		final FileInfo fileInfo = mBookInfo.getFileInfo();
		if (fileInfo == null)
			return;
		final Bookmark bmk = doc != null ? doc.getCurrentPageBookmark() : null;
		final PositionProperties props = bmk != null ? doc.getPositionProps(bmk.getStartPos()) : null;
		if (props != null) BackgroundThread.instance().postGUI(new Runnable() {
			@Override
			public void run() {
				mActivity.updateCurrentPositionStatus(fileInfo, bmk, props);
			}
		});
	}
	
	public void doCommandFromBackgroundThread( final ReaderCommand cmd, final int param )
	{
		log.d("doCommandFromBackgroundThread("+cmd + ", " + param +")");
		BackgroundThread.ensureBackground();
		boolean res = doc.doCommand(cmd.nativeId, param);
		if ( res ) {
			BackgroundThread.instance().executeGUI(new Runnable() {
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
		doc.updateBookInfo(mBookInfo);
		updateCurrentPositionStatus();
		// check whether current book properties updated on another devices
		// TODO: fix and reenable
		//syncUpdater.syncExternalChanges(mBookInfo);
	}
	
	private class SyncExternalChangesUpdater {
		
		private long lastUpdateId = 0;
		private void scheduleExternalChangesUpdate(long interval) {
			final long mySyncId = ++lastUpdateId;
			BackgroundThread.instance().postGUI(new Runnable() {
				@Override
				public void run() {
					if (mySyncId == lastUpdateId)
						processExternalChanges();
				}}, 60000);
		}

		private long lastSyncId = 0;
		private void scheduleExternalChangesSync(long interval) {
			final long mySyncId = ++lastSyncId;
			BackgroundThread.instance().postGUI(new Runnable() {
				@Override
				public void run() {
					if (mySyncId == lastSyncId)
						syncExternalChanges(mBookInfo);
				}}, interval);
		}
	
		private void syncExternalChanges(BookInfo book) {
			if (book == null)
				return;
			String fn = book.getFileInfo().getPathName();
			if (fn == null)
				return;
			int LIMIT = 5000;
			List<ChangeInfo> changes = getSyncService().checkChangesSync(LIMIT);
			if (changes == null)
				return;
			ArrayList<ChangeInfo> thisBookChanges = new ArrayList<ChangeInfo>();
			ArrayList<ChangeInfo> anotherBookChanges = new ArrayList<ChangeInfo>();
			for (ChangeInfo ci : changes) {
				if (fn.equalsIgnoreCase(ci.getFileName()))
					thisBookChanges.add(ci);
				else
					anotherBookChanges.add(ci);
			}
			// update current book's parameters
			for (ChangeInfo ci : thisBookChanges) {
				processCurrentBookChange(book, ci);
			}
			if (anotherBookChanges.size() > 0)
				addExternalChangesToProcess(anotherBookChanges);
			scheduleExternalChangesUpdate(120000);
			if (changes.size() == LIMIT)
				scheduleExternalChangesSync(120000);
		}
	
		private void processCurrentBookChange(BookInfo book, ChangeInfo ci) {
			if (ci.isDeleted() && ci.getBookmark() == null)
				return; // ignore REMOVE BOOK events
			if (ci.isDeleted()) {
				book.removeBookmark(ci.getBookmark());
			} else {
				book.syncBookmark(ci.getBookmark());
			}
		}
		
		private void processExternalBookChange(final ChangeInfo ci) {
			FileInfo file = new FileInfo(ci.getFileName());
			if (!file.fileExists()) {
				log.w("Ignoring sync change for not existing file " + ci.getFileName());
				return;
			}
			if (ci.isDeleted() && ci.getBookmark() == null) {
				Services.getHistory().removeBookInfo(mActivity.getDB(), file, true, true);
				return; // ignore REMOVE BOOK events
			}
			Services.getHistory().getOrCreateBookInfo(mActivity.getDB(), file, new History.BookInfoLoadedCallack() {
				@Override
				public void onBookInfoLoaded(BookInfo bookInfo) {
					// process
					if (ci.isDeleted()) {
						bookInfo.removeBookmark(ci.getBookmark());
					} else {
						bookInfo.syncBookmark(ci.getBookmark());
					}
				}
			});
		}
		
		private void addExternalChangesToProcess(ArrayList<ChangeInfo> changes) {
			synchronized(externalChangesToProcess) {
				externalChangesToProcess.addAll(changes);
			}
		}
	
		private void processExternalChanges() {
			if (externalChangesToProcess.size() == 0)
				return;
			final ArrayList<ChangeInfo> changes = new ArrayList<ChangeInfo>();
			synchronized(externalChangesToProcess) {
				int count = 100;
				if (count > externalChangesToProcess.size())
					count = externalChangesToProcess.size();
				changes.addAll(externalChangesToProcess.subList(0, count));
				for (int i = count - 1; i >= 0; i--)
					externalChangesToProcess.remove(i);
			}
			BackgroundThread.instance().postGUI(new Runnable() {
				@Override
				public void run() {
					String currentBook = mBookInfo != null ? mBookInfo.getFileInfo().getPathName() : "";
					boolean currentBookChanged = false;
					boolean lastPosChanged = false;
					for (ChangeInfo ci : changes) {
						if (currentBook.equals(ci.getFileName())) {
							processCurrentBookChange(mBookInfo, ci);
							currentBookChanged = true;
						} else {
							processExternalBookChange(ci);
						}
						if (ci.getBookmark().getType() == Bookmark.TYPE_LAST_POSITION)
							lastPosChanged = true;
					}
					if (currentBookChanged)
						notifyCurrentBookBookmarksChanged();
					if (lastPosChanged)
						notifyRecentBooksDirectoryChanged();
				}
			});
		}
		
		private void notifyCurrentBookBookmarksChanged() {
			// TODO: update UI?
		}
	
		private void notifyRecentBooksDirectoryChanged() {
			// TODO: update UI?
		}
	
		private final ArrayList<ChangeInfo> externalChangesToProcess = new ArrayList<ChangeInfo>();
	}
	private final SyncExternalChangesUpdater syncUpdater = new SyncExternalChangesUpdater();

	public void syncExternalChanges() {
		syncUpdater.scheduleExternalChangesSync(10000);
	}
	
	private void applySettings(Properties props)
	{
		props = new Properties(props); // make a copy
		props.remove(PROP_TXT_OPTION_PREFORMATTED);
		props.remove(PROP_EMBEDDED_STYLES);
		props.remove(PROP_EMBEDDED_FONTS);
		BackgroundThread.ensureBackground();
		log.v("applySettings()");
		boolean isFullScreen = props.getBool(PROP_APP_FULLSCREEN, false );
		props.setBool(PROP_SHOW_BATTERY, isFullScreen); 
		props.setBool(PROP_SHOW_TIME, isFullScreen);
		String backgroundImageId = props.getProperty(PROP_PAGE_BACKGROUND_IMAGE);
		int backgroundColor = props.getColor(PROP_BACKGROUND_COLOR, 0xFFFFFF);
		setBackgroundTexture(backgroundImageId, backgroundColor);
		props.setInt(PROP_STATUS_LINE, props.getInt(PROP_STATUS_LOCATION, VIEWER_STATUS_TOP) == VIEWER_STATUS_PAGE ? 0 : 1);		

		int updMode      = props.getInt(PROP_APP_SCREEN_UPDATE_MODE, 0);
		int updInterval  = props.getInt(PROP_APP_SCREEN_UPDATE_INTERVAL, 10);
		mActivity.setScreenUpdateMode(updMode, surface);
		mActivity.setScreenUpdateInterval(updInterval, surface);		
		
		doc.applySettings(props);
        //syncViewSettings(props, save, saveDelayed);
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
		mActivity.setSettings(settings, 0, false);
	}
	
	/**
	 * Read JNI view settings, update and save if changed 
	 */
	private void syncViewSettings( final Properties currSettings, final boolean save, final boolean saveDelayed )
	{
		post( new Task() {
			Properties props;
			public void work() {
				BackgroundThread.ensureBackground();
				java.util.Properties internalProps = doc.getSettings(); 
				props = new Properties(internalProps);
			}
			public void done() {
				Properties changedSettings = props.diff(currSettings);
		        for ( Map.Entry<Object, Object> entry : changedSettings.entrySet() ) {
	        		currSettings.setProperty((String)entry.getKey(), (String)entry.getValue());
		        }
	        	mSettings = currSettings;
	        	if ( save ) {
        			mActivity.setSettings(mSettings, saveDelayed ? 5000 : 0, false);
	        	} else {
        			mActivity.setSettings(mSettings, -1, false);
	        	}
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

	private String getManualFileName() {
		Scanner s = Services.getScanner();
		if (s != null) {
			FileInfo fi = s.getDownloadDirectory();
			if (fi != null) {
				File bookDir = new File(fi.getPathName());
				return HelpFileGenerator.getHelpFileName(bookDir, mActivity.getCurrentLanguage()).getAbsolutePath();
			}
		}
		log.e("cannot get manual file name!");
		return "/sdcard/books/manual_ru.fb2";
	}
	
	private File generateManual() {
		HelpFileGenerator generator = new HelpFileGenerator(mActivity, mEngine, getSettings(), mActivity.getCurrentLanguage());
		FileInfo downloadDir = Services.getScanner().getDownloadDirectory();
		File bookDir;
		if (downloadDir != null)
			bookDir = new File(Services.getScanner().getDownloadDirectory().getPathName());
		else {
			log.e("cannot download directory file name!");
			bookDir = new File("/tmp/");
		}
		int settingsHash = generator.getSettingsHash();
		String helpFileContentId = mActivity.getCurrentLanguage() + settingsHash + "v" + mActivity.getVersion();
		String lastHelpFileContentId = mActivity.getLastGeneratedHelpFileSignature();
		File manual = generator.getHelpFileName(bookDir); 
		if (!manual.exists() || lastHelpFileContentId == null || !lastHelpFileContentId.equals(helpFileContentId)) {
			log.d("Generating help file " + manual.getAbsolutePath());
			mActivity.setLastGeneratedHelpFileSignature(helpFileContentId);
			manual = generator.generateHelpFile(bookDir);
		}
		return manual;
	}
	
	/**
	 * Generate help file (if necessary) and show it.
	 * @return true if opened successfully
	 */
	public boolean showManual() {
		return loadDocument(getManualFileName(), new Runnable() {
			@Override
			public void run() {
				mActivity.showToast("Error while opening manual");
			}
		});
	}
	
	private boolean hiliteTapZoneOnTap = false;
	private boolean enableVolumeKeys = true; 
	static private final int DEF_PAGE_FLIP_MS = 300; 
	public void applyAppSetting( String key, String value )
	{
		boolean flg = "1".equals(value);
        if ( key.equals(PROP_APP_TAP_ZONE_HILIGHT) ) {
        	hiliteTapZoneOnTap = flg;
        } else if ( key.equals(PROP_APP_DOUBLE_TAP_SELECTION) ) {
        	doubleTapSelectionEnabled = flg;
        } else if ( key.equals(PROP_APP_GESTURE_PAGE_FLIPPING) ) {
        	gesturePageFlippingEnabled = flg;
        } else if ( key.equals(PROP_APP_SECONDARY_TAP_ACTION_TYPE) ) {
        	secondaryTapActionType = flg ? TAP_ACTION_TYPE_DOUBLE : TAP_ACTION_TYPE_LONGPRESS;
        } else if ( key.equals(PROP_APP_FLICK_BACKLIGHT_CONTROL) ) {
        	isBacklightControlFlick = "1".equals(value) ? 1 : ("2".equals(value) ? 2 : 0);
        } else if (PROP_APP_HIGHLIGHT_BOOKMARKS.equals(key)) {
        	flgHighlightBookmarks = !"0".equals(value);
        	clearSelection();
        } else if (PROP_APP_VIEW_AUTOSCROLL_SPEED.equals(key)) {
        	int n = 1500;
        	try {
        		n = Integer.parseInt(value);
        	} catch (NumberFormatException e) {
        		// ignore
        	}
        	if (n < 200)
        		n = 200;
        	if (n > 10000)
        		n = 10000;
        	autoScrollSpeed = n;
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
        } else if ( PROP_APP_SELECTION_ACTION.equals(key) ) {
        	try {
        		int n = Integer.valueOf(value);
        		mSelectionAction = n;
        	} catch ( Exception e ) {
        		// ignore
        	}
        } else if ( PROP_APP_MULTI_SELECTION_ACTION.equals(key) ) {
        	try {
        		int n = Integer.valueOf(value);
        		mMultiSelectionAction = n;
        	} catch ( Exception e ) {
        		// ignore
        	}
        } else {
        	//mActivity.applyAppSetting(key, value);
        }
        //
	}
	
	public void setAppSettings(Properties newSettings, Properties oldSettings)
	{
		log.v("setAppSettings()"); //|| keyCode == KeyEvent.KEYCODE_DPAD_LEFT 
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
    		} else if ( PROP_APP_SCREEN_ORIENTATION.equals(key) 
    				|| PROP_PAGE_ANIMATION.equals(key)
    				|| PROP_CONTROLS_ENABLE_VOLUME_KEYS.equals(key) 
    				|| PROP_APP_SHOW_COVERPAGES.equals(key) 
    				|| PROP_APP_COVERPAGE_SIZE.equals(key) 
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
    				|| PROP_APP_GESTURE_PAGE_FLIPPING.equals(key)
    				|| PROP_APP_HIGHLIGHT_BOOKMARKS.equals(key)
    				|| PROP_HIGHLIGHT_SELECTION_COLOR.equals(key)
    				|| PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT.equals(key)
    				|| PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION.equals(key)
    				// TODO: redesign all this mess!
    				) {
    			newSettings.setProperty(key, value);
    		} else if ( PROP_HYPHENATION_DICT.equals(key) ) {
    			Engine.HyphDict dict = HyphDict.byCode(value);
    			if ( mEngine.setHyphenationDictionary(dict) ) {
    				if ( isBookLoaded() ) {
    					String language = getBookInfo().getFileInfo().getLanguage();
    					mEngine.setHyphenationLanguage(language);
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
	public void updateSettings(Properties newSettings)
	{
		log.v("updateSettings() " + newSettings.toString());
		log.v("oldNightMode=" + mSettings.getProperty(PROP_NIGHT_MODE) + " newNightMode=" + newSettings.getProperty(PROP_NIGHT_MODE));
		BackgroundThread.ensureGUI();
		final Properties currSettings = new Properties(mSettings);
		setAppSettings( newSettings, currSettings );
		Properties changedSettings = newSettings.diff(currSettings);
		currSettings.setAll(changedSettings);
		mSettings = currSettings;
    	BackgroundThread.instance().postBackground(new Runnable() {
    		public void run() {
    			applySettings(currSettings);
    		}
    	});
	}

	private void setBackgroundTexture(String textureId, int color) {
		BackgroundTextureInfo[] textures = mEngine.getAvailableTextures();
		for ( BackgroundTextureInfo item : textures ) {
			if ( item.id.equals(textureId) ) {
				setBackgroundTexture(item, color);
				return;
			}
		}
		setBackgroundTexture(Engine.NO_TEXTURE, color);
	}

	private void setBackgroundTexture(BackgroundTextureInfo texture, int color) {
		log.v("setBackgroundTexture(" + texture + ", " + color + ")");
		if (!currentBackgroundTexture.equals(texture) || currentBackgroundColor != color) {
			log.d("setBackgroundTexture( " + texture + " )");
			currentBackgroundColor = color;
			currentBackgroundTexture = texture;
			byte[] data = mEngine.getImageData(currentBackgroundTexture);
			doc.setPageBackgroundTexture(data, texture.tiled ? 1 : 0);
			currentBackgroundTextureTiled = texture.tiled;
			if (data != null && data.length > 0) {
				if (currentBackgroundTextureBitmap != null)
					currentBackgroundTextureBitmap.recycle();
				try {
					currentBackgroundTextureBitmap = android.graphics.BitmapFactory.decodeByteArray(data, 0, data.length);
				} catch (Exception e) {
					log.e("Exception while decoding image data", e);
					currentBackgroundTextureBitmap = null;
				}
			} else {
				currentBackgroundTextureBitmap = null;
			}
		}
	}
	
	BackgroundTextureInfo currentBackgroundTexture = Engine.NO_TEXTURE;
	Bitmap currentBackgroundTextureBitmap = null;
	boolean currentBackgroundTextureTiled = false;
	int currentBackgroundColor = 0;
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
			doc.setPageBackgroundTexture(data, currentBackgroundTexture.tiled?1:0);
			
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
	        	doc.setStylesheet(css);
   			applySettings(props);
   			mInitialized = true;
   	        log.i("CreateViewTask - finished");
		}
		public void done() {
			log.d("InitializationFinishedEvent");
			//BackgroundThread.ensureGUI();
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

	public boolean reloadDocument() {
		if (this.mBookInfo!=null && this.mBookInfo.getFileInfo() != null) {
			save(); // save current position
			post(new LoadDocumentTask(this.mBookInfo, null));
			return true;
		}
		return false;
	}
	
	public boolean loadDocument( final FileInfo fileInfo, final Runnable errorHandler )
	{
		log.v("loadDocument(" + fileInfo.getPathName() + ")");
		if ( this.mBookInfo!=null && this.mBookInfo.getFileInfo().pathname.equals(fileInfo.pathname) && mOpened ) {
			log.d("trying to load already opened document");
			mActivity.showReader();
			drawPage();
			return false;
		}
		Services.getHistory().getOrCreateBookInfo(mActivity.getDB(), fileInfo, new History.BookInfoLoadedCallack() {
			@Override
			public void onBookInfoLoaded(final BookInfo bookInfo) {
				log.v("posting LoadDocument task to background thread");
				BackgroundThread.instance().postBackground(new Runnable() {
					@Override
					public void run() {
						log.v("posting LoadDocument task to GUI thread");
						BackgroundThread.instance().postGUI(new Runnable() {
							@Override
							public void run() {
								log.v("synced posting LoadDocument task to GUI thread");
								post(new LoadDocumentTask(bookInfo, errorHandler));
							}
						});
					}
				});
			}
		});
		return true;
	}

	/**
	 * When current book is opened, switch to previous book.
	 * @param errorHandler
	 * @return
	 */
	public boolean loadPreviousDocument( final Runnable errorHandler )
	{
		BackgroundThread.ensureGUI();
		BookInfo bi = Services.getHistory().getPreviousBook();
		if (bi!=null && bi.getFileInfo()!=null) {
			save();
			log.i("loadPreviousDocument() is called, prevBookName = " + bi.getFileInfo().getPathName());
			return loadDocument( bi.getFileInfo().getPathName(), errorHandler );
		}
		errorHandler.run();
		return false;
	}
	
	public boolean loadDocument( String fileName, final Runnable errorHandler )
	{
		BackgroundThread.ensureGUI();
		save();
		log.i("loadDocument(" + fileName + ")");
		if (fileName == null) {
			log.v("loadDocument() : no filename specified");
			if (errorHandler != null)
				errorHandler.run();
			return false;
		}
		if ("@manual".equals(fileName)) {
			fileName = getManualFileName();
			log.i("Manual document: " + fileName);
		}
		String normalized = mEngine.getPathCorrector().normalize(fileName);
		if (normalized == null) {
			log.e("Trying to load book from non-standard path " + fileName);
			mActivity.showToast("Trying to load book from non-standard path " + fileName);
			hideProgress();
			if (errorHandler != null)
				errorHandler.run();
			return false;
		} else if (!normalized.equals(fileName)) {
			log.w("Filename normalized to " + normalized);
			fileName = normalized;
		}
		if (fileName.equals(getManualFileName())) {
			// ensure manual file is up to date
			if (generateManual() == null) {
				log.v("loadDocument() : no filename specified");
				if (errorHandler != null)
					errorHandler.run();
				return false;
			}
		}
		BookInfo book = Services.getHistory().getBookInfo(fileName);
		if ( book!=null )
			log.v("loadDocument() : found book in history : " + book);
		FileInfo fi = null;
		if ( book==null ) {
			log.v("loadDocument() : book not found in history, looking for location directory");
			FileInfo dir = Services.getScanner().findParent(new FileInfo(fileName), Services.getScanner().getRoot());
			if ( dir!=null ) {
				log.v("loadDocument() : document location found : " + dir);
				fi = dir.findItemByPathName(fileName);
				log.v("loadDocument() : item inside location : " + fi);
			}
			if ( fi==null ) {
				log.v("loadDocument() : no file item " + fileName + " found inside " + dir);
				if (errorHandler != null)
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
		return loadDocument(fi, errorHandler);
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
			if (!DeviceInfo.EINK_SCREEN && !isAutoScrollActive()) {
				drawPage();
			}
		}
	}
	
	public int getBatteryState() {
		return mBatteryState;
	}
	
	private static final VMRuntimeHack runtime = new VMRuntimeHack();
	
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
				runtime.trackAlloc(bmp.getWidth() * bmp.getHeight() * 2);
				//log.d("Recycling free bitmap "+bmp.getWidth()+"x"+bmp.getHeight());
				//bmp.recycle(); //20110109 
			}
			Bitmap bmp = Bitmap.createBitmap(dx, dy, DeviceInfo.BUFFER_COLOR_FORMAT);
			runtime.trackFree(dx*dy*2);
			//bmp.setDensity(0);
			usedList.add(bmp);
			//log.d("Created new bitmap "+dx+"x"+dy+". New bitmap list size = " + usedList.size());
			return bmp;
		}
		public synchronized void compact() {
			while ( freeList.size()>0 ) {
				//freeList.get(0).recycle();//20110109
				Bitmap bmp = freeList.remove(0);
				runtime.trackAlloc(bmp.getWidth() * bmp.getHeight() * 2);
			}
		}
		public synchronized void release( Bitmap bmp ) {
			for ( int i=0; i<usedList.size(); i++ ) {
				if ( usedList.get(i)==bmp ) {
					freeList.add(bmp);
					usedList.remove(i);
					while ( freeList.size()>MAX_FREE_LIST_SIZE ) {
						//freeList.get(0).recycle(); //20110109
						Bitmap b = freeList.remove(0);
						runtime.trackAlloc(b.getWidth() * b.getHeight() * 2);
						//b.recycle();
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
		ImageInfo imageInfo;
		void recycle()
		{
			factory.release(bitmap);
			bitmap = null;
			position = null;
			imageInfo = null;
		}
		boolean isReleased() {
			return bitmap == null;
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
		//if (offset == 0) {
//			// DEBUG stack trace
//			try {
//				if (currentAutoScrollAnimation!=null)
//					log.v("preparePageImage from autoscroll");
//				throw new Exception("stack trace");
//			} catch (Exception e) {
//				Log.d("cr3", "stack trace", e);
//			}
		//}
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
			if (requestedWidth > 0 && requestedHeight > 0) {
				internalDX = requestedWidth;
				internalDY = requestedHeight;
				doc.resize(internalDX, internalDY);
			} else {
				internalDX = surface.getWidth();
				internalDY = surface.getHeight();
				doc.resize(internalDX, internalDY);
			}
//			internalDX=200;
//			internalDY=300;
//			doc.resize(internalDX, internalDY);
//			BackgroundThread.instance().postGUI(new Runnable() {
//				@Override
//				public void run() {
//					log.d("invalidating view due to resize");
//					//ReaderView.this.invalidate();
//					drawPage(null, false);
//					//redraw();
//				}
//			});
		}
		
		if (currentImageViewer != null)
			return currentImageViewer.prepareImage();

		PositionProperties currpos = doc.getPositionProps(null);
		
		boolean isPageView = currpos.pageMode!=0;
		
		BitmapInfo currposBitmap = null;
		if ( mCurrentPageInfo!=null && mCurrentPageInfo.position.equals(currpos) && mCurrentPageInfo.imageInfo == null)
			currposBitmap = mCurrentPageInfo;
		else if ( mNextPageInfo!=null && mNextPageInfo.position.equals(currpos) && mNextPageInfo.imageInfo == null )
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
			bi.bitmap = factory.get(internalDX > 0 ? internalDX : requestedWidth, 
					internalDY > 0 ? internalDY : requestedHeight);
			doc.setBatteryState(mBatteryState);
			doc.getPageImage(bi.bitmap);
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
			if ( doc.doCommand(cmd1, offset) ) {
				// can move to next page
				PositionProperties nextpos = doc.getPositionProps(null);
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
					doc.setBatteryState(mBatteryState);
					doc.getPageImage(bi.bitmap);
			        mNextPageInfo = bi;
			        nextposBitmap = bi;
			        //log.v("Prepared new current page image " + mNextPageInfo);
				}
				// return back to previous page
				doc.doCommand(cmd2, offset);
				return nextposBitmap;
			} else {
				// cannot move to page: out of document range
				return null;
			}
		} else {
			// SCROLL next or prev page requested, with pixel offset specified
			int y = currpos.y + offset;
			if ( doc.doCommand(ReaderCommand.DCMD_GO_POS.nativeId, y) ) {
				PositionProperties nextpos = doc.getPositionProps(null);
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
					doc.setBatteryState(mBatteryState);
					doc.getPageImage(bi.bitmap);
			        mNextPageInfo = bi;
			        nextposBitmap = bi;
				}
				// return back to prev position
				doc.doCommand(ReaderCommand.DCMD_GO_POS.nativeId, currpos.y);
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
		boolean isPartially;
		DrawPageTask(Runnable doneHandler, boolean isPartially)
		{
//			// DEBUG stack trace
//			try {
//				throw new Exception("DrawPageTask() stack trace");
//			} catch (Exception e) {
//				Log.d("cr3", "stack trace", e);
//			}
			this.id = ++lastDrawTaskId;
			this.doneHandler = doneHandler;
			this.isPartially = isPartially;
			cancelGc();
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
				bookView.draw(isPartially);
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
   			//hideProgress();
   			if ( doneHandler!=null )
   				doneHandler.run();
   			scheduleGc();
		}
		@Override
		public void fail(Exception e) {
   			hideProgress();
		}
	};
	
	static class ReaderSurfaceView extends SurfaceView {
		public ReaderSurfaceView( Context context )
		{
			super(context);
		}
	}
	
//	private boolean mIsOnFront = false;
	private int requestedWidth = 0;
	private int requestedHeight = 0;
//	public void setOnFront(boolean front) {
//		if (mIsOnFront == front)
//			return;
//		mIsOnFront = front;
//		log.d("setOnFront(" + front + ")");
//		if (mIsOnFront) {
//			checkSize();
//		} else {
//			// save position immediately
//			scheduleSaveCurrentPositionBookmark(0);
//		}
//	}

	private void requestResize(int width, int height) {
		requestedWidth = width;
		requestedHeight = height;
		checkSize();
	}

	private void checkSize() {
		boolean changed = (requestedWidth != internalDX) || (requestedHeight != internalDY);
		if (!changed)
			return;
//		if (mIsOnFront || !mOpened) {
			log.d("checkSize() : calling resize");
			resize();
//		} else {
//			log.d("Skipping resize request");
//		}
	}
	
	private void resize() {
		final int thisId = ++lastResizeTaskId;
//	    if ( w<h && mActivity.isLandscape() ) {
//	    	log.i("ignoring size change to portrait since landscape is set");
//	    	return;
//	    }
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
	    		post(new Task() {
	    			public void work() {
	    				BackgroundThread.ensureBackground();
	    				if ( thisId != lastResizeTaskId ) {
	    					log.d("skipping duplicate resize request");
	    					return;
	    				}
	    		        internalDX = requestedWidth;
	    		        internalDY = requestedHeight;
	    				log.d("ResizeTask: resizeInternal(" + internalDX + "," + internalDY + ")");
	    		        doc.resize(internalDX, internalDY);
//	    		        if ( mOpened ) {
//	    					log.d("ResizeTask: done, drawing page");
//	    			        drawPage();
//	    		        }
	    			}
	    			public void done() {
	    				clearImageCache();
	    				drawPage(null, false);
	    				//redraw();
	    			}
	    		});
	    	}
	    };
	    if ( mOpened ) {
	    	log.d("scheduling delayed resize task id=" + thisId);
	    	BackgroundThread.instance().postGUI(task, 300);
	    } else {
	    	log.d("executing resize without delay");
	    	task.run();
	    }
	}
	
	int hackMemorySize = 0;
	// SurfaceView callbacks
	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, final int width,
			final int height) {
		log.i("surfaceChanged(" + width + ", " + height + ")");

		if (hackMemorySize <= 0) {
			hackMemorySize = width * height * 2;
			runtime.trackFree(hackMemorySize);
		}

		
		surface.invalidate();
		//if (!isProgressActive())
		bookView.draw();
		//requestResize(width, height);
		//draw();
	}

	boolean mSurfaceCreated = false;
	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		log.i("surfaceCreated()");
		mSurfaceCreated = true;
		//draw();
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		log.i("surfaceDestroyed()");
		mSurfaceCreated = false;
		if (hackMemorySize > 0) {
			runtime.trackAlloc(hackMemorySize);
			hackMemorySize = 0;
		}
	}
	
	enum AnimationType {
		SCROLL, // for scroll mode
		PAGE_SHIFT, // for simple page shift
	}

	
	
	private ViewAnimationControl currentAnimation = null;

	private int pageFlipAnimationSpeedMs = DEF_PAGE_FLIP_MS; // if 0 : no animation
	private int pageFlipAnimationMode = PAGE_ANIMATION_SLIDE2; //PAGE_ANIMATION_PAPER; // if 0 : no animation
//	private void animatePageFlip( final int dir ) {
//		animatePageFlip(dir, null);
//	}
	private void animatePageFlip( final int dir, final Runnable onFinishHandler )
	{
		BackgroundThread.instance().executeBackground(new Runnable() {
			@Override
			public void run() {
				BackgroundThread.ensureBackground();
				if ( currentAnimation==null ) {
					PositionProperties currPos = doc.getPositionProps(null);
					if ( currPos==null )
						return;
					if (mCurrentPageInfo == null)
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
						if (currentAnimation != null) {
							if (currentAnimation != null) {
								nextHiliteId++;
								hiliteRect = null;
								currentAnimation.update(toX, h/2);
								currentAnimation.move(speed, true);
								currentAnimation.stop(-1, -1);
							}
							if ( onFinishHandler!=null )
								BackgroundThread.instance().executeGUI(onFinishHandler);
						}
					} else {
						//new ScrollViewAnimation(startY, maxY);
						int fromY = dir>0 ? h*7/8 : 0;
						int toY = dir>0 ? 0 : h*7/8;
						new ScrollViewAnimation(fromY, h);
						if (currentAnimation != null) {
							if (currentAnimation != null) {
								nextHiliteId++;
								hiliteRect = null;
								currentAnimation.update(w/2, toY);
								currentAnimation.move(speed, true);
								currentAnimation.stop(-1, -1);
							}
							if ( onFinishHandler!=null )
								BackgroundThread.instance().executeGUI(onFinishHandler);
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
		hiliteTapZone( false, 0, 0, surface.getWidth(), surface.getHeight() );
	}
	private void hiliteTapZone( final boolean hilite, final int startX, final int startY, final int maxX, final int maxY )
	{
		alog.d("highliteTapZone("+startX + ", " + startY+")");
		final int myHiliteId = ++nextHiliteId;
		int txcolor = mSettings.getColor(PROP_FONT_COLOR, Color.BLACK);
		final int color = (txcolor & 0xFFFFFF) | (HILITE_RECT_ALPHA<<24);
		BackgroundThread.instance().executeBackground(new Runnable() {
			@Override
			public void run() {
				if ( myHiliteId != nextHiliteId || (!hilite && hiliteRect==null) )
					return;
				
				if (currentAutoScrollAnimation!=null) {
					hiliteRect = null;
					return;
				}
				
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
//					    			if ( true ) {
					    				canvas.drawRect(new Rect(rc.left, rc.top, rc.right-2, rc.top+2), p);
					    				canvas.drawRect(new Rect(rc.left, rc.top+2, rc.left+2, rc.bottom-2), p);
					    				canvas.drawRect(new Rect(rc.right-2-2, rc.top+2, rc.right-2, rc.bottom-2), p);
					    				canvas.drawRect(new Rect(rc.left+2, rc.bottom-2-2, rc.right-2-2, rc.bottom-2), p);
//					    			} else {
//					    				canvas.drawRect(rc, p);
//					    			}
				    			}
				    		}
						}
						
					}, rc, false);
				}
			}
			
		});
	}
	private void scheduleUnhilite( int delay ) {
		final int myHiliteId = nextHiliteId;
		BackgroundThread.instance().postGUI(new Runnable() {
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
		int index = n - 1 - y * n / surface.getHeight();
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
			if ( showBrightnessFlickToast ) {
				String s = OptionsDialog.mBacklightLevelsTitles[currentBrightnessValueIndex];
				mActivity.showToast(s);
			}
			saveSettings(mSettings);
			currentBrightnessValueIndex = -1;
		}
	}
	private static final boolean showBrightnessFlickToast = false;


	private void startAnimation( final int startX, final int startY, final int maxX, final int maxY, final int newX, final int newY )
	{
		alog.d("startAnimation("+startX + ", " + startY+")");
		BackgroundThread.instance().executeBackground(new Runnable() {
			@Override
			public void run() {
				BackgroundThread.ensureBackground();
				PositionProperties currPos = doc.getPositionProps(null);
				if ( currPos!=null && currPos.pageMode!=0 ) {
					//int dir = startX > maxX/2 ? currPos.pageMode : -currPos.pageMode;
					//int dir = startX > maxX/2 ? 1 : -1;
					int dir = newX - startX < 0 ? 1 : -1;
					int sx = startX;
//					if ( dir<0 )
//						sx = 0;
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

	private volatile int updateSerialNumber = 0;
	private class AnimationUpdate {
		private int x;
		private int y;
		ViewAnimationControl myAnimation;
		public void set(int x, int y) {
			this.x = x;
			this.y = y;
		}
		public AnimationUpdate(int x, int y) {
			this.x = x;
			this.y = y;
			this.myAnimation = currentAnimation;
			scheduleUpdate();
		}
		private void scheduleUpdate() {
			BackgroundThread.instance().postBackground(new Runnable() {
				@Override
				public void run() {
					alog.d("updating("+x + ", " + y+")");
					boolean animate = false;
					synchronized (AnimationUpdate.class) {
						
						if (currentAnimation != null && currentAnimationUpdate == AnimationUpdate.this) {
							currentAnimationUpdate = null;
							currentAnimation.update(x, y);
							animate = true;
						}
					}
					if (animate)
						currentAnimation.animate();
				}
			});
		}
		
	}
	private AnimationUpdate currentAnimationUpdate;
	private void updateAnimation( final int x, final int y )
	{
		alog.d("updateAnimation("+x + ", " + y+")");
		synchronized(AnimationUpdate.class) {
			if (currentAnimationUpdate != null)
				currentAnimationUpdate.set(x, y);
			else
				currentAnimationUpdate = new AnimationUpdate(x, y);
		}
		try {
			// give a chance to background thread to process event faster
			Thread.sleep(0);
		} catch ( InterruptedException e ) {
			// ignore
		}
	}
	
	private void stopAnimation( final int x, final int y )
	{
		alog.d("stopAnimation("+x+", "+y+")");
		BackgroundThread.instance().executeBackground(new Runnable() {
			@Override
			public void run() {
				if ( currentAnimation!=null ) {
					currentAnimation.stop(x, y);
				}
			}
			
		});
	}

	DelayedExecutor animationScheduler = DelayedExecutor.createBackground("animation");
	private void scheduleAnimation()
	{
		animationScheduler.post(new Runnable() {
			@Override
			public void run() {
				if (currentAnimation != null) {
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
		abstract void draw( Canvas canvas );
	}

//	private Object surfaceLock = new Object(); 

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
		int pos = x1 > x0 ? 100 * intervals * (x - x0) / (x1-x0) : x1;
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
	private void drawCallback( DrawCanvasCallback callback, Rect rc, boolean isPartially )
	{
		if ( !mSurfaceCreated )
			return;
		//synchronized(surfaceLock) { }
		//log.v("draw() - in thread " + Thread.currentThread().getName());
		final SurfaceHolder holder = surface.getHolder();
		//log.v("before synchronized(surfaceLock)");
		if ( holder!=null )
		//synchronized(surfaceLock) 
		{
			Canvas canvas = null;
			long startTs = android.os.SystemClock.uptimeMillis();
			try {
				canvas = holder.lockCanvas(rc);
				//log.v("before draw(canvas)");
				if ( canvas!=null ) {
					if (DeviceInfo.EINK_SCREEN){
						EinkScreen.PrepareController(surface, isPartially);
					}
					callback.drawTo(canvas);
				}
			} finally {
				//log.v("exiting finally");
				if ( canvas!=null && surface.getHolder()!=null ) {
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
		//long startTimeStamp;
		boolean started;
		public boolean isStarted()
		{
			return started;
		}

		ViewAnimationBase()
		{
			//startTimeStamp = android.os.SystemClock.uptimeMillis();
			cancelGc();
		}

		public void close()
		{
			animationScheduler.cancel();
			currentAnimation = null;
			scheduleSaveCurrentPositionBookmark(DEF_SAVE_POSITION_INTERVAL);
			updateCurrentPositionStatus();
			scheduleGc();
		}

		public void draw()
		{
			draw(false);
		}

		public void draw(boolean isPartially)
		{
			drawCallback( new DrawCanvasCallback() {
				@Override
				public void drawTo(Canvas c) {
				//	long startTs = android.os.SystemClock.uptimeMillis();
					draw(c);
				}
			}, null, isPartially);
		}
	}
	
	//private static final int PAGE_ANIMATION_DURATION = 3000;
	class ScrollViewAnimation extends ViewAnimationBase {
		int startY;
		int maxY;
		int pointerStartPos;
		int pointerDestPos;
		int pointerCurrPos;
		BitmapInfo image1;
		BitmapInfo image2;
		ScrollViewAnimation( int startY, int maxY )
		{
			super();
			this.startY = startY;
			this.maxY = maxY;
			long start = android.os.SystemClock.uptimeMillis();
			log.v("ScrollViewAnimation -- creating: drawing two pages to buffer");
			PositionProperties currPos = doc.getPositionProps(null);
			int pos = currPos.y;
			int pos0 = pos - (maxY - startY);
			if ( pos0<0 )
				pos0 = 0;
			pointerStartPos = pos;
			pointerCurrPos = pos;
			pointerDestPos = startY;
			doc.doCommand(ReaderCommand.DCMD_GO_POS.nativeId, pos0);
			image1 = preparePageImage(0);
			if (image1 == null) {
				log.v("ScrollViewAnimation -- not started: image is null");
				return;
			}
			image2 = preparePageImage(image1.position.pageHeight);
			doc.doCommand(ReaderCommand.DCMD_GO_POS.nativeId, pos);
			if (image2 == null) {
				log.v("ScrollViewAnimation -- not started: image is null");
				return;
			}
			long duration = android.os.SystemClock.uptimeMillis() - start;
			log.v("ScrollViewAnimation -- created in " + duration + " millis");
			currentAnimation = this;
		}
		
		@Override
		public void stop(int x, int y) {
			if (currentAnimation == null)
				return;
			//if ( started ) {
				if ( y!=-1 ) {
					int delta = startY - y;
					pointerCurrPos = pointerStartPos + delta;
				}
				pointerDestPos = pointerCurrPos;
				draw();
				doc.doCommand(ReaderCommand.DCMD_GO_POS.nativeId, pointerDestPos);
			//}
			scheduleSaveCurrentPositionBookmark(DEF_SAVE_POSITION_INTERVAL);
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
//			BitmapInfo image1 = mCurrentPageInfo;
//			BitmapInfo image2 = mNextPageInfo;
			if (image1 == null || image1.isReleased() || image2 == null || image2.isReleased())
				return;
			int h = image1.position.pageHeight;
			int rowsFromImg1 = image1.position.y + h - pointerCurrPos;
			int rowsFromImg2 = h - rowsFromImg1;
    		Rect src1 = new Rect(0, h-rowsFromImg1, mCurrentPageInfo.bitmap.getWidth(), h);
    		Rect dst1 = new Rect(0, 0, mCurrentPageInfo.bitmap.getWidth(), rowsFromImg1);
    		drawDimmedBitmap(canvas, image1.bitmap, src1, dst1);
			if (image2 != null) {
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

		BitmapInfo image1;
		BitmapInfo image2;
		
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
				currPos = doc.getPositionProps(null);
			page1 = currPos.pageNumber;
			page2 = currPos.pageNumber + direction;
			if ( page2<0 || page2>=currPos.pageCount) {
				currentAnimation = null;
				return;
			}
			this.pageCount = currPos.pageMode;
			image1 = preparePageImage(0);
			image2 = preparePageImage(direction);
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
			divPaint.setColor(mActivity.isNightMode() ? Color.argb(96, 64, 64, 64) : Color.argb(128, 128, 128, 128));
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
					hilitePaints[i].setColor(Color.argb((i+1)*96 / numPaints, 64, 64, 64));
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
		
		private final static int DISTORT_PART_PERCENT = 30;
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
				if (index > DST_TABLE.length)
					index = DST_TABLE.length;
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
			if (currentAnimation == null)
				return;
			alog.v("PageViewAnimation.stop(" + x + ", " + y + ")");
			//if ( started ) {
				boolean moved = false;
				if ( x!=-1 ) {
					int threshold = mActivity.getPalmTipPixels() * 7/8;
					if ( direction > 0 ) {
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
				doc.doCommand(ReaderCommand.DCMD_GO_PAGE_DONT_SAVE_HISTORY.nativeId, moved ? page2 : page1);
			//}
			scheduleSaveCurrentPositionBookmark(DEF_SAVE_POSITION_INTERVAL);
			close();
			// preparing images for next page flip
			preparePageImage(0);
			preparePageImage(direction);
			updateCurrentPositionStatus();
			//if ( started )
			//	drawPage();
		}

		@Override
		public void update(int x, int y) {
			alog.v("PageViewAnimation.update(" + x + ", " + y + ")");
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
			alog.v("PageViewAnimation.animate("+currShift + " => " + destShift + ") speed=" + pageFlipAnimationSpeedMs);
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
					alog.v("PageViewAnimation.animate("+currShift + " => " + destShift + "  step=" + step + ")");
				}
				//pointerCurrPos = pointerDestPos;
				draw();
				if ( currShift != destShift )
					scheduleAnimation();
			}
		}

		public void draw(Canvas canvas)
		{
			alog.v("PageViewAnimation.draw("+currShift + ")");
//			BitmapInfo image1 = mCurrentPageInfo;
//			BitmapInfo image2 = mNextPageInfo;
			if (image1.isReleased() || image2.isReleased())
				return;
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

	private int drawAnimationPos = 0;
	private Long[] drawAnimationStats = new Long[8];
	private long avgDrawAnimationDuration = 200;
	private long getAvgAnimationDrawDuration()
	{
		return avgDrawAnimationDuration; 
	}
	
	private void updateAnimationDurationStats( long duration )
	{
		if ( duration<=0 )
			duration = 1;
		else if ( duration>1000 )
			return;
		int pos = drawAnimationPos + 1;
		if (pos >= drawAnimationStats.length)
			pos = 0;
		drawAnimationStats[pos] = duration;
		drawAnimationPos = pos;
		long sum = 0;
		int count = 0;
		for (Long item : drawAnimationStats) {
			if (item != null) {
				sum += item;
				count++;
			}
		}
		avgDrawAnimationDuration = sum / count;
	}
	
	private void drawPage()
	{
		drawPage(null, false);
	}
	private void drawPage(boolean isPartially)
	{
		drawPage(null, isPartially);
	}
	private void drawPage( Runnable doneHandler, boolean isPartially )
	{
		if ( !mInitialized || !mOpened )
			return;
		log.v("drawPage() : submitting DrawPageTask");
		scheduleSaveCurrentPositionBookmark(DEF_SAVE_POSITION_INTERVAL);
		post( new DrawPageTask(doneHandler, isPartially) );
	}
	
	private int internalDX = 0;
	private int internalDY = 0;

	private byte[] coverPageBytes = null;
	private void findCoverPage()
	{
    	log.d("document is loaded succesfull, checking coverpage data");
    	byte[] coverpageBytes = doc.getCoverPageData();
    	if ( coverpageBytes!=null ) {
    		log.d("Found cover page data: " + coverpageBytes.length + " bytes");
			coverPageBytes = coverpageBytes;
    	}
	}

	private int currentProgressPosition = 1;
	private int currentProgressTitle = R.string.progress_loading;
	private void showProgress(int position, int titleResource) {
		log.v("showProgress(" + position + ")");
		boolean first = currentProgressTitle == 0;
		if (currentProgressPosition != position || currentProgressTitle != titleResource) {
			currentProgressPosition = position;
			currentProgressTitle = titleResource;
			bookView.draw(!first);
		}
	}
	
	private void hideProgress() {
		log.v("hideProgress()");
		if (currentProgressTitle != 0) {
			currentProgressPosition = -1;
			currentProgressTitle = 0;
			bookView.draw(false);
		}
	}
	
	private boolean isProgressActive() {
		return currentProgressPosition > 0;
	}
	
	private class LoadDocumentTask extends Task
	{
		String filename;
		String path;
		Runnable errorHandler;
		String pos;
		int profileNumber;
		boolean disableInternalStyles;
		boolean disableTextAutoformat;
		Properties props;
		LoadDocumentTask(BookInfo bookInfo, Runnable errorHandler)
		{
			BackgroundThread.ensureGUI();
			mBookInfo = bookInfo;
			FileInfo fileInfo = bookInfo.getFileInfo();
			log.v("LoadDocumentTask for " + fileInfo);
			if (fileInfo.getTitle() == null) {
				// As a book 'should' have a title, no title means we should
				// retrieve the book metadata from the engine to get the
				// book language.
				// Is it OK to do this here???  Should we use isScanned?
				// Should we use another fileInfo flag or a new flag?
				mEngine.scanBookProperties(fileInfo);
			}
			String language = fileInfo.getLanguage();
			log.v("update hyphenation language: " + language + " for " + fileInfo.getTitle());
			mEngine.setHyphenationLanguage(language);
			this.filename = fileInfo.getPathName();
			this.path = fileInfo.arcname != null ? fileInfo.arcname : fileInfo.pathname;
			this.errorHandler = errorHandler;
			//FileInfo fileInfo = new FileInfo(filename);
			disableInternalStyles = mBookInfo.getFileInfo().getFlag(FileInfo.DONT_USE_DOCUMENT_STYLES_FLAG);
			disableTextAutoformat = mBookInfo.getFileInfo().getFlag(FileInfo.DONT_REFLOW_TXT_FILES_FLAG);
			profileNumber = mBookInfo.getFileInfo().getProfileId();
			Properties oldSettings = new Properties(mSettings);
			// TODO: enable storing of profile per book
			mActivity.setCurrentProfile(profileNumber);
	    	if ( mBookInfo!=null && mBookInfo.getLastPosition()!=null )
	    		pos = mBookInfo.getLastPosition().getStartPos();
			log.v("LoadDocumentTask : book info " + mBookInfo);
			log.v("LoadDocumentTask : last position = " + pos);
			if (mBookInfo != null && mBookInfo.getLastPosition() != null)
			    setTimeElapsed(mBookInfo.getLastPosition().getTimeElapsed());			
    		//mBitmap = null;
	        //showProgress(1000, R.string.progress_loading);
	        //draw();
	        BackgroundThread.instance().postGUI(new Runnable() {
				@Override
				public void run() {
					bookView.draw(false);
				}
			});
	        //init();
	        // close existing document
			log.v("LoadDocumentTask : closing current book");
	        close();
	        if (props != null) {
		        setAppSettings(props, oldSettings);
	    		BackgroundThread.instance().postBackground(new Runnable() {
	    			@Override
	    			public void run() {
	    				log.v("LoadDocumentTask : switching current profile");
	    				applySettings(props);
	    				log.i("Switching done");
	    			}
	    		});
	        }
		}

		@Override
		public void work() throws IOException {
			BackgroundThread.ensureBackground();
			coverPageBytes = null;
			log.i("Loading document " + filename);
			doc.doCommand(ReaderCommand.DCMD_SET_INTERNAL_STYLES.nativeId, disableInternalStyles ? 0 : 1);
			doc.doCommand(ReaderCommand.DCMD_SET_TEXT_FORMAT.nativeId, disableTextAutoformat ? 0 : 1);
	        boolean success = doc.loadDocument(filename);
	        if ( success ) {
				log.v("loadDocumentInternal completed successfully");
				
				doc.requestRender();
				
	        	findCoverPage();
				log.v("requesting page image, to render");
				if (internalDX == 0 || internalDY == 0) {
					internalDX = surface.getWidth();
					internalDY = surface.getHeight();
					log.d("LoadDocument task: no size defined, resizing using widget size");
					doc.resize(internalDX, internalDY);
				}
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

		@Override
		public void done()
		{
			BackgroundThread.ensureGUI();
			log.d("LoadDocumentTask, GUI thread is finished successfully");
			if (Services.getHistory() != null) {
				Services.getHistory().updateBookAccess(mBookInfo, getTimeElapsed());
				if (mActivity.getDB() != null)
					mActivity.getDB().saveBookInfo(mBookInfo);
		        if (coverPageBytes!=null && mBookInfo!=null && mBookInfo.getFileInfo()!=null) {
		        	if (mBookInfo.getFileInfo().format.needCoverPageCaching()) {
		        		// TODO: fix it
//		        		if (mActivity.getBrowser() != null)
//		        			mActivity.getBrowser().setCoverpageData(new FileInfo(mBookInfo.getFileInfo()), coverPageBytes);
		        	}
		        	if (DeviceInfo.EINK_NOOK)
		        		updateNookTouchCoverpage(mBookInfo.getFileInfo().getPathName(), coverPageBytes);
		        	//mEngine.setProgressDrawable(coverPageDrawable);
		        }
		        if (DeviceInfo.EINK_SONY) {
		            SonyBookSelector selector = new SonyBookSelector(mActivity);
		            long l = selector.getContentId(path);
		            if(l != 0) {
		                 selector.setReadingTime(l);
		                 selector.requestBookSelection(l);
		            }		        	
		        }
		        mOpened = true;
		        
		        highlightBookmarks();
		        
		        drawPage();
		        BackgroundThread.instance().postGUI(new Runnable() {
		        	public void run() {
		    			mActivity.showReader();
		        	}
		        });
		        mActivity.setLastBook(filename);
			}
		}
		public void fail( Exception e )
		{
			BackgroundThread.ensureGUI();
			log.v("LoadDocumentTask failed for " + mBookInfo, e);
			Services.getHistory().removeBookInfo(mActivity.getDB(), mBookInfo.getFileInfo(), true, false );
			mBookInfo = null;
			log.d("LoadDocumentTask is finished with exception " + e.getMessage());
	        mOpened = false;
			drawPage();
			hideProgress();
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
		if (DeviceInfo.EINK_SCREEN)
			return; // no backlight
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
	
	protected void drawPageBackground(Canvas canvas, Rect dst, int side) {
		Bitmap bmp = currentBackgroundTextureBitmap;
		if (bmp != null) {
			int h = bmp.getHeight();
			int w = bmp.getWidth();
    		Rect src = new Rect(0, 0, w, h);
			if (currentBackgroundTextureTiled) {
				// TILED
				for (int x = 0; x < dst.width(); x += w) {
					int ww = w;
					if (x + ww > dst.width())
						ww = dst.width() - x;
					for (int y = 0; y < dst.height(); y += h) {
						int hh = h;
						if (y + hh > dst.height())
							hh = dst.height() - y;
						Rect d = new Rect(x, y, x + ww, y + hh);
						Rect s = new Rect(0, 0, ww, hh);
		        		drawDimmedBitmap(canvas, bmp, s, d);
					}
				}
			} else {
				// STRETCHED
				if (side == VIEWER_TOOLBAR_LONG_SIDE)
					side = canvas.getWidth() > canvas.getHeight() ? VIEWER_TOOLBAR_TOP : VIEWER_TOOLBAR_LEFT;
				else if (side == VIEWER_TOOLBAR_SHORT_SIDE)
					side = canvas.getWidth() < canvas.getHeight() ? VIEWER_TOOLBAR_TOP : VIEWER_TOOLBAR_LEFT;
				switch(side) {
				case VIEWER_TOOLBAR_LEFT:
					{
						int d = dst.width() * dst.height() / h;
						if (d > w)
							d = w;
						src.left = src.right - d;
					}
					break;
				case VIEWER_TOOLBAR_RIGHT:
					{
						int d = dst.width() * dst.height() / h;
						if (d > w)
							d = w;
						src.right = src.left + d;
					}
					break;
				case VIEWER_TOOLBAR_TOP:
					{
						int d = dst.height() * dst.width() / w;
						if (d > h)
							d = h;
						src.top = src.bottom - d;
					}
					break;
				case VIEWER_TOOLBAR_BOTTOM:
					{
						int d = dst.height() * dst.width() / w;
						if (d > h)
							d = h;
						src.bottom = src.top + d;
					}
					break;
				}
        		drawDimmedBitmap(canvas, bmp, src, dst);
			}
		} else {
			canvas.drawColor(currentBackgroundColor | 0xFF000000);
		}
	}

	protected void drawPageBackground(Canvas canvas) {
		Rect dst = new Rect(0, 0, canvas.getWidth(), canvas.getHeight());
		drawPageBackground(canvas, dst, VIEWER_TOOLBAR_NONE);
	}
	
	public class ToolbarBackgroundDrawable extends Drawable {
		private int location = VIEWER_TOOLBAR_NONE;
		private int alpha;
		public void setLocation(int location) {
			this.location = location;
		}
		@Override
		public void draw(Canvas canvas) {
			Rect dst = new Rect(0, 0, canvas.getWidth(), canvas.getHeight());
			try {
				drawPageBackground(canvas, dst, location);
			} catch (Exception e) {
				L.e("Exception in ToolbarBackgroundDrawable.draw", e);
			}
		}
		@Override
		public int getOpacity() {
			return 255 - alpha;
		}
		@Override
		public void setAlpha(int alpha) {
			this.alpha = alpha;
			
		}
		@Override
		public void setColorFilter(ColorFilter cf) {
			// not supported
		}
	}
	
	public ToolbarBackgroundDrawable createToolbarBackgroundDrawable() {
		return new ToolbarBackgroundDrawable();
	}
	
	protected void doDrawProgress(Canvas canvas, int position, int titleResource) {
		log.v("doDrawProgress(" + position + ")");
		if (titleResource == 0)
			return;
		int w = canvas.getWidth();
		int h = canvas.getHeight();
		int mins = (w < h ? w : h) * 7 / 10;
		int ph = mins / 20;
		int textColor = mSettings.getColor(PROP_FONT_COLOR, 0x000000);
		Rect rc = new Rect(w / 2 - mins / 2, h / 2 - ph / 2, w / 2 + mins / 2, h / 2 + ph / 2);
		
		Utils.drawFrame(canvas, rc, Utils.createSolidPaint(0xC0000000 | textColor));
		//canvas.drawRect(rc, createSolidPaint(0xFFC0C0A0));
		rc.left += 2;
		rc.right -= 2;
		rc.top += 2;
		rc.bottom -= 2;
		int x = rc.left + (rc.right - rc.left) * position / 10000;
		Rect rc1 = new Rect(rc);
		rc1.right = x;
		canvas.drawRect(rc1, Utils.createSolidPaint(0x80000000 | textColor));
		Paint textPaint = Utils.createSolidPaint(0xFF000000 | textColor);
		textPaint.setTextAlign(Paint.Align.CENTER);
		textPaint.setTextSize(22f);
		textPaint.setSubpixelText(true);
		canvas.drawText(String.valueOf(mActivity.getText(titleResource)), (rc.left + rc.right) / 2, rc1.top - 12, textPaint);
		//canvas.drawText(String.valueOf(position * 100 / 10000) + "%", rc.left + 4, rc1.bottom - 4, textPaint);
//		Rect rc2 = new Rect(rc);
//		rc.left = x;
//		canvas.drawRect(rc2, createSolidPaint(0xFFC0C0A0));
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
					bookView.draw();
				}
    			
    		});
    	}
    }

    private void restorePositionBackground( String pos )
    {
		BackgroundThread.ensureBackground();
    	if (pos != null) {
			BackgroundThread.ensureBackground();
			doc.goToPosition(pos, false);
    		preparePageImage(0);
    		hideProgress();
    		drawPage();
    		updateCurrentPositionStatus();
    	}
    }
    
    private int lastSavePositionTaskId = 0;
    
    private final static int DEF_SAVE_POSITION_INTERVAL = 180000; // 3 minutes
    private void scheduleSaveCurrentPositionBookmark(final int delayMillis) {
    	// GUI thread required
    	BackgroundThread.instance().executeGUI(new Runnable() {
			@Override
			public void run() {
		    	final int mylastSavePositionTaskId = ++lastSavePositionTaskId;
				if (isBookLoaded() && mBookInfo != null) {
			    	Bookmark bmk = doc.getCurrentPageBookmarkNoRender();
			    	if (bmk != null) {
			            bmk.setTimeStamp(System.currentTimeMillis());
			            bmk.setType(Bookmark.TYPE_LAST_POSITION);
		                mBookInfo.setLastPosition(bmk);
		                if (delayMillis <= 1)
		                	getSyncService().saveBookmark(mBookInfo.getFileInfo().getPathName(), bmk, true);
			    	}
			    	final BookInfo bookInfo = mBookInfo;
			    	if (delayMillis <= 1) {
						if (bookInfo != null && mActivity.getDB() != null) {
							log.v("saving last immediately");
							Services.getHistory().updateBookAccess(bookInfo, getTimeElapsed());
							mActivity.getDB().saveBookInfo(bookInfo);
							mActivity.getDB().flush();
						}
			    	} else {
				    	BackgroundThread.instance().postGUI(new Runnable() {
							@Override
							public void run() {
								if (mylastSavePositionTaskId == lastSavePositionTaskId) {
									if (bookInfo != null) {
										log.v("saving last position");
										if (Services.getHistory() != null) {
											Services.getHistory().updateBookAccess(bookInfo, getTimeElapsed());
											if (mActivity.getDB() != null)
												mActivity.getDB().saveBookInfo(bookInfo);
										}
						                //mActivity.getDB().flush();
									}
								}
							}}, delayMillis);
			    	}
				}
			}
    	});
    }
    
    public interface PositionPropertiesCallback {
    	void onPositionProperties(PositionProperties props, String positionText);
    }
    public void getCurrentPositionProperties(final PositionPropertiesCallback callback) {
    	BackgroundThread.instance().postBackground(new Runnable() {
			@Override
			public void run() {
				final Bookmark bmk = (doc != null) ? doc.getCurrentPageBookmarkNoRender() : null;
				final PositionProperties props = (bmk != null) ? doc.getPositionProps(bmk.getStartPos()) : null;
		    	BackgroundThread.instance().postBackground(new Runnable() {
					@Override
					public void run() {
						String posText = null;
						if (props != null) {
							int percent = (int)(10000 * (long)props.y / props.fullHeight);
							String percentText = "" + (percent/100) + "." + (percent%10) + "%";
							posText = "" + props.pageNumber + " / " + props.pageCount + " (" + percentText + ")";
						}
						callback.onPositionProperties(props, posText);
					}
		    	});
				
			}
    	});
    }
    
    public Bookmark saveCurrentPositionBookmarkSync( final boolean saveToDB ) {
    	++lastSavePositionTaskId;
        Bookmark bmk = BackgroundThread.instance().callBackground(new Callable<Bookmark>() {
            @Override
            public Bookmark call() throws Exception {
                if ( !mOpened )
                    return null;
                return doc.getCurrentPageBookmark();
            }
        });
        if ( bmk!=null ) {
            bmk.setTimeStamp(System.currentTimeMillis());
            bmk.setType(Bookmark.TYPE_LAST_POSITION);
            if ( mBookInfo!=null )
                mBookInfo.setLastPosition(bmk);
            if ( saveToDB ) {
            	Services.getHistory().updateRecentDir();
            	mActivity.getDB().saveBookInfo(mBookInfo);
            	mActivity.getDB().flush();
            }
        }
        return bmk;
    }

    public void save()
    {
    	mActivity.einkRefresh();
		BackgroundThread.ensureGUI();
		if (isBookLoaded() && mBookInfo != null) {
			log.v("saving last immediately");
			log.d("bookmark count 1 = " + mBookInfo.getBookmarkCount());
			Services.getHistory().updateBookAccess(mBookInfo, getTimeElapsed());
			log.d("bookmark count 2 = " + mBookInfo.getBookmarkCount());
			mActivity.getDB().saveBookInfo(mBookInfo);
			log.d("bookmark count 3 = " + mBookInfo.getBookmarkCount());
			mActivity.getDB().flush();
		}
		//scheduleSaveCurrentPositionBookmark(0);
    	//post( new SavePositionTask() );
    }
    
    public void close()
    {
		BackgroundThread.ensureGUI();
    	log.i("ReaderView.close() is called");
    	if ( !mOpened )
    		return;
		cancelSwapTask();
		stopImageViewer();
		save();
        //scheduleSaveCurrentPositionBookmark(0);
		//save();
    	post( new Task() {
    		public void work() {
    			BackgroundThread.ensureBackground();
    			if ( mOpened ) {
	    			mOpened = false;
					log.i("ReaderView().close() : closing current document");
					doc.doCommand(ReaderCommand.DCMD_CLOSE_BOOK.nativeId, 0);
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
    	if (mInitialized) {
        	//close();
        	BackgroundThread.instance().postBackground(new Runnable() {
        		public void run() {
        			BackgroundThread.ensureBackground();
        	    	if ( mInitialized ) {
        	        	log.i("ReaderView.destroyInternal() calling");
        	        	doc.destroy();
        	    		mInitialized = false;
        	    		currentBackgroundTexture = Engine.NO_TEXTURE;
        	    	}
        		}
        	});
    		//engine.waitTasksCompletion();
    	}
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
				String css = Engine.loadFileUtf8(file);
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
	    	log.d("readerCallback.OnExportProgress " + percent);
			return true;
		}
		public void OnExternalLink(String url, String nodeXPath) {
		}
		public void OnFormatEnd() {
	    	log.d("readerCallback.OnFormatEnd");
			//mEngine.hideProgress();
	    	hideProgress();
			drawPage();
			scheduleSwapTask();
		}
		public boolean OnFormatProgress(final int percent) {
			if ( enable_progress_callback ) {
		    	log.d("readerCallback.OnFormatProgress " + percent);
		    	showProgress( percent*4/10 + 5000, R.string.progress_formatting);
			}
//			executeSync( new Callable<Object>() {
//				public Object call() {
//					BackgroundThread.ensureGUI();
//			    	log.d("readerCallback.OnFormatProgress " + percent);
//			    	showProgress( percent*4/10 + 5000, R.string.progress_formatting);
//			    	return null;
//				}
//			});
			return true;
		}
		public void OnFormatStart() {
	    	log.d("readerCallback.OnFormatStart");
		}
		public void OnLoadFileEnd() {
	    	log.d("readerCallback.OnLoadFileEnd");
			if (internalDX == 0 && internalDY == 0) {
				internalDX = requestedWidth;
				internalDY = requestedHeight;
				log.d("OnLoadFileEnd: resizeInternal(" + internalDX + "," + internalDY + ")");
				doc.resize(internalDX, internalDY);
				hideProgress();
			}
	    	
		}
		public void OnLoadFileError(String message) {
	    	log.d("readerCallback.OnLoadFileError(" + message + ")");
		}
		public void OnLoadFileFirstPagesReady() {
	    	log.d("readerCallback.OnLoadFileFirstPagesReady");
		}
		public String OnLoadFileFormatDetected(final DocumentFormat fileFormat) {
			log.i("readerCallback.OnLoadFileFormatDetected " + fileFormat);
			if (fileFormat != null) {
				String s = getCSSForFormat(fileFormat);
				return s;
			}
			return null;
//
//			String res = executeSync( new Callable<String>() {
//				public String call() {
//					BackgroundThread.ensureGUI();
//					log.i("readerCallback.OnLoadFileFormatDetected " + fileFormat);
//					if (fileFormat != null) {
//						String s = getCSSForFormat(fileFormat);
//						log.i("setting .css for file format " + fileFormat + " from resource " + fileFormat.getCssName());
//						return s;
//					}
//			    	return null;
//				}
//			});
////			int internalStyles = mBookInfo.getFileInfo().getFlag(FileInfo.DONT_USE_DOCUMENT_STYLES_FLAG) ? 0 : 1;
////			int txtReflow = mBookInfo.getFileInfo().getFlag(FileInfo.DONT_REFLOW_TXT_FILES_FLAG) ? 0 : 2;
////			log.d("internalStyles: " + internalStyles);
////			doc.doCommand(ReaderCommand.DCMD_SET_INTERNAL_STYLES.nativeId, internalStyles | txtReflow);
//			return res;
		}
		public boolean OnLoadFileProgress(final int percent) {
			BackgroundThread.ensureBackground();
			if ( enable_progress_callback ) {
		    	log.d("readerCallback.OnLoadFileProgress " + percent);
		    	showProgress( percent*4/10 + 1000, R.string.progress_loading);
			}
//			executeSync( new Callable<Object>() {
//				public Object call() {
//					BackgroundThread.ensureGUI();
//			    	log.d("readerCallback.OnLoadFileProgress " + percent);
//			    	showProgress( percent*4/10 + 1000, R.string.progress_loading);
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
	    public boolean OnRequestReload() {
	    	//reloadDocument();
	    	return true;
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
			int res = doc.swapToCache();
			isTimeout = res==DocView.SWAP_TIMEOUT;
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
    public void clearImageCache()
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
        			doc.setStylesheet(css);
        		}
        	});
        }
    }
    
    public void goToPosition( int position )
    {
		BackgroundThread.ensureGUI();
		doEngineCommand(ReaderCommand.DCMD_GO_POS, position);
    }
    
    public void moveBy( final int delta )
    {
		BackgroundThread.ensureGUI();
		log.d("moveBy(" + delta + ")");
		post(new Task() {
			public void work() {
				BackgroundThread.ensureBackground();
				doc.doCommand(ReaderCommand.DCMD_SCROLL_BY.nativeId, delta);
				scheduleSaveCurrentPositionBookmark(DEF_SAVE_POSITION_INTERVAL);
			}
			public void done() {
				drawPage();
			}
		});
    }
    
    public void goToPage( int pageNumber )
    {
		BackgroundThread.ensureGUI();
		doEngineCommand(ReaderCommand.DCMD_GO_PAGE, pageNumber-1);
    }
    
    public void goToPercent( final int percent )
    {
		BackgroundThread.ensureGUI();
    	if ( percent>=0 && percent<=100 )
	    	post( new Task() {
	    		public void work() {
	    			PositionProperties pos = doc.getPositionProps(null);
	    			if ( pos!=null && pos.pageCount>0) {
	    				int pageNumber = pos.pageCount * percent / 100; 
						doCommandFromBackgroundThread(ReaderCommand.DCMD_GO_PAGE, pageNumber);
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
				res = doc.moveSelection(selection, command.nativeId, param);
			}

			@Override
			public void done() {
				if ( callback!=null ) {
					clearImageCache();
					surface.invalidate();
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

	private void showSwitchProfileDialog() {
		SwitchProfileDialog dlg = new SwitchProfileDialog(mActivity, this);
		dlg.show();
	}
	
//	private int currentProfile = 0;
//	public int getCurrentProfile() {
//		if (currentProfile == 0) {
//			currentProfile = mSettings.getInt(PROP_PROFILE_NUMBER, 1);
//			if (currentProfile < 1 || currentProfile > MAX_PROFILES)
//				currentProfile = 1;
//		}
//		return currentProfile;
//	}

	public void setCurrentProfile(int profile) {
		if (mActivity.getCurrentProfile() == profile)
			return;
		if (mBookInfo != null && mBookInfo.getFileInfo() != null) {
			mBookInfo.getFileInfo().setProfileId(profile);
			mActivity.getDB().saveBookInfo(mBookInfo);
		}
		log.i("Apply new profile settings");
		mActivity.setCurrentProfile(profile);
	}
    
    private final static String NOOK_TOUCH_COVERPAGE_DIR = "/media/screensavers/currentbook";
	private void updateNookTouchCoverpage(String bookFileName,
			byte[] coverpageBytes) {
		try {
			String imageFileName;
			int lastSlash = bookFileName.lastIndexOf("/");
			// exclude path and extension
			if (lastSlash >= 0 && lastSlash < bookFileName.length()) {
				imageFileName = bookFileName.substring(lastSlash);
			} else {
				imageFileName = bookFileName;
			}
			int lastDot = imageFileName.lastIndexOf(".");
			if (lastDot > 0) {
				imageFileName = imageFileName.substring(0, lastDot);
			}
			// guess image type
			if (coverpageBytes.length > 8 // PNG signature length
					&& coverpageBytes[0] == (byte)0x89 // PNG signature start 4 bytes
					&& coverpageBytes[1] == 0x50
					&& coverpageBytes[2] == 0x4E
					&& coverpageBytes[3] == 0x47) {
				imageFileName += ".png";
			} else if (coverpageBytes.length > 3 // Checking only the first 3
													// bytes of JPEG header
					&& coverpageBytes[0] == (byte)0xFF
					&& coverpageBytes[1] == (byte)0xD8
					&& coverpageBytes[2] == (byte)0xFF) {
				imageFileName += ".jpg";
			} else if (coverpageBytes.length > 3 // Checking only the first 3
													// bytes of GIF header
					&& coverpageBytes[0] == 0x47
					&& coverpageBytes[1] == 0x49
					&& coverpageBytes[2] == 0x46) {
				imageFileName += ".gif";
			} else if (coverpageBytes.length > 2 // Checking only the first 2
													// bytes of BMP signature
					&& coverpageBytes[0] == 0x42 && coverpageBytes[1] == 0x4D) {
				imageFileName += ".bmp";
			} else {
				imageFileName += ".jpg"; // default image type
			}
			// create directory if it does not exist
			File d = new File(NOOK_TOUCH_COVERPAGE_DIR);
			if (!d.exists()) {
				d.mkdir();
			}
			// create file only if file with same name does not exist
			File f = new File(d, imageFileName);
			if (!f.exists()) {
				// delete other files in directory so that only current cover is
				// shown all the time
				File[] files = d.listFiles();
				for (File oldFile : files) {
					oldFile.delete();
				}
				// write the image file
				FileOutputStream fos = new FileOutputStream(f);
				fos.write(coverpageBytes);
				fos.close();
			}
		} catch (Exception ex) {
			log.e("Error writing cover page: ", ex);
		}
	}
    
    private static final int GC_INTERVAL = 15000; // 15 seconds
    DelayedExecutor gcTask = DelayedExecutor.createGUI("gc");
    public void scheduleGc() {
    	try {
	    	gcTask.postDelayed(new Runnable() {
				@Override
				public void run() {
					log.v("Initiating garbage collection");
					System.gc();
				}
			}, GC_INTERVAL);
    	} catch (Exception e) {
    		// ignore
    	}
    }
    public void cancelGc() {
    	try {
    		gcTask.cancel();
    	} catch (Exception e) {
    		// ignore
    	}
    }

	private void switchFontFace(int direction) {
		String currentFontFace = mSettings.getProperty(PROP_FONT_FACE, "");
		String[] mFontFaces = Engine.getFontFaceList();
		int index = 0;
		int countFaces = mFontFaces.length;
		for (int i = 0; i < countFaces; i++) {
			if (mFontFaces[i].equals(currentFontFace)) {
				index = i;
				break;
			}
		}
		index += direction;
		if (index < 0)
			index = countFaces - 1;
		else if (index >= countFaces)
			index = 0;
		saveSetting(PROP_FONT_FACE, mFontFaces[index]);
        syncViewSettings(getSettings(), true, true);
	}

	public void showInputDialog( final String title, final String prompt, final boolean isNumberEdit, final int minValue, final int maxValue, final int lastValue, final InputHandler handler )
	{
		BackgroundThread.instance().executeGUI(new Runnable() {
			@Override
			public void run() {
		        final InputDialog dlg = new InputDialog(mActivity, title, prompt, isNumberEdit, minValue, maxValue, lastValue, handler);
		        dlg.show();
			}
			
		});
	}

	public void showGoToPageDialog() {
		getCurrentPositionProperties(new PositionPropertiesCallback() {
			@Override
			public void onPositionProperties(final PositionProperties props, final String positionText) {
				if (props == null)
					return;
				String pos = mActivity.getString(R.string.dlg_goto_current_position) + " " + positionText;
				String prompt = mActivity.getString(R.string.dlg_goto_input_page_number);
				showInputDialog(mActivity.getString(R.string.mi_goto_page), pos + "\n" + prompt, true,
						1, props.pageCount, props.pageNumber,
						new InputHandler() {
					int pageNumber = 0;
					@Override
					public boolean validate(String s) {
						pageNumber = Integer.valueOf(s); 
						return pageNumber>0 && pageNumber <= props.pageCount;
					}
					@Override
					public void onOk(String s) {
						goToPage(pageNumber);
					}
					@Override
					public void onCancel() {
					}
				});
			}
		});
	}
	public void showGoToPercentDialog() {
		getCurrentPositionProperties(new PositionPropertiesCallback() {
			@Override
			public void onPositionProperties(PositionProperties props, String positionText) {
				if (props == null)
					return;
				String pos = mActivity.getString(R.string.dlg_goto_current_position) + " " + positionText;
				String prompt = mActivity.getString(R.string.dlg_goto_input_percent);
				showInputDialog(mActivity.getString(R.string.mi_goto_percent), pos + "\n" + prompt, true,
						0, 100, props.y * 100 / props.fullHeight,
						new InputHandler() {
					int percent = 0;
					@Override
					public boolean validate(String s) {
						percent = Integer.valueOf(s); 
						return percent>=0 && percent<=100;
					}
					@Override
					public void onOk(String s) {
						goToPercent(percent);
					}
					@Override
					public void onCancel() {
					}
				});
			}
		});
	}

	@Override
	public boolean onKey(View v, int keyCode, KeyEvent event) {
		// TODO Auto-generated method stub
		if (event.getAction() == KeyEvent.ACTION_DOWN)
			return onKeyDown(keyCode, event);
		else if (event.getAction() == KeyEvent.ACTION_UP)
			return onKeyUp(keyCode, event);
		return false;
	}

	@Override
	public boolean onTouch(View v, MotionEvent event) {
		return onTouchEvent(event);
	}

	public boolean onKeyDown(int keyCode, final KeyEvent event) {
		
		if (keyCode == 0)
			keyCode = event.getScanCode();
		keyCode = translateKeyCode(keyCode);

		mActivity.onUserActivity();
		
		if (currentImageViewer != null)
			return currentImageViewer.onKeyDown(keyCode, event);

//		backKeyDownHere = false;
		if ( event.getRepeatCount()==0 ) {
			log.v("onKeyDown("+keyCode + ", " + event +")");
			keyDownTimestampMap.put(keyCode, System.currentTimeMillis());
			
			if (keyCode == KeyEvent.KEYCODE_BACK) {
				// force saving position on BACK key press
				scheduleSaveCurrentPositionBookmark(1);
			}
		}
		if ( keyCode==KeyEvent.KEYCODE_POWER || keyCode==KeyEvent.KEYCODE_ENDCALL ) {
			mActivity.releaseBacklightControl();
			return false;
		}

    	if ( keyCode==KeyEvent.KEYCODE_VOLUME_UP || keyCode==KeyEvent.KEYCODE_VOLUME_DOWN ) {
    		if (isAutoScrollActive()) {
    			if (keyCode==KeyEvent.KEYCODE_VOLUME_UP)
    				changeAutoScrollSpeed(1);
    			else
    				changeAutoScrollSpeed(-1);
    			return true;
    		}
    		if (!enableVolumeKeys) {
    			return false;
    		}
    	}
    	
		if (isAutoScrollActive())
			return true; // autoscroll will be stopped in onKeyUp
    	
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
		
/*		if ( keyCode>=KeyEvent.KEYCODE_0 && keyCode<=KeyEvent.KEYCODE_9 ) {
			// will process in keyup handler
			startTrackingKey(event);
			return true;
		}*/
		if ( action.isNone() && longAction.isNone() )
			return false;
		startTrackingKey(event);
		return true;
	}

	public boolean onKeyUp(int keyCode, final KeyEvent event) {
		if (keyCode == 0)
			keyCode = event.getScanCode();
		mActivity.onUserActivity();
		keyCode = translateKeyCode(keyCode);
		if (currentImageViewer != null)
			return currentImageViewer.onKeyUp(keyCode, event);
		if ( keyCode==KeyEvent.KEYCODE_VOLUME_DOWN || keyCode==KeyEvent.KEYCODE_VOLUME_UP ) {
    		if (isAutoScrollActive())
    			return true;
			if ( !enableVolumeKeys )
				return false;
		}
		if (isAutoScrollActive()) {
			stopAutoScroll();
			return true;
		}
		if ( keyCode==KeyEvent.KEYCODE_POWER || keyCode==KeyEvent.KEYCODE_ENDCALL ) {
			mActivity.releaseBacklightControl();
			return false;
		}
		boolean tracked = isTracked(event);
//		if ( keyCode!=KeyEvent.KEYCODE_BACK )
//			backKeyDownHere = false;

		if ( keyCode==KeyEvent.KEYCODE_BACK && !tracked )
			return true;
		//backKeyDownHere = false;
		
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

/*		if ( keyCode>=KeyEvent.KEYCODE_0 && keyCode<=KeyEvent.KEYCODE_9 && tracked ) {
			// goto/set shortcut bookmark
			int shortcut = keyCode - KeyEvent.KEYCODE_0;
			if ( shortcut==0 )
				shortcut = 10;
			if ( isLongPress )
				addBookmark(shortcut);
			else
				goToBookmark(shortcut);
			return true;
		}*/
		if ( action.isNone() || !tracked ) {
			return false;
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
		return false;
	}

	public boolean onTouchEvent(MotionEvent event) {
		
		if ( !isTouchScreenEnabled ) {
			return true;
		}
		if (event.getX()==0 && event.getY()==0)
			return true;
		mActivity.onUserActivity();
		
		if (currentImageViewer != null)
			return currentImageViewer.onTouchEvent(event);
		
		if (isAutoScrollActive()) {
			//if (currentTapHandler != null && currentTapHandler.isInitialState()) {
			if (event.getAction() == MotionEvent.ACTION_DOWN) {
				int x = (int)event.getX();
				int y = (int)event.getY();
				int z = getTapZone(x, y, surface.getWidth(), surface.getHeight());
				if (z == 7)
					changeAutoScrollSpeed(-1);
				else if (z == 9)
					changeAutoScrollSpeed(1);
				else
					stopAutoScroll();
			}
			return true;
		}
		
		if (currentTapHandler == null)
			currentTapHandler = new TapHandler();
		currentTapHandler.checkExpiration();
		return currentTapHandler.onTouchEvent(event);
	}

	@Override
	public void onFocusChange(View arg0, boolean arg1) {
		stopTracking();
		if (currentAutoScrollAnimation != null)
			stopAutoScroll();
	}

	public void redraw() {
		//BackgroundThread.instance().executeBackground(new Runnable() {
		BackgroundThread.instance().executeGUI(new Runnable() {
			@Override
			public void run() {
				surface.invalidate();
				invalidImages = true;
				//preparePageImage(0);
				bookView.draw();
			}
		});
	}

	public ReaderView(CoolReader activity, Engine engine, Properties props) 
    {
        //super(activity);
		log.i("Creating normal SurfaceView");
		surface = new ReaderSurface(activity);

		bookView = (BookView)surface;
		surface.setOnTouchListener(this);
		surface.setOnKeyListener(this);
		surface.setOnFocusChangeListener(this);
        doc = new DocView(engine);
        doc.setReaderCallback(readerCallback);
        SurfaceHolder holder = surface.getHolder();
        holder.addCallback(this);
        
		BackgroundThread.ensureGUI();
        this.mActivity = activity;
        this.mEngine = engine;
        surface.setFocusable(true);
        surface.setFocusableInTouchMode(true);
        
        BackgroundThread.instance().postBackground(new Runnable() {

			@Override
			public void run() {
				log.d("ReaderView - in background thread: calling createInternal()");
				doc.create();
				mInitialized = true;
			}
        	
        });

        log.i("Posting create view task");
        post(new CreateViewTask( props ));

    }
	
}
