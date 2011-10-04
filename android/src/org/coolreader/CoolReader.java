// Main Class
package org.coolreader;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.lang.reflect.Field;

import org.coolreader.crengine.AboutDialog;
import org.coolreader.crengine.BackgroundThread;
import org.coolreader.crengine.BaseDialog;
import org.coolreader.crengine.BookmarksDlg;
import org.coolreader.crengine.CRDB;
import org.coolreader.crengine.DeviceInfo;
import org.coolreader.crengine.Engine;
import org.coolreader.crengine.Engine.HyphDict;
import org.coolreader.crengine.FileBrowser;
import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.History;
import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;
import org.coolreader.crengine.OptionsDialog;
import org.coolreader.crengine.Properties;
import org.coolreader.crengine.ReaderAction;
import org.coolreader.crengine.ReaderView;
import org.coolreader.crengine.Scanner;
import org.coolreader.crengine.TTS;
import org.coolreader.crengine.TTS.OnTTSCreatedListener;
import org.coolreader.crengine.EinkScreen;

import android.app.Activity;
import android.app.SearchManager;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Debug;
import android.os.PowerManager;
import android.text.ClipboardManager;
import android.text.method.DigitsKeyListener;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.Toast;

public class CoolReader extends Activity
{
	public static final Logger log = L.create("cr");
	
	Engine mEngine;
	ReaderView mReaderView;
	Scanner mScanner;
	FileBrowser mBrowser;
	FrameLayout mFrame;
	//View startupView;
	History mHistory;
	CRDB mDB;
	private BackgroundThread mBackgroundThread;
	
	
	public CoolReader() {
	    brightnessHackError = DeviceInfo.SAMSUNG_BUTTONS_HIGHLIGHT_PATCH;
	}
	
	public Scanner getScanner()
	{
		return mScanner;
	}
	
	public History getHistory() 
	{
		return mHistory;
	}
	
	public Engine getEngine() {
		return mEngine;
	}
	
	public FileBrowser getBrowser() {
		return mBrowser;
	}
	
	public ReaderView getReaderView() 
	{
		return mReaderView;
	}
	
	public CRDB getDB()
	{
		return mDB;
	}
	
	private static String PREF_FILE = "CR3LastBook";
	private static String PREF_LAST_BOOK = "LastBook";
	public String getLastSuccessfullyOpenedBook()
	{
		SharedPreferences pref = getSharedPreferences(PREF_FILE, 0);
		String res = pref.getString(PREF_LAST_BOOK, null);
		pref.edit().putString(PREF_LAST_BOOK, null).commit();
		return res;
	}
	
	public void setLastSuccessfullyOpenedBook( String filename )
	{
		SharedPreferences pref = getSharedPreferences(PREF_FILE, 0);
		pref.edit().putString(PREF_LAST_BOOK, filename).commit();
	}
	
	private int mScreenUpdateMode = 0;
	public int getScreenUpdateMode() {
		return mScreenUpdateMode;
	}
	public void setScreenUpdateMode( int screenUpdateMode, View view ) {
		if (mReaderView != null) {
			mScreenUpdateMode = screenUpdateMode;
			if (EinkScreen.UpdateMode != screenUpdateMode || EinkScreen.UpdateMode == 2) {
				EinkScreen.ResetController(screenUpdateMode, view);
			}
		}
	}

	private int mScreenUpdateInterval = 0;
	public int getScreenUpdateInterval() {
		return mScreenUpdateInterval;
	}
	public void setScreenUpdateInterval( int screenUpdateInterval, View view ) {
		mScreenUpdateInterval = screenUpdateInterval;
		if (EinkScreen.UpdateModeInterval != screenUpdateInterval) {
			EinkScreen.UpdateModeInterval = screenUpdateInterval;
			EinkScreen.ResetController(mScreenUpdateMode, view);
		}
	}

	private boolean mNightMode = false;
	public boolean isNightMode() {
		return mNightMode;
	}
	public void setNightMode( boolean nightMode ) {
		mNightMode = nightMode;
	}

	private boolean mFullscreen = false;
	public boolean isFullscreen() {
		return mFullscreen;
	}

