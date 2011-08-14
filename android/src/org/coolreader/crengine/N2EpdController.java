package org.coolreader.crengine;

import java.lang.reflect.Constructor;
import java.lang.reflect.Method;

/**
 * Nook Touch EPD controller interface wrapper.
 * This class is created by DairyKnight for Nook Touch screen support in FBReaderJ.
 * @author DairyKnight <dairyknight@gmail.com>
 * http://forum.xda-developers.com/showthread.php?t=1183173
 */
public class N2EpdController {

	public static int vCurrentNode = -2;
	public static int GL16_MODE = 4;
	// 0 - blink
	// 1 - ACTIVE
	// 2 - ONESHOT
	// 3 - CLEAR good image (blink) - slow, good
	// 4 - ACTIVE_All - good, slow 			
	// 5 - ONESHOT_ALL
	// 6 - CLEAR_All - good, blink 

/*	W/System.err( 6883): GC
	W/System.err( 6883): GU
	W/System.err( 6883): DU
	W/System.err( 6883): A2
	W/System.err( 6883): GL16
	W/System.err( 6883): AUTO

	W/System.err( 6982): APP_1
	W/System.err( 6982): APP_2
	W/System.err( 6982): APP_3
	W/System.err( 6982): APP_4*/
	
	public static void exitA2Mode() {
		System.err.println("Coolreader::exitA2Mode");
		vCurrentNode = 0;
		try {
			Class epdControllerClass = Class
					.forName("android.hardware.EpdController");
			Class epdControllerRegionClass = Class
					.forName("android.hardware.EpdController$Region");
			Class epdControllerRegionParamsClass = Class
					.forName("android.hardware.EpdController$RegionParams");
			Class epdControllerWaveClass = Class
					.forName("android.hardware.EpdController$Wave");

			Object[] waveEnums = null;
			if (epdControllerWaveClass.isEnum()) {
//				System.err.println("EpdController Wave Enum successfully retrived.");
				waveEnums = epdControllerWaveClass.getEnumConstants();
			}

			Object[] regionEnums = null;
			if (epdControllerRegionClass.isEnum()) {
//				System.err.println("EpdController Region Enum successfully retrived.");
				regionEnums = epdControllerRegionClass.getEnumConstants();
			}

			Constructor RegionParamsConstructor = epdControllerRegionParamsClass.getConstructor(new Class[] { Integer.TYPE, Integer.TYPE,
							Integer.TYPE, Integer.TYPE, epdControllerWaveClass, Integer.TYPE });

			Object localRegionParams = RegionParamsConstructor.newInstance(new Object[] { 0, 0, 600, 800, waveEnums[3], 16}); // Wave = A2

			Method epdControllerSetRegionMethod = epdControllerClass.getMethod("setRegion", new Class[] { String.class,
							epdControllerRegionClass,
							epdControllerRegionParamsClass });
			epdControllerSetRegionMethod.invoke(null, new Object[] { "Coolreader-ReadingView", regionEnums[2], localRegionParams });
				
		} catch (Exception e) {
			e.printStackTrace();
		}		
	}
	
	public static void enterA2Mode() {
		System.err.println("Coolreader::enterA2Mode");
		System.err.println(vCurrentNode);
		try {
			Class epdControllerClass = Class
					.forName("android.hardware.EpdController");
			Class epdControllerRegionClass = Class
					.forName("android.hardware.EpdController$Region");
			Class epdControllerRegionParamsClass = Class
					.forName("android.hardware.EpdController$RegionParams");
			Class epdControllerWaveClass = Class
					.forName("android.hardware.EpdController$Wave");

			Object[] waveEnums = null;
			if (epdControllerWaveClass.isEnum()) {
//				System.err.println("EpdController Wave Enum successfully retrived.");
				waveEnums = epdControllerWaveClass.getEnumConstants();
			}

			Object[] regionEnums = null;
			if (epdControllerRegionClass.isEnum()) {
//				System.err.println("EpdController Region Enum successfully retrived.");
				regionEnums = epdControllerRegionClass.getEnumConstants();
			}

			Constructor RegionParamsConstructor = epdControllerRegionParamsClass.getConstructor(new Class[] { Integer.TYPE, Integer.TYPE,
							Integer.TYPE, Integer.TYPE, epdControllerWaveClass, Integer.TYPE });

			Object localRegionParams = RegionParamsConstructor.newInstance(new Object[] { 0, 0, 600, 800, waveEnums[2], 16}); // Wave = DU

			Method epdControllerSetRegionMethod = epdControllerClass.getMethod("setRegion", new Class[] { String.class,
							epdControllerRegionClass,
							epdControllerRegionParamsClass });
			epdControllerSetRegionMethod.invoke(null, new Object[] { "Coolreader-ReadingView", regionEnums[2], localRegionParams });
			
			

			Thread.sleep(100L);
			localRegionParams = RegionParamsConstructor.newInstance(new Object[] { 0, 0, 600, 800, waveEnums[3], 14}); // Wave = A2
			epdControllerSetRegionMethod.invoke(null, new Object[] { "Coolreader-ReadingView", regionEnums[2], localRegionParams});
			vCurrentNode = 3;
			
		} catch (Exception e) {
			e.printStackTrace();
		}
		
	}
	
