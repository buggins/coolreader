package org.eink_onyx_reflections;

import android.os.Build;
import android.util.Log;

import java.lang.reflect.Method;

public class OnyxDevice {
	private static OnyxEinkDeviceImpl mCurrentDevice = null;

	public static OnyxEinkDeviceImpl currentDevice() {
		if (null == mCurrentDevice) {
			String platform = getBoardPlatform().trim();
			if (Build.HARDWARE.contains("freescale")) {
				if ("imx7".equals(platform)) {
					mCurrentDevice = new IMX7DeviceImpl();
				} else {
					mCurrentDevice = new IMX6DeviceImpl();
				}
			} else if ("rk312x".equals(platform)) {
				mCurrentDevice = new RK31XXDeviceImpl();
			} else if ("rk3288".equals(platform)) {
				mCurrentDevice = new RK32XXDeviceImpl();
			} else if ("rk3368".equals(platform)) {
				mCurrentDevice = new RK33XXDeviceImpl();
			} else if ("msm8953".equals(platform)) {
				mCurrentDevice = new SDMDeviceImpl();
			} else if ("sdm660".equals(platform)) {
				mCurrentDevice = new SDMDeviceImpl();
			} else if (Build.HARDWARE.contentEquals("rk30board")) {
				mCurrentDevice = new RK3026DeviceImpl();
			} else {
				mCurrentDevice = new DeviceStubImpl();
			}
		}
		return mCurrentDevice;
	}

	public static DeviceType currentDeviceType() {
		return currentDevice().deviceType();
	}

	private static String getBoardPlatform() {
		Method method_getString = Utils.getDeclaredMethod(Build.class, "getString", String.class);
		Object result = Utils.invokeMethod(method_getString, null, "ro.board.platform");
		if (result instanceof String)
			return (String) result;
		return "";
		//return Build.BOARD;
	}

	private static void bootstrap() {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
			try {
				// Allow Private API on Android 10 (API 29)
				Class<?> aClass = Class.class;
				Method methodForName = aClass.getDeclaredMethod("forName", String.class);
				Method methodGetDeclaredMethod = aClass.getDeclaredMethod("getDeclaredMethod", String.class, Class[].class);
				Class<?> VMRuntimeClass = (Class<?>) methodForName.invoke(null, "dalvik.system.VMRuntime");
				Method methodGetRuntime = (Method) methodGetDeclaredMethod.invoke(VMRuntimeClass, "getRuntime", null);
				Method methodSetHiddenApiExemptions = (Method) methodGetDeclaredMethod.invoke(VMRuntimeClass, "setHiddenApiExemptions", new Class[] { String[].class });
				Object runtime = methodGetRuntime.invoke(null);
				methodSetHiddenApiExemptions.invoke(runtime, (Object) new String[] { "L" });
			} catch (Exception e) {
				Log.e("onyx", "reflect bootstrap failed:", e);
			}
		}
	}

	static {
		bootstrap();
	}
}
