package org.coolreader.crengine;

import java.lang.reflect.Field;

import android.graphics.PixelFormat;
import android.os.Build;
import android.util.Log;

public class DeviceInfo {

	public final static String MANUFACTURER;
	public final static String MODEL;
	public final static String DEVICE;
	public final static String PRODUCT;
	public final static int MIN_SCREEN_BRIGHTNESS_PERCENT;
	public final static boolean SAMSUNG_BUTTONS_HIGHLIGHT_PATCH;
	public final static boolean EINK_SCREEN;
	public final static boolean EINK_SCREEN_UPDATE_MODES_SUPPORTED;
	public final static boolean NOOK_NAVIGATION_KEYS;
	public final static boolean EINK_NOOK;
	public final static boolean EINK_NOOK_120;
	public final static boolean EINK_ONYX;
	public final static boolean EINK_DNS;
	public final static boolean FORCE_LIGHT_THEME;
	public final static boolean EINK_SONY;
	public final static boolean SONY_NAVIGATION_KEYS;
	public final static boolean USE_CUSTOM_TOAST;
	public final static boolean AMOLED_SCREEN;
	public final static boolean POCKETBOOK;
	public final static boolean NOFLIBUSTA;
	public final static boolean NAVIGATE_LEFTRIGHT; // map left/right keys to single page flip
	public final static boolean REVERT_LANDSCAPE_VOLUME_KEYS; // revert volume keys in landscape mode
	public final static android.graphics.Bitmap.Config BUFFER_COLOR_FORMAT;
	public final static boolean USE_OPENGL = true;
	public final static int PIXEL_FORMAT;
	public final static String  DEF_FONT_FACE;
	public final static boolean USE_BITMAP_MEMORY_HACK; // revert volume keys in landscape mode
	public final static Integer DEF_FONT_SIZE;
	public final static boolean ONE_COLUMN_IN_LANDSCAPE;
	
	// minimal screen backlight level percent for different devices
	private static final String[] MIN_SCREEN_BRIGHTNESS_DB = {
		"LGE;LG-P500",       "6", // LG Optimus One
		"samsung;GT-I9003",  "6", // Samsung i9003
		"Samsung;GT-I9000",  "1", // Samsung Galaxy S
		"Samsung;GT-I9100",  "1", // Samsung Galaxy S2
		"Samsung;GT-I9300",  "1", // Samsung Galaxy S3
		"Samsung;GT-I9500",  "1", // Samsung Galaxy S4
		"HTC;HTC EVO 3D*",   "1", // HTC EVO
		"Archos;A70S",       "1", // Archos
		"HTC;HTC Desire",    "6", // HTC Desire
		"HTC;HTC Desire S",  "6",
		"HTC;HTC Incredible*","6",// HTC Incredible, HTC Incredible S
		"HTC;Legend",        "6",
		"LGE;LG-E510",       "6",
		"*;Kindle Fire",     "6",
		"Samsung;GT-S5830",  "6",
		"HUAWEI;U8800",      "6",
		"Motorola;Milestone XT720", "6",
		"Foxconn;PocketBook A10", "3",
		// TODO: more devices here
	};

	public final static int ICE_CREAM_SANDWICH = 14;
	public final static int HONEYCOMB = 11;

	private static int sdkInt = 0;
	public static int getSDKLevel() {
		if (sdkInt > 0)
			return sdkInt;
		// hack for Android 1.5
		sdkInt = 3;
		Field fld;
		try {
			Class<?> cl = android.os.Build.VERSION.class;
			fld = cl.getField("SDK_INT");
			sdkInt = fld.getInt(cl);
			Log.i("cr3", "API LEVEL " + sdkInt + " detected");
		} catch (SecurityException e) {
			// ignore
		} catch (NoSuchFieldException e) {
			// ignore
		} catch (IllegalArgumentException e) {
			// ignore
		} catch (IllegalAccessException e) {
			// ignore
		}
		return sdkInt;
	}
	
	public static boolean supportsActionBar() {
		return getSDKLevel() >= HONEYCOMB;
	}
	