	public static void setGL16Mode(int reset) {
//		System.err.println("Coolreader::setGL16Mode");
		try {
			/*
			 * Loading the Epson EPD Controller Classes
			 * 
			 * */
			Class epdControllerClass = Class
					.forName("android.hardware.EpdController");
			Class epdControllerRegionClass = Class
					.forName("android.hardware.EpdController$Region");
			Class epdControllerRegionParamsClass = Class
					.forName("android.hardware.EpdController$RegionParams");
			Class epdControllerWaveClass = Class
					.forName("android.hardware.EpdController$Wave");
			Class epdControllerModeClass = Class
					.forName("android.hardware.EpdController$Mode");

			/*
			 * Creating EPD enums
			 * 
			 * */
			Object[] waveEnums = null;
			if (epdControllerWaveClass.isEnum()) {
//				System.err.println("EpdController Wave Enum successfully retrived.");
				waveEnums = epdControllerWaveClass.getEnumConstants();
			}

			Object[] regionEnums = null;
			if (epdControllerRegionClass.isEnum()) {
//				System.err.println("EpdController Region Enum successfully retrived.");
				regionEnums = epdControllerRegionClass.getEnumConstants();
			}
			
			Object[] modeEnums = null;
			if (epdControllerModeClass.isEnum()) {
//				System.err.println("EpdController Region Enum successfully retrived.");
				modeEnums = epdControllerModeClass.getEnumConstants();
				System.err.println(modeEnums);
			}

			Constructor RegionParamsConstructor = epdControllerRegionParamsClass
					.getConstructor(new Class[] { Integer.TYPE, Integer.TYPE,
							Integer.TYPE, Integer.TYPE, epdControllerWaveClass});

			Object localRegionParams = RegionParamsConstructor
					.newInstance(new Object[] { 0, 0, 600, 800, waveEnums[1]}); // Wave = GU (1)

			Method epdControllerSetRegionMethod = epdControllerClass.getMethod(
					"setRegion", new Class[] { String.class,
							epdControllerRegionClass,
							epdControllerRegionParamsClass, epdControllerModeClass });

//			System.err.println("get " + String.valueOf(reset) + " " + String.valueOf(vCurrentNode));
			
			if (reset > 0 || vCurrentNode < 0) {
//				System.err.println("EpdController resetting.");
				epdControllerSetRegionMethod
					.invoke(null, new Object[] { "Coolreader-ReadingView",
							regionEnums[2], localRegionParams, modeEnums[6] });
				if (reset > 0) {
					vCurrentNode = 1 - reset;
				} else {
					vCurrentNode++;
				}
			} else {
				if (vCurrentNode == 0 || GL16_MODE == 1 || GL16_MODE == 2) {
//					System.err.println("EpdController setting.");
					if (vCurrentNode == 0) {
						epdControllerSetRegionMethod
							.invoke(null, new Object[] { "Coolreader-ReadingView",
									regionEnums[2], localRegionParams, modeEnums[GL16_MODE] });
					} 
					if (vCurrentNode == 0) 
						vCurrentNode++;
				}
			}
//			System.err.println("set " + String.valueOf(vCurrentNode));
		} catch (Exception e) {
			e.printStackTrace();
		}		
	}
	
	public static void setDUMode() {
		System.err.println("Coolreader::setDUMode");
		System.err.println(vCurrentNode);
		try {
			Class epdControllerClass = Class
					.forName("android.hardware.EpdController");
			Class epdControllerRegionClass = Class
					.forName("android.hardware.EpdController$Region");
			Class epdControllerRegionParamsClass = Class
					.forName("android.hardware.EpdController$RegionParams");
			Class epdControllerWaveClass = Class
					.forName("android.hardware.EpdController$Wave");

			Object[] waveEnums = null;
			if (epdControllerWaveClass.isEnum()) {
//				System.err.println("EpdController Wave Enum successfully retrived.");
				waveEnums = epdControllerWaveClass.getEnumConstants();
			}

			Object[] regionEnums = null;
			if (epdControllerRegionClass.isEnum()) {
//				System.err.println("EpdController Region Enum successfully retrived.");
				regionEnums = epdControllerRegionClass.getEnumConstants();
			}

			Constructor RegionParamsConstructor = epdControllerRegionParamsClass
					.getConstructor(new Class[] { Integer.TYPE, Integer.TYPE,
							Integer.TYPE, Integer.TYPE, epdControllerWaveClass,
							Integer.TYPE });

			Object localRegionParams = RegionParamsConstructor
					.newInstance(new Object[] { 0, 0, 600, 800, waveEnums[2],
							14 });

			Method epdControllerSetRegionMethod = epdControllerClass.getMethod(
					"setRegion", new Class[] { String.class,
							epdControllerRegionClass,
							epdControllerRegionParamsClass });
			epdControllerSetRegionMethod
					.invoke(null, new Object[] { "Coolreader-ReadingView",
							regionEnums[2], localRegionParams });
			vCurrentNode = 2;
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