	public void applyFullscreen( Window wnd )
	{
		if ( mFullscreen ) {
			//mActivity.getWindow().requestFeature(Window.)
			wnd.setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, 
			        WindowManager.LayoutParams.FLAG_FULLSCREEN );
		} else {
			wnd.setFlags(0, 
			        WindowManager.LayoutParams.FLAG_FULLSCREEN );
		}
	}
	public void setFullscreen( boolean fullscreen )
	{
		if ( mFullscreen!=fullscreen ) {
			mFullscreen = fullscreen;
			applyFullscreen( getWindow() );
		}
	}
	
	private boolean mWakeLockEnabled = false;
	public boolean isWakeLockEnabled() {
		return mWakeLockEnabled;
	}

	public void setWakeLockEnabled( boolean wakeLockEnabled )
	{
		if ( mWakeLockEnabled != wakeLockEnabled ) {
			mWakeLockEnabled = wakeLockEnabled;
			if ( !mWakeLockEnabled )
				backlightControl.release();
			else
				backlightControl.onUserActivity();
		}
	}
	
	int screenOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR;
	public void applyScreenOrientation( Window wnd )
	{
		if ( wnd!=null ) {
			WindowManager.LayoutParams attrs = wnd.getAttributes();
			attrs.screenOrientation = screenOrientation;
			wnd.setAttributes(attrs);
		}
	}

	public int getScreenOrientation()
	{
		switch ( screenOrientation ) {
		case ActivityInfo.SCREEN_ORIENTATION_PORTRAIT:
			return 0;
		case ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE:
			return 1;
		default:
			return orientationFromSensor;
		}
	}

	public boolean isLandscape()
	{
		return screenOrientation == ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
	}

	public void setScreenOrientation( int angle )
	{
		int newOrientation = screenOrientation;
//		{
//			ActivityManager am = (ActivityManager)getSystemService(
//		            Context.ACTIVITY_SERVICE);
			//am.getDeviceConfigurationInfo().

//			WindowManager wm = (WindowManager)getSystemService(
//		            Context.WINDOW_SERVICE);
			
//		}
		//getWindowManager(). //getDefaultDisplay().getMetrics(outMetrics)
		if ( angle==4 )
			newOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR;
		else if ( (angle&1)!=0 )
			newOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
		else
			newOrientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
		if ( newOrientation!=screenOrientation ) {
			screenOrientation = newOrientation;
			setRequestedOrientation(screenOrientation);
			applyScreenOrientation(getWindow());
//			if ( newOrientation == ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE )
//				Surface.setOrientation(Display.DEFAULT_DISPLAY, Surface.ROTATION_270);
//			else if ( newOrientation == ActivityInfo.SCREEN_ORIENTATION_PORTRAIT )
//				Surface.setOrientation(Display.DEFAULT_DISPLAY, Surface.ROTATION_180);
		}
	}

	private Runnable backlightTimerTask = null; 
	private class ScreenBacklightControl
	{
		PowerManager.WakeLock wl = null;
		public ScreenBacklightControl()
		{
		}
		public static final int SCREEN_BACKLIGHT_DURATION_STEPS = 3;
		public static final int SCREEN_BACKLIGHT_TIMER_STEP = 60*1000;
		int backlightCountDown = 0; 
		public void onUserActivity()
		{
			if ( !isWakeLockEnabled() )
				return;
			if ( wl==null ) {
				PowerManager pm = (PowerManager)getSystemService(
			            Context.POWER_SERVICE);
				wl = pm.newWakeLock(
			        PowerManager.SCREEN_BRIGHT_WAKE_LOCK
			        /* | PowerManager.ON_AFTER_RELEASE */,
			        "cr3");
			}
			if ( !isStarted() ) {
			    release();
			    return;
			}
			if ( !wl.isHeld() )
				wl.acquire();
			backlightCountDown = SCREEN_BACKLIGHT_DURATION_STEPS;
			if ( backlightTimerTask==null ) {
				backlightTimerTask = new Runnable() {
					public void run() {
						if ( backlightTimerTask!=this )
							return;
						if ( backlightCountDown<=0 || !isStarted())
							release();
						else {
							backlightCountDown--;
							BackgroundThread.instance().postGUI(backlightTimerTask, SCREEN_BACKLIGHT_TIMER_STEP);
						}
					}
				};
				BackgroundThread.instance().postGUI(backlightTimerTask, SCREEN_BACKLIGHT_TIMER_STEP);
			}
		}
		public boolean isHeld()
		{
			return wl!=null && wl.isHeld();
		}
		public void release()
		{
			if ( wl!=null && wl.isHeld() )
				wl.release();
			backlightTimerTask = null;
		}
	}
	ScreenBacklightControl backlightControl = new ScreenBacklightControl();
	
	public int getPalmTipPixels()
	{
		return densityDpi / 3; // 1/3"
	}
	
	private int densityDpi = 120;
	int initialBatteryState = -1;
	String fileToLoadOnStart = null;
	BroadcastReceiver intentReceiver;
	
	private String mVersion = "3.0";
	
	public String getVersion() {
		return mVersion;
	}
	
	TTS tts;
	boolean ttsInitialized;
	boolean ttsError;
	
	public boolean initTTS(final OnTTSCreatedListener listener) {
		if ( ttsError || !TTS.isFound() ) {
			if ( !ttsError ) {
				ttsError = true;
				showToast("TTS is not available");
			}
			return false;
		}
		if ( ttsInitialized && tts!=null ) {
			BackgroundThread.instance().executeGUI(new Runnable() {
				@Override
				public void run() {
					listener.onCreated(tts);
				}
			});
			return true;
		}
		if ( ttsInitialized && tts!=null ) {
			showToast("TTS initialization is already called");
			return false;
		}
		showToast("Initializing TTS");
    	tts = new TTS(this, new TTS.OnInitListener() {
			@Override
			public void onInit(int status) {
				//tts.shutdown();
				L.i("TTS init status: " + status);
				if ( status==TTS.SUCCESS ) {
					ttsInitialized = true;
					BackgroundThread.instance().executeGUI(new Runnable() {
						@Override
						public void run() {
							listener.onCreated(tts);
						}
					});
				} else {
					ttsError = true;
					BackgroundThread.instance().executeGUI(new Runnable() {
						@Override
						public void run() {
							showToast("Cannot initialize TTS");
						}
					});
				}
			}
		});
		return true;
	}
	
	private AudioManager am;
	private int maxVolume;
	public AudioManager getAudioManager() {
		if ( am==null ) {
			am = (AudioManager)getSystemService(AUDIO_SERVICE);
			maxVolume = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
		}
		return am;
	}
	
	public int getVolume() {
		AudioManager am = getAudioManager();
		if (am!=null) {
			return am.getStreamVolume(AudioManager.STREAM_MUSIC) * 100 / maxVolume;
		}
		return 0;
	}
	
	public void setVolume( int volume ) {
		AudioManager am = getAudioManager();
		if (am!=null) {
			am.setStreamVolume(AudioManager.STREAM_MUSIC, volume * maxVolume / 100, 0);
		}
	}
	
	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
    	
    	
		log.i("CoolReader.onCreate() entered");
		super.onCreate(savedInstanceState);

		setVolumeControlStream(AudioManager.STREAM_MUSIC);
		
		try {
			PackageInfo pi = getPackageManager().getPackageInfo(getPackageName(), 0);
			mVersion = pi.versionName;
		} catch ( NameNotFoundException e ) {
			// ignore
		}
		log.i("CoolReader version : " + getVersion());
		
		Display d = getWindowManager().getDefaultDisplay();
		DisplayMetrics m = new DisplayMetrics(); 
		d.getMetrics(m);
		try {
			Field fld = d.getClass().getField("densityDpi");
			if ( fld!=null ) {
				Object v = fld.get(m);
				if ( v!=null && v instanceof Integer ) {
					densityDpi = ((Integer)v).intValue();
					log.i("Screen density detected: " + densityDpi + "DPI");
				}
			}
		} catch ( Exception e ) {
			log.e("Cannot find field densityDpi, using default value");
		}
		
		intentReceiver = new BroadcastReceiver() {

			@Override
			public void onReceive(Context context, Intent intent) {
				int level = intent.getIntExtra("level", 0);
				if ( mReaderView!=null )
					mReaderView.setBatteryState(level);
				else
					initialBatteryState = level;
			}
			
		};
		registerReceiver(intentReceiver, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));


		log.i("CoolReader.window=" + getWindow());
		WindowManager.LayoutParams lp = new WindowManager.LayoutParams();
		lp.alpha = 1.0f;
		lp.dimAmount = 0.0f;
		lp.format = PixelFormat.RGB_565;
		lp.gravity = Gravity.CENTER_HORIZONTAL | Gravity.CENTER_VERTICAL;
		lp.horizontalMargin = 0;
		lp.verticalMargin = 0;
		lp.windowAnimations = 0;
		lp.layoutAnimationParameters = null;
		lp.memoryType = WindowManager.LayoutParams.MEMORY_TYPE_NORMAL;
		getWindow().setAttributes(lp);
		
		// testing background thread
    	mBackgroundThread = BackgroundThread.instance();
		mFrame = new FrameLayout(this);
		log.i("initializing scanner");
		mEngine = new Engine(this, mBackgroundThread);
		mBackgroundThread.setGUI(mFrame);
		requestWindowFeature(Window.FEATURE_NO_TITLE);

		// load settings
		Properties props = loadSettings();
		
		setFullscreen( props.getBool(ReaderView.PROP_APP_FULLSCREEN, (DeviceInfo.EINK_SCREEN?true:false)));
		int orientation = props.getInt(ReaderView.PROP_APP_SCREEN_ORIENTATION, (DeviceInfo.EINK_SCREEN?0:4));
		if ( orientation!=1 && orientation!=4 )
			orientation = 0;
		setScreenOrientation(orientation);
		int backlight = props.getInt(ReaderView.PROP_APP_SCREEN_BACKLIGHT, -1);
		if ( backlight<-1 || backlight>100 )
			backlight = -1;
		setScreenBacklightLevel(backlight);
		
        mEngine.showProgress( 0, R.string.progress_starting_cool_reader );

        // wait until all background tasks are executed
        mBackgroundThread.syncWithBackground();
        
		mEngine.setHyphenationDictionary(HyphDict.byCode(props.getProperty(ReaderView.PROP_HYPHENATION_DICT, Engine.HyphDict.RUSSIAN.toString())));
		
		//this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, 
        //       WindowManager.LayoutParams.FLAG_FULLSCREEN );