	static {
		MANUFACTURER = getBuildField("MANUFACTURER");
		MODEL = getBuildField("MODEL");
		DEVICE = getBuildField("DEVICE");
		PRODUCT = getBuildField("PRODUCT");
		SAMSUNG_BUTTONS_HIGHLIGHT_PATCH = MANUFACTURER.toLowerCase().contentEquals("samsung") &&
		        (MODEL.contentEquals("GT-S5830") || MODEL.contentEquals("GT-S5660")); // More models?
		AMOLED_SCREEN = MANUFACTURER.toLowerCase().contentEquals("samsung") &&
        		(MODEL.toLowerCase().startsWith("gt-i")); // AMOLED screens: GT-IXXXX
		EINK_NOOK = MANUFACTURER.toLowerCase().contentEquals("barnesandnoble") &&
				(PRODUCT.contentEquals("NOOK") || MODEL.contentEquals("NOOK") || MODEL.contentEquals("BNRV350") || MODEL.contentEquals("BNRV300") || MODEL.contentEquals("BNRV500")) &&
				DEVICE.toLowerCase().contentEquals("zoom2");
		EINK_NOOK_120 = EINK_NOOK && (MODEL.contentEquals("BNRV350") || MODEL.contentEquals("BNRV300") || MODEL.contentEquals("BNRV500"));
		EINK_SONY = MANUFACTURER.toLowerCase().contentEquals("sony") && MODEL.startsWith("PRS-T");
		//MANUFACTURER=Onyx, MODEL=C63ML, DEVICE=C63ML, PRODUCT=C63ML
		EINK_ONYX = MANUFACTURER.toLowerCase().contentEquals("onyx") && MODEL.startsWith("C") && MODEL.endsWith("ML");
		//MANUFACTURER -DNS, DEVICE -BK6004C, MODEL - DNS Airbook EGH602, PRODUCT - BK6004C
		EINK_DNS = MANUFACTURER.toLowerCase().contentEquals("dns") && MODEL.startsWith("DNS Airbook EGH");
		EINK_SCREEN = EINK_SONY || EINK_NOOK || EINK_ONYX || EINK_DNS; // TODO: set to true for eink devices like Nook Touch

		POCKETBOOK = MODEL.toLowerCase().startsWith("pocketbook") || MODEL.toLowerCase().startsWith("obreey");
		
		NOOK_NAVIGATION_KEYS = EINK_NOOK; // TODO: add autodetect
		SONY_NAVIGATION_KEYS = EINK_SONY;
		EINK_SCREEN_UPDATE_MODES_SUPPORTED = EINK_SCREEN && EINK_NOOK; // TODO: add autodetect
		FORCE_LIGHT_THEME = EINK_SCREEN || MODEL.equalsIgnoreCase("pocketbook vision");
		USE_CUSTOM_TOAST = EINK_SCREEN;
		NOFLIBUSTA = POCKETBOOK;
		NAVIGATE_LEFTRIGHT = POCKETBOOK && DEVICE.startsWith("EP10");
		REVERT_LANDSCAPE_VOLUME_KEYS = POCKETBOOK && DEVICE.startsWith("EP5A");
		MIN_SCREEN_BRIGHTNESS_PERCENT = getMinBrightness(AMOLED_SCREEN ? 2 : (getSDKLevel() >= ICE_CREAM_SANDWICH ? 8 : 16));
		//BUFFER_COLOR_FORMAT = getSDKLevel() >= HONEYCOMB ? android.graphics.Bitmap.Config.ARGB_8888 : android.graphics.Bitmap.Config.RGB_565;
		//BUFFER_COLOR_FORMAT = android.graphics.Bitmap.Config.ARGB_8888;
		BUFFER_COLOR_FORMAT = EINK_SCREEN || USE_OPENGL ? android.graphics.Bitmap.Config.ARGB_8888 : android.graphics.Bitmap.Config.RGB_565;
		PIXEL_FORMAT = (DeviceInfo.BUFFER_COLOR_FORMAT == android.graphics.Bitmap.Config.RGB_565) ? PixelFormat.RGB_565 : PixelFormat.RGBA_8888;
		
		DEF_FONT_FACE = getSDKLevel() >= ICE_CREAM_SANDWICH ? "Roboto" : "Droid Sans";
		
		USE_BITMAP_MEMORY_HACK = getSDKLevel() < ICE_CREAM_SANDWICH;
		ONE_COLUMN_IN_LANDSCAPE = POCKETBOOK && DEVICE.endsWith("SURFPAD");
		DEF_FONT_SIZE = POCKETBOOK && DEVICE.endsWith("SURFPAD") ? 18 : null;
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
		Log.i("cr3", "DeviceInfo: MANUFACTURER=" + MANUFACTURER + ", MODEL=" + MODEL + ", DEVICE=" + DEVICE + ", PRODUCT=" + PRODUCT);
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
		String[] patterns = pattern.split("\\|");
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

//	// TEST
//	private static boolean testMatchDevice(String manufacturer, String model, String device, String pattern) {
//		String[] patterns = pattern.split(";");
//		if (patterns.length >= 1)
//			if (!match(manufacturer, patterns[0]))
//				return false;
//		if (patterns.length >= 2)
//			if (!match(model, patterns[1]))
//				return false;
//		if (patterns.length >= 3)
//			if (!match(device, patterns[2]))
//				return false;
//		Log.v("cr3", "matched : " + pattern + " == " + manufacturer + "," + model + "," + device);
//		return true;
//	}
//	
//	static {
//		testMatchDevice("Archos", "A70S", "A70S", "Archos;A70S");
//		testMatchDevice("MegaMan", "A70S", "A70S", "mega*;A70*");
//		testMatchDevice("MegaMan", "A70", "A70S", "*man;A70*");
//	}

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
