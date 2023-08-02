/*
 * CoolReader for Android
 * Copyright (C) 2011-2015,2018,2020 Vadim Lopatin <coolreader.org@gmail.com>
 * Copyright (C) 2011 a_lone
 * Copyright (C) 2011 Viktor Soskin <xorzone@gmail.com>
 * Copyright (C) 2012 madlynx
 * Copyright (C) 2013 Alexey Kabelitskiy <akabelytskyi@hmstn.com>
 * Copyright (C) 2013 Jeff Doozan <jeff@doozan.com>
 * Copyright (C) 2014 klush
 * Copyright (C) 2018 S-trace <S-trace@list.ru>
 * Copyright (C) 2018 norbi24 <norbert.bartalsky@gmail.com>
 * Copyright (C) 2018 Yuri Plotnikov <plotnikovya@gmail.com>
 * Copyright (C) 2021 ourairquality <info@ourairquality.org>
 * Copyright (C) 2018-2022 Aleksey Chernov <valexlin@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.coolreader.crengine;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.os.Build;
import android.os.Bundle;
import android.os.PowerManager;
import android.text.ClipboardManager;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.Window;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.Toast;

import org.coolreader.Dictionaries;
import org.coolreader.Dictionaries.DictInfo;
import org.coolreader.R;
import org.coolreader.db.CRDBService;
import org.coolreader.db.CRDBServiceAccessor;
import org.coolreader.genrescollection.GenresCollection;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map.Entry;
import java.util.Set;

@SuppressLint("Registered")
public class BaseActivity extends Activity implements Settings {

	private static final Logger log = L.create("ba");
	private View mDecorView;

	private CRDBServiceAccessor mCRDBService;
	protected Dictionaries mDictionaries;

	protected void unbindCRDBService() {
		if (mCRDBService != null) {
			mCRDBService.unbind();
			mCRDBService = null;
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
	 *
	 * @param readyCallback to be called after DB is ready
	 */
	public synchronized void waitForCRDBService(Runnable readyCallback) {
		if (mCRDBService == null) {
			mCRDBService = new CRDBServiceAccessor(this, Engine.getInstance(this).getPathCorrector());
		}
		mCRDBService.bind(readyCallback);
	}

	public CRDBServiceAccessor getDBService() {
		return mCRDBService;
	}

	public CRDBService.LocalBinder getDB() {
		return mCRDBService != null ? mCRDBService.get() : null;
	}

	public Properties settings() {
		return mSettingsManager.mSettings;
	}

	private SettingsManager mSettingsManager;

	private EinkScreen mEinkScreen;

	public EinkScreen getEinkScreen() {
		return mEinkScreen;
	}

	protected void startServices() {
		if (DeviceInfo.EINK_NOOK)
			mEinkScreen = new EinkScreenNook();
		else if (DeviceInfo.EINK_TOLINO)
			mEinkScreen = new EinkScreenTolino();
		/*
		 * Support for ONYX devices is disabled until the ONYX SDK is released under a GPL compatible license.
		 * When enabling this code don't forget to update related code in DeviceInfo.java and EinkScreenOnyx.java
		 *
		else if (DeviceInfo.EINK_ONYX)
			mEinkScreen = new EinkScreenOnyx();
		 */
		else
			mEinkScreen = new EinkScreenDummy();

		DisplayMetrics dm = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(dm);

		try {
			Field fld = dm.getClass().getField("densityDpi");
			if (fld != null) {
				Object v = fld.get(dm);
				if (v != null && v instanceof Integer) {
					densityDpi = ((Integer) v).intValue();
					log.i("Screen density detected: " + densityDpi + "DPI");
				}
			}
		} catch (Exception e) {
			log.e("Cannot find field densityDpi, using default value");
		}
		float widthInches = (float)dm.widthPixels / (float)densityDpi;
		float heightInches = (float)dm.heightPixels / (float)densityDpi;
		diagonalInches = (float) Math.sqrt(widthInches * widthInches + heightInches * heightInches);
		log.i("diagonal=" + diagonalInches + "  isSmartphone=" + isSmartphone());

		int sz = dm.widthPixels;
		if (sz > dm.heightPixels)
			sz = dm.heightPixels;
		minFontSize = 5*densityDpi/72;			// 5pt
		//maxFontSize = 100*densityDpi/72;		// 100pt
		maxFontSize = sz/8;

		// create settings
		mSettingsManager = new SettingsManager(this);
		// create rest of settings
		Services.startServices(this);
	}

	@SuppressLint("NewApi")
	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);
		if (hasFocus)
			setSystemUiVisibility();
	}

	/**
	 * Called when the activity is first created.
	 */
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		log.i("BaseActivity.onCreate() entered");
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		mDecorView = getWindow().getDecorView();

		super.onCreate(savedInstanceState);

		mDictionaries = new Dictionaries(this);

		try {
			PackageInfo pi = getPackageManager().getPackageInfo(getPackageName(), 0);
			mVersion = pi.versionName;
		} catch (NameNotFoundException e) {
			// ignore
		}
		log.i("CoolReader version : " + getVersion());
		//log.i("CoolReader.window=" + getWindow());
		if (!DeviceInfo.EINK_SCREEN) {
			WindowManager.LayoutParams lp = new WindowManager.LayoutParams();
			lp.alpha = 1.0f;
			lp.dimAmount = 0.0f;
			lp.format = DeviceInfo.PIXEL_FORMAT;
			lp.gravity = Gravity.CENTER_HORIZONTAL | Gravity.CENTER_VERTICAL;
			lp.horizontalMargin = 0;
			lp.verticalMargin = 0;
			lp.windowAnimations = 0;
			lp.layoutAnimationParameters = null;
			lp.memoryType = WindowManager.LayoutParams.MEMORY_TYPE_NORMAL;
			getWindow().setAttributes(lp);
		}

		// load settings
		Properties props = settings();
		String theme = props.getProperty(ReaderView.PROP_APP_THEME, DeviceInfo.FORCE_HC_THEME ? "HICONTRAST1" : "LIGHT");
		String lang = props.getProperty(ReaderView.PROP_APP_LOCALE, Lang.DEFAULT.code);
		setLanguage(lang);
		setCurrentTheme(theme);

		setScreenBacklightDuration(props.getInt(ReaderView.PROP_APP_SCREEN_BACKLIGHT_LOCK, 3));

		setFullscreen(props.getBool(ReaderView.PROP_APP_FULLSCREEN, DeviceInfo.EINK_SCREEN));
		int orientation = props.getInt(ReaderView.PROP_APP_SCREEN_ORIENTATION, 0); //(DeviceInfo.EINK_SCREEN?0:4)
		if (orientation < 0 || orientation > 5)
			orientation = 5;
		setScreenOrientation(orientation);
		int backlight = props.getInt(ReaderView.PROP_APP_SCREEN_BACKLIGHT, -1);
		if (backlight < -1 || backlight > DeviceInfo.MAX_SCREEN_BRIGHTNESS_VALUE)
			backlight = -1;
		if (!DeviceInfo.EINK_SCREEN)
			setScreenBacklightLevel(backlight);

		bindCRDBService();
	}

	protected BaseDialog currentDialog;
	public void onDialogCreated(BaseDialog dlg) {
		currentDialog = dlg;
	}
	public void onDialogClosed(BaseDialog dlg) {
    	if (currentDialog == dlg) {
    		currentDialog = null;
		}
	}
	public BaseDialog getCurrentDialog() {
    	return currentDialog;
	}
	public boolean isDialogActive() {
    	return currentDialog != null;
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		unbindCRDBService();
	}

	@Override
	protected void onStart() {
		super.onStart();

//		Properties props = settings().get();
//		String theme = props.getProperty(ReaderView.PROP_APP_THEME, DeviceInfo.FORCE_HC_THEME ? "WHITE" : "LIGHT");
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
		releaseBacklightControl();
		super.onPause();
	}

	protected static String PREF_FILE = "CR3LastBook";
	protected static String PREF_LAST_BOOK = "LastBook";
	protected static String PREF_LAST_LOCATION = "LastLocation";
	protected static String PREF_LAST_NOTIFICATION_MASK = "LastNoticeMask";
	protected static String PREF_EXT_DATADIR_CREATETIME = "ExtDataDirCreateTime";
	protected static String PREF_LAST_LOGCAT = "LastLogcat";

	@Override
	protected void onResume() {
		log.i("CoolReader.onResume()");
		mPaused = false;
		mIsStarted = true;
		backlightControl.onUserActivity();
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
			Display display = ((WindowManager) getSystemService(WINDOW_SERVICE)).getDefaultDisplay();
			if (null != display) {
				onScreenRotationChanged(display.getRotation());
			}
		}
		super.onResume();
	}

	private boolean mIsStarted = false;
	private boolean mPaused = false;

	public boolean isStarted() {
		return mIsStarted;
	}

	private String mVersion = "3.1";

	public String getVersion() {
		return mVersion;
	}

	public void rebaseSettings() {
		// if rootFs changed (for example, when external storage permission firstly granted) open config from new root
		Properties oldProps = mSettingsManager.mSettings;
		mSettingsManager.rebaseSettings();
		onSettingsChanged(settings(), oldProps);
	}

	public Properties loadSettings(int profile) {
		return mSettingsManager.loadSettings(profile);
	}

	public void saveSettings(int profile, Properties settings) {
		mSettingsManager.saveSettings(profile, settings);
	}

	public void saveSettings(File f, Properties settings) {
		mSettingsManager.saveSettings(f, settings);
	}

	public int getPalmTipPixels() {
		return densityDpi / 3; // 1/3"
	}

	public int getDensityDpi() {
		return densityDpi;
	}

	public float getDensityFactor() {
		return ((float) densityDpi) / 160f;
	}

	public float getDiagonalInches() {
		return diagonalInches;
	}

	public String getSettingsFile(int profile) {
		File file = mSettingsManager.getSettingsFile(profile);
		if (file.exists())
			return file.getAbsolutePath();
		return null;
	}

	public boolean isSmartphone() {
		return diagonalInches <= 6.8; //5.8;
	}

	private int densityDpi = 160;
	private float diagonalInches = 5;


	private InterfaceTheme currentTheme = null;

	public InterfaceTheme getCurrentTheme() {
		return currentTheme;
	}

	public void setCurrentTheme(String themeCode) {
		InterfaceTheme theme = InterfaceTheme.findByCode(themeCode);
		if (null == theme)
			theme = DeviceInfo.FORCE_HC_THEME ? InterfaceTheme.HICONTRAST1 : InterfaceTheme.LIGHT;
		if (currentTheme != theme) {
			setCurrentTheme(theme);
		}
	}

	private int preferredItemHeight = 36;

	public int getPreferredItemHeight() {
		return preferredItemHeight;
	}

	private int minFontSize = 9;

	public int getMinFontSize() {
		return minFontSize;
	}

	private int maxFontSize = 90;

	public int getMaxFontSize() {
		return maxFontSize;
	}

	public void updateBackground() {
		TypedArray a = getTheme().obtainStyledAttributes(new int[]{android.R.attr.windowBackground, android.R.attr.background, android.R.attr.textColor, android.R.attr.colorBackground, android.R.attr.colorForeground, android.R.attr.listPreferredItemHeight});
		int bgRes = a.getResourceId(0, 0);
		//int clText = a.getColor(1, 0);
		int clBackground = a.getColor(2, 0);
		//int clForeground = a.getColor(3, 0);
		preferredItemHeight = densityDpi / 3; //a.getDimensionPixelSize(5, 36);
		//View contentView = getContentView();
		if (contentView != null) {
			if (bgRes != 0) {
				//Drawable d = getResources().getDrawable(bgRes);
				//log.v("Setting background resource " + d.getIntrinsicWidth() + "x" + d.getIntrinsicHeight());
				//contentView.setBackgroundResource(null);
				contentView.setBackgroundResource(bgRes);
				//getWindow().setBackgroundDrawableResource(bgRes);//Drawable(d);
				//getWindow().setBackgroundDrawable(d);
			} else if (clBackground != 0) {
				contentView.setBackgroundColor(clBackground);
				//getWindow().setBackgroundDrawable(Utils.solidColorDrawable(clBackground));
			}
		} else {
//			if (bgRes != 0)
//				getWindow().setBackgroundDrawableResource(bgRes);
//			else if (clBackground != 0)
//				getWindow().setBackgroundDrawable(Utils.solidColorDrawable(clBackground));
		}
		a.recycle();
	}

	@SuppressLint("ResourceType")
	public void updateActionsIcons() {
		int[] attrs = {R.attr.cr3_button_prev_drawable, R.attr.cr3_button_next_drawable, R.attr.cr3_viewer_toc_drawable,
				R.attr.cr3_viewer_find_drawable, R.attr.cr3_viewer_settings_drawable, R.attr.cr3_button_bookmarks_drawable,
				R.attr.cr3_browser_folder_root_drawable, R.attr.cr3_option_night_drawable, R.attr.cr3_option_touch_drawable,
				R.attr.cr3_button_go_page_drawable, R.attr.cr3_button_go_percent_drawable, R.attr.cr3_browser_folder_drawable,
				R.attr.cr3_button_tts_drawable, R.attr.cr3_browser_folder_recent_drawable, R.attr.cr3_button_scroll_go_drawable,
				R.attr.cr3_btn_books_swap_drawable, R.attr.cr3_logo_button_drawable, R.attr.cr3_viewer_exit_drawable,
				R.attr.cr3_button_book_open_drawable, R.attr.cr3_browser_folder_current_book_drawable, R.attr.cr3_browser_folder_opds_drawable,
				/*R.attr.google_drive_drawable,*/ R.attr.cr3_button_log_drawable, R.attr.cr3_button_light_drawable };
		TypedArray a = getTheme().obtainStyledAttributes(attrs);
		int btnPrevDrawableRes = a.getResourceId(0, 0);
		int btnNextDrawableRes = a.getResourceId(1, 0);
		int viewerTocDrawableRes = a.getResourceId(2, 0);
		int viewerFindDrawableRes = a.getResourceId(3, 0);
		int viewerSettingDrawableRes = a.getResourceId(4, 0);
		int btnBookmarksDrawableRes = a.getResourceId(5, 0);
		int brFolderRootDrawableRes = a.getResourceId(6, 0);
		int optionNightDrawableRes = a.getResourceId(7, 0);
		int optionTouchDrawableRes = a.getResourceId(8, 0);
		int btnGoPageDrawableRes = a.getResourceId(9, 0);
		int btnGoPercentDrawableRes = a.getResourceId(10, 0);
		int brFolderDrawableRes = a.getResourceId(11, 0);
		int btnTtsDrawableRes = a.getResourceId(12, 0);
		int brFolderRecentDrawableRes = a.getResourceId(13, 0);
		int btnScrollGoDrawableRes = a.getResourceId(14, 0);
		int btnBooksSwapDrawableRes = a.getResourceId(15, 0);
		int logoBtnDrawableRes = a.getResourceId(16, 0);
		int viewerExitDrawableRes = a.getResourceId(17, 0);
		int btnBookOpenDrawableRes = a.getResourceId(18, 0);
		int brFolderCurrBookDrawableRes = a.getResourceId(19, 0);
		int brFolderOpdsDrawableRes = a.getResourceId(20, 0);
		//int googleDriveDrawableRes = a.getResourceId(21, 0);
		int btnLogDrawableRes = a.getResourceId(22, 0);
		int btnLightDrawableRes = a.getResourceId(23, 0);
		a.recycle();
		if (btnPrevDrawableRes != 0) {
			ReaderAction.GO_BACK.setIconId(btnPrevDrawableRes);
			ReaderAction.FILE_BROWSER_UP.setIconId(btnPrevDrawableRes);
		}
		if (btnNextDrawableRes != 0)
			ReaderAction.GO_FORWARD.setIconId(btnNextDrawableRes);
		if (viewerTocDrawableRes != 0)
			ReaderAction.TOC.setIconId(viewerTocDrawableRes);
		if (viewerFindDrawableRes != 0)
			ReaderAction.SEARCH.setIconId(viewerFindDrawableRes);
		if (viewerSettingDrawableRes != 0)
			ReaderAction.OPTIONS.setIconId(viewerSettingDrawableRes);
		if (btnBookmarksDrawableRes != 0)
			ReaderAction.BOOKMARKS.setIconId(btnBookmarksDrawableRes);
		if (brFolderRootDrawableRes != 0)
			ReaderAction.FILE_BROWSER_ROOT.setIconId(brFolderRootDrawableRes);
		if (optionNightDrawableRes != 0)
			ReaderAction.TOGGLE_DAY_NIGHT.setIconId(optionNightDrawableRes);
		if (optionTouchDrawableRes != 0)
			ReaderAction.TOGGLE_SELECTION_MODE.setIconId(optionTouchDrawableRes);
		if (btnGoPageDrawableRes != 0)
			ReaderAction.GO_PAGE.setIconId(btnGoPageDrawableRes);
		if (btnGoPercentDrawableRes != 0)
			ReaderAction.GO_PERCENT.setIconId(btnGoPercentDrawableRes);
		if (brFolderDrawableRes != 0)
			ReaderAction.FILE_BROWSER.setIconId(brFolderDrawableRes);
		if (btnTtsDrawableRes != 0)
			ReaderAction.TTS_PLAY.setIconId(btnTtsDrawableRes);
		if (brFolderRecentDrawableRes != 0)
			ReaderAction.RECENT_BOOKS.setIconId(brFolderRecentDrawableRes);
		if (btnScrollGoDrawableRes != 0)
			ReaderAction.TOGGLE_AUTOSCROLL.setIconId(btnScrollGoDrawableRes);
		if (btnBooksSwapDrawableRes != 0)
			ReaderAction.OPEN_PREVIOUS_BOOK.setIconId(btnBooksSwapDrawableRes);
		if (logoBtnDrawableRes != 0)
			ReaderAction.ABOUT.setIconId(logoBtnDrawableRes);
		if (viewerExitDrawableRes != 0)
			ReaderAction.EXIT.setIconId(viewerExitDrawableRes);
		if (btnBookOpenDrawableRes != 0)
			ReaderAction.CURRENT_BOOK.setIconId(btnBookOpenDrawableRes);
		if (brFolderCurrBookDrawableRes != 0)
			ReaderAction.CURRENT_BOOK_DIRECTORY.setIconId(brFolderCurrBookDrawableRes);
		if (brFolderOpdsDrawableRes != 0)
			ReaderAction.OPDS_CATALOGS.setIconId(brFolderOpdsDrawableRes);
		/*
		if (googleDriveDrawableRes != 0) {
			ReaderAction.GDRIVE_SYNCTO.setIconId(googleDriveDrawableRes);
			ReaderAction.GDRIVE_SYNCFROM.setIconId(googleDriveDrawableRes);
		}
		 */
		if (btnLogDrawableRes != 0)
			ReaderAction.SAVE_LOGCAT.setIconId(btnLogDrawableRes);
		if (btnLightDrawableRes != 0)
			ReaderAction.SHOW_SYSTEM_BACKLIGHT_DIALOG.setIconId(btnLightDrawableRes);
	}

	public void setCurrentTheme(InterfaceTheme theme) {
		log.i("setCurrentTheme(" + theme + ")");
		currentTheme = theme;
		getApplication().setTheme(theme.getThemeId());
		setTheme(theme.getThemeId());
		updateBackground();
		updateActionsIcons();
	}

	int screenOrientation = ActivityInfo.SCREEN_ORIENTATION_USER;

	public void applyScreenOrientation(Window wnd) {
		if (wnd != null) {
			WindowManager.LayoutParams attrs = wnd.getAttributes();
			attrs.screenOrientation = screenOrientation;
			wnd.setAttributes(attrs);
			if (DeviceInfo.EINK_SCREEN) {
				//TODO:
				//EinkScreen.ResetController(mReaderView);
			}
		}
	}

	public int getScreenOrientation() {
		switch (screenOrientation) {
			case ActivityInfo.SCREEN_ORIENTATION_PORTRAIT:
				return 0;
			case ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE:
				return 1;
			case ActivityInfo_SCREEN_ORIENTATION_REVERSE_PORTRAIT:
				return 2;
			case ActivityInfo_SCREEN_ORIENTATION_REVERSE_LANDSCAPE:
				return 3;
			case ActivityInfo.SCREEN_ORIENTATION_USER:
				return 5;
			default:
				return orientationFromSensor;
		}
	}

	@TargetApi(Build.VERSION_CODES.GINGERBREAD)
	private boolean isReverseLandscape() {
		return screenOrientation == ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
	}

	public boolean isLandscape() {
		if (screenOrientation == ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE)
			return true;
		if (DeviceInfo.getSDKLevel() >= 9 && isReverseLandscape())
			return true;
		return false;
	}

	// support pre API LEVEL 9
	final static public int ActivityInfo_SCREEN_ORIENTATION_SENSOR_PORTRAIT = 7;
	final static public int ActivityInfo_SCREEN_ORIENTATION_SENSOR_LANDSCAPE = 6;
	final static public int ActivityInfo_SCREEN_ORIENTATION_REVERSE_PORTRAIT = 9;
	final static public int ActivityInfo_SCREEN_ORIENTATION_REVERSE_LANDSCAPE = 8;
	final static public int ActivityInfo_SCREEN_ORIENTATION_FULL_SENSOR = 10;

	public void setScreenOrientation(int angle) {
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
			case 5:
				newOrientation = ActivityInfo.SCREEN_ORIENTATION_USER;
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

	public int getOrientationFromSensor() {
		return orientationFromSensor;
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		// pass
		orientationFromSensor = newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE ? 1 : 0;
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
			Display display = ((WindowManager) getSystemService(WINDOW_SERVICE)).getDefaultDisplay();
			if (null != display) {
				onScreenRotationChanged(display.getRotation());
			}
		}
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

	protected void onScreenRotationChanged(int rotation) {
		// Override this...
	}

	private boolean mFullscreen = false;

	public boolean isFullscreen() {
		return mFullscreen;
	}

	public void applyFullscreen(Window wnd) {
		if (mFullscreen) {
			//mActivity.getWindow().requestFeature(Window.)
			wnd.setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
					WindowManager.LayoutParams.FLAG_FULLSCREEN);
		} else {
			wnd.setFlags(0, WindowManager.LayoutParams.FLAG_FULLSCREEN);
		}
		// enforce new window ui visibility flags
		lastSystemUiVisibility = -1;
		setSystemUiVisibility();
	}

	public void setFullscreen(boolean fullscreen) {
		if (mFullscreen != fullscreen) {
			mFullscreen = fullscreen;
			applyFullscreen(getWindow());
		}
	}


