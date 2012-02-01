package org.coolreader.crengine;

import android.os.Build;
import android.util.Log;

public class DeviceInfo {

	public final static String MANUFACTURER;
	public final static String MODEL;
	public final static String DEVICE;
	public final static int MIN_SCREEN_BRIGHTNESS_PERCENT;
	public final static boolean SAMSUNG_BUTTONS_HIGHLIGHT_PATCH;
	public final static boolean EINK_SCREEN;
	public final static boolean EINK_SCREEN_UPDATE_MODES_SUPPORTED;
	public final static boolean NOOK_NAVIGATION_KEYS;
	public final static boolean EINK_NOOK;
	public final static boolean FORCE_LIGHT_THEME;
	public final static boolean EINK_SONY;
	public final static boolean SONY_NAVIGATION_KEYS;
	public final static boolean USE_CUSTOM_TOAST;
	public final static boolean AMOLED_SCREEN;
	public final static boolean POCKETBOOK;
	public final static boolean NOFLIBUSTA;
	public final static boolean NAVIGATE_LEFTRIGHT; // map left/right keys to single page flip
	public final static boolean REVERT_LANDSCAPE_VOLUME_KEYS; // revert volume keys in landscape mode

	// minimal screen backlight level percent for different devices
	private static final String[] MIN_SCREEN_BRIGHTNESS_DB = {
		"LGE;LG-P500", "6", // LG Optimus One
		"samsung;GT-I9003", "6", // Samsung i9003
		"HTC;HTC EVO 3D X5*", "1", // HTC EVO
		"Archos;A70S", "1", // Archos
		// TODO: more devices here
	};
	
	static {
		MANUFACTURER = getBuildField("MANUFACTURER");
		MODEL = getBuildField("MODEL");
		DEVICE = getBuildField("DEVICE");
		SAMSUNG_BUTTONS_HIGHLIGHT_PATCH = MANUFACTURER.toLowerCase().contentEquals("samsung") &&
		        (MODEL.contentEquals("GT-S5830") || MODEL.contentEquals("GT-S5660")); // More models?
		AMOLED_SCREEN = MANUFACTURER.toLowerCase().contentEquals("samsung") &&
        		(MODEL.toLowerCase().startsWith("gt-i")); // AMOLED screens: GT-IXXXX
		EINK_NOOK = MANUFACTURER.toLowerCase().contentEquals("barnesandnoble") && MODEL.contentEquals("NOOK") &&
				DEVICE.toLowerCase().contentEquals("zoom2");
		EINK_SONY = MANUFACTURER.toLowerCase().contentEquals("sony") && MODEL.contentEquals("PRS-T1");
		EINK_SCREEN = EINK_SONY || EINK_NOOK; // TODO: set to true for eink devices like Nook Touch

		POCKETBOOK = MODEL.toLowerCase().startsWith("pocketbook");
		
		NOOK_NAVIGATION_KEYS = EINK_NOOK; // TODO: add autodetect
		SONY_NAVIGATION_KEYS = EINK_SONY;
		EINK_SCREEN_UPDATE_MODES_SUPPORTED = EINK_SCREEN && EINK_NOOK; // TODO: add autodetect
		FORCE_LIGHT_THEME = EINK_SCREEN || MODEL.equalsIgnoreCase("pocketbook vision");
		USE_CUSTOM_TOAST = EINK_SCREEN;
		NOFLIBUSTA = POCKETBOOK;
		NAVIGATE_LEFTRIGHT = POCKETBOOK && DEVICE.startsWith("EP10");
		REVERT_LANDSCAPE_VOLUME_KEYS = POCKETBOOK && DEVICE.startsWith("EP5A");
		MIN_SCREEN_BRIGHTNESS_PERCENT = getMinBrightness(AMOLED_SCREEN ? 2 : 16);
	}
	
	private static String getBuildField(String fieldName) {
		
		try {
			return (String)Build.class.getField(fieldName).get(null);
		} catch (Exception e) {
			Log.d("cr3", "Exception while trying to check Build." + fieldName);
			return "";
		}
	}
	
	
	static {
		Log.i("cr3", "DeviceInfo: MANUFACTURER=" + MANUFACTURER + ", MODEL=" + MODEL + ", DEVICE=" + DEVICE);
		Log.i("cr3", "DeviceInfo: MIN_SCREEN_BRIGHTNESS_PERCENT=" + MIN_SCREEN_BRIGHTNESS_PERCENT + ", EINK_SCREEN=" + EINK_SCREEN + ", AMOLED_SCREEN=" + AMOLED_SCREEN + ", POCKETBOOK=" + POCKETBOOK);
	}

	// multiple patterns divided by |, * wildcard can be placed at beginning and/or end of pattern
	// samples: "samsung", "p500|p510", "sgs*|sgh*"
	private static boolean match(String value, String pattern) {
		if (pattern == null || pattern.length() == 0 || "*".equals(pattern))
			return true; // matches any value
		if (value == null || value.length() == 0)
			return false;
		value = value.toLowerCase();
		pattern = pattern.toLowerCase();
		String[] patterns = pattern.split("|");
		for (String p : patterns) {
			boolean startingWildcard = false;
			boolean endingWildcard = false;
			if (p.startsWith("*")) {
				startingWildcard = true;
				p = p.substring(1);
			}
			if (p.endsWith("*")) {
				endingWildcard = true;
				p = p.substring(0, p.length()-1);
			}
			if (startingWildcard && endingWildcard) {
				if (value.indexOf(p) < 0)
					return false;
			} else if (startingWildcard) {
				if (!value.endsWith(p))
					return false;
			} else if (endingWildcard) {
				if (!value.startsWith(p))
					return false;
			} else {
				if (!value.equals(p))
					return false;
			}
		}
		return true;
	}

	// delimited by ;
	// "manufacturer;model;device" or "manufacturer;model" or "manufacturer" 
	private static boolean matchDevice(String pattern) {
		String[] patterns = pattern.split(";");
		if (patterns.length >= 1)
			if (!match(MANUFACTURER, patterns[0]))
				return false;
		if (patterns.length >= 2)
			if (!match(MODEL, patterns[1]))
				return false;
		if (patterns.length >= 3)
			if (!match(DEVICE, patterns[2]))
				return false;
		return true;
	}

	private static int getMinBrightness(int defValue) {
		try {
			for (int i=0; i<MIN_SCREEN_BRIGHTNESS_DB.length - 1; i+=2) {
				if (matchDevice(MIN_SCREEN_BRIGHTNESS_DB[i])) {
					return Integer.valueOf(MIN_SCREEN_BRIGHTNESS_DB[i+1]);
				}
			}
		} catch (NumberFormatException e) {
			// ignore
		}
		return defValue;
	}
	
}