//		startupView = new View(this) {
//		};
//		startupView.setBackgroundColor(Color.BLACK);
		setWakeLockEnabled(props.getBool(ReaderView.PROP_APP_SCREEN_BACKLIGHT_LOCK, false));

		// open DB
		final String SQLITE_DB_NAME = "cr3db.sqlite";
		File dbdir = getDir("db", Context.MODE_PRIVATE);
		dbdir.mkdirs();
		File dbfile = new File(dbdir, SQLITE_DB_NAME);
		File externalDir = Engine.getExternalSettingsDir();
		if ( externalDir!=null ) {
			dbfile = Engine.checkOrMoveFile(externalDir, dbdir, SQLITE_DB_NAME);
		}
		mDB = new CRDB(dbfile);

       	mScanner = new Scanner(this, mDB, mEngine);
       	mScanner.initRoots(mEngine.getMountedRootsMap());
		
       	mHistory = new History(this, mDB);
		mHistory.setCoverPagesEnabled(props.getBool(ReaderView.PROP_APP_SHOW_COVERPAGES, true));

//		if ( DeviceInfo.FORCE_LIGHT_THEME ) {
//			setTheme(android.R.style.Theme_Light);
//			getWindow().setBackgroundDrawableResource(drawable.editbox_background);
//		}
		if ( DeviceInfo.FORCE_LIGHT_THEME ) {
			mFrame.setBackgroundColor( Color.WHITE );
			setTheme(R.style.Dialog_Fullscreen_Day);
		}
		
		mReaderView = new ReaderView(this, mEngine, mBackgroundThread, props);

		mScanner.setDirScanEnabled(props.getBool(ReaderView.PROP_APP_BOOK_PROPERTY_SCAN_ENABLED, true));
		
		mBrowser = new FileBrowser(this, mEngine, mScanner, mHistory);

		
		mFrame.addView(mReaderView);
		mFrame.addView(mBrowser);
