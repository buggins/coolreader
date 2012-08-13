package org.coolreader.crengine;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;

import org.coolreader.crengine.Settings.Lang;

import android.app.SearchManager;
import android.content.Context;
import android.content.Intent;
import android.util.DisplayMetrics;
import android.view.KeyEvent;

/**
 * Singleton to manage settings.
 * @author lve
 *
 */
public class SettingsManager {

	public static final Logger log = L.create("cr");
	
	private static SettingsManager instance;
	public static SettingsManager instance(BaseActivity activity) {
		if (instance == null)
			instance = new SettingsManager(activity);
		return instance;
	}
	
	private Properties mSettings;
	private boolean isSmartphone;
	
    private final DisplayMetrics displayMetrics = new DisplayMetrics();
    private final File defaultSettingsDir;
	private SettingsManager(BaseActivity activity) {
	    activity.getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
	    defaultSettingsDir = activity.getDir("settings", Context.MODE_PRIVATE);
	    isSmartphone = activity.isSmartphone();
	    mSettings = loadSettings();
	}
	
	public Properties get() {
		return mSettings;
	}

	int lastSaveId = 0;
	public void setSettings(Properties settings, int delayMillis) {
		mSettings = new Properties(settings);
		if (delayMillis >= 0) {
			saveSettingsTask.postDelayed(new Runnable() {
	    		public void run() {
	    			BackgroundThread.instance().postGUI(new Runnable() {
	    				@Override
	    				public void run() {
	   						saveSettings(mSettings);
	    				}
	    			});
	    		}
	    	}, delayMillis);
		}
		Activities.onSettingsChanged(mSettings);
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
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_UP, ReaderAction.LONG, (DeviceInfo.EINK_SONY? ReaderAction.PAGE_UP_10 : ReaderAction.REPEAT)),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_DOWN, ReaderAction.LONG, (DeviceInfo.EINK_SONY? ReaderAction.PAGE_DOWN_10 : ReaderAction.REPEAT)),
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
		
		new DefKeyAction(ReaderView.NOOK_KEY_NEXT_RIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(ReaderView.NOOK_KEY_SHIFT_DOWN, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(ReaderView.NOOK_KEY_PREV_LEFT, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(ReaderView.NOOK_KEY_PREV_RIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(ReaderView.NOOK_KEY_SHIFT_UP, ReaderAction.NORMAL, ReaderAction.PAGE_UP),

		new DefKeyAction(ReaderView.NOOK_12_KEY_NEXT_LEFT, ReaderAction.NORMAL, (DeviceInfo.EINK_NOOK ? ReaderAction.PAGE_UP : ReaderAction.PAGE_DOWN)),
		new DefKeyAction(ReaderView.NOOK_12_KEY_NEXT_LEFT, ReaderAction.LONG, (DeviceInfo.EINK_NOOK ? ReaderAction.PAGE_UP_10 : ReaderAction.PAGE_DOWN_10)),
		
		new DefKeyAction(ReaderView.KEYCODE_PAGE_BOTTOMLEFT, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
//		new DefKeyAction(ReaderView.KEYCODE_PAGE_BOTTOMRIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_TOPLEFT, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_TOPRIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_BOTTOMLEFT, ReaderAction.LONG, ReaderAction.PAGE_UP_10),
//		new DefKeyAction(ReaderView.KEYCODE_PAGE_BOTTOMRIGHT, ReaderAction.LONG, ReaderAction.PAGE_UP_10),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_TOPLEFT, ReaderAction.LONG, ReaderAction.PAGE_DOWN_10),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_TOPRIGHT, ReaderAction.LONG, ReaderAction.PAGE_DOWN_10),
		
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

	public boolean fixFontSettings(Properties props) {
		boolean res = false;
        res = applyDefaultFont(props, ReaderView.PROP_FONT_FACE, DeviceInfo.DEF_FONT_FACE) || res;
        res = applyDefaultFont(props, ReaderView.PROP_STATUS_FONT_FACE, DeviceInfo.DEF_FONT_FACE) || res;
        res = applyDefaultFont(props, ReaderView.PROP_FALLBACK_FONT_FACE, "Droid Sans Fallback") || res;
        return res;
	}
	
	public Properties loadSettings(File file) {
        Properties props = new Properties();

        if ( file.exists() && !DEBUG_RESET_OPTIONS ) {
        	try {
        		FileInputStream is = new FileInputStream(file);
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

        props.applyDefault(ReaderView.PROP_APP_LOCALE, Lang.DEFAULT.code);
        
        props.applyDefault(ReaderView.PROP_APP_THEME, DeviceInfo.FORCE_LIGHT_THEME ? "WHITE" : "LIGHT");
        props.applyDefault(ReaderView.PROP_APP_THEME_DAY, DeviceInfo.FORCE_LIGHT_THEME ? "WHITE" : "LIGHT");
        props.applyDefault(ReaderView.PROP_APP_THEME_NIGHT, DeviceInfo.FORCE_LIGHT_THEME ? "BLACK" : "DARK");
        props.applyDefault(ReaderView.PROP_APP_SELECTION_PERSIST, "0");
        props.applyDefault(ReaderView.PROP_APP_SCREEN_BACKLIGHT_LOCK, "3");
        if ("1".equals(props.getProperty(ReaderView.PROP_APP_SCREEN_BACKLIGHT_LOCK)))
            props.setProperty(ReaderView.PROP_APP_SCREEN_BACKLIGHT_LOCK, "3");
        props.applyDefault(ReaderView.PROP_APP_BOOK_PROPERTY_SCAN_ENABLED, "1");
        props.applyDefault(ReaderView.PROP_APP_KEY_BACKLIGHT_OFF, DeviceInfo.SAMSUNG_BUTTONS_HIGHLIGHT_PATCH ? "0" : "1");
        props.applyDefault(ReaderView.PROP_LANDSCAPE_PAGES, DeviceInfo.ONE_COLUMN_IN_LANDSCAPE ? "0" : "1");
        // autodetect best initial font size based on display resolution
        int screenWidth = displayMetrics.widthPixels;//getWindowManager().getDefaultDisplay().getWidth();
        int fontSize = 20;
        String hmargin = "4";
        String vmargin = "2";
        if ( screenWidth<=320 ) {
        	fontSize = 20;
            hmargin = "4";
            vmargin = "2";
        } else if ( screenWidth<=400 ) {
        	fontSize = 24;
            hmargin = "10";
            vmargin = "4";
        } else if ( screenWidth<=600 ) {
        	fontSize = 28;
            hmargin = "20";
            vmargin = "8";
        } else {
        	fontSize = 32;
            hmargin = "25";
            vmargin = "15";
        }
        if (DeviceInfo.DEF_FONT_SIZE != null)
        	fontSize = DeviceInfo.DEF_FONT_SIZE;

        fixFontSettings(props);
        props.applyDefault(ReaderView.PROP_FONT_SIZE, String.valueOf(fontSize));
        props.applyDefault(ReaderView.PROP_FONT_HINTING, "2");
        props.applyDefault(ReaderView.PROP_STATUS_FONT_SIZE, DeviceInfo.EINK_NOOK ? "15" : "16");
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
        props.applyDefault(ReaderView.PROP_APP_VIEW_AUTOSCROLL_SPEED, "1500");
        props.applyDefault(ReaderView.PROP_APP_SCREEN_BACKLIGHT, "-1");
		props.applyDefault(ReaderView.PROP_SHOW_BATTERY, "1"); 
		props.applyDefault(ReaderView.PROP_SHOW_POS_PERCENT, "0"); 
		props.applyDefault(ReaderView.PROP_SHOW_PAGE_COUNT, "1"); 
		props.applyDefault(ReaderView.PROP_SHOW_TIME, "1");
		props.applyDefault(ReaderView.PROP_FONT_ANTIALIASING, "2");
		props.applyDefault(ReaderView.PROP_APP_GESTURE_PAGE_FLIPPING, "1");
		props.applyDefault(ReaderView.PROP_APP_SHOW_COVERPAGES, "1");
		props.applyDefault(ReaderView.PROP_APP_COVERPAGE_SIZE, "1");
		props.applyDefault(ReaderView.PROP_APP_SCREEN_ORIENTATION, DeviceInfo.EINK_SCREEN ? "0" : "4"); // "0"
		props.applyDefault(ReaderView.PROP_CONTROLS_ENABLE_VOLUME_KEYS, "1");
		props.applyDefault(ReaderView.PROP_APP_TAP_ZONE_HILIGHT, "0");
		props.applyDefault(ReaderView.PROP_APP_BOOK_SORT_ORDER, FileInfo.DEF_SORT_ORDER.name());
		props.applyDefault(ReaderView.PROP_APP_DICTIONARY, dicts[0].id);
		props.applyDefault(ReaderView.PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS, "0");
		props.applyDefault(ReaderView.PROP_APP_SELECTION_ACTION, "0");
		props.applyDefault(ReaderView.PROP_APP_MULTI_SELECTION_ACTION, "0");

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
		
        props.applyDefault(ReaderView.PROP_APP_SCREEN_UPDATE_MODE, "0");
        props.applyDefault(ReaderView.PROP_APP_SCREEN_UPDATE_INTERVAL, "10");
        
        props.applyDefault(ReaderView.PROP_NIGHT_MODE, "0");
        if (DeviceInfo.FORCE_LIGHT_THEME) {
        	props.applyDefault(ReaderView.PROP_PAGE_BACKGROUND_IMAGE, Engine.NO_TEXTURE.id);
        } else {
        	if ( props.getBool(ReaderView.PROP_NIGHT_MODE, false) )
        		props.applyDefault(ReaderView.PROP_PAGE_BACKGROUND_IMAGE, Engine.DEF_NIGHT_BACKGROUND_TEXTURE);
        	else
        		props.applyDefault(ReaderView.PROP_PAGE_BACKGROUND_IMAGE, Engine.DEF_DAY_BACKGROUND_TEXTURE);
        }
        props.applyDefault(ReaderView.PROP_PAGE_BACKGROUND_IMAGE_DAY, Engine.DEF_DAY_BACKGROUND_TEXTURE);
        props.applyDefault(ReaderView.PROP_PAGE_BACKGROUND_IMAGE_NIGHT, Engine.DEF_NIGHT_BACKGROUND_TEXTURE);
        
        props.applyDefault(ReaderView.PROP_FONT_GAMMA, DeviceInfo.EINK_SCREEN ? "1.5" : "1.0");
		
		props.setProperty(ReaderView.PROP_MIN_FILE_SIZE_TO_CACHE, "100000");
		props.setProperty(ReaderView.PROP_FORCED_MIN_FILE_SIZE_TO_CACHE, "32768");
		props.applyDefault(ReaderView.PROP_HYPHENATION_DICT, Engine.HyphDict.RUSSIAN.toString());
		props.applyDefault(ReaderView.PROP_APP_FILE_BROWSER_SIMPLE_MODE, "0");
		
		props.applyDefault(ReaderView.PROP_STATUS_LOCATION, Settings.VIEWER_STATUS_PAGE);
		props.applyDefault(ReaderView.PROP_TOOLBAR_LOCATION, isSmartphone ? Settings.VIEWER_TOOLBAR_NONE : Settings.VIEWER_TOOLBAR_SHORT_SIDE);
		props.applyDefault(ReaderView.PROP_TOOLBAR_HIDE_IN_FULLSCREEN, "0");

		
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
	private Properties loadSettings()
	{
		File[] dataDirs = Engine.getDataDirectories(null, false, true);
		File existingFile = null;
		for ( File dir : dataDirs ) {
			File f = new File(dir, SETTINGS_FILE_NAME);
			if ( f.exists() && f.isFile() ) {
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
        
        Properties props = loadSettings(propsFile);

		return props;
	}

	public Properties loadSettings(int profile) {
		File f = getSettingsFile(profile);
		if (!f.exists() && profile != 0)
			f = getSettingsFile(0);
		Properties res = loadSettings(f);
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
			String key = (String)k;
			String value = settings.getProperty(key);
			boolean found = false;
			for (String pattern : Settings.PROFILE_SETTINGS) {
				if (pattern.endsWith("*")) {
					if (key.startsWith(pattern.substring(0, pattern.length()-1))) {
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
		File f = getSettingsFile(profile);
		if (profile != 0) {
			settings = filterProfileSettings(settings);
			settings.setInt(Settings.PROP_PROFILE_NUMBER, profile);
		}
		saveSettings(f, settings);
	}
	
	DelayedExecutor saveSettingsTask = DelayedExecutor.createBackground("saveSettings"); 
	public void saveSettings(File f, Properties settings)
	{
		try {
			log.v("saveSettings()");
    		FileOutputStream os = new FileOutputStream(f);
    		settings.store(os, "Cool Reader 3 settings");
			log.i("Settings successfully saved to file " + f.getAbsolutePath());
		} catch ( Exception e ) {
			log.e("exception while saving settings", e);
		}
	}

	public void saveSettings(Properties settings)
	{
		saveSettings(propsFile, settings);
	}


	public static class DictInfo {
		public final String id; 
		public final String name;
		public final String packageName;
		public final String className;
		public final String action;
		public final Integer internal;
		public String dataKey = SearchManager.QUERY; 
		public DictInfo ( String id, String name, String packageName, String className, String action, Integer internal ) {
			this.id = id;
			this.name = name;
			this.packageName = packageName;
			this.className = className;
			this.action = action;
			this.internal = internal;
		}
		public DictInfo setDataKey(String key) { this.dataKey = key; return this; }
	}
	
	private static final DictInfo dicts[] = {
		new DictInfo("Fora", "Fora Dictionary", "com.ngc.fora", "com.ngc.fora.ForaDictionary", Intent.ACTION_SEARCH, 0),
		new DictInfo("ColorDict", "ColorDict", "com.socialnmobile.colordict", "com.socialnmobile.colordict.activity.Main", Intent.ACTION_SEARCH, 0),
		new DictInfo("ColorDictApi", "ColorDict new / GoldenDict", "com.socialnmobile.colordict", "com.socialnmobile.colordict.activity.Main", Intent.ACTION_SEARCH, 1),
		new DictInfo("AardDict", "Aard Dictionary", "aarddict.android", "aarddict.android.Article", Intent.ACTION_SEARCH, 0),
		new DictInfo("AardDictLookup", "Aard Dictionary Lookup", "aarddict.android", "aarddict.android.Lookup", Intent.ACTION_SEARCH, 0),
		new DictInfo("Dictan", "Dictan Dictionary", "", "", Intent.ACTION_VIEW, 2),
		new DictInfo("FreeDictionary.org", "Free Dictionary . org", "org.freedictionary.MainActivity", "org.freedictionary", Intent.ACTION_VIEW, 0),
		new DictInfo("LingoQuizLite", "Lingo Quiz Lite", "mnm.lite.lingoquiz", "mnm.lite.lingoquiz.ExchangeActivity", "lingoquiz.intent.action.ADD_WORD", 0).setDataKey("EXTRA_WORD"),
		new DictInfo("LingoQuiz", "Lingo Quiz", "mnm.lingoquiz", "mnm.lingoquiz.ExchangeActivity", "lingoquiz.intent.action.ADD_WORD", 0).setDataKey("EXTRA_WORD"),
		new DictInfo("LEODictionary", "LEO Dictionary", "org.leo.android.dict", "org.leo.android.dict.LeoDict", "android.intent.action.SEARCH", 0),
	};

	public static DictInfo[] getDictList() {
		return dicts;
	}

	public String getSetting( String name ) {
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

}
