package org.coolreader.crengine;

import java.lang.reflect.Method;
import java.lang.reflect.Constructor;
import android.view.View;
//import java.lang.reflect.InvocationTargetException;

/**
 * Nook Touch EPD controller interface wrapper.
 * This class is created by DairyKnight for Nook Touch screen support in FBReaderJ.
 * @author DairyKnight <dairyknight@gmail.com>
 * http://forum.xda-developers.com/showthread.php?t=1183173
 */

public class N2EpdController {
	public static final int REGION_APP_1 = 0;
	public static final int REGION_APP_2 = 1;
	public static final int REGION_APP_3 = 2;
	public static final int REGION_APP_4 = 3;
	
	public static final int WAVE_GC = 0;
	public static final int WAVE_GU = 1;
	public static final int WAVE_DU = 2;
	public static final int WAVE_A2 = 3;
	public static final int WAVE_GL16 = 4;
	public static final int WAVE_AUTO = 5;
	
	public static final int MODE_BLINK = 0;
	public static final int MODE_ACTIVE = 1;
	public static final int MODE_ONESHOT = 2;
	public static final int MODE_CLEAR = 3;
	public static final int MODE_ACTIVE_ALL = 4;
	public static final int MODE_ONESHOT_ALL = 5;
	public static final int MODE_CLEAR_ALL = 6;
	
	public static String strN2EpdInit = " N2EpdInit: ";
	
	private static Method mtSetRegion = null;
	private static Constructor RegionParamsConstructor= null;

	private static Object[] enumsWave 	= null;
	private static Object[] enumsRegion	= null;
	private static Object[] enumsMode	= null;

	static {
		if (DeviceInfo.EINK_NOOK) {
			try {
				Class clEpdController     	= Class.forName("android.hardware.EpdController");
				Class clEpdControllerWave 	= Class.forName("android.hardware.EpdController$Wave");
				Class clEpdControllerMode 	= Class.forName("android.hardware.EpdController$Mode");
				Class clEpdControllerRegion = Class.forName("android.hardware.EpdController$Region");

				Class clEpdControllerRegionParams = Class.forName("android.hardware.EpdController$RegionParams");
				
				enumsWave = clEpdControllerWave.getEnumConstants();

				enumsMode = clEpdControllerMode.getEnumConstants();

				enumsRegion = clEpdControllerRegion.getEnumConstants();

//				mtSetRegion = clEpdController.getMethod("setRegion", String.class, clEpdControllerRegion, View.class,
//								clEpdControllerWave, clEpdControllerMode);
				RegionParamsConstructor = clEpdControllerRegionParams.getConstructor(
						new Class[] { Integer.TYPE, Integer.TYPE, Integer.TYPE, Integer.TYPE, clEpdControllerWave});
				mtSetRegion = clEpdController.getMethod("setRegion", String.class, clEpdControllerRegion, 
						clEpdControllerRegionParams, clEpdControllerMode);
				
				strN2EpdInit += "Ok!";
			} catch (Exception e) {
				System.err.println("Failed to init refresh EPD");
				System.err.println(e.toString());
				strN2EpdInit += "Failed: " + e.toString();
				e.printStackTrace();
			}
		}
	}

//	public static void setMode(int region, int wave, int mode, View view) {
	public static void setMode(int region, int wave, int mode) {
		if (mtSetRegion != null) {
			try {
				Object regionParams =  RegionParamsConstructor.newInstance(new Object[] { 0, 0, 600, 800, enumsWave[wave]});
//				mtSetRegion.invoke(null, "CoolReader", enumsRegion[region], view, enumsWave[wave], enumsMode[mode]);
				mtSetRegion.invoke(null, "CoolReader", enumsRegion[region], regionParams, enumsMode[mode]);
			} catch (Exception e) {
				System.err.println("Failed: SetMode");
				System.err.println(e.toString());
				strN2EpdInit += "Failed: setMode: " + e.toString();
				e.printStackTrace();
			}
		}
	}
}