//		mFrame.addView(startupView);
		setContentView( mFrame );
        log.i("initializing browser");
        mBrowser.init();
		showView(mBrowser, false);
        log.i("initializing reader");
        mBrowser.setSortOrder( props.getProperty(ReaderView.PROP_APP_BOOK_SORT_ORDER));
		mBrowser.setSimpleViewMode(props.getBool(ReaderView.PROP_APP_FILE_BROWSER_SIMPLE_MODE, false));
        mBrowser.showDirectory(mScanner.getRoot(), null);
        
        fileToLoadOnStart = null;
		Intent intent = getIntent();
		if ( intent!=null && Intent.ACTION_VIEW.equals(intent.getAction()) ) {
			Uri uri = intent.getData();
			if ( uri!=null ) {
				fileToLoadOnStart = extractFileName(uri);
			}
			intent.setData(null);
		}
		if ( initialBatteryState>=0 )
			mReaderView.setBatteryState(initialBatteryState);
        
        log.i("CoolReader.onCreate() exiting");
    }
    
    public ClipboardManager getClipboardmanager() {
    	return (ClipboardManager)getSystemService(CLIPBOARD_SERVICE);
    }
    
    public void setScreenBacklightLevel( int percent )
    {
    	if ( percent<-1 )
    		percent = -1;
    	else if ( percent>100 )
    		percent = -1;
    	screenBacklightBrightness = percent;
    	onUserActivity();
    }
    
    private int screenBacklightBrightness = -1; // use default
    //private boolean brightnessHackError = false;
    private boolean brightnessHackError = false;
    	    
    public void onUserActivity()
    {
    	if ( backlightControl==null )
    		return;
    	backlightControl.onUserActivity();
    	// Hack
    	//if ( backlightControl.isHeld() )
    	BackgroundThread.guiExecutor.execute(new Runnable() {
			@Override
			public void run() {
				try {
			        Window wnd = getWindow();
			        if ( wnd!=null ) {
			        	LayoutParams attrs =  wnd.getAttributes();
			        	boolean changed = false;
			        	float b;
			        	int dimmingAlpha = 255;
			        	if ( screenBacklightBrightness>=0 ) {
		        			float minb = 1/16f; 
			        		if ( screenBacklightBrightness >= 10 ) {
			        			b = (screenBacklightBrightness - 10) / 90.0f;
			        			b = minb + b * (1-minb);
				        		//b = (screenBacklightBrightness - 10) * 10.0f / 9.0f / 95.0f + 0.5f;
					        	if ( b<0.0f ) // BRIGHTNESS_OVERRIDE_OFF
					        		b = 0.0f;
					        	else if ( b>1.0f )
					        		b = 1.0f; //BRIGHTNESS_OVERRIDE_FULL
			        		} else {
				        		b = minb;
				        		dimmingAlpha = 255 - (11-screenBacklightBrightness) * 255 / 10; 
			        		}
			        	} else
			        		b = -1.0f; //BRIGHTNESS_OVERRIDE_NONE
			        	mReaderView.setDimmingAlpha(dimmingAlpha);
			        	log.d("Brightness: " + b + ", dim: " + dimmingAlpha);
			        	if ( attrs.screenBrightness != b ) {
			        		attrs.screenBrightness = b;
			        		changed = true;
			        	}
			        	// hack to set buttonBrightness field
			        	if ( !brightnessHackError )
			        	try {
				        	Field bb = attrs.getClass().getField("buttonBrightness");
				        	if ( bb!=null ) {
				        		Float oldValue = (Float)bb.get(attrs);
				        		//if ( oldValue==null || oldValue.floatValue()!=0 ) {
				        			bb.set(attrs, Float.valueOf(0.0f));
					        		changed = true;
				        		//}
				        	}
			        	} catch ( Exception e ) {
			        		log.e("WindowManager.LayoutParams.buttonBrightness field is not found, cannot turn buttons backlight off");
			        		brightnessHackError = true;
			        	}
			        	//attrs.buttonBrightness = 0;
			        	if ( changed ) {
			        		log.d("Window attribute changed: " + attrs);
			        		wnd.setAttributes(attrs);
			        	}
			        	//attrs.screenOrientation = LayoutParams.SCREEN_;
			        }
				} catch ( Exception e ) {
					// ignore
				}
			}
    	});
    }
    
    boolean mDestroyed = false;
	@Override
	protected void onDestroy() {

		log.i("CoolReader.onDestroy() entered");
		mDestroyed = true;
		if ( !CLOSE_BOOK_ON_STOP )
			mReaderView.close();
		
		//if ( mReaderView!=null )
		//	mReaderView.close();
		
		//if ( mHistory!=null && mDB!=null ) {
			//history.saveToDB();
		//}
		if ( intentReceiver!=null ) {
			unregisterReceiver(intentReceiver);
			intentReceiver = null;
		}

		if ( mReaderView!=null ) {
			mReaderView.destroy();
		}
		
		if ( tts!=null ) {
			tts.shutdown();
			tts = null;
			ttsInitialized = false;
			ttsError = false;
		}
		
		if ( mEngine!=null ) {
			//mEngine.uninit();
		}

		if ( mDB!=null ) {
			final CRDB db = mDB;
			mBackgroundThread.executeBackground(new Runnable() {
				public void run() {
					db.close();
				}
			});
		}
//		if ( mBackgroundThread!=null ) {
//			mBackgroundThread.quit();
//		}
			
		mDB = null;
		mReaderView = null;
		mEngine = null;
		mBackgroundThread = null;
		log.i("CoolReader.onDestroy() exiting");
		super.onDestroy();
	}

	private String extractFileName( Uri uri )
	{
		if ( uri!=null ) {
			if ( uri.equals(Uri.parse("file:///")) )
				return null;
			else
				return uri.getPath();
		}
		return null;
	}

	public void showHomeScreen() {
		Intent intent = new Intent(Intent.ACTION_MAIN);
		intent.addCategory(Intent.CATEGORY_HOME);
		startActivity(intent);
	}
	
	@Override
	protected void onNewIntent(Intent intent) {
		log.i("onNewIntent : " + intent);
		if ( mDestroyed ) {
			log.e("engine is already destroyed");
			return;
		}
		String fileToOpen = null;
		if ( Intent.ACTION_VIEW.equals(intent.getAction()) ) {
			Uri uri = intent.getData();
			if ( uri!=null ) {
				fileToOpen = extractFileName(uri);
			}
			intent.setData(null);
		}
		log.v("onNewIntent, fileToOpen=" + fileToOpen);
		if ( fileToOpen!=null ) {
			// load document
			final String fn = fileToOpen;
			mReaderView.loadDocument(fileToOpen, new Runnable() {
				public void run() {
					log.v("onNewIntent, loadDocument error handler called");
					showToast("Error occured while loading " + fn);
					mEngine.hideProgress();
				}
			});
		}
	}

	private boolean mPaused = false; 
	public boolean isPaused() {
		return mPaused;
	}
	
	@Override
	protected void onPause() {
		log.i("CoolReader.onPause() : saving reader state");
		mIsStarted = false;
		mPaused = true;
//		setScreenUpdateMode(-1, mReaderView);
		releaseBacklightControl();
		mReaderView.saveCurrentPositionBookmarkSync(true);
		super.onPause();
	}
	
	public void releaseBacklightControl()
	{
		backlightControl.release();
	}

	@Override
	protected void onPostCreate(Bundle savedInstanceState) {
		log.i("CoolReader.onPostCreate()");
		super.onPostCreate(savedInstanceState);
	}

	@Override
	protected void onPostResume() {
		log.i("CoolReader.onPostResume()");
		super.onPostResume();
	}

	private boolean restarted = false;
	@Override
	protected void onRestart() {
		log.i("CoolReader.onRestart()");
		restarted = true;
		super.onRestart();
	}

	@Override
	protected void onRestoreInstanceState(Bundle savedInstanceState) {
		log.i("CoolReader.onRestoreInstanceState()");
		super.onRestoreInstanceState(savedInstanceState);
	}

	@Override
	protected void onResume() {
		log.i("CoolReader.onResume()");
		mPaused = false;
		mIsStarted = true;
		Properties props = mReaderView.getSettings();
		
		if (DeviceInfo.EINK_SCREEN) {
			setScreenUpdateMode(props.getInt(ReaderView.PROP_APP_SCREEN_UPDATE_MODE, 0), mReaderView);
		}
		
		backlightControl.onUserActivity();
		super.onResume();
	}

	@Override
	protected void onSaveInstanceState(Bundle outState) {
		log.i("CoolReader.onSaveInstanceState()");
		super.onSaveInstanceState(outState);
	}

	static final boolean LOAD_LAST_DOCUMENT_ON_START = true; 
	
	private boolean mIsStarted = false;
	
	public boolean isStarted() { return mIsStarted; }
	
	@Override
	protected void onStart() {
		log.i("CoolReader.onStart() fileToLoadOnStart=" + fileToLoadOnStart);
		super.onStart();
		
		mPaused = false;
		
		backlightControl.onUserActivity();
		

		if ( fileToLoadOnStart==null ) {
			if ( mReaderView!=null && currentView==mReaderView && mReaderView.isBookLoaded() ) {
				log.v("Book is already opened, showing ReaderView");
				showReader();
				return;
			}
			
			//!stopped && 
//			if ( restarted && mReaderView!=null && mReaderView.isBookLoaded() ) {
//				log.v("Book is already opened, showing ReaderView");
//		        restarted = false;
//		        return;
//			}
		}
		if ( !stopped ) {
	        mEngine.showProgress( 500, R.string.progress_starting_cool_reader );
			//mEngine.setHyphenationDictionary( HyphDict.RUSSIAN );
		}
        //log.i("waiting for engine tasks completion");
        //engine.waitTasksCompletion();
		restarted = false;
		stopped = false;
		final String fileName = fileToLoadOnStart;
		mBackgroundThread.postGUI(new Runnable() {
			public void run() {
		        log.i("onStart, scheduled runnable: submitting task");
		        mEngine.execute(new LoadLastDocumentTask(fileName));
			}
		});
		log.i("CoolReader.onStart() exiting");
	}
	
	class LoadLastDocumentTask implements Engine.EngineTask {

		final String fileName;
		public LoadLastDocumentTask( String fileName ) {
			super();
			this.fileName = fileName;
		}
		
		public void done() {
	        log.i("onStart, scheduled task: trying to load " + fileToLoadOnStart);
			if ( fileName!=null || LOAD_LAST_DOCUMENT_ON_START ) {
				//currentView=mReaderView;
				if ( fileName!=null ) {
					log.v("onStart() : loading " + fileName);
					mReaderView.loadDocument(fileName, new Runnable() {
						public void run() {
							// cannot open recent book: load another one
							log.e("Cannot open document " + fileToLoadOnStart + " starting file browser");
							showBrowser(null);
						}
					});
				} else {
					log.v("onStart() : loading last document");
					mReaderView.loadLastDocument(new Runnable() {
						public void run() {
							// cannot open recent book: load another one
							log.e("Cannot open last document, starting file browser");
							showBrowser(null);
						}
					});
				}
			} else {
				showBrowser(null);
			}
			fileToLoadOnStart = null;
		}

		public void fail(Exception e) {
	        log.e("onStart, scheduled task failed", e);
		}

		public void work() throws Exception {
	        log.v("onStart, scheduled task work()");
		}
    }
 

	public final static boolean CLOSE_BOOK_ON_STOP = false;
	private boolean stopped = false;
	@Override
	protected void onStop() {
		log.i("CoolReader.onStop() entering");
		stopped = true;
		mPaused = false;
		// will close book at onDestroy()
		if ( CLOSE_BOOK_ON_STOP )
			mReaderView.close();
		super.onStop();
		log.i("CoolReader.onStop() exiting");
	}

	private View currentView;
	public void showView( View view )
	{
		showView( view, true );
	}
	public void showView( View view, boolean hideProgress )
	{
		if ( mBackgroundThread==null )
			return;
		if ( hideProgress )
		mBackgroundThread.postGUI(new Runnable() {
			public void run() {
				mEngine.hideProgress();
			}
		});
		if ( currentView==view ) {
			log.v("showView : view " + view.getClass().getSimpleName() + " is already shown");
			return;
		}
		log.v("showView : showing view " + view.getClass().getSimpleName());
		mFrame.bringChildToFront(view);
		for ( int i=0; i<mFrame.getChildCount(); i++ ) {
			View v = mFrame.getChildAt(i);
			v.setVisibility(view==v?View.VISIBLE:View.INVISIBLE);
		}
		currentView = view;
	}
	
	public void showReader()
	{
		log.v("showReader() is called");
		showView(mReaderView);
	}
	
	public boolean isBookOpened()
	{
		return mReaderView.isBookLoaded();
	}
	
	public void loadDocument( FileInfo item )
	{
		//showView(readerView);
		//setContentView(readerView);
		mReaderView.loadDocument(item);
	}
	
	public void showBrowser( final FileInfo fileToShow )
	{
		log.v("showBrowser() is called");
		if ( currentView == mReaderView )
			mReaderView.save();
		mEngine.runInGUI( new Runnable() {
			public void run() {
				showView(mBrowser);
		        if (fileToShow==null || mBrowser.isBookShownInRecentList(fileToShow))
		        	mBrowser.showLastDirectory();
		        else
		        	mBrowser.showDirectory(fileToShow, fileToShow);
			}
		});
	}

	public void showBrowserRecentBooks()
	{
		log.v("showBrowserRecentBooks() is called");
		if ( currentView == mReaderView )
			mReaderView.save();
		mEngine.runInGUI( new Runnable() {
			public void run() {
				showView(mBrowser);
	        	mBrowser.showRecentBooks();
			}
		});
	}

	public void showBrowserRoot()
	{
		log.v("showBrowserRoot() is called");
		if ( currentView == mReaderView )
			mReaderView.save();
		mEngine.runInGUI( new Runnable() {
			public void run() {
				showView(mBrowser);
	        	mBrowser.showRootDirectory();
			}
		});
	}

	private void fillMenu(Menu menu) {
		menu.clear();
	    MenuInflater inflater = getMenuInflater();
	    if ( currentView==mReaderView ) {
	    	inflater.inflate(R.menu.cr3_reader_menu, menu);
	    	MenuItem item = menu.findItem(R.id.cr3_mi_toggle_document_styles);
	    	if ( item!=null )
	    		item.setTitle(mReaderView.getDocumentStylesEnabled() ? R.string.mi_book_styles_disable : R.string.mi_book_styles_enable);
	    	item = menu.findItem(R.id.cr3_mi_toggle_day_night);
	    	if ( item!=null )
	    		item.setTitle(mReaderView.isNightMode() ? R.string.mi_night_mode_disable : R.string.mi_night_mode_enable);
	    } else {
	    	inflater.inflate(R.menu.cr3_browser_menu, menu);
	    	if ( !isBookOpened() ) {
	    		MenuItem item = menu.findItem(R.id.book_back_to_reading);
	    		if ( item!=null )
	    			item.setEnabled(false);
	    	}
    		MenuItem item = menu.findItem(R.id.book_toggle_simple_mode);
    		if ( item!=null )
    			item.setTitle(mBrowser.isSimpleViewMode() ? R.string.mi_book_browser_normal_mode : R.string.mi_book_browser_simple_mode );
	    }
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		fillMenu(menu);
	    return true;
	}

	@Override
	public boolean onPrepareOptionsMenu(Menu menu) {
		fillMenu(menu);
	    return true;
	}

	public void showToast( int stringResourceId )
	{
		String s = getString(stringResourceId);
		if ( s!=null )
			showToast(s);
	}

	public void showToast( String msg )
	{
		log.v("showing toast: " + msg);
		Toast toast = Toast.makeText(this, msg, Toast.LENGTH_LONG);
		toast.show();
	}

	public interface InputHandler {
		boolean validate( String s ) throws Exception;
		void onOk( String s ) throws Exception;
		void onCancel();
	};
	
	public static class InputDialog extends BaseDialog {
		private InputHandler handler;
		private EditText input;
		public InputDialog( CoolReader activity, final String title, boolean isNumberEdit, final InputHandler handler )
		{
			super(activity, R.string.dlg_button_ok, R.string.dlg_button_cancel, true);
			this.handler = handler;
			setTitle(title);
	        input = new EditText(getContext());
	        if ( isNumberEdit )
	        	input.setKeyListener(DigitsKeyListener.getInstance("0123456789."));
//		        input.getText().setFilters(new InputFilter[] {
//		        	new DigitsKeyListener()        
//		        });
	        setView(input);
		}
		@Override
		protected void onNegativeButtonClick() {
            cancel();
            handler.onCancel();
		}
		@Override
		protected void onPositiveButtonClick() {
            String value = input.getText().toString().trim();
            try {
            	if ( handler.validate(value) )
            		handler.onOk(value);
            	else
            		handler.onCancel();
            } catch ( Exception e ) {
            	handler.onCancel();
            }
            cancel();
		}
	}
	
	public void showInputDialog( final String title, boolean isNumberEdit, final InputHandler handler )
	{
        final InputDialog dlg = new InputDialog(this, title, isNumberEdit, handler);
        dlg.show();
	}

	private int orientationFromSensor = 0;
	public int getOrientationFromSensor()
	{
		return orientationFromSensor;
	}
	
	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		// pass
		orientationFromSensor = newConfig.orientation==Configuration.ORIENTATION_LANDSCAPE ? 1 : 0;
		//final int orientation = newConfig.orientation==Configuration.ORIENTATION_LANDSCAPE ? ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE : ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
