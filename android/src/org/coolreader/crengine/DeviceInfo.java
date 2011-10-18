package org.coolreader.crengine;

import android.os.Build;
import android.util.Log;

public class DeviceInfo {

	public final static String MANUFACTURER;
	public final static String MODEL;
	public final static String DEVICE;
	public final static boolean SAMSUNG_BUTTONS_HIGHLIGHT_PATCH;
	public final static boolean EINK_SCREEN;
	public final static boolean EINK_SCREEN_UPDATE_MODES_SUPPORTED;
	public final static boolean NOOK_NAVIGATION_KEYS;
	public final static boolean EINK_NOOK;
	public final static boolean FORCE_LIGHT_THEME;
	public final static boolean EINK_SONY;
	public final static boolean SONY_NAVIGATION_KEYS;
	
	static {
		MANUFACTURER = getBuildField("MANUFACTURER");
		MODEL = getBuildField("MODEL");
		DEVICE = getBuildField("DEVICE");
		SAMSUNG_BUTTONS_HIGHLIGHT_PATCH = MANUFACTURER.toLowerCase().contentEquals("samsung") &&
		               (MODEL.contentEquals("GT-S5830") || MODEL.contentEquals("GT-S5660")); // More models?
		EINK_NOOK = MANUFACTURER.toLowerCase().contentEquals("barnesandnoble") && MODEL.contentEquals("NOOK") &&
				DEVICE.toLowerCase().contentEquals("zoom2");
		EINK_SONY = MANUFACTURER.toLowerCase().contentEquals("sony") && MODEL.contentEquals("PRS-T1");
		EINK_SCREEN = EINK_SONY || EINK_NOOK; // TODO: set to true for eink devices like Nook Touch
		
		NOOK_NAVIGATION_KEYS = EINK_NOOK; // TODO: add autodetect
		SONY_NAVIGATION_KEYS = EINK_SONY;
		EINK_SCREEN_UPDATE_MODES_SUPPORTED = EINK_SCREEN && EINK_NOOK; // TODO: add autodetect
		FORCE_LIGHT_THEME = EINK_SCREEN;
	}
	
	private static String getBuildField(String fieldName) {
		
		try {
			return (String)Build.class.getField(fieldName).get(null);
		} catch (Exception e) {
			Log.d("cr3", "Exception while trying to check Build." + fieldName);
			return "";
		}
	}
}
