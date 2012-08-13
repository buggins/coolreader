package org.coolreader.crengine;

import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Locale;

import org.coolreader.R;
import org.coolreader.db.CRDBService;
import org.coolreader.db.CRDBServiceAccessor;
import org.coolreader.sync.SyncServiceAccessor;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.PixelFormat;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.PowerManager;
import android.text.ClipboardManager;
import android.util.DisplayMetrics;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Display;
import android.view.Gravity;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.View;
import android.view.View.OnCreateContextMenuListener;
import android.view.Window;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.Toast;

public class BaseActivity extends Activity implements Settings {

	private static final Logger log = L.create("ba");

	private CRDBServiceAccessor mCRDBService;
	private SyncServiceAccessor mSyncService;
	
	protected void unbindCRDBService() {
		if (mCRDBService != null) {
			mCRDBService.unbind();
			mCRDBService = null;
		}
	}

	protected void unbindSyncService() {
		if (mSyncService != null) {
			mSyncService.unbind();
			mSyncService = null;
		}
	}

	protected void bindSyncService() {
		if (mSyncService == null) {
	       	mSyncService = new SyncServiceAccessor(this);
			mSyncService.bind(new Runnable() {
				@Override
				public void run() {
					log.i("Initialization after SyncService is bound");
					BackgroundThread.instance().postGUI(new Runnable() {
						@Override
						public void run() {
							FileInfo downloadDirectory = Services.getScanner().getDownloadDirectory();
							if (downloadDirectory != null && mSyncService != null)
								mSyncService.setSyncDirectory(new File(downloadDirectory.getPathName()));
						}
					});
				}
			});
		}
	}

	
	protected void bindCRDBService() {
		if (mCRDBService == null) {
			mCRDBService = new CRDBServiceAccessor(this, Engine.getInstance(this).getPathCorrector());
		}
        mCRDBService.bind(null);
	}

	/**
	 * Wait until database is bound.
	 * @param readyCallback to be called after DB is ready
	 */
	public void waitForCRDBService(Runnable readyCallback) {
		if (mCRDBService == null) {
			mCRDBService = new CRDBServiceAccessor(this, Engine.getInstance(this).getPathCorrector());
		}
        mCRDBService.bind(readyCallback);
	}

	public CRDBServiceAccessor getDBService() { return mCRDBService; }
	public CRDBService.LocalBinder getDB() { return mCRDBService.get(); }
	public SyncServiceAccessor getSyncService() { return mSyncService; }
	
	/** Called when the activity is first created. */
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
		log.i("BaseActivity.onCreate() entered");
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		super.onCreate(savedInstanceState);

		
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
			Field fld = m.getClass().getField("densityDpi");
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
		float widthInches = m.widthPixels / densityDpi;
		float heightInches = m.heightPixels / densityDpi;
		diagonalInches = (float)Math.sqrt(widthInches * widthInches + heightInches * heightInches);
		
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

		// load settings
		Properties props = SettingsManager.instance(this).get();
		String theme = props.getProperty(ReaderView.PROP_APP_THEME, DeviceInfo.FORCE_LIGHT_THEME ? "WHITE" : "LIGHT");
		String lang = props.getProperty(ReaderView.PROP_APP_LOCALE, Lang.DEFAULT.code);
		setLanguage(lang);
		setCurrentTheme(theme);

		
		setScreenBacklightDuration(props.getInt(ReaderView.PROP_APP_SCREEN_BACKLIGHT_LOCK, 3));

		setFullscreen( props.getBool(ReaderView.PROP_APP_FULLSCREEN, (DeviceInfo.EINK_SCREEN?true:false)));
		int orientation = props.getInt(ReaderView.PROP_APP_SCREEN_ORIENTATION, 4); //(DeviceInfo.EINK_SCREEN?0:4)
		if ( orientation < 0 || orientation > 4 )
			orientation = 0;
		setScreenOrientation(orientation);
		int backlight = props.getInt(ReaderView.PROP_APP_SCREEN_BACKLIGHT, -1);
		if ( backlight<-1 || backlight>100 )
			backlight = -1;
		setScreenBacklightLevel(backlight);

    
		