//		if ( orientation!=screenOrientation ) {
//			log.d("Screen orientation has been changed: ask for change");
//			AlertDialog.Builder dlg = new AlertDialog.Builder(this);
//			dlg.setTitle(R.string.win_title_screen_orientation_change_apply);//R.string.win_title_options_apply);
//			dlg.setPositiveButton(R.string.dlg_button_ok, new OnClickListener() {
//				public void onClick(DialogInterface arg0, int arg1) {
//					//onPositiveButtonClick();
//					Properties oldSettings = mReaderView.getSettings();
//					Properties newSettings = new Properties(oldSettings);
//					newSettings.setInt(ReaderView.PROP_APP_SCREEN_ORIENTATION, orientation==ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE ? 1 : 0);
//					mReaderView.setSettings(newSettings, oldSettings);
//				}
//			});
//			dlg.setNegativeButton(R.string.dlg_button_cancel, new OnClickListener() {
//				public void onClick(DialogInterface arg0, int arg1) {
//					//onNegativeButtonClick();
//				}
//			});
//		}
		super.onConfigurationChanged(newConfig);
	}

	String[] mFontFaces;

	public void showOptionsDialog()
	{
		final CoolReader _this = this;
		mBackgroundThread.executeBackground(new Runnable() {
			public void run() {
				mFontFaces = mEngine.getFontFaceList();
				mBackgroundThread.executeGUI(new Runnable() {
					public void run() {
						OptionsDialog dlg = new OptionsDialog(_this, mReaderView, mFontFaces);
						dlg.show();
					}
				});
			}
		});
	}
	
	public void saveSetting( String name, String value ) {
		mReaderView.saveSetting(name, value);
	}
	public String getSetting( String name ) {
		return mReaderView.getSetting(name);
	}
	
	public void showBookmarksDialog()
	{
		BackgroundThread.instance().executeGUI(new Runnable() {
			@Override
			public void run() {
				BookmarksDlg dlg = new BookmarksDlg(CoolReader.this, mReaderView);
				dlg.show();
			}
		});
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		int itemId = item.getItemId();
		if ( mReaderView.onMenuItem(itemId))
			return true; // processed by ReaderView
		// other commands
		switch ( itemId ) {
		case R.id.book_toggle_simple_mode:
			mBrowser.setSimpleViewMode(!mBrowser.isSimpleViewMode());
			mReaderView.saveSetting(ReaderView.PROP_APP_FILE_BROWSER_SIMPLE_MODE, mBrowser.isSimpleViewMode()?"1":"0");
			return true;
		case R.id.book_sort_order:
			mBrowser.showSortOrderMenu();
			return true;
		case R.id.book_root:
			mBrowser.showRootDirectory();
			return true;
		case R.id.book_opds_root:
			mBrowser.showOPDSRootDirectory();
			return true;
		case R.id.book_recent_books:
			mBrowser.showRecentBooks();
			return true;
		case R.id.book_find:
			mBrowser.showFindBookDialog();
			return true;
		case R.id.book_scan_recursive:
			mBrowser.scanCurrentDirectoryRecursive();
			return true;
		case R.id.book_back_to_reading:
			if ( isBookOpened() )
				showReader();
			else
				showToast("No book opened");
			return true;
		default:
			return false;
			//return super.onOptionsItemSelected(item);
		}
	}
	
	public void showGoToPageDialog() {
		showInputDialog("Enter page number", true, new InputHandler() {
			int pageNumber = 0;
			public boolean validate(String s) {
				pageNumber = Integer.valueOf(s); 
				return pageNumber>0;
			}
			public void onOk(String s) {
				mReaderView.goToPage(pageNumber);
			}
			public void onCancel() {
			}
		});
	}
	public void showGoToPercentDialog() {
		showInputDialog("Enter position %", true, new InputHandler() {
			int percent = 0;
			public boolean validate(String s) {
				percent = Integer.valueOf(s); 
				return percent>=0 && percent<=100;
			}
			public void onOk(String s) {
				mReaderView.goToPercent(percent);
			}
			public void onCancel() {
			}
		});
	}

	private static class DefKeyAction {
		public int keyCode;
		public int type;
		public ReaderAction action;
		public DefKeyAction(int keyCode, int type, ReaderAction action) {
			this.keyCode = keyCode;
			this.type = type;
			this.action = action;
		}
		public String getProp() {
			return ReaderView.PROP_APP_KEY_ACTIONS_PRESS + ReaderAction.getTypeString(type) + keyCode;			
		}
	}
	private static class DefTapAction {
		public int zone;
		public boolean longPress;
		public ReaderAction action;
		public DefTapAction(int zone, boolean longPress, ReaderAction action) {
			this.zone = zone;
			this.longPress = longPress;
			this.action = action;
		}
	}
	private static DefKeyAction[] DEF_KEY_ACTIONS = {
		new DefKeyAction(KeyEvent.KEYCODE_BACK, ReaderAction.NORMAL, ReaderAction.FILE_BROWSER),
		new DefKeyAction(KeyEvent.KEYCODE_BACK, ReaderAction.LONG, ReaderAction.EXIT),
		new DefKeyAction(KeyEvent.KEYCODE_BACK, ReaderAction.DOUBLE, ReaderAction.EXIT),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_CENTER, ReaderAction.NORMAL, ReaderAction.RECENT_BOOKS),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_CENTER, ReaderAction.LONG, ReaderAction.BOOKMARKS),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_UP, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_DOWN, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_UP, ReaderAction.LONG, ReaderAction.REPEAT),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_DOWN, ReaderAction.LONG, ReaderAction.REPEAT),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_LEFT, ReaderAction.NORMAL, ReaderAction.PAGE_UP_10),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_RIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN_10),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_LEFT, ReaderAction.LONG, ReaderAction.REPEAT),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_RIGHT, ReaderAction.LONG, ReaderAction.REPEAT),
		new DefKeyAction(KeyEvent.KEYCODE_VOLUME_UP, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(KeyEvent.KEYCODE_VOLUME_DOWN, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(KeyEvent.KEYCODE_VOLUME_UP, ReaderAction.LONG, ReaderAction.REPEAT),
		new DefKeyAction(KeyEvent.KEYCODE_VOLUME_DOWN, ReaderAction.LONG, ReaderAction.REPEAT),
		new DefKeyAction(KeyEvent.KEYCODE_MENU, ReaderAction.NORMAL, ReaderAction.READER_MENU),
		new DefKeyAction(KeyEvent.KEYCODE_MENU, ReaderAction.LONG, ReaderAction.OPTIONS),
		new DefKeyAction(KeyEvent.KEYCODE_CAMERA, ReaderAction.NORMAL, ReaderAction.NONE),
		new DefKeyAction(KeyEvent.KEYCODE_CAMERA, ReaderAction.LONG, ReaderAction.NONE),
		new DefKeyAction(KeyEvent.KEYCODE_SEARCH, ReaderAction.NORMAL, ReaderAction.SEARCH),
		new DefKeyAction(KeyEvent.KEYCODE_SEARCH, ReaderAction.LONG, ReaderAction.TOGGLE_SELECTION_MODE),
		new DefKeyAction(ReaderView.NOOK_KEY_NEXT_LEFT, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(ReaderView.NOOK_KEY_NEXT_RIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(ReaderView.NOOK_KEY_SHIFT_DOWN, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(ReaderView.NOOK_KEY_PREV_LEFT, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(ReaderView.NOOK_KEY_PREV_RIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(ReaderView.NOOK_KEY_SHIFT_UP, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_BOTTOMLEFT, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_BOTTOMRIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_TOPLEFT, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_TOPRIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_BOTTOMLEFT, ReaderAction.LONG, ReaderAction.PAGE_UP_10),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_BOTTOMRIGHT, ReaderAction.LONG, ReaderAction.PAGE_UP_10),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_TOPLEFT, ReaderAction.LONG, ReaderAction.PAGE_DOWN_10),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_TOPRIGHT, ReaderAction.LONG, ReaderAction.PAGE_DOWN_10),
//	    public static final int KEYCODE_PAGE_BOTTOMLEFT = 0x5d; // fwd
//	    public static final int KEYCODE_PAGE_BOTTOMRIGHT = 0x5f; // fwd
//	    public static final int KEYCODE_PAGE_TOPLEFT = 0x5c; // back
//	    public static final int KEYCODE_PAGE_TOPRIGHT = 0x5e; // back
		
	};
	private static DefTapAction[] DEF_TAP_ACTIONS = {
		new DefTapAction(1, false, ReaderAction.PAGE_UP),
		new DefTapAction(2, false, ReaderAction.PAGE_UP),
		new DefTapAction(4, false, ReaderAction.PAGE_UP),
		new DefTapAction(1, true, ReaderAction.GO_BACK), // back by link
		new DefTapAction(2, true, ReaderAction.TOGGLE_DAY_NIGHT),
		new DefTapAction(4, true, ReaderAction.PAGE_UP_10),
		new DefTapAction(3, false, ReaderAction.PAGE_DOWN),
		new DefTapAction(6, false, ReaderAction.PAGE_DOWN),
		new DefTapAction(7, false, ReaderAction.PAGE_DOWN),
		new DefTapAction(8, false, ReaderAction.PAGE_DOWN),
		new DefTapAction(9, false, ReaderAction.PAGE_DOWN),
		new DefTapAction(3, true, ReaderAction.PAGE_DOWN_10),
		new DefTapAction(6, true, ReaderAction.PAGE_DOWN_10),
		new DefTapAction(7, true, ReaderAction.PAGE_DOWN_10),
		new DefTapAction(8, true, ReaderAction.PAGE_DOWN_10),
		new DefTapAction(9, true, ReaderAction.PAGE_DOWN_10),
		new DefTapAction(5, false, ReaderAction.READER_MENU),
		new DefTapAction(5, true, ReaderAction.OPTIONS),
	};
	File propsFile;
	private static final String SETTINGS_FILE_NAME = "cr3.ini";
	private static boolean DEBUG_RESET_OPTIONS = false;
	private Properties loadSettings()
	{
        Properties props = new Properties();

		File[] dataDirs = mEngine.getDataDirectories(null, false, true);
		File existingFile = null;
		for ( File dir : dataDirs ) {
			File f = new File(dir, SETTINGS_FILE_NAME);
			if ( f.exists() && f.isFile() ) {
				existingFile = f;
				break;
			}
		}
        if ( existingFile!=null )
        	propsFile = existingFile;
        else {
	        File propsDir = getDir("settings", Context.MODE_PRIVATE);
			propsFile = new File( propsDir, SETTINGS_FILE_NAME);
			File dataDir = Engine.getExternalSettingsDir();
			if ( dataDir!=null ) {
				log.d("external settings dir: " + dataDir);
				propsFile = Engine.checkOrMoveFile(dataDir, propsDir, SETTINGS_FILE_NAME);
			} else {
				propsDir.mkdirs();
			}
        }
        if ( propsFile.exists() && !DEBUG_RESET_OPTIONS ) {
        	try {
        		FileInputStream is = new FileInputStream(propsFile);
        		props.load(is);
        		log.v("" + props.size() + " settings items loaded from file " + propsFile.getAbsolutePath() );
        	} catch ( Exception e ) {
        		log.e("error while reading settings");
        	}
        }
        
        // default key actions
        for ( DefKeyAction ka : DEF_KEY_ACTIONS ) {
        		props.applyDefault(ka.getProp(), ka.action.id);
        }
        // default tap zone actions
        for ( DefTapAction ka : DEF_TAP_ACTIONS ) {
        	if ( ka.longPress )
        		props.applyDefault(ReaderView.PROP_APP_TAP_ZONE_ACTIONS_TAP + ".long." + ka.zone, ka.action.id);
        	else
        		props.applyDefault(ReaderView.PROP_APP_TAP_ZONE_ACTIONS_TAP + "." + ka.zone, ka.action.id);
        }
        
        if ( DeviceInfo.EINK_SCREEN ) {
    		props.applyDefault(ReaderView.PROP_PAGE_ANIMATION, ReaderView.PAGE_ANIMATION_NONE);
        } else {
    		props.applyDefault(ReaderView.PROP_PAGE_ANIMATION, ReaderView.PAGE_ANIMATION_SLIDE2);
        }
        
        props.applyDefault(ReaderView.PROP_APP_SCREEN_BACKLIGHT_LOCK, "0");
        props.applyDefault(ReaderView.PROP_APP_BOOK_PROPERTY_SCAN_ENABLED, "1");
        // autodetect best initial font size based on display resolution
        int screenWidth = getWindowManager().getDefaultDisplay().getWidth();
        int fontSize = 20;
        if ( screenWidth>=400 )
        	fontSize = 24;
        else if ( screenWidth>=600 )
        	fontSize = 28;
        props.applyDefault(ReaderView.PROP_FONT_SIZE, String.valueOf(fontSize));
        props.applyDefault(ReaderView.PROP_FONT_FACE, "Droid Sans");
        props.applyDefault(ReaderView.PROP_STATUS_FONT_FACE, "Droid Sans");
        props.applyDefault(ReaderView.PROP_STATUS_FONT_SIZE, "16");
        props.applyDefault(ReaderView.PROP_FONT_COLOR, "#000000");
        props.applyDefault(ReaderView.PROP_FONT_COLOR_DAY, "#000000");
        props.applyDefault(ReaderView.PROP_FONT_COLOR_NIGHT, "#808080");
        props.applyDefault(ReaderView.PROP_BACKGROUND_COLOR, "#FFFFFF");
        props.applyDefault(ReaderView.PROP_BACKGROUND_COLOR_DAY, "#FFFFFF");
        props.applyDefault(ReaderView.PROP_BACKGROUND_COLOR_NIGHT, "#101010");
        props.applyDefault(ReaderView.PROP_STATUS_FONT_COLOR, "#FF000000"); // don't use separate color
        props.applyDefault(ReaderView.PROP_STATUS_FONT_COLOR_DAY, "#FF000000"); // don't use separate color
        props.applyDefault(ReaderView.PROP_STATUS_FONT_COLOR_NIGHT, "#80000000"); // don't use separate color
        props.setProperty(ReaderView.PROP_ROTATE_ANGLE, "0"); // crengine's rotation will not be user anymore
        props.setProperty(ReaderView.PROP_DISPLAY_INVERSE, "0");
        props.applyDefault(ReaderView.PROP_APP_FULLSCREEN, "0");
        props.applyDefault(ReaderView.PROP_APP_SCREEN_BACKLIGHT, "-1");
		props.applyDefault(ReaderView.PROP_SHOW_BATTERY, "1"); 
		props.applyDefault(ReaderView.PROP_SHOW_POS_PERCENT, "0"); 
		props.applyDefault(ReaderView.PROP_SHOW_PAGE_COUNT, "1"); 
		props.applyDefault(ReaderView.PROP_SHOW_TIME, "1");
		props.applyDefault(ReaderView.PROP_FONT_ANTIALIASING, "2");
		props.applyDefault(ReaderView.PROP_APP_SHOW_COVERPAGES, "1");
		props.applyDefault(ReaderView.PROP_APP_SCREEN_ORIENTATION, "4");
		props.applyDefault(ReaderView.PROP_CONTROLS_ENABLE_VOLUME_KEYS, "1");
		props.applyDefault(ReaderView.PROP_APP_TAP_ZONE_HILIGHT, "0");
		props.applyDefault(ReaderView.PROP_APP_BOOK_SORT_ORDER, FileInfo.DEF_SORT_ORDER.name());
		props.applyDefault(ReaderView.PROP_APP_DICTIONARY, dicts[0].id);
		props.applyDefault(ReaderView.PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS, "0");
		props.applyDefault(ReaderView.PROP_APP_SELECTION_ACTION, "0");
		//props.applyDefault(ReaderView.PROP_FALLBACK_FONT_FACE, "Droid Fallback");
		props.put(ReaderView.PROP_FALLBACK_FONT_FACE, "Droid Sans Fallback");

		props.applyDefault(ReaderView.PROP_IMG_SCALING_ZOOMOUT_BLOCK_MODE, "1");
		props.applyDefault(ReaderView.PROP_IMG_SCALING_ZOOMIN_BLOCK_MODE, "1");
		props.applyDefault(ReaderView.PROP_IMG_SCALING_ZOOMOUT_INLINE_MODE, "1");
		props.applyDefault(ReaderView.PROP_IMG_SCALING_ZOOMIN_INLINE_MODE, "1");
		props.applyDefault(ReaderView.PROP_IMG_SCALING_ZOOMOUT_BLOCK_SCALE, "0");
		props.applyDefault(ReaderView.PROP_IMG_SCALING_ZOOMIN_BLOCK_SCALE, "0");
		props.applyDefault(ReaderView.PROP_IMG_SCALING_ZOOMOUT_INLINE_SCALE, "0");
		props.applyDefault(ReaderView.PROP_IMG_SCALING_ZOOMIN_INLINE_SCALE, "0");
		
		props.applyDefault(ReaderView.PROP_PAGE_MARGIN_LEFT, densityDpi > 160 ? "10" : "4");
		props.applyDefault(ReaderView.PROP_PAGE_MARGIN_RIGHT, densityDpi > 160 ? "10" : "4");
		props.applyDefault(ReaderView.PROP_PAGE_MARGIN_TOP, densityDpi > 160 ? "8" : "2");
		props.applyDefault(ReaderView.PROP_PAGE_MARGIN_BOTTOM, densityDpi > 160 ? "8" : "2");
		
        props.applyDefault(ReaderView.PROP_APP_SCREEN_UPDATE_MODE, "0");
        props.applyDefault(ReaderView.PROP_APP_SCREEN_UPDATE_INTERVAL, "10");
        
        props.applyDefault(ReaderView.PROP_NIGHT_MODE, "0");
        if (DeviceInfo.EINK_SCREEN) {
        	props.applyDefault(ReaderView.PROP_PAGE_BACKGROUND_IMAGE, Engine.NO_TEXTURE.id);
        } else {
        	if ( props.getBool(ReaderView.PROP_NIGHT_MODE, false) )
        		props.applyDefault(ReaderView.PROP_PAGE_BACKGROUND_IMAGE, Engine.DEF_NIGHT_BACKGROUND_TEXTURE);
        	else
        		props.applyDefault(ReaderView.PROP_PAGE_BACKGROUND_IMAGE, Engine.DEF_DAY_BACKGROUND_TEXTURE);
        }
        props.applyDefault(ReaderView.PROP_PAGE_BACKGROUND_IMAGE_DAY, Engine.DEF_DAY_BACKGROUND_TEXTURE);
        props.applyDefault(ReaderView.PROP_PAGE_BACKGROUND_IMAGE_NIGHT, Engine.DEF_NIGHT_BACKGROUND_TEXTURE);
        
        props.applyDefault(ReaderView.PROP_FONT_GAMMA, "1.0");
		
		props.setProperty(ReaderView.PROP_MIN_FILE_SIZE_TO_CACHE, "100000");
		props.setProperty(ReaderView.PROP_FORCED_MIN_FILE_SIZE_TO_CACHE, "32768");
		props.applyDefault(ReaderView.PROP_HYPHENATION_DICT, Engine.HyphDict.RUSSIAN.toString());
		props.applyDefault(ReaderView.PROP_APP_FILE_BROWSER_SIMPLE_MODE, "0");
		
		return props;
	}

	public static class DictInfo {
		public final String id; 
		public final String name;
		public final String packageName;
		public final String className;
		public final String action;
		public final Integer internal;
		public DictInfo ( String id, String name, String packageName, String className, String action, Integer internal ) {
			this.id = id;
			this.name = name;
			this.packageName = packageName;
			this.className = className;
			this.action = action;
			this.internal = internal;
		}
	}
	private static final DictInfo dicts[] = {
		new DictInfo("Fora", "Fora Dictionary", "com.ngc.fora", "com.ngc.fora.ForaDictionary", Intent.ACTION_SEARCH, 0),
		new DictInfo("ColorDict", "ColorDict", "com.socialnmobile.colordict", "com.socialnmobile.colordict.activity.Main", Intent.ACTION_SEARCH, 0),
		new DictInfo("ColorDictApi", "ColorDict (new)", "com.socialnmobile.colordict", "com.socialnmobile.colordict.activity.Main", Intent.ACTION_SEARCH, 1),
	};

	public DictInfo[] getDictList() {
		return dicts;
	}
	
	private DictInfo currentDict = dicts[0];
	
	public void setDict( String id ) {
		for ( DictInfo d : dicts ) {
			if ( d.id.equals(id) ) {
				currentDict = d;
				return;
			}
		}
	}

	private void findInDictionaryInternal(String s) {
		switch (currentDict.internal) {
		case 0:
			Intent intent0 = new Intent(currentDict.action).setComponent(new ComponentName(
				currentDict.packageName, currentDict.className
				)).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			intent0.putExtra(SearchManager.QUERY, s);
			try {
				startActivity( intent0 );
			} catch ( ActivityNotFoundException e ) {
				showToast("Dictionary \"" + currentDict.name + "\" is not installed");
			}
			break;
		case 1:
			final String SEARCH_ACTION  = "colordict.intent.action.SEARCH";
			final String EXTRA_QUERY   = "EXTRA_QUERY";
			final String EXTRA_FULLSCREEN = "EXTRA_FULLSCREEN";
			final String EXTRA_HEIGHT  = "EXTRA_HEIGHT";
			final String EXTRA_WIDTH   = "EXTRA_WIDTH";
			final String EXTRA_GRAVITY  = "EXTRA_GRAVITY";
			final String EXTRA_MARGIN_LEFT = "EXTRA_MARGIN_LEFT";
			final String EXTRA_MARGIN_TOP  = "EXTRA_MARGIN_TOP";
			final String EXTRA_MARGIN_BOTTOM = "EXTRA_MARGIN_BOTTOM";
			final String EXTRA_MARGIN_RIGHT = "EXTRA_MARGIN_RIGHT";

			Intent intent1 = new Intent(SEARCH_ACTION);
			intent1.putExtra(EXTRA_QUERY, s); //Search Query
			intent1.putExtra(EXTRA_FULLSCREEN, true); //
			try
			{
				startActivity(intent1);
			} catch ( ActivityNotFoundException e ) {
				showToast("Dictionary \"" + currentDict.name + "\" is not installed");
			}
			break;
		}
	}
	
	public void findInDictionary( String s ) {
		
		if ( s!=null && s.length()!=0 ) {
			s = s.trim();
			for ( ;s.length()>0; ) {
				char ch = s.charAt(s.length()-1);
				if ( ch>=128 )
					break;
				if ( ch>='0' && ch<='9' || ch>='A' && ch<='Z' || ch>='a' && ch<='z' )
					break;
				s = s.substring(0, s.length()-1);
			}
			if ( s.length()>0 ) {
				//
				final String pattern = s;
				BackgroundThread.instance().executeBackground(new Runnable() {
					@Override
					public void run() {
						BackgroundThread.instance().postGUI(new Runnable() {
							@Override
							public void run() {
								findInDictionaryInternal(pattern);
							}
						}, 100);
					}
				});
			}
		}
	}
	
	public void saveSettings( Properties settings )
	{
		try {
			log.v("saveSettings() " + settings);
    		FileOutputStream os = new FileOutputStream(propsFile);
    		settings.store(os, "Cool Reader 3 settings");
			log.i("Settings successfully saved to file " + propsFile.getAbsolutePath());
		} catch ( Exception e ) {
			log.e("exception while saving settings", e);
		}
	}

	private static Debug.MemoryInfo info = new Debug.MemoryInfo();
	private static Field[] infoFields = Debug.MemoryInfo.class.getFields();
	private static String dumpFields( Field[] fields, Object obj) {
		StringBuilder buf = new StringBuilder();
		try {
			for ( Field f : fields ) {
				if ( buf.length()>0 )
					buf.append(", ");
				buf.append(f.getName());
				buf.append("=");
				buf.append(f.get(obj));
			}
		} catch ( Exception e ) {
			
		}
		return buf.toString();
	}
	public static void dumpHeapAllocation() {
		Debug.getMemoryInfo(info);
		log.d("nativeHeapAlloc=" + Debug.getNativeHeapAllocatedSize() + ", nativeHeapSize=" + Debug.getNativeHeapSize() + ", info: " + dumpFields(infoFields, info));
	}
	
	public void showAboutDialog() {
		AboutDialog dlg = new AboutDialog(this);
		dlg.show();
	}
	
	public void openURL(String url) {
		try {
			Intent i = new Intent(Intent.ACTION_VIEW);  
			i.setData(Uri.parse(url));  
			startActivity(i);
		} catch (Exception e) {
			log.e("Exception " + e + " while trying to open URL " + url);
			showToast("Cannot open URL " + url);
		}
	}
}
