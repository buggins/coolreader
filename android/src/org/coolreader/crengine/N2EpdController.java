package org.coolreader.crengine;

import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import android.app.Activity;

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
	private static Constructor<?> RegionParamsConstructor= null;
	private static Constructor<?> EpdControllerConstructors[] = null;
	public static Activity n2MainActivity =  null;
	private static Object mEpdController = null;

	private static Object[] enumsWave 	= null;
	private static Object[] enumsRegion	= null;
	private static Object[] enumsMode	= null;

	static {
		if (DeviceInfo.EINK_NOOK) {
			try {
				Class<?> clEpdController     	= Class.forName("android.hardware.EpdController");
				Class<?> clEpdControllerWave;
				if (DeviceInfo.EINK_NOOK_120)
					clEpdControllerWave = Class.forName("android.hardware.EpdRegionParams$Wave");
				else
					clEpdControllerWave = Class.forName("android.hardware.EpdController$Wave");
				Class<?> clEpdControllerMode 	= Class.forName("android.hardware.EpdController$Mode");
				Class<?> clEpdControllerRegion = Class.forName("android.hardware.EpdController$Region");

				Class<?> clEpdControllerRegionParams;
				if (DeviceInfo.EINK_NOOK_120)
					clEpdControllerRegionParams = Class.forName("android.hardware.EpdRegionParams");
				else
					clEpdControllerRegionParams = Class.forName("android.hardware.EpdController$RegionParams");
				
				enumsWave = clEpdControllerWave.getEnumConstants();

				enumsMode = clEpdControllerMode.getEnumConstants();

				enumsRegion = clEpdControllerRegion.getEnumConstants();

				RegionParamsConstructor = clEpdControllerRegionParams.getConstructor(
						new Class[] { Integer.TYPE, Integer.TYPE, Integer.TYPE, Integer.TYPE, clEpdControllerWave});
				mtSetRegion = clEpdController.getMethod("setRegion", String.class, clEpdControllerRegion, 
						clEpdControllerRegionParams, clEpdControllerMode);
				
				if (DeviceInfo.EINK_NOOK_120) 
					EpdControllerConstructors = clEpdController.getConstructors();
				
				strN2EpdInit += "Ok!";
			} catch (Exception e) {
				System.err.println("Failed to init refresh EPD");
				System.err.println(e.toString());
				strN2EpdInit += "Failed: " + e.toString();
				e.printStackTrace();
			}
		}
	}

	public static void setMode(int region, int wave, int mode) {
		if (mtSetRegion != null) {
			try {
				if (DeviceInfo.EINK_NOOK_120 && mEpdController == null)
					mEpdController = EpdControllerConstructors[0].newInstance(new Object[] { n2MainActivity });
				Object regionParams =  RegionParamsConstructor.newInstance(new Object[] { 0, 0, 600, 800, enumsWave[wave]});
				mtSetRegion.invoke(mEpdController, "CoolReader", enumsRegion[region], regionParams, enumsMode[mode]);
			} catch (Exception e) {
				System.err.println("Failed: SetMode");
				System.err.println(e.toString());
				strN2EpdInit += "Failed: setMode: " + e.toString();
				e.printStackTrace();
			}
		}
	}
}
