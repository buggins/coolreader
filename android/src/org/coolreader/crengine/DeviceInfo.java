/*
 * CoolReader for Android
 * Copyright (C) 2011-2015,2019 Vadim Lopatin <coolreader.org@gmail.com>
 * Copyright (C) 2011 a_lone
 * Copyright (C) 2012 madlynx
 * Copyright (C) 2012 klush
 * Copyright (C) 2012 Jeff Doozan <jeff@doozan.com>
 * Copyright (C) 2014 fuero <the_master_of_disaster@gmx.at>
 * Copyright (C) 2018 norbi24 <norbert.bartalsky@gmail.com>
 * Copyright (C) 2018,2020,2021 Aleksey Chernov <valexlin@gmail.com>
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

import android.graphics.PixelFormat;
import android.os.Build;
import android.util.Log;

import java.lang.reflect.Field;

public class DeviceInfo {

	public final static String MANUFACTURER;
	public final static String MODEL;
	public final static String DEVICE;
	public final static String PRODUCT;
	public final static String BRAND;
	public final static int MIN_SCREEN_BRIGHTNESS_VALUE;
	public final static int MAX_SCREEN_BRIGHTNESS_VALUE;
	public final static int MAX_SCREEN_BRIGHTNESS_WARM_VALUE;
	public final static boolean SAMSUNG_BUTTONS_HIGHLIGHT_PATCH;
	public final static boolean EINK_SCREEN;
	public final static boolean EINK_SCREEN_REGAL;
	public final static boolean EINK_HAVE_FRONTLIGHT;
	public final static boolean EINK_HAVE_NATURAL_BACKLIGHT;
	public final static boolean EINK_SCREEN_UPDATE_MODES_SUPPORTED;
	public final static boolean NOOK_NAVIGATION_KEYS;
	public final static boolean EINK_NOOK;
	public final static boolean EINK_NOOK_120;
	public final static boolean EINK_ONYX;
	public final static boolean EINK_DNS;
	public final static boolean EINK_TOLINO;
	public final static boolean FORCE_HC_THEME;
	public final static boolean EINK_SONY;
	public final static boolean EINK_ENERGYSYSTEM;
	public final static boolean SONY_NAVIGATION_KEYS;
	public final static boolean USE_CUSTOM_TOAST;
	public final static boolean AMOLED_SCREEN;
	public final static boolean POCKETBOOK;
	public final static boolean ONYX_BUTTONS_LONG_PRESS_NOT_AVAILABLE;
	public final static boolean ONYX_HAVE_FRONTLIGHT;
	public final static boolean ONYX_HAVE_NATURAL_BACKLIGHT;
	public final static boolean ONYX_HAVE_BRIGHTNESS_SYSTEM_DIALOG;
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
		"*;*;*;tolino",	     "1",
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
		BRAND = getBuildField("BRAND");
		SAMSUNG_BUTTONS_HIGHLIGHT_PATCH = MANUFACTURER.toLowerCase().contentEquals("samsung") &&
		        (MODEL.contentEquals("GT-S5830") || MODEL.contentEquals("GT-S5660")); // More models?
		AMOLED_SCREEN = MANUFACTURER.toLowerCase().contentEquals("samsung") &&
        		(MODEL.toLowerCase().startsWith("gt-i")); // AMOLED screens: GT-IXXXX
		EINK_NOOK = MANUFACTURER.toLowerCase().contentEquals("barnesandnoble") &&
				(PRODUCT.contentEquals("NOOK") || MODEL.contentEquals("NOOK") || MODEL.contentEquals("BNRV350") || MODEL.contentEquals("BNRV300") || MODEL.contentEquals("BNRV500")) &&
				DEVICE.toLowerCase().contentEquals("zoom2");
		EINK_NOOK_120 = EINK_NOOK && (MODEL.contentEquals("BNRV350") || MODEL.contentEquals("BNRV300") || MODEL.contentEquals("BNRV500"));
		EINK_SONY = MANUFACTURER.toLowerCase().contentEquals("sony") && MODEL.startsWith("PRS-T");
		//MANUFACTURER=Onyx, MODEL=*; All ONYX BOOX Readers have e-ink screen
		EINK_ONYX = (MANUFACTURER.toLowerCase().contentEquals("onyx") || MANUFACTURER.toLowerCase().contentEquals("onyx-intl")) &&
				(BRAND.toLowerCase().contentEquals("onyx") || BRAND.toLowerCase().contentEquals("maccentre") || BRAND.toLowerCase().contentEquals("maccenter")) &&
				MODEL.length() > 0;
		EINK_ENERGYSYSTEM = (
			(BRAND.toLowerCase().contentEquals("energysistem")||BRAND.toLowerCase().contentEquals("energysystem")) &&  MODEL.toLowerCase().startsWith("ereader"));
		//MANUFACTURER -DNS, DEVICE -BK6004C, MODEL - DNS Airbook EGH602, PRODUCT - BK6004C
		EINK_DNS = MANUFACTURER.toLowerCase().contentEquals("dns") && MODEL.startsWith("DNS Airbook EGH");

		EINK_TOLINO = (BRAND.toLowerCase().contentEquals("tolino") && (MODEL.toLowerCase().contentEquals("imx50_rdp")) ) || 		// SHINE
				(MODEL.toLowerCase().contentEquals("tolino") && DEVICE.toLowerCase().contentEquals("tolino_vision2")); //Tolino Vision HD4 doesn't show any Brand, only Model=tolino and  DEVICE=tolino_vision2)


		EINK_SCREEN = EINK_SONY || EINK_NOOK || EINK_ENERGYSYSTEM || EINK_DNS || EINK_TOLINO; // TODO: set to true for eink devices like Nook Touch

		// On Onyx Boox Monte Cristo 3 (and possible Monte Cristo, Monte Cristo 2) long press action on buttons are catch by system and not available for application
		// TODO: check this on other ONYX BOOX Readers
		ONYX_BUTTONS_LONG_PRESS_NOT_AVAILABLE = EINK_ONYX;
		boolean onyx_have_frontlight = false;
		boolean onyx_have_natural_backlight = false;
		int onyx_max_screen_brightness_value = 100;
		int onyx_max_screen_brightness_warm_value = 100;
		boolean onyx_support_regal = false;
		boolean onyx_have_brightness_system_dialog = false;
		/*
		 * Support for ONYX devices is disabled until the ONYX SDK is released under a GPL compatible license.
		 * When enabling this code don't forget to update related code in EinkScreenOnyx.java and BaseActivity.java
		 *
		if (EINK_ONYX) {
			OnyxEinkDeviceImpl onyxEinkDevice = OnyxDevice.currentDevice();
			onyx_support_regal = onyxEinkDevice.supportRegal();
			Application app = null;
			try {
				Class<?> clazz = Class.forName("android.app.ActivityThread");
				Method method = clazz.getMethod("currentApplication");
				app = (Application) method.invoke(null);
			} catch (Exception ignored) {}
			if (null != app) {
				onyx_have_frontlight = onyxEinkDevice.hasFLBrightness(app);
				List<Integer> list = null;
				try {
					list = onyxEinkDevice.getFrontLightValueList(app);
				} catch (Exception ignored) {}
				if (list != null && list.size() > 0) {
					onyx_max_screen_brightness_value = list.get(list.size() - 1);
					if (!onyx_have_frontlight) {
						// For ONYX BOOX MC3 and may be other too...
						onyx_have_frontlight = true;
					}
				}
				// natural (cold & warm) backlight support
				onyx_have_natural_backlight = onyxEinkDevice.hasCTMBrightness(app);
				if (onyx_have_natural_backlight) {
					list = onyxEinkDevice.getWarmLightValues(app);
					if (list != null && list.size() > 0) {
						onyx_max_screen_brightness_warm_value = list.get(list.size() - 1);
					}
				}
				if (!onyx_have_frontlight && onyx_have_natural_backlight) {
					onyx_have_frontlight = true;
					list = onyxEinkDevice.getColdLightValues(app);
					if (list != null && list.size() > 0) {
						onyx_max_screen_brightness_value = list.get(list.size() - 1);
					}
				}
			}
			switch (OnyxDevice.currentDeviceType()) {
				case rk31xx:
				case rk32xx:
				case rk33xx:
				case sdm:
					onyx_have_brightness_system_dialog = true;
					break;
			}
			EINK_SCREEN = true; // error, just to say that you need update line above `EINK_SCREEN = ... `
		}
		*/
		ONYX_HAVE_BRIGHTNESS_SYSTEM_DIALOG = onyx_have_brightness_system_dialog;
		ONYX_HAVE_FRONTLIGHT = onyx_have_frontlight;
		ONYX_HAVE_NATURAL_BACKLIGHT = onyx_have_natural_backlight;
		MAX_SCREEN_BRIGHTNESS_VALUE = onyx_max_screen_brightness_value;
		MAX_SCREEN_BRIGHTNESS_WARM_VALUE = onyx_max_screen_brightness_warm_value;

		EINK_SCREEN_REGAL = onyx_support_regal;		// TODO: add other e-ink devices with regal support

		EINK_HAVE_FRONTLIGHT = ONYX_HAVE_FRONTLIGHT; // TODO: add other e-ink devices with frontlight support
		EINK_HAVE_NATURAL_BACKLIGHT = ONYX_HAVE_NATURAL_BACKLIGHT;	// TODO: add other e-ink devices with natural backlight support

		POCKETBOOK = MODEL.toLowerCase().startsWith("pocketbook") || MODEL.toLowerCase().startsWith("obreey");
		
		NOOK_NAVIGATION_KEYS = EINK_NOOK; // TODO: add autodetect
		SONY_NAVIGATION_KEYS = EINK_SONY;
		EINK_SCREEN_UPDATE_MODES_SUPPORTED = EINK_SCREEN && ( EINK_NOOK || EINK_TOLINO || EINK_ONYX ); // TODO: add autodetect
		FORCE_HC_THEME = EINK_SCREEN || MODEL.equalsIgnoreCase("pocketbook vision");
		USE_CUSTOM_TOAST = EINK_SCREEN;
		NOFLIBUSTA = POCKETBOOK;
		NAVIGATE_LEFTRIGHT = POCKETBOOK && DEVICE.startsWith("EP10");
		REVERT_LANDSCAPE_VOLUME_KEYS = POCKETBOOK && DEVICE.startsWith("EP5A");
		MIN_SCREEN_BRIGHTNESS_VALUE = getMinBrightness(AMOLED_SCREEN ? 2 : (getSDKLevel() >= ICE_CREAM_SANDWICH ? 8 : 16));
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
		Log.i("cr3", "DeviceInfo: MANUFACTURER=" + MANUFACTURER + ", MODEL=" + MODEL + ", DEVICE=" + DEVICE + ", PRODUCT=" + PRODUCT + ", BRAND=" + BRAND);
		Log.i("cr3", "DeviceInfo: MIN_SCREEN_BRIGHTNESS_VALUE=" + MIN_SCREEN_BRIGHTNESS_VALUE + "; MAX_SCREEN_BRIGHTNESS_VALUE=" + MAX_SCREEN_BRIGHTNESS_VALUE + "; EINK_SCREEN=" + EINK_SCREEN + "; EINK_SCREEN_REGAL=" + EINK_SCREEN_REGAL + ", AMOLED_SCREEN=" + AMOLED_SCREEN + ", POCKETBOOK=" + POCKETBOOK);
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
	// "manufacturer;model;device;brand", "manufacturer;model;device" or "manufacturer;model" or "manufacturer" 
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
		if (patterns.length >= 4)
			if (!match(BRAND, patterns[3]))
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