		bindSyncService();
		bindCRDBService();
	}
	
    
	@Override
	protected void onDestroy() {
		super.onDestroy();
		unbindCRDBService();
		unbindSyncService();
	}

	@Override
	protected void onStart() {
		super.onStart();

//		Properties props = SettingsManager.instance(this).get();
//		String theme = props.getProperty(ReaderView.PROP_APP_THEME, DeviceInfo.FORCE_LIGHT_THEME ? "WHITE" : "LIGHT");
//		setCurrentTheme(theme);
		
		mIsStarted = true;
		mPaused = false;
		onUserActivity();
	}

	@Override
	protected void onStop() {
		mIsStarted = false;
		super.onStop();
	}

	@Override
	protected void onPause() {
		log.i("CoolReader.onPause() : saving reader state");
		mIsStarted = false;
		mPaused = true;
//		setScreenUpdateMode(-1, mReaderView);
		releaseBacklightControl();
		super.onPause();
	}
	
	protected static String PREF_FILE = "CR3LastBook";
	protected static String PREF_LAST_BOOK = "LastBook";
	
	@Override
	protected void onResume() {
		log.i("CoolReader.onResume()");
		mPaused = false;
		mIsStarted = true;
		backlightControl.onUserActivity();
		super.onResume();
	}
	
    private boolean mIsStarted = false;
    private boolean mPaused = false;
	
	public boolean isStarted() { return mIsStarted; }
	
	private String mVersion = "3.0";
	
	public String getVersion() {
		return mVersion;
	}
	
	public Properties loadSettings(int profile) {
		return SettingsManager.instance(this).loadSettings(profile);
	}
	
	public void saveSettings(int profile, Properties settings) {
		SettingsManager.instance(this).saveSettings(profile, settings);
	}
	
	public void saveSettings(File f, Properties settings)
	{
		SettingsManager.instance(this).saveSettings(f, settings);
	}

	public int getPalmTipPixels()
	{
		return densityDpi / 3; // 1/3"
	}
	
	public int getDensityDpi()
	{
		return densityDpi;
	}
	
	public float getDiagonalInches()
	{
		return diagonalInches;
	}
	
	public boolean isSmartphone() {
		return diagonalInches <= 5.5;
	}
	
	private int densityDpi = 160;
	private float diagonalInches = 4;



	private InterfaceTheme currentTheme = DeviceInfo.FORCE_LIGHT_THEME ? InterfaceTheme.WHITE : InterfaceTheme.LIGHT;
	
	public InterfaceTheme getCurrentTheme() {
		return currentTheme;
	}

	public void setCurrentTheme(String themeCode) {
		InterfaceTheme theme = InterfaceTheme.findByCode(themeCode);
		if (theme != null && currentTheme != theme) {
			setCurrentTheme(theme);
		}
	}

	private int preferredItemHeight = 36;
	public int getPreferredItemHeight() {
		return preferredItemHeight;
	}
	
	public void updateBackground() {
		TypedArray a = getTheme().obtainStyledAttributes(new int[] {android.R.attr.windowBackground, android.R.attr.background, android.R.attr.textColor, android.R.attr.colorBackground, android.R.attr.colorForeground, android.R.attr.listPreferredItemHeight});
		int bgRes = a.getResourceId(0, 0);
		//int clText = a.getColor(1, 0);
		int clBackground = a.getColor(2, 0);
		//int clForeground = a.getColor(3, 0);
		preferredItemHeight = a.getDimensionPixelSize(5, 36);
		//View contentView = getContentView();
//		if (contentView != null) {
//			if (bgRes != 0) {
//				//Drawable d = getResources().getDrawable(bgRes);
//				//log.v("Setting background resource " + d.getIntrinsicWidth() + "x" + d.getIntrinsicHeight());
//				//contentView.setBackgroundResource(null);
//				contentView.setBackgroundResource(bgRes);
//				getWindow().setBackgroundDrawableResource(bgRes);//Drawable(d);
//				//getWindow().setBackgroundDrawable(d);
//			} else if (clBackground != 0) {
//				//contentView.setBackgroundColor(clBackground);
//				getWindow().setBackgroundDrawable(Utils.solidColorDrawable(clBackground));
//			}
//		} else {
//			if (bgRes != 0)
//				getWindow().setBackgroundDrawableResource(bgRes);
//			else if (clBackground != 0)
//				getWindow().setBackgroundDrawable(Utils.solidColorDrawable(clBackground));
//		}
		a.recycle();
	}

	public void setCurrentTheme(InterfaceTheme theme) {
		log.i("setCurrentTheme(" + theme + ")");
		currentTheme = theme;
		getApplication().setTheme(theme.getThemeId());
		setTheme(theme.getThemeId());
		updateBackground();
	}

	int screenOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR;
	public void applyScreenOrientation( Window wnd )
	{
		if ( wnd!=null ) {
			WindowManager.LayoutParams attrs = wnd.getAttributes();
			attrs.screenOrientation = screenOrientation;
			wnd.setAttributes(attrs);
			if (DeviceInfo.EINK_SCREEN){
				//TODO:
				//EinkScreen.ResetController(mReaderView);
			}
			
		}
	}

	public int getScreenOrientation()
	{
		switch ( screenOrientation ) {
		case ActivityInfo.SCREEN_ORIENTATION_PORTRAIT:
			return 0;
		case ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE:
			return 1;
		case ActivityInfo_SCREEN_ORIENTATION_REVERSE_PORTRAIT:
			return 2;
		case ActivityInfo_SCREEN_ORIENTATION_REVERSE_LANDSCAPE:
			return 3;
		default:
			return orientationFromSensor;
		}
	}

	public boolean isLandscape()
	{
		return screenOrientation == ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE || screenOrientation == ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
	}

	// support pre API LEVEL 9
	final static public int ActivityInfo_SCREEN_ORIENTATION_SENSOR_PORTRAIT = 7;
	final static public int ActivityInfo_SCREEN_ORIENTATION_SENSOR_LANDSCAPE = 6;
	final static public int ActivityInfo_SCREEN_ORIENTATION_REVERSE_PORTRAIT = 9;
	final static public int ActivityInfo_SCREEN_ORIENTATION_REVERSE_LANDSCAPE = 8;
	final static public int ActivityInfo_SCREEN_ORIENTATION_FULL_SENSOR = 10;

	public void setScreenOrientation( int angle )
	{
		int newOrientation = screenOrientation;
		boolean level9 = DeviceInfo.getSDKLevel() >= 9;
		switch (angle) {
		case 0:
			newOrientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT; // level9 ? ActivityInfo_SCREEN_ORIENTATION_SENSOR_PORTRAIT : ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
			break;
		case 1:
			newOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE; // level9 ? ActivityInfo_SCREEN_ORIENTATION_SENSOR_LANDSCAPE : ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
			break;
		case 2:
			newOrientation = level9 ? ActivityInfo_SCREEN_ORIENTATION_REVERSE_PORTRAIT : ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
			break;
		case 3:
			newOrientation = level9 ? ActivityInfo_SCREEN_ORIENTATION_REVERSE_LANDSCAPE : ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
			break;
		case 4:
			newOrientation = level9 ? ActivityInfo_SCREEN_ORIENTATION_FULL_SENSOR : ActivityInfo.SCREEN_ORIENTATION_SENSOR;
			break;
		}
		if (newOrientation != screenOrientation) {
			log.d("setScreenOrientation(" + angle + ")");
			screenOrientation = newOrientation;
			setRequestedOrientation(screenOrientation);
			applyScreenOrientation(getWindow());
		}
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
		setSystemUiVisibility();
	}
	public void setFullscreen( boolean fullscreen )
	{
		if ( mFullscreen!=fullscreen ) {
			mFullscreen = fullscreen;
			applyFullscreen( getWindow() );
		}
	}
	
	private final static int SYSTEM_UI_FLAG_LOW_PROFILE = 1;
	private final static int SYSTEM_UI_FLAG_HIDE_NAVIGATION = 2;
	
	private final static int SYSTEM_UI_FLAG_VISIBLE = 0;

	public boolean setSystemUiVisibility() {
		if (DeviceInfo.getSDKLevel() >= DeviceInfo.HONEYCOMB) {
			int flags = 0;
			if (getKeyBacklight() == 0)
				flags |= SYSTEM_UI_FLAG_LOW_PROFILE;
			if (isFullscreen())
				flags |= SYSTEM_UI_FLAG_HIDE_NAVIGATION;
			setSystemUiVisibility(flags);
			return true;
		}
		return false;
	}
	

	private int lastSystemUiVisibility = -1;
	private boolean setSystemUiVisibility(int value) {
		if (DeviceInfo.getSDKLevel() >= DeviceInfo.HONEYCOMB) {
			boolean a4 = DeviceInfo.getSDKLevel() >= DeviceInfo.ICE_CREAM_SANDWICH;
			if (value == lastSystemUiVisibility)// && a4)
				return false;
			lastSystemUiVisibility = value;
			if (!a4)
				value &= SYSTEM_UI_FLAG_LOW_PROFILE;
			View view;
			//if (a4)
				view = getWindow().getDecorView(); // getReaderView();
			//else
			//	view = mActivity.getContentView(); // getReaderView();
			
			if (view == null)
				return false;
			Method m;
			try {
				m = view.getClass().getMethod("setSystemUiVisibility", int.class);
				m.invoke(view, value);
				return true;
			} catch (SecurityException e) {
				// ignore
			} catch (NoSuchMethodException e) {
				// ignore
			} catch (IllegalArgumentException e) {
				// ignore
			} catch (IllegalAccessException e) {
				// ignore
			} catch (InvocationTargetException e) {
				// ignore
			}
		}
		return false;
	}

	private int currentKeyBacklightLevel = 1;
	public int getKeyBacklight() {
		return currentKeyBacklightLevel;
	}
	public boolean setKeyBacklight(int value) {
		currentKeyBacklightLevel = value;
		// Try ICS way
		if (DeviceInfo.getSDKLevel() >= DeviceInfo.HONEYCOMB) {
			setSystemUiVisibility();
		}
		// thread safe
		return Engine.getInstance(this).setKeyBacklight(value);
	}
	


    private boolean keyBacklightOff = true;
    public boolean isKeyBacklightDisabled() {
    	return keyBacklightOff;
    }
    
    public void setKeyBacklightDisabled(boolean disabled) {
    	keyBacklightOff = disabled;
    	onUserActivity();
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
    private boolean brightnessHackError = DeviceInfo.SAMSUNG_BUTTONS_HIGHLIGHT_PATCH;

    private void turnOffKeyBacklight() {
    	if (!isStarted())
    		return;
		if (DeviceInfo.getSDKLevel() >= DeviceInfo.HONEYCOMB) {
			setKeyBacklight(0);
		}
    	// repeat again in short interval
    	if (!Engine.getInstance(this).setKeyBacklight(0)) {
    		//log.w("Cannot control key backlight directly");
    		return;
    	}
    	// repeat again in short interval
    	Runnable task = new Runnable() {
			@Override
			public void run() {
		    	if (!isStarted())
		    		return;
		    	if (!Engine.getInstance(BaseActivity.this).setKeyBacklight(0)) {
		    		//log.w("Cannot control key backlight directly (delayed)");
		    	}
			}
		};
		BackgroundThread.instance().postGUI(task, 1);
		//BackgroundThread.instance().postGUI(task, 10);
    }
    
    private void updateBacklightBrightness(float b) {
        Window wnd = getWindow();
        if (wnd != null) {
	    	LayoutParams attrs =  wnd.getAttributes();
	    	boolean changed = false;
	    	if (b < 0 && b > -0.99999f) {
	    		//log.d("dimming screen by " + (int)((1 + b)*100) + "%");
	    		b = -b * attrs.screenBrightness;
	    		if (b < 0.15)
	    			return;
	    	}
	    	float delta = attrs.screenBrightness - b;
	    	if (delta < 0)
	    		delta = -delta;
	    	if (delta > 0.01) {
	    		attrs.screenBrightness = b;
	    		changed = true;
	    	}
	    	if ( changed ) {
	    		log.d("Window attribute changed: " + attrs);
	    		wnd.setAttributes(attrs);
	    	}
        }
    }

    private void updateButtonsBrightness(float buttonBrightness) {
        Window wnd = getWindow();
        if (wnd != null) {
	    	LayoutParams attrs =  wnd.getAttributes();
	    	boolean changed = false;
	    	// hack to set buttonBrightness field
	    	//float buttonBrightness = keyBacklightOff ? 0.0f : -1.0f;
	    	if (!brightnessHackError)
	    	try {
	        	Field bb = attrs.getClass().getField("buttonBrightness");
	        	if (bb != null) {
	        		Float oldValue = (Float)bb.get(attrs);
	        		if (oldValue == null || oldValue.floatValue() != buttonBrightness) {
	        			bb.set(attrs, buttonBrightness);
		        		changed = true;
	        		}
	        	}
	    	} catch ( Exception e ) {
	    		log.e("WindowManager.LayoutParams.buttonBrightness field is not found, cannot turn buttons backlight off");
	    		brightnessHackError = true;
	    	}
	    	//attrs.buttonBrightness = 0;
	    	if (changed) {
	    		log.d("Window attribute changed: " + attrs);
	    		wnd.setAttributes(attrs);
	    	}
	    	if (keyBacklightOff)
	    		turnOffKeyBacklight();
        }
    }

    private final static int MIN_BACKLIGHT_LEVEL_PERCENT = DeviceInfo.MIN_SCREEN_BRIGHTNESS_PERCENT;
    
    protected void setDimmingAlpha(int alpha) {
    	// override it
    }
    
    public void onUserActivity()
    {
    	if (backlightControl != null)
      	    backlightControl.onUserActivity();
    	// Hack
    	//if ( backlightControl.isHeld() )
    	BackgroundThread.instance().executeGUI(new Runnable() {
			@Override
			public void run() {
				try {
		        	float b;
		        	int dimmingAlpha = 255;
		        	// screenBacklightBrightness is 0..100
		        	if (screenBacklightBrightness >= 0) {
	        			float minb = MIN_BACKLIGHT_LEVEL_PERCENT / 100.0f; 
		        		if ( screenBacklightBrightness >= 10 ) {
		        			// real brightness control, no colors dimming
		        			b = (screenBacklightBrightness - 10) / (100.0f - 10.0f); // 0..1
		        			b = minb + b * (1-minb); // minb..1
				        	if (b < minb) // BRIGHTNESS_OVERRIDE_OFF
				        		b = minb;
				        	else if (b > 1.0f)
				        		b = 1.0f; //BRIGHTNESS_OVERRIDE_FULL
		        		} else {
			        		// minimal brightness with colors dimming
			        		b = minb;
			        		dimmingAlpha = 255 - (11-screenBacklightBrightness) * 180 / 10; 
		        		}
		        	} else {
		        		// system
		        		b = -1.0f; //BRIGHTNESS_OVERRIDE_NONE
		        	}
		        	setDimmingAlpha(dimmingAlpha);
			    	//log.v("Brightness: " + b + ", dim: " + dimmingAlpha);
			    	updateBacklightBrightness(b);
			    	updateButtonsBrightness(keyBacklightOff ? 0.0f : -1.0f);
				} catch ( Exception e ) {
					// ignore
				}
			}
    	});
    }

    
	public boolean isWakeLockEnabled() {
		return screenBacklightDuration > 0;
	}

	/**
	 * @param backlightDurationMinutes 0 = system default, 1 == 3 minutes, 2..5 == 2..5 minutes
	 */
	public void setScreenBacklightDuration(int backlightDurationMinutes)
	{
		if (backlightDurationMinutes == 1)
			backlightDurationMinutes = 3;
		if (screenBacklightDuration != backlightDurationMinutes * 60 * 1000) {
			screenBacklightDuration = backlightDurationMinutes * 60 * 1000;
			if (screenBacklightDuration == 0)
				backlightControl.release();
			else
				backlightControl.onUserActivity();
		}
	}
	
	private Runnable backlightTimerTask = null;
	private static long lastUserActivityTime;
	public static final int DEF_SCREEN_BACKLIGHT_TIMER_INTERVAL = 3 * 60 * 1000;
	private int screenBacklightDuration = DEF_SCREEN_BACKLIGHT_TIMER_INTERVAL;

	private class ScreenBacklightControl {
		PowerManager.WakeLock wl = null;

		public ScreenBacklightControl() {
		}

		long lastUpdateTimeStamp;
		
		public void onUserActivity() {
			lastUserActivityTime = Utils.timeStamp();
			if (Utils.timeInterval(lastUpdateTimeStamp) < 5000)
				return;
			lastUpdateTimeStamp = android.os.SystemClock.uptimeMillis();
			if (!isWakeLockEnabled())
				return;
			if (wl == null) {
				PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
				wl = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK
				/* | PowerManager.ON_AFTER_RELEASE */, "cr3");
				log.d("ScreenBacklightControl: WakeLock created");
			}
			if (!isStarted()) {
				log.d("ScreenBacklightControl: user activity while not started");
				release();
				return;
			}

			if (!isHeld()) {
				log.d("ScreenBacklightControl: acquiring WakeLock");
				wl.acquire();
			}

			if (backlightTimerTask == null) {
				log.v("ScreenBacklightControl: timer task started");
				backlightTimerTask = new BacklightTimerTask();
				BackgroundThread.instance().postGUI(backlightTimerTask,
						screenBacklightDuration / 10);
			}
		}

		public boolean isHeld() {
			return wl != null && wl.isHeld();
		}

		public void release() {
			if (wl != null && wl.isHeld()) {
				log.d("ScreenBacklightControl: wl.release()");
				wl.release();
			}
			backlightTimerTask = null;
			lastUpdateTimeStamp = 0;
		}

		private class BacklightTimerTask implements Runnable {

			@Override
			public void run() {
				if (backlightTimerTask == null)
					return;
				long interval = Utils.timeInterval(lastUserActivityTime);
//				log.v("ScreenBacklightControl: timer task, lastActivityMillis = "
//						+ interval);
				int nextTimerInterval = screenBacklightDuration / 20;
				boolean dim = false;
				if (interval > screenBacklightDuration * 8 / 10) {
					nextTimerInterval = nextTimerInterval / 8;
					dim = true;
				}
				if (interval > screenBacklightDuration) {
					log.v("ScreenBacklightControl: interval is expired");
					release();
				} else {
					BackgroundThread.instance().postGUI(backlightTimerTask, nextTimerInterval);
					if (dim) {
						updateBacklightBrightness(-0.9f); // reduce by 9%
					}
				}
			}

		};

	}

	public void releaseBacklightControl()
	{
		backlightControl.release();
	}

	ScreenBacklightControl backlightControl = new ScreenBacklightControl();
    


	private int mScreenUpdateMode = 0;
	public int getScreenUpdateMode() {
		return mScreenUpdateMode;
	}
	public void setScreenUpdateMode( int screenUpdateMode, View view ) {
		//if (mReaderView != null) {
			mScreenUpdateMode = screenUpdateMode;
			if (EinkScreen.UpdateMode != screenUpdateMode || EinkScreen.UpdateMode == 2) {
				EinkScreen.ResetController(screenUpdateMode, view);
			}
		//}
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


	
	
	
	public void showToast(int stringResourceId) {
		showToast(stringResourceId, Toast.LENGTH_LONG);
	}

	public void showToast(int stringResourceId, int duration) {
		String s = getString(stringResourceId);
		if (s != null)
			showToast(s, duration);
	}

	public void showToast(String msg) {
		showToast(msg, Toast.LENGTH_LONG);
	}

	public void showToast(String msg, int duration) {
		log.v("showing toast: " + msg);
		if (DeviceInfo.USE_CUSTOM_TOAST) {
			ToastView.showToast(getContentView(), msg, Toast.LENGTH_LONG);
		} else {
			// classic Toast
			Toast toast = Toast.makeText(this, msg, duration);
			toast.show();
		}
	}


	protected View contentView;
	public View getContentView() {
		return contentView;
	}
	public void setContentView(View view) {
		this.contentView = view;
		super.setContentView(view);
		updateBackground();
	}
	

	private boolean mNightMode = false;
	public boolean isNightMode() {
		return mNightMode;
	}
	public void setNightMode( boolean nightMode ) {
		mNightMode = nightMode;
	}
	

	
    public ClipboardManager getClipboardmanager() {
    	return (ClipboardManager)getSystemService(CLIPBOARD_SERVICE);
    }

    
	public void showHomeScreen() {
		Intent intent = new Intent(Intent.ACTION_MAIN);
		intent.addCategory(Intent.CATEGORY_HOME);
		startActivity(intent);
	}
	

	

	private static String PREF_HELP_FILE = "HelpFile";
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
	
	public String getLastGeneratedHelpFileSignature()
	{
		SharedPreferences pref = getSharedPreferences(PREF_FILE, 0);
		String res = pref.getString(PREF_HELP_FILE, null);
		return res;
	}
	
	public void setLastGeneratedHelpFileSignature(String v)
	{
		SharedPreferences pref = getSharedPreferences(PREF_FILE, 0);
		pref.edit().putString(PREF_HELP_FILE, v).commit();
	}

	
	
	private String currentLanguage;
	
	public String getCurrentLanguage() {
		return currentLanguage;
	}
	
	public void setLanguage(String lang) {
		setLanguage(Lang.byCode(lang));
	}
	
	public void setLanguage(Lang lang) {
		try {
			Resources res = getResources();
		    // Change locale settings in the app.
		    DisplayMetrics dm = res.getDisplayMetrics();
		    android.content.res.Configuration conf = res.getConfiguration();
		    conf.locale = (lang == Lang.DEFAULT) ? defaultLocale : lang.getLocale();
		    currentLanguage = (lang == Lang.DEFAULT) ? Lang.getCode(defaultLocale) : lang.code;
		    res.updateConfiguration(conf, dm);
		} catch (Exception e) {
			log.e("error while setting locale " + lang, e);
		}
	}
	
	// Store system locale here, on class creation
	private static final Locale defaultLocale = Locale.getDefault();

	
	static public int stringToInt( String value, int defValue ) {
		if ( value==null )
			return defValue;
		try {
			return Integer.valueOf(value);
		} catch ( NumberFormatException e ) {
			return defValue;
		}
	}
	
	public void applyAppSetting( String key, String value )
	{
		boolean flg = "1".equals(value);
        if ( key.equals(PROP_APP_FULLSCREEN) ) {
			setFullscreen( "1".equals(value) );
        } else if ( key.equals(PROP_APP_LOCALE) ) {
			setLanguage(value);
        } else if ( key.equals(PROP_APP_KEY_BACKLIGHT_OFF) ) {
			setKeyBacklightDisabled(flg);
        } else if ( key.equals(PROP_APP_SCREEN_BACKLIGHT_LOCK) ) {
        	int n = 0;
        	try {
        		n = Integer.parseInt(value);
        	} catch (NumberFormatException e) {
        		// ignore
        	}
			setScreenBacklightDuration(n);
        } else if ( key.equals(PROP_NIGHT_MODE) ) {
			setNightMode(flg);
        } else if ( key.equals(PROP_APP_SCREEN_UPDATE_MODE) ) {
			setScreenUpdateMode(stringToInt(value, 0), getContentView());
        } else if ( key.equals(PROP_APP_SCREEN_UPDATE_INTERVAL) ) {
			setScreenUpdateInterval(stringToInt(value, 10), getContentView());
        } else if ( key.equals(PROP_APP_THEME) ) {
        	setCurrentTheme(value);
        } else if ( key.equals(PROP_APP_SCREEN_ORIENTATION) ) {
        	int orientation = 0;
        	try {
        		orientation = Integer.parseInt(value);
        	} catch (NumberFormatException e) {
        		// ignore
        	}
        	setScreenOrientation(orientation);
        } else if ( !DeviceInfo.EINK_SCREEN && PROP_APP_SCREEN_BACKLIGHT.equals(key) ) {
        	try {
        		final int n = Integer.valueOf(value);
        		// delay before setting brightness
        		BackgroundThread.instance().postGUI(new Runnable() {
        			public void run() {
                		BackgroundThread.instance().postBackground(new Runnable() {
                			public void run() {
                        		BackgroundThread.instance().postGUI(new Runnable() {
                        			public void run() {
        				        		setScreenBacklightLevel(n);
                        			}
                        		});
                			}
                		});
        			}
        		}, 100);
        	} catch ( Exception e ) {
        		// ignore
        	}
        } else if ( key.equals(PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS) ) {
        	Services.getScanner().setHideEmptyDirs(flg);
        }
	}

	
	public void askConfirmation(int questionResourceId, final Runnable action) {
		AlertDialog.Builder dlg = new AlertDialog.Builder(this);
		dlg.setTitle(questionResourceId);
		dlg.setPositiveButton(R.string.dlg_button_ok, new OnClickListener() {
			public void onClick(DialogInterface arg0, int arg1) {
				action.run();
			}
		});
		dlg.setNegativeButton(R.string.dlg_button_cancel, new OnClickListener() {
			public void onClick(DialogInterface arg0, int arg1) {
				// do nothing
			}
		});
		dlg.show();
	}

	public void directoryUpdated(FileInfo dir) {
		// override it to use
	}

	public void onSettingsChanged(Properties props) {
		// override for specific actions
	}
	
	public void showActionsPopupMenu(final ArrayList<ReaderAction> actions, final CRToolBar.OnActionHandler onActionHandler) {
		registerForContextMenu(contentView);
		contentView.setOnCreateContextMenuListener(new OnCreateContextMenuListener() {
			@Override
			public void onCreateContextMenu(ContextMenu menu, View v,
					ContextMenuInfo menuInfo) {
				int order = 0;
				for (final ReaderAction action : actions) {
					MenuItem item = menu.add(0, action.menuItemId, order++, action.nameId);
					item.setOnMenuItemClickListener(new OnMenuItemClickListener() {
						@Override
						public boolean onMenuItemClick(MenuItem item) {
							return onActionHandler.onActionSelected(action);
						}
					});
				}
			}
		});
		contentView.showContextMenu();
	}
	
}
