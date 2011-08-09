package org.coolreader.crengine;

import android.os.Build;
import android.util.Log;

public class DeviceInfo {

	public final static String MANUFACTURER;
	public final static String MODEL;
	public final static boolean SAMSUNG_BUTTONS_HIGHLIGHT_PATCH;
	public final static boolean EINK_SCREEN;
	public final static boolean NOOK_NAVIGATION_KEYS;
	
	static {
		MANUFACTURER = getBuildField("MANUFACTURER");
		MODEL = getBuildField("MODEL");
		SAMSUNG_BUTTONS_HIGHLIGHT_PATCH = MANUFACTURER.toLowerCase().contentEquals("samsung") &&
		               (MODEL.contentEquals("GT-S5830") || MODEL.contentEquals("GT-S5660")); // More models?
		EINK_SCREEN = false; // TODO: set to true for eink devices like Nook Touch
		NOOK_NAVIGATION_KEYS = false; // TODO: add autodetect
	}
	
	private static String getBuildField(String fieldName) {
		
		try {
			return (String)Build.class.getField(fieldName).get(null);
		} catch (Exception e) {
			Log.d("cr3", "Exception while trying to check Biuild.MANUFACTURER");
			return "";
		}
	}
}