//	public void simulateTouch() {
//		// Obtain MotionEvent object
//		long downTime = SystemClock.uptimeMillis();
//		long eventTime = SystemClock.uptimeMillis() + 10;
//		float x = 0.0f;
//		float y = 0.0f;
//		// List of meta states found here: developer.android.com/reference/android/view/KeyEvent.html#getMetaState()
//		int metaState = 0;
//		MotionEvent motionEvent = MotionEvent.obtain(
//		    downTime, 
//		    downTime, 
//		    MotionEvent.ACTION_DOWN, 
//		    x, 
//		    y, 
//		    metaState
//		);
//		MotionEvent motionEvent2 = MotionEvent.obtain(
//			    downTime, 
//			    eventTime, 
//			    MotionEvent.ACTION_UP, 
//			    x, 
//			    y, 
//			    metaState
//			);
//		//motionEvent.setEdgeFlags(flags)
//		// Dispatch touch event to view
//		//new Handler().dispatchMessage(motionEvent);
//		if (getContentView() != null) {
//			getContentView().dispatchTouchEvent(motionEvent);
//			getContentView().dispatchTouchEvent(motionEvent2);
//		}
//
//	}

	protected boolean wantHideNavbarInFullscreen() {
		return false;
	}

	@TargetApi(Build.VERSION_CODES.HONEYCOMB)
	@SuppressLint("NewApi")
	public boolean setSystemUiVisibility() {
		if (DeviceInfo.getSDKLevel() >= DeviceInfo.HONEYCOMB) {
			int flags = 0;
			if (getKeyBacklight() == 0) {
				if (DeviceInfo.getSDKLevel() < 19)
					// backlight of hardware buttons enabled/disabled
					// in updateButtonsBrightness(), turnOffKeyBacklight(), turnOnKeyBacklight()
					// entry point onUserActivity().
					// This flag just shade software navigation bar and system UI
					flags |= View.SYSTEM_UI_FLAG_LOW_PROFILE;
			}
			if (isFullscreen() /*&& wantHideNavbarInFullscreen() && isSmartphone()*/) {
				if (DeviceInfo.getSDKLevel() >= 19)
					// Flag View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY added in API 19
					// without this flag  SYSTEM_UI_FLAG_HIDE_NAVIGATION will be force cleared by the system on any user interaction,
					// and SYSTEM_UI_FLAG_FULLSCREEN will be force-cleared by the system if the user swipes from the top of the screen.
					// So use this flags only on API >= 19
					flags |= View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
							View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
							View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
							View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
							View.SYSTEM_UI_FLAG_FULLSCREEN |
							View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
				else
					flags |= View.SYSTEM_UI_FLAG_LOW_PROFILE;
			}
			setSystemUiVisibility(flags);
//			if (isFullscreen() && DeviceInfo.getSDKLevel() >= DeviceInfo.ICE_CREAM_SANDWICH)
//				simulateTouch();
			return true;
		}
		return false;
	}


	private int lastSystemUiVisibility = -1;
	private boolean systemUiVisibilityListenerIsSet = false;

	@TargetApi(Build.VERSION_CODES.HONEYCOMB)
	@SuppressLint("NewApi")
	private boolean setSystemUiVisibility(int value) {
		if (DeviceInfo.getSDKLevel() >= DeviceInfo.HONEYCOMB) {
			if (!systemUiVisibilityListenerIsSet && null != mDecorView) {
				mDecorView.setOnSystemUiVisibilityChangeListener(visibility -> lastSystemUiVisibility = visibility);
				systemUiVisibilityListenerIsSet = true;
			}
			boolean a4 = DeviceInfo.getSDKLevel() >= DeviceInfo.ICE_CREAM_SANDWICH;
			if (!a4)
				value &= View.SYSTEM_UI_FLAG_LOW_PROFILE;
			if (value == lastSystemUiVisibility)// && a4)
				return false;
			//lastSystemUiVisibility = value;

			if (null == mDecorView)
				return false;
			try {
				Method m = mDecorView.getClass().getMethod("setSystemUiVisibility", int.class);
				m.invoke(mDecorView, value);
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

	public int getScreenBacklightLevel() {
		if (!DeviceInfo.EINK_SCREEN)
			return screenBacklightBrightness;
		else if (DeviceInfo.EINK_HAVE_FRONTLIGHT) {
			// on E-INK devices fetch the system backlight level
			return mEinkScreen.getFrontLightValue(this);
		}
		return 0;
	}

	public int getWarmBacklightLevel() {
		if (!DeviceInfo.EINK_SCREEN)
			return screenWarmBacklightBrightness;
		else if (DeviceInfo.EINK_HAVE_NATURAL_BACKLIGHT)
			return mEinkScreen.getWarmLightValue(this);
		return 0;
	}

	public void setScreenBacklightLevel(int value) {
		if (value < -1)
			value = -1;
		else if (value > DeviceInfo.MAX_SCREEN_BRIGHTNESS_VALUE)
			value = -1;
		screenBacklightBrightness = value;
		if (!DeviceInfo.EINK_SCREEN)
			onUserActivity();
		else if (null != mEinkScreen)
			mEinkScreen.setFrontLightValue(this, value);
	}

	public void setScreenWarmBacklightLevel(int value) {
		if (value < -1)
			value = -1;
		else if (value > DeviceInfo.MAX_SCREEN_BRIGHTNESS_WARM_VALUE)
			value = -1;
		if (null != mEinkScreen) {
			if (mEinkScreen.setWarmLightValue(this, value))
				screenWarmBacklightBrightness = value;
		}
	}

	private int screenBacklightBrightness = -1; // use default
	private int screenWarmBacklightBrightness = -1; // use default
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
		Runnable task = () -> {
			if (!isStarted())
				return;
			if (!Engine.getInstance(BaseActivity.this).setKeyBacklight(0)) {
				//log.w("Cannot control key backlight directly (delayed)");
			}
		};
		BackgroundThread.instance().postGUI(task, 1);
		//BackgroundThread.instance().postGUI(task, 10);
	}

	private void turnOnKeyBacklight() {
		if (!isStarted())
			return;
		setKeyBacklight(1);
		// repeat again in short interval
		if (!Engine.getInstance(this).setKeyBacklight(1)) {
			//log.w("Cannot control key backlight directly");
			return;
		}
		// repeat again in short interval
		Runnable task = () -> {
			if (!isStarted())
				return;
			if (!Engine.getInstance(BaseActivity.this).setKeyBacklight(1)) {
				//log.w("Cannot control key backlight directly (delayed)");
			}
		};
		BackgroundThread.instance().postGUI(task, 1);
		//BackgroundThread.instance().postGUI(task, 10);
	}

	private void updateBacklightBrightness(float b) {
		Window wnd = getWindow();
		if (wnd != null) {
			LayoutParams attrs = wnd.getAttributes();
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
			if (changed) {
				log.d("Window attribute changed: " + attrs);
				wnd.setAttributes(attrs);
			}
		}
	}

	private void updateButtonsBrightness(float buttonBrightness) {
		Window wnd = getWindow();
		if (wnd != null) {
			LayoutParams attrs = wnd.getAttributes();
			boolean changed = false;
			// hack to set buttonBrightness field
			//float buttonBrightness = keyBacklightOff ? 0.0f : -1.0f;
			if (!brightnessHackError)
				try {
					Field bb = attrs.getClass().getField("buttonBrightness");
					if (bb != null) {
						Float oldValue = (Float) bb.get(attrs);
						if (oldValue == null || oldValue.floatValue() != buttonBrightness) {
							bb.set(attrs, buttonBrightness);
							changed = true;
						}
					}
				} catch (Exception e) {
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
			else
				turnOnKeyBacklight();
		}
	}

	private final static int MIN_BACKLIGHT_LEVEL_PERCENT = DeviceInfo.MIN_SCREEN_BRIGHTNESS_VALUE;

	protected void setDimmingAlpha(int alpha) {
		// override it
	}

	protected boolean allowLowBrightness() {
		// override to force higher brightness in non-reading mode (to avoid black screen on some devices when brightness level set to small value)
		return true;
	}

	private final static int MIN_BRIGHTNESS_IN_BROWSER = 12;

	public void onUserActivity() {
		if (backlightControl != null)
			backlightControl.onUserActivity();
		// Hack
		//if ( backlightControl.isHeld() )
		BackgroundThread.instance().executeGUI(() -> {
			try {
				float b;
				int dimmingAlpha = 255;
				// screenBacklightBrightness is 0..100
				if (screenBacklightBrightness >= 0) {
					int percent = screenBacklightBrightness;
					if (!allowLowBrightness() && percent < MIN_BRIGHTNESS_IN_BROWSER)
						percent = MIN_BRIGHTNESS_IN_BROWSER;
					float minb = MIN_BACKLIGHT_LEVEL_PERCENT / 100.0f;
					if (percent >= 10) {
						// real brightness control, no colors dimming
						b = (percent - 10) / (100.0f - 10.0f); // 0..1
						b = minb + b * (1 - minb); // minb..1
						if (b < minb) // BRIGHTNESS_OVERRIDE_OFF
							b = minb;
						else if (b > 1.0f)
							b = 1.0f; //BRIGHTNESS_OVERRIDE_FULL
					} else {
						// minimal brightness with colors dimming
						b = minb;
						dimmingAlpha = 255 - (11 - percent) * 180 / 10;
					}
				} else {
					// system
					b = -1.0f; //BRIGHTNESS_OVERRIDE_NONE
				}
				setDimmingAlpha(dimmingAlpha);
				//log.v("Brightness: " + b + ", dim: " + dimmingAlpha);
				updateBacklightBrightness(b);
				updateButtonsBrightness(keyBacklightOff ? 0.0f : -1.0f);
			} catch (Exception e) {
				// ignore
			}
		});
	}


	public boolean isWakeLockEnabled() {
		return screenBacklightDuration > 0;
	}

	/**
	 * @param backlightDurationMinutes 0 = system default, 1 == 3 minutes, 2..5 == 2..5 minutes
	 */
	public void setScreenBacklightDuration(int backlightDurationMinutes) {
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
						/* | PowerManager.ON_AFTER_RELEASE */, "cr3:wakelock");
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

		}

		;

	}

	public void releaseBacklightControl() {
		backlightControl.release();
	}

	ScreenBacklightControl backlightControl = new ScreenBacklightControl();


	private EinkScreen.EinkUpdateMode mScreenUpdateMode = EinkScreen.EinkUpdateMode.Clear;

	public EinkScreen.EinkUpdateMode getScreenUpdateMode() {
		return mScreenUpdateMode;
	}

	public void setScreenUpdateMode(EinkScreen.EinkUpdateMode screenUpdateMode, View view) {
		//if (mReaderView != null) {
		if (null != mEinkScreen) {
			mScreenUpdateMode = screenUpdateMode;
			if (mEinkScreen.getUpdateMode() != screenUpdateMode || mEinkScreen.getUpdateMode() == EinkScreen.EinkUpdateMode.Active) {
				mEinkScreen.setupController(mScreenUpdateMode, mScreenUpdateInterval, view);
			}
		}
		//}
	}

	private int mScreenUpdateInterval = 0;

	public int getScreenUpdateInterval() {
		return mScreenUpdateInterval;
	}

	public void setScreenUpdateInterval(int screenUpdateInterval, View view) {
		if (null != mEinkScreen) {
			mScreenUpdateInterval = screenUpdateInterval;
			if (mEinkScreen.getUpdateInterval() != screenUpdateInterval) {
				mEinkScreen.setupController(mScreenUpdateMode, screenUpdateInterval, view);
			}
		}
	}


	public void showToast(int stringResourceId) {
		showToast(stringResourceId, Toast.LENGTH_LONG);
	}

	public void showToast(int stringResourceId, Object... formatArgs) {
		String s = getString(stringResourceId, formatArgs);
		if (s != null)
			showToast(s, Toast.LENGTH_LONG);
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
			ToastView.showToast(getContentView(), msg, Toast.LENGTH_LONG, settings().getInt(ReaderView.PROP_FONT_SIZE, 20));
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
		//systemUiVisibilityListenerIsSet = false;
		//updateBackground();
		setCurrentTheme(currentTheme);
	}


	private boolean mNightMode = false;

	public boolean isNightMode() {
		return mNightMode;
	}

	public void setNightMode(boolean nightMode) {
		mNightMode = nightMode;
	}


	public ClipboardManager getClipboardmanager() {
		return (ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
	}


	public void showHomeScreen() {
		Intent intent = new Intent(Intent.ACTION_MAIN);
		intent.addCategory(Intent.CATEGORY_HOME);
		startActivity(intent);
	}


	private static String PREF_HELP_FILE = "HelpFile";

	public String getLastGeneratedHelpFileSignature() {
		SharedPreferences pref = getSharedPreferences(PREF_FILE, 0);
		String res = pref.getString(PREF_HELP_FILE, null);
		return res;
	}

	public void setLastGeneratedHelpFileSignature(String v) {
		SharedPreferences pref = getSharedPreferences(PREF_FILE, 0);
		pref.edit().putString(PREF_HELP_FILE, v).commit();
	}


	private String currentLanguage;

	public String getCurrentLanguage() {
		return currentLanguage;
	}

	public void setLanguage(String lang) {
		setLanguage(Lang.byCode(lang));
		// reload Genres Collection
		GenresCollection.reloadGenresFromResource(this);
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


	public void applyAppSetting(String key, String value) {
		boolean flg = "1".equals(value);
		if (key.equals(PROP_APP_FULLSCREEN)) {
			setFullscreen("1".equals(value));
		} else if (key.equals(PROP_APP_LOCALE)) {
			setLanguage(value);
		} else if (key.equals(PROP_APP_KEY_BACKLIGHT_OFF)) {
			setKeyBacklightDisabled(flg);
		} else if (key.equals(PROP_APP_SCREEN_BACKLIGHT_LOCK)) {
			setScreenBacklightDuration(Utils.parseInt(value, 0));
		} else if (key.equals(PROP_NIGHT_MODE)) {
			setNightMode(flg);
		} else if (key.equals(PROP_APP_SCREEN_UPDATE_MODE)) {
			setScreenUpdateMode(EinkScreen.EinkUpdateMode.byCode(Utils.parseInt(value, 0)), getContentView());
		} else if (key.equals(PROP_APP_SCREEN_UPDATE_INTERVAL)) {
			setScreenUpdateInterval(Utils.parseInt(value, 10), getContentView());
		} else if (key.equals(PROP_APP_THEME)) {
			setCurrentTheme(value);
		} else if (key.equals(PROP_APP_SCREEN_ORIENTATION)) {
			setScreenOrientation(Utils.parseInt(value, 0));
		} else if (PROP_APP_SCREEN_BACKLIGHT.equals(key) && !DeviceInfo.EINK_SCREEN) {
			final int n = Utils.parseInt(value, -1, -1, 100);
			// delay before setting brightness
			BackgroundThread.instance().postGUI(() -> BackgroundThread.instance()
					.postBackground(() -> BackgroundThread.instance()
							.postGUI(() -> setScreenBacklightLevel(n))), 100);
		} else if (key.equals(PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS)) {
			Services.getScanner().setHideEmptyDirs(flg);
		}
		// Don't apply screen brightness on e-ink devices on program startup and at any other events
		// On e-ink in ReaderView gesture handlers setScreenBacklightLevel() & setScreenWarmBacklightLevel() called directly
	}

	private boolean noticeDialogVisible = false;
	public void validateSettings() {
		if (noticeDialogVisible)
			return;
		Properties props = settings();
		boolean menuKeyActionFound = false;
		for (SettingsManager.DefKeyAction ka : SettingsManager.DEF_KEY_ACTIONS) {
			if (ReaderAction.READER_MENU.id.equals(ka.action.id)) {
				if (ka.keyCode != KeyEvent.KEYCODE_MENU || hasHardwareMenuKey()) {
					menuKeyActionFound = true;
					break;
				}
			}
		}
		boolean menuTapActionFound = false;
		for (SettingsManager.DefTapAction ka : SettingsManager.DEF_TAP_ACTIONS) {
			String paramName = ka.longPress ? ReaderView.PROP_APP_TAP_ZONE_ACTIONS_TAP + ".long." + ka.zone : ReaderView.PROP_APP_TAP_ZONE_ACTIONS_TAP + "." + ka.zone;
			String value = props.getProperty(paramName);
			if (ReaderAction.READER_MENU.id.equals(value))
				menuTapActionFound = true;
		}
		boolean propToolbarEnabled = props.getInt(Settings.PROP_TOOLBAR_LOCATION, Settings.VIEWER_TOOLBAR_NONE) != Settings.VIEWER_TOOLBAR_NONE;
		boolean toolbarEnabled = ((propToolbarEnabled && !isFullscreen())
				|| (propToolbarEnabled && isFullscreen() && !props.getBool(PROP_TOOLBAR_HIDE_IN_FULLSCREEN, false)));
		if (!menuTapActionFound && !menuKeyActionFound && !toolbarEnabled) {
			showNotice(R.string.inconsistent_options,
					R.string.inconsistent_options_toolbar, () -> {
						// enabled toolbar
						setSetting(PROP_TOOLBAR_LOCATION, String.valueOf(VIEWER_TOOLBAR_SHORT_SIDE), false);
						setSetting(PROP_TOOLBAR_HIDE_IN_FULLSCREEN, String.valueOf(0), true);
					},
					R.string.inconsistent_options_tap_reading_menu, () -> {
						String paramName = ReaderView.PROP_APP_TAP_ZONE_ACTIONS_TAP + ".5";
						setSetting(paramName, ReaderAction.READER_MENU.id, true);
					},
					() -> noticeDialogVisible = false
			);
			noticeDialogVisible = true;
		}
		// TODO: check any other options for compatibility
	}


	public void showNotice(int questionResourceId, final Runnable action, final Runnable cancelAction) {
		NoticeDialog dlg = new NoticeDialog(this, action, cancelAction);
		dlg.setMessage(questionResourceId);
		dlg.show();
	}

	public void showNotice(int questionResourceId, int button1TextRes, final Runnable button1Runnable,
						   int button2TextRes, final Runnable button2Runnable) {
		showNotice(questionResourceId, button1TextRes, button1Runnable, button2TextRes, button2Runnable, null);
	}

	public void showNotice(int questionResourceId, int button1TextRes, final Runnable button1Runnable,
						   int button2TextRes, final Runnable button2Runnable, final Runnable dismissRunnable) {
		NoticeDialog dlg = new NoticeDialog(this, button1TextRes, button1Runnable, button2TextRes, button2Runnable, dismissRunnable);
		dlg.setMessage(questionResourceId);
		dlg.show();
	}

	public void showNotice(int questionResourceId, final Runnable action) {
		NoticeDialog dlg = new NoticeDialog(this, action);
		dlg.setMessage(questionResourceId);
		dlg.show();
	}

	public void showMessage(String title, String message) {
		AlertDialog.Builder dlg = new AlertDialog.Builder(this);
		if (null != title)
			dlg.setTitle(title);
		dlg.setMessage(message);
		dlg.setPositiveButton(R.string.dlg_button_ok, (arg0, arg1) -> {});
		dlg.show();
	}

	public void askConfirmation(int questionResourceId, final Runnable action) {
		askConfirmation(questionResourceId, action, null);
	}

	public void askConfirmation(int questionResourceId, final Runnable action, final Runnable cancelAction) {
		String question = getString(questionResourceId);
		askConfirmation(null, question, action, cancelAction);
	}

	public void askConfirmation(int titleResourceId, int questionResourceId, final Runnable action, final Runnable cancelAction) {
		String title = getString(titleResourceId);
		String question = getString(questionResourceId);
		askConfirmation(title, question, action, cancelAction);
	}

	public void askQuestion(int titleResourceId, int questionResourceId, final Runnable yesAction, final Runnable noAction) {
		String title = getString(titleResourceId);
		String question = getString(questionResourceId);
		askQuestion(title, question, yesAction, noAction);
	}

	public void askConfirmation(String title, String question, final Runnable action, final Runnable cancelAction) {
		AlertDialog.Builder dlg = new AlertDialog.Builder(this);
		if (null != title)
			dlg.setTitle(title);
		dlg.setMessage(question);
		dlg.setPositiveButton(R.string.dlg_button_ok, (arg0, arg1) -> action.run());
		dlg.setNegativeButton(R.string.dlg_button_cancel, (arg0, arg1) -> {
			if (cancelAction != null)
				cancelAction.run();
		});
		dlg.show();
	}

	public void askQuestion(String title, String question, final Runnable yesAction, final Runnable noAction) {
		AlertDialog.Builder dlg = new AlertDialog.Builder(this);
		if (null != title)
			dlg.setTitle(title);
		dlg.setMessage(question);
		dlg.setPositiveButton(R.string.dlg_button_yes, (arg0, arg1) -> yesAction.run());
		dlg.setNegativeButton(R.string.dlg_button_no, (arg0, arg1) -> {
			if (noAction != null)
				noAction.run();
		});
		dlg.show();
	}

	public void askConfirmation(String question, final Runnable action) {
		AlertDialog.Builder dlg = new AlertDialog.Builder(this);
		dlg.setTitle(question);
		dlg.setPositiveButton(R.string.dlg_button_ok, (arg0, arg1) -> action.run());
		dlg.setNegativeButton(R.string.dlg_button_cancel, (arg0, arg1) -> {
			// do nothing
		});
		dlg.show();
	}

	public void directoryUpdated(FileInfo dir) {
		// override it to use
	}

	public void onSettingsChanged(Properties props, Properties oldProps) {
		// override for specific actions
	}

	public void showActionsPopupMenu(final ReaderAction[] actions, final CRToolBar.OnActionHandler onActionHandler) {
		ArrayList<ReaderAction> list = new ArrayList<>(actions.length);
		Collections.addAll(list, actions);
		showActionsPopupMenu(list, onActionHandler);
	}

	public void showActionsPopupMenu(final ArrayList<ReaderAction> actions, final CRToolBar.OnActionHandler onActionHandler) {
		registerForContextMenu(contentView);
		contentView.setOnCreateContextMenuListener((menu, v, menuInfo) -> {
			//populate only it is not populated by children
			if (menu.size() == 0) {
				int order = 0;
				for (final ReaderAction action : actions) {
					MenuItem item = menu.add(0, action.menuItemId, order++, action.nameId);
					item.setOnMenuItemClickListener(item1 -> onActionHandler.onActionSelected(action));
				}
			}
		});
		contentView.showContextMenu();
	}

	public void showBrowserOptionsDialog() {
		OptionsDialog dlg = new OptionsDialog(BaseActivity.this, OptionsDialog.Mode.BROWSER, null, null, null);
		dlg.show();
	}

	private int currentProfile = 0;

	public int getCurrentProfile() {
		if (currentProfile == 0) {
			currentProfile = mSettingsManager.getInt(PROP_PROFILE_NUMBER, 1);
			if (currentProfile < 1 || currentProfile > MAX_PROFILES)
				currentProfile = 1;
		}
		return currentProfile;
	}

	public void setCurrentProfile(int profile) {
		if (profile == 0 || profile == getCurrentProfile())
			return;
		log.i("Switching from profile " + currentProfile + " to " + profile);
		mSettingsManager.saveSettings(currentProfile, null);
		final Properties loadedSettings = mSettingsManager.loadSettings(profile);
		mSettingsManager.setSettings(loadedSettings, 0, true);
		currentProfile = profile;
	}

	public void setSetting(String name, String value, boolean notify) {
		mSettingsManager.setSetting(name, value, notify);
	}

	public void setSettings(Properties settings, int delayMillis, boolean notify) {
		mSettingsManager.setSettings(settings, delayMillis, notify);
	}

	public void mergeSettings(Properties settings, boolean notify) {
		mSettingsManager.mergeSettings(settings, notify);
	}

	public void notifySettingsChanged() {
		setSettings(mSettingsManager.get(), -1, true);
	}

	private static class SettingsManager {

		public static final Logger log = L.create("cr");

		private final BaseActivity mActivity;
		private Properties mSettings;
		private final File defaultSettingsDir;

		public SettingsManager(BaseActivity activity) {
			this.mActivity = activity;
			defaultSettingsDir = activity.getDir("settings", Context.MODE_PRIVATE);
			mSettings = loadSettings();
		}

		public void rebaseSettings() {
			mSettings = loadSettings();
		}

		//int lastSaveId = 0;
		public void setSettings(Properties settings, int delayMillis, boolean notify) {
			Properties oldSettings = mSettings;
			mSettings = new Properties(settings);
			saveSettings(mSettings);
//			if (delayMillis >= 0) {
//				saveSettingsTask.postDelayed(new Runnable() {
//		    		public void run() {
//		    			BackgroundThread.instance().postGUI(new Runnable() {
//		    				@Override
//		    				public void run() {
//		   						saveSettings(mSettings);
//		    				}
//		    			});
//		    		}
//		    	}, delayMillis);
//			}
			if (notify)
				mActivity.onSettingsChanged(mSettings, oldSettings);
		}

		public void mergeSettings(Properties settings, boolean notify) {
			Properties oldSettings = mSettings;
			mSettings = new Properties(oldSettings);
			Set<Entry<Object, Object>> entries = settings.entrySet();
			for (Entry<Object, Object> entry : entries) {
				mSettings.put(entry.getKey(), entry.getValue());
			}
			saveSettings(mSettings);
			if (notify)
				mActivity.onSettingsChanged(mSettings, oldSettings);
		}

		public void setSetting(String name, String value, boolean notify) {
			Properties props = new Properties(mSettings);
			if (value.equals(mSettings.getProperty(name)))
				return;
			props.setProperty(name, value);
			setSettings(props, 1000, notify);
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
				new DefKeyAction(KeyEvent.KEYCODE_BACK, ReaderAction.NORMAL, ReaderAction.GO_BACK),
				new DefKeyAction(KeyEvent.KEYCODE_BACK, ReaderAction.LONG, ReaderAction.EXIT),
				new DefKeyAction(KeyEvent.KEYCODE_BACK, ReaderAction.DOUBLE, ReaderAction.EXIT),
				new DefKeyAction(KeyEvent.KEYCODE_DPAD_CENTER, ReaderAction.NORMAL, ReaderAction.RECENT_BOOKS),
				new DefKeyAction(KeyEvent.KEYCODE_DPAD_CENTER, ReaderAction.LONG, ReaderAction.BOOKMARKS),
				new DefKeyAction(KeyEvent.KEYCODE_DPAD_UP, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
				new DefKeyAction(KeyEvent.KEYCODE_DPAD_DOWN, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
				new DefKeyAction(KeyEvent.KEYCODE_DPAD_UP, ReaderAction.LONG, (DeviceInfo.EINK_SONY ? ReaderAction.PAGE_UP_10 : ReaderAction.REPEAT)),
				new DefKeyAction(KeyEvent.KEYCODE_DPAD_DOWN, ReaderAction.LONG, (DeviceInfo.EINK_SONY ? ReaderAction.PAGE_DOWN_10 : ReaderAction.REPEAT)),
				new DefKeyAction(KeyEvent.KEYCODE_DPAD_LEFT, ReaderAction.NORMAL, (DeviceInfo.NAVIGATE_LEFTRIGHT ? ReaderAction.PAGE_UP : ReaderAction.PAGE_UP_10)),
				new DefKeyAction(KeyEvent.KEYCODE_DPAD_RIGHT, ReaderAction.NORMAL, (DeviceInfo.NAVIGATE_LEFTRIGHT ? ReaderAction.PAGE_DOWN : ReaderAction.PAGE_DOWN_10)),
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

				new DefKeyAction(KeyEvent.KEYCODE_PAGE_UP, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
				new DefKeyAction(KeyEvent.KEYCODE_PAGE_UP, ReaderAction.LONG, ReaderAction.NONE),
				new DefKeyAction(KeyEvent.KEYCODE_PAGE_UP, ReaderAction.DOUBLE, ReaderAction.NONE),
				new DefKeyAction(KeyEvent.KEYCODE_PAGE_DOWN, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
				new DefKeyAction(KeyEvent.KEYCODE_PAGE_DOWN, ReaderAction.LONG, ReaderAction.NONE),
				new DefKeyAction(KeyEvent.KEYCODE_PAGE_DOWN, ReaderAction.DOUBLE, ReaderAction.NONE),

				new DefKeyAction(ReaderView.SONY_DPAD_DOWN_SCANCODE, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
				new DefKeyAction(ReaderView.SONY_DPAD_UP_SCANCODE, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
				new DefKeyAction(ReaderView.SONY_DPAD_DOWN_SCANCODE, ReaderAction.LONG, ReaderAction.PAGE_DOWN_10),
				new DefKeyAction(ReaderView.SONY_DPAD_UP_SCANCODE, ReaderAction.LONG, ReaderAction.PAGE_UP_10),

				new DefKeyAction(KeyEvent.KEYCODE_8, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
				new DefKeyAction(KeyEvent.KEYCODE_2, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
				new DefKeyAction(KeyEvent.KEYCODE_8, ReaderAction.LONG, ReaderAction.PAGE_DOWN_10),
				new DefKeyAction(KeyEvent.KEYCODE_2, ReaderAction.LONG, ReaderAction.PAGE_UP_10),

				new DefKeyAction(ReaderView.KEYCODE_ESCAPE, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
				new DefKeyAction(ReaderView.KEYCODE_ESCAPE, ReaderAction.LONG, ReaderAction.REPEAT),

//		    public static final int KEYCODE_PAGE_BOTTOMLEFT = 0x5d; // fwd
//		    public static final int KEYCODE_PAGE_BOTTOMRIGHT = 0x5f; // fwd
//		    public static final int KEYCODE_PAGE_TOPLEFT = 0x5c; // back
//		    public static final int KEYCODE_PAGE_TOPRIGHT = 0x5e; // back

		};
		// Some key codes on Nook devices conflicted with standard keyboard, for example, KEYCODE_PAGE_BOTTOMLEFT with PAGE_DOWN
		private static DefKeyAction[] DEF_NOOK_KEY_ACTIONS = {
				new DefKeyAction(ReaderView.NOOK_KEY_NEXT_RIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
				new DefKeyAction(ReaderView.NOOK_KEY_SHIFT_DOWN, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
				new DefKeyAction(ReaderView.NOOK_KEY_PREV_LEFT, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
				new DefKeyAction(ReaderView.NOOK_KEY_PREV_RIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
				new DefKeyAction(ReaderView.NOOK_KEY_SHIFT_UP, ReaderAction.NORMAL, ReaderAction.PAGE_UP),

				new DefKeyAction(ReaderView.NOOK_12_KEY_NEXT_LEFT, ReaderAction.NORMAL, (DeviceInfo.EINK_NOOK ? ReaderAction.PAGE_UP : ReaderAction.PAGE_DOWN)),
				new DefKeyAction(ReaderView.NOOK_12_KEY_NEXT_LEFT, ReaderAction.LONG, (DeviceInfo.EINK_NOOK ? ReaderAction.PAGE_UP_10 : ReaderAction.PAGE_DOWN_10)),

				new DefKeyAction(ReaderView.KEYCODE_PAGE_BOTTOMLEFT, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
//			    new DefKeyAction(ReaderView.KEYCODE_PAGE_BOTTOMRIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
				new DefKeyAction(ReaderView.KEYCODE_PAGE_TOPLEFT, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
				new DefKeyAction(ReaderView.KEYCODE_PAGE_TOPRIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
				new DefKeyAction(ReaderView.KEYCODE_PAGE_BOTTOMLEFT, ReaderAction.LONG, ReaderAction.PAGE_UP_10),
//			    new DefKeyAction(ReaderView.KEYCODE_PAGE_BOTTOMRIGHT, ReaderAction.LONG, ReaderAction.PAGE_UP_10),
				new DefKeyAction(ReaderView.KEYCODE_PAGE_TOPLEFT, ReaderAction.LONG, ReaderAction.PAGE_DOWN_10),
				new DefKeyAction(ReaderView.KEYCODE_PAGE_TOPRIGHT, ReaderAction.LONG, ReaderAction.PAGE_DOWN_10),
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
				new DefTapAction(3, true, ReaderAction.TOGGLE_AUTOSCROLL),
				new DefTapAction(6, true, ReaderAction.PAGE_DOWN_10),
				new DefTapAction(7, true, ReaderAction.PAGE_DOWN_10),
				new DefTapAction(8, true, ReaderAction.PAGE_DOWN_10),
				new DefTapAction(9, true, ReaderAction.PAGE_DOWN_10),
				new DefTapAction(5, false, ReaderAction.READER_MENU),
				new DefTapAction(5, true, ReaderAction.OPTIONS),
		};


		private boolean isValidFontFace(String face) {
			String[] fontFaces = Engine.getFontFaceList();
			if (fontFaces == null)
				return true;
			for (String item : fontFaces) {
				if (item.equals(face))
					return true;
			}
			return false;
		}

		private boolean applyDefaultFont(Properties props, String propName, String defFontFace) {
			String currentValue = props.getProperty(propName);
			boolean changed = false;
			if (currentValue == null) {
				currentValue = defFontFace;
				changed = true;
			}
			if (!isValidFontFace(currentValue)) {
				if (isValidFontFace("Droid Sans"))
					currentValue = "Droid Sans";
				else if (isValidFontFace("Roboto"))
					currentValue = "Roboto";
				else if (isValidFontFace("Droid Serif"))
					currentValue = "Droid Serif";
				else if (isValidFontFace("Arial"))
					currentValue = "Arial";
				else if (isValidFontFace("Times New Roman"))
					currentValue = "Times New Roman";
				else if (isValidFontFace("Droid Sans Fallback"))
					currentValue = "Droid Sans Fallback";
				else {
					String[] fontFaces = Engine.getFontFaceList();
					if (fontFaces != null)
						currentValue = fontFaces[0];
				}
				changed = true;
			}
			if (changed)
				props.setProperty(propName, currentValue);
			return changed;
		}

		private boolean applyDefaultFallbackFontList(Properties props, String propName, String defFontList) {
			String currentValue = props.getProperty(propName);
			boolean changed = false;
			if (currentValue == null) {
				currentValue = defFontList;
				changed = true;
			}
			List<String> faces = Arrays.asList(currentValue.split(";"));
			StringBuilder allowedFaces = new StringBuilder();
			Iterator<String> it = faces.iterator();
			while (it.hasNext()) {
				String face = it.next().trim();
				if (isValidFontFace(face)) {
					allowedFaces.append(face);
					if (it.hasNext())
						allowedFaces.append("; ");
				}
			}
			if (!changed)
				changed = !allowedFaces.toString().equals(currentValue);
			if (changed) {
				currentValue = allowedFaces.toString();
				props.setProperty(propName, currentValue);
			}
			return changed;
		}

		public boolean fixFontSettings(Properties props) {
			boolean res = false;
			res = applyDefaultFont(props, ReaderView.PROP_FONT_FACE, DeviceInfo.DEF_FONT_FACE) || res;
			res = applyDefaultFont(props, ReaderView.PROP_STATUS_FONT_FACE, DeviceInfo.DEF_FONT_FACE) || res;
			res = applyDefaultFallbackFontList(props, ReaderView.PROP_FALLBACK_FONT_FACES, "Noto Color Emoji; Droid Sans Fallback; Noto Sans CJK SC; Noto Sans Arabic UI; Noto Sans Devanagari UI; Roboto; FreeSans; FreeSerif; Noto Serif; Noto Sans; Arial Unicode MS") || res;
			return res;
		}

		private void upgradeSettings(Properties props) {
			String oldHyphenCode = props.getProperty("crengine.hyphenation.dictionary.code");
			if (null != oldHyphenCode && oldHyphenCode.length() > 1) {
				String newHyphenValue = props.getProperty(ReaderView.PROP_HYPHENATION_DICT);
				if (null == newHyphenValue || newHyphenValue.length() == 0) {
					if ("RUSSIAN".equals(oldHyphenCode)) {
						newHyphenValue = "Russian_EnUS";
					} else if ("ENGLISH".equals(oldHyphenCode)) {
						newHyphenValue = "English_US";
					} else {
						newHyphenValue = oldHyphenCode.substring(0, 1);
						newHyphenValue = newHyphenValue + oldHyphenCode.substring(1).toLowerCase();
					}
					props.applyDefault(ReaderView.PROP_HYPHENATION_DICT, newHyphenValue);
					props.remove("crengine.hyphenation.dictionary.code");
				}
			}
			String oldEmbolden = props.getProperty(ReaderView.PROP_FONT_WEIGHT_EMBOLDEN_OBSOLETED);
			if (null != oldEmbolden && oldEmbolden.length() > 0) {
				boolean flg = "1".equals(oldEmbolden);
				props.applyDefault(ReaderView.PROP_FONT_BASE_WEIGHT, flg ? 700 : 400);
				props.remove(PROP_FONT_WEIGHT_EMBOLDEN_OBSOLETED);
			}
		}

		public Properties loadSettings(BaseActivity activity, File file) {
			Properties props = new Properties();

			if (file.exists() && !DEBUG_RESET_OPTIONS) {
				try {
					FileInputStream is = new FileInputStream(file);
					props.load(is);
					log.v("" + props.size() + " settings items loaded from file " + propsFile.getAbsolutePath());
				} catch (Exception e) {
					log.e("error while reading settings");
				}
			}

			// default key actions
			for (DefKeyAction ka : DEF_KEY_ACTIONS) {
				props.applyDefault(ka.getProp(), ka.action.id);
			}
			if (DeviceInfo.NOOK_NAVIGATION_KEYS) {
				// Add default key mappings for Nook devices & also override defaults for some keys (PAGE_UP, PAGE_DOWN)
				for (DefKeyAction ka : DEF_NOOK_KEY_ACTIONS) {
					props.applyDefault(ka.getProp(), ka.action.id);
				}
			}

			boolean menuKeyActionFound = false;
			for (DefKeyAction ka : DEF_KEY_ACTIONS) {
				if (ReaderAction.READER_MENU.id.equals(ka.action.id)) {
					menuKeyActionFound = true;
					break;
				}
			}

			boolean menuTapActionFound = false;
			for (DefTapAction ka : DEF_TAP_ACTIONS) {
				String paramName = ka.longPress ? ReaderView.PROP_APP_TAP_ZONE_ACTIONS_TAP + ".long." + ka.zone : ReaderView.PROP_APP_TAP_ZONE_ACTIONS_TAP + "." + ka.zone;
				String value = props.getProperty(paramName);
				if (ReaderAction.READER_MENU.id.equals(value))
					menuTapActionFound = true;
			}

			boolean toolbarEnabled = props.getInt(Settings.PROP_TOOLBAR_LOCATION, Settings.VIEWER_TOOLBAR_NONE) != Settings.VIEWER_TOOLBAR_NONE;

			// default tap zone actions
			for (DefTapAction ka : DEF_TAP_ACTIONS) {
				String paramName = ka.longPress ? ReaderView.PROP_APP_TAP_ZONE_ACTIONS_TAP + ".long." + ka.zone : ReaderView.PROP_APP_TAP_ZONE_ACTIONS_TAP + "." + ka.zone;

				if (ka.zone == 5 && !ka.longPress && !menuTapActionFound && !(activity.hasHardwareMenuKey() && menuKeyActionFound) && !toolbarEnabled) {
					// force assignment of central tap zone
					log.w("force assignment of central tap zone to " + ka.action.id);
					props.setProperty(paramName, ka.action.id);
				} else {
					props.applyDefault(paramName, ka.action.id);
				}
			}

			if (DeviceInfo.EINK_NOOK) {
				props.applyDefault(ReaderView.PROP_PAGE_ANIMATION, ReaderView.PAGE_ANIMATION_NONE);
			} else {
				props.applyDefault(ReaderView.PROP_PAGE_ANIMATION, ReaderView.PAGE_ANIMATION_SLIDE2);
			}

			props.applyDefault(ReaderView.PROP_APP_LOCALE, Lang.DEFAULT.code);

			// By default enable workaround to disable processing of abbreviations at the end of a sentence when using "Google Speech Services".
			props.applyDefault(ReaderView.PROP_APP_TTS_GOOGLE_END_OF_SENTENCE_ABBR, "1");

			props.applyDefault(ReaderView.PROP_APP_TTS_USE_AUDIOBOOK, "1");

			props.applyDefault(ReaderView.PROP_APP_THEME, DeviceInfo.FORCE_HC_THEME ? "HICONTRAST1" : "LIGHT");
			props.applyDefault(ReaderView.PROP_APP_THEME_DAY, DeviceInfo.FORCE_HC_THEME ? "HICONTRAST1" : "LIGHT");
			props.applyDefault(ReaderView.PROP_APP_THEME_NIGHT, DeviceInfo.FORCE_HC_THEME ? "HICONTRAST2" : "DARK");
			props.applyDefault(ReaderView.PROP_APP_SELECTION_PERSIST, "0");
			props.applyDefault(ReaderView.PROP_APP_SCREEN_BACKLIGHT_LOCK, "3");
			if ("1".equals(props.getProperty(ReaderView.PROP_APP_SCREEN_BACKLIGHT_LOCK)))
				props.setProperty(ReaderView.PROP_APP_SCREEN_BACKLIGHT_LOCK, "3");
			props.applyDefault(ReaderView.PROP_APP_MOTION_TIMEOUT, "0");
			props.applyDefault(ReaderView.PROP_APP_BOUNCE_TAP_INTERVAL, "-1");
			props.applyDefault(ReaderView.PROP_APP_BOOK_PROPERTY_SCAN_ENABLED, "1");
			props.applyDefault(ReaderView.PROP_APP_KEY_BACKLIGHT_OFF, DeviceInfo.SAMSUNG_BUTTONS_HIGHLIGHT_PATCH ? "0" : "1");
			props.applyDefault(ReaderView.PROP_LANDSCAPE_PAGES, DeviceInfo.ONE_COLUMN_IN_LANDSCAPE ? "0" : "1");
			//props.applyDefault(ReaderView.PROP_TOOLBAR_APPEARANCE, "0");
			// autodetect best initial font size based on display resolution
			int fontSize = 12*activity.densityDpi/72;			// 12pt
			int statusFontSize = 8*activity.densityDpi/72;		// 8pt
			int hmargin = activity.densityDpi/16;
			int vmargin = activity.densityDpi/32;
			if (DeviceInfo.DEF_FONT_SIZE != null)
				fontSize = DeviceInfo.DEF_FONT_SIZE;

			int statusLocation = props.getInt(PROP_STATUS_LOCATION, VIEWER_STATUS_PAGE_HEADER);
			if (statusLocation == VIEWER_STATUS_BOTTOM || statusLocation == VIEWER_STATUS_TOP)
				statusLocation = VIEWER_STATUS_PAGE_HEADER;
			props.setInt(PROP_STATUS_LOCATION, statusLocation);


			fixFontSettings(props);
			upgradeSettings(props);
			props.applyDefault(ReaderView.PROP_FONT_SIZE, String.valueOf(fontSize));
			props.applyDefault(ReaderView.PROP_FONT_BASE_WEIGHT, 400);
			props.applyDefault(ReaderView.PROP_FONT_HINTING, "2");
			props.applyDefault(ReaderView.PROP_STATUS_FONT_SIZE, DeviceInfo.EINK_NOOK ? "15" : String.valueOf(statusFontSize));
			props.applyDefault(ReaderView.PROP_FONT_COLOR, "#000000");
			props.applyDefault(ReaderView.PROP_FONT_COLOR_DAY, "#000000");
			props.applyDefault(ReaderView.PROP_FONT_COLOR_NIGHT, !DeviceInfo.EINK_SCREEN ? "#D0B070" : "#FFFFFF");
			props.applyDefault(ReaderView.PROP_BACKGROUND_COLOR, "#FFFFFF");
			props.applyDefault(ReaderView.PROP_BACKGROUND_COLOR_DAY, "#FFFFFF");
			props.applyDefault(ReaderView.PROP_BACKGROUND_COLOR_NIGHT, !DeviceInfo.EINK_SCREEN ? "#101010" : "#000000");
			props.applyDefault(ReaderView.PROP_STATUS_FONT_COLOR, "#FF000000"); // don't use separate color
			props.applyDefault(ReaderView.PROP_STATUS_FONT_COLOR_DAY, "#FF000000"); // don't use separate color
			props.applyDefault(ReaderView.PROP_STATUS_FONT_COLOR_NIGHT, "#80000000"); // don't use separate color
			props.setProperty(ReaderView.PROP_ROTATE_ANGLE, "0"); // crengine's rotation will not be user anymore
			props.setProperty(ReaderView.PROP_DISPLAY_INVERSE, "0");
			props.applyDefault(ReaderView.PROP_APP_FULLSCREEN, "0");
			props.applyDefault(ReaderView.PROP_APP_VIEW_AUTOSCROLL_SPEED, "1500");
			props.applyDefault(ReaderView.PROP_APP_SCREEN_BACKLIGHT, "-1");
			props.applyDefault(ReaderView.PROP_APP_SCREEN_WARM_BACKLIGHT, "-1");
			props.applyDefault(ReaderView.PROP_SHOW_BATTERY, "1");
			props.applyDefault(ReaderView.PROP_SHOW_POS_PERCENT, "0");
			props.applyDefault(ReaderView.PROP_SHOW_PAGE_COUNT, "1");
			props.applyDefault(ReaderView.PROP_FONT_KERNING_ENABLED, "0");		// by default disabled
			props.applyDefault(ReaderView.PROP_FONT_SHAPING, "1");				// by default 'Light (HarfBuzz without ligatures)'
			props.applyDefault(ReaderView.PROP_SHOW_TIME, "1");
			props.applyDefault(ReaderView.PROP_FONT_ANTIALIASING, "2");
			props.applyDefault(ReaderView.PROP_APP_GESTURE_PAGE_FLIPPING, "1");
			props.applyDefault(ReaderView.PROP_APP_SHOW_COVERPAGES, "1");
			props.applyDefault(ReaderView.PROP_APP_COVERPAGE_SIZE, "1");
			props.applyDefault(ReaderView.PROP_APP_SCREEN_ORIENTATION, "0"); // "0"
			props.applyDefault(ReaderView.PROP_CONTROLS_ENABLE_VOLUME_KEYS, "1");
			props.applyDefault(ReaderView.PROP_APP_TAP_ZONE_HILIGHT, "0");
			props.applyDefault(ReaderView.PROP_APP_BOOK_SORT_ORDER, FileInfo.DEF_SORT_ORDER.name());
			DictInfo dict = Dictionaries.defaultDictionary();
			props.applyDefault(ReaderView.PROP_APP_DICTIONARY, (dict != null) ? dict.id : "");
			props.applyDefault(ReaderView.PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS, "0");
			props.applyDefault(ReaderView.PROP_APP_FILE_BROWSER_HIDE_EMPTY_GENRES, "0");
			props.applyDefault(ReaderView.PROP_APP_SELECTION_ACTION, "0");
			props.applyDefault(ReaderView.PROP_APP_MULTI_SELECTION_ACTION, "0");

			// Here use mActivity.getDensityDpi() is incorrect
			// since lvrend.cpp assumes a base DPI of 96 and *not* 160 when using the PROP_RENDER_DPI property!
			props.setProperty(ReaderView.PROP_RENDER_DPI, Integer.valueOf((int)(96*mActivity.getDensityFactor())).toString());

			props.applyDefault(ReaderView.PROP_IMG_SCALING_ZOOMOUT_BLOCK_MODE, "1");
			props.applyDefault(ReaderView.PROP_IMG_SCALING_ZOOMIN_BLOCK_MODE, "1");
			props.applyDefault(ReaderView.PROP_IMG_SCALING_ZOOMOUT_INLINE_MODE, "1");
			props.applyDefault(ReaderView.PROP_IMG_SCALING_ZOOMIN_INLINE_MODE, "1");
			props.applyDefault(ReaderView.PROP_IMG_SCALING_ZOOMOUT_BLOCK_SCALE, "0");
			props.applyDefault(ReaderView.PROP_IMG_SCALING_ZOOMIN_BLOCK_SCALE, "0");
			props.applyDefault(ReaderView.PROP_IMG_SCALING_ZOOMOUT_INLINE_SCALE, "0");
			props.applyDefault(ReaderView.PROP_IMG_SCALING_ZOOMIN_INLINE_SCALE, "0");

			props.applyDefault(ReaderView.PROP_PAGE_MARGIN_LEFT, hmargin);
			props.applyDefault(ReaderView.PROP_PAGE_MARGIN_RIGHT, hmargin);
			props.applyDefault(ReaderView.PROP_PAGE_MARGIN_TOP, vmargin);
			props.applyDefault(ReaderView.PROP_PAGE_MARGIN_BOTTOM, vmargin);
			props.applyDefault(ReaderView.PROP_ROUNDED_CORNERS_MARGIN, "0");

			if (DeviceInfo.EINK_SCREEN_REGAL)
				props.applyDefault(ReaderView.PROP_APP_SCREEN_UPDATE_MODE, String.valueOf(EinkScreen.EinkUpdateMode.Regal.code));
			else
				props.applyDefault(ReaderView.PROP_APP_SCREEN_UPDATE_MODE, String.valueOf(EinkScreen.EinkUpdateMode.Clear.code));
			props.applyDefault(ReaderView.PROP_APP_SCREEN_UPDATE_INTERVAL, "10");

			props.applyDefault(ReaderView.PROP_NIGHT_MODE, "0");
			if (DeviceInfo.FORCE_HC_THEME) {
				props.applyDefault(ReaderView.PROP_PAGE_BACKGROUND_IMAGE, Engine.NO_TEXTURE.id);
			} else {
				if (props.getBool(ReaderView.PROP_NIGHT_MODE, false))
					props.applyDefault(ReaderView.PROP_PAGE_BACKGROUND_IMAGE, Engine.DEF_NIGHT_BACKGROUND_TEXTURE);
				else
					props.applyDefault(ReaderView.PROP_PAGE_BACKGROUND_IMAGE, Engine.DEF_DAY_BACKGROUND_TEXTURE);
			}
			props.applyDefault(ReaderView.PROP_PAGE_BACKGROUND_IMAGE_DAY, !DeviceInfo.EINK_SCREEN ? Engine.DEF_DAY_BACKGROUND_TEXTURE : Engine.NO_TEXTURE.id);
			props.applyDefault(ReaderView.PROP_PAGE_BACKGROUND_IMAGE_NIGHT, !DeviceInfo.EINK_SCREEN ? Engine.DEF_NIGHT_BACKGROUND_TEXTURE : Engine.NO_TEXTURE.id);

			props.applyDefault(ReaderView.PROP_FONT_GAMMA, DeviceInfo.EINK_SCREEN ? "1.5" : "1.0");

			props.setProperty(ReaderView.PROP_MIN_FILE_SIZE_TO_CACHE, "100000");
			props.setProperty(ReaderView.PROP_FORCED_MIN_FILE_SIZE_TO_CACHE, "32768");
			props.applyDefault(ReaderView.PROP_HYPHENATION_DICT, Engine.HyphDict.HYPH_RU_RU_EN_US.toString());
			props.applyDefault(ReaderView.PROP_APP_FILE_BROWSER_SIMPLE_MODE, "0");

			props.applyDefault(ReaderView.PROP_TEXTLANG_EMBEDDED_LANGS_ENABLED, "0");
			props.applyDefault(ReaderView.PROP_TEXTLANG_HYPHENATION_ENABLED, "1");
			props.applyDefault(ReaderView.PROP_TEXTLANG_HYPH_SOFT_HYPHENS_ONLY, "0");
			props.applyDefault(ReaderView.PROP_TEXTLANG_HYPH_FORCE_ALGORITHMIC, "0");

			props.applyDefault(ReaderView.PROP_STATUS_LOCATION, !DeviceInfo.EINK_SCREEN ? Settings.VIEWER_STATUS_PAGE_HEADER : Settings.VIEWER_STATUS_PAGE_FOOTER);
			//props.applyDefault(ReaderView.PROP_TOOLBAR_LOCATION, DeviceInfo.getSDKLevel() < DeviceInfo.HONEYCOMB ? Settings.VIEWER_TOOLBAR_NONE : Settings.VIEWER_TOOLBAR_SHORT_SIDE);
			props.applyDefault(ReaderView.PROP_TOOLBAR_LOCATION, Settings.VIEWER_TOOLBAR_NONE);
			props.applyDefault(ReaderView.PROP_TOOLBAR_HIDE_IN_FULLSCREEN, "0");

			/*
			  Commented until the appearance of free implementation of the binding to the Google Drive (R)
			props.applyDefault(ReaderView.PROP_APP_CLOUDSYNC_GOOGLEDRIVE_ENABLED, "0");
			props.applyDefault(ReaderView.PROP_APP_CLOUDSYNC_GOOGLEDRIVE_SETTINGS, "0");
			props.applyDefault(ReaderView.PROP_APP_CLOUDSYNC_GOOGLEDRIVE_BOOKMARKS, "0");
			props.applyDefault(ReaderView.PROP_APP_CLOUDSYNC_GOOGLEDRIVE_CURRENTBOOK_INFO, "0");
			props.applyDefault(ReaderView.PROP_APP_CLOUDSYNC_GOOGLEDRIVE_CURRENTBOOK_BODY, "0");
			props.applyDefault(ReaderView.PROP_APP_CLOUDSYNC_GOOGLEDRIVE_AUTOSAVEPERIOD, "5");		// 5 min.
			props.applyDefault(ReaderView.PROP_APP_CLOUDSYNC_CONFIRMATIONS, "1");
			props.applyDefault(ReaderView.PROP_APP_CLOUDSYNC_DATA_KEEPALIVE, "14");				// 2 weeks
			 */

			if (!DeviceInfo.EINK_SCREEN) {
				props.applyDefault(ReaderView.PROP_APP_HIGHLIGHT_BOOKMARKS, "1");
				props.applyDefault(ReaderView.PROP_HIGHLIGHT_SELECTION_COLOR, "#AAAAAA");
				props.applyDefault(ReaderView.PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT, "#AAAA55");
				props.applyDefault(ReaderView.PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION, "#C07070");
				props.applyDefault(ReaderView.PROP_HIGHLIGHT_SELECTION_COLOR_DAY, "#AAAAAA");
				props.applyDefault(ReaderView.PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT_DAY, "#AAAA55");
				props.applyDefault(ReaderView.PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION_DAY, "#C07070");
				props.applyDefault(ReaderView.PROP_HIGHLIGHT_SELECTION_COLOR_NIGHT, "#808080");
				props.applyDefault(ReaderView.PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT_NIGHT, "#A09060");
				props.applyDefault(ReaderView.PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION_NIGHT, "#906060");
			} else {
				props.applyDefault(ReaderView.PROP_APP_HIGHLIGHT_BOOKMARKS, "2");
				props.applyDefault(ReaderView.PROP_HIGHLIGHT_SELECTION_COLOR, "#808080");
				props.applyDefault(ReaderView.PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT, "#000000");
				props.applyDefault(ReaderView.PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION, "#000000");
			}

			return props;
		}

		public File getSettingsFile(int profile) {
			if (profile == 0)
				return propsFile;
			return new File(propsFile.getAbsolutePath() + ".profile" + profile);
		}

		File propsFile;
		private static final String SETTINGS_FILE_NAME = "cr3.ini";
		private static boolean DEBUG_RESET_OPTIONS = false;

		private Properties loadSettings() {
			File[] dataDirs = Engine.getDataDirectories(null, false, true);
			File existingFile = null;
			for (File dir : dataDirs) {
				File f = new File(dir, SETTINGS_FILE_NAME);
				if (f.exists() && f.isFile()) {
					existingFile = f;
					break;
				}
			}
			if (existingFile != null) {
				propsFile = existingFile;
			} else {
				File propsDir = defaultSettingsDir;
				propsFile = new File(propsDir, SETTINGS_FILE_NAME);
				File dataDir = Engine.getExternalSettingsDir();
				if (dataDir != null) {
					log.d("external settings dir: " + dataDir);
					propsFile = Engine.checkOrMoveFile(dataDir, propsDir, SETTINGS_FILE_NAME);
				} else {
					propsDir.mkdirs();
				}
			}

			Properties props = loadSettings(mActivity, propsFile);

			return props;
		}

		public Properties loadSettings(int profile) {
			File f = getSettingsFile(profile);
			if (!f.exists() && profile != 0)
				f = getSettingsFile(0);
			Properties res = loadSettings(mActivity, f);
			if (profile != 0) {
				res = filterProfileSettings(res);
				res.setInt(Settings.PROP_PROFILE_NUMBER, profile);
			}
			return res;
		}

		public static Properties filterProfileSettings(Properties settings) {
			Properties res = new Properties();
			res.entrySet();
			for (Object k : settings.keySet()) {
				String key = (String) k;
				String value = settings.getProperty(key);
				boolean found = false;
				for (String pattern : Settings.PROFILE_SETTINGS) {
					if (pattern.endsWith("*")) {
						if (key.startsWith(pattern.substring(0, pattern.length() - 1))) {
							found = true;
							break;
						}
					} else if (pattern.equalsIgnoreCase(key)) {
						found = true;
						break;
					} else if (key.startsWith("styles.")) {
						found = true;
						break;
					}
				}
				if (found) {
					res.setProperty(key, value);
				}
			}
			return res;
		}

		public void saveSettings(int profile, Properties settings) {
			if (settings == null)
				settings = mSettings;
			File f = getSettingsFile(profile);
			if (profile != 0) {
				settings = filterProfileSettings(settings);
				settings.setInt(Settings.PROP_PROFILE_NUMBER, profile);
			}
			saveSettings(f, settings);
		}

		DelayedExecutor saveSettingsTask = DelayedExecutor.createBackground("saveSettings");

		public void saveSettings(File f, Properties settings) {
			try {
				log.v("saveSettings()");
				FileOutputStream os = new FileOutputStream(f);
				settings.store(os, "Cool Reader 3 settings");
				log.i("Settings successfully saved to file " + f.getAbsolutePath());
			} catch (Exception e) {
				log.e("exception while saving settings", e);
			}
		}

		public void saveSettings(Properties settings) {
			saveSettings(propsFile, settings);
		}


		public String getSetting(String name) {
			return mSettings.getProperty(name);
		}

		public String getSetting(String name, String defaultValue) {
			return mSettings.getProperty(name, defaultValue);
		}

		public boolean getBool(String name, boolean defaultValue) {
			return mSettings.getBool(name, defaultValue);
		}

		public int getInt(String name, int defaultValue) {
			return mSettings.getInt(name, defaultValue);
		}

		public Properties get() {
			return new Properties(mSettings);
		}

	}


	public static DictInfo[] getDictList() {
		return Dictionaries.getDictList();
	}

	public boolean isPackageInstalled(String packageName) {
		PackageManager pm = getPackageManager();
		try {
			pm.getPackageInfo(packageName, 0); //PackageManager.GET_ACTIVITIES);
			return true;
		} catch (PackageManager.NameNotFoundException e) {
			return false;
		}
	}

	private Boolean hasHardwareMenuKey = null;

	public boolean hasHardwareMenuKey() {
		if (hasHardwareMenuKey == null) {
			ViewConfiguration vc = ViewConfiguration.get(this);
			if (DeviceInfo.getSDKLevel() >= 14) {
				//boolean vc.hasPermanentMenuKey();
				try {
					Method m = vc.getClass().getMethod("hasPermanentMenuKey", new Class<?>[]{});
					try {
						hasHardwareMenuKey = (Boolean) m.invoke(vc, new Object[]{});
					} catch (IllegalArgumentException e) {
						hasHardwareMenuKey = false;
					} catch (IllegalAccessException e) {
						hasHardwareMenuKey = false;
					} catch (InvocationTargetException e) {
						hasHardwareMenuKey = false;
					}
				} catch (NoSuchMethodException e) {
					hasHardwareMenuKey = false;
				}
			}
			if (hasHardwareMenuKey == null) {
				if (DeviceInfo.EINK_SCREEN)
					hasHardwareMenuKey = false;
				else if (DeviceInfo.getSDKLevel() < DeviceInfo.ICE_CREAM_SANDWICH)
					hasHardwareMenuKey = true;
				else
					hasHardwareMenuKey = false;
			}
		}
		return hasHardwareMenuKey;
	}
}
