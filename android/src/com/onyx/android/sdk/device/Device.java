package com.onyx.android.sdk.device;

import android.os.Build;

import com.onyx.android.sdk.utils.ReflectUtil;

import java.lang.reflect.Method;

public class Device {
	private static String TAG = Device.class.getSimpleName();
	public static final BaseDevice currentDevice = detectDevice();

	public static BaseDevice currentDevice() {
		return currentDevice;
	}

	public static synchronized BaseDevice detectDevice() {
		if ((Build.HARDWARE.contains("freescale")) && ("imx7".equals(getBoardName()))) {
			return IMX7Device.createDevice();
		}
		if (Build.HARDWARE.contains("freescale")) {
			return IMX6Device.createDevice();
		}
		if ((Build.HARDWARE.contentEquals("rk30board")) && ("rk3288".equals(getBoardName()))) {
		    return RK32XXDevice.createDevice();
		}
		if ((Build.HARDWARE.contentEquals("rk30board")) && ("rk312x".equals(getBoardName()))) {
		    return RK31XXDevice.createDevice();
		}
		if (Build.HARDWARE.contentEquals("rk30board")) {
			return RK3026Device.createDevice();
		}
		return new BaseDevice();
	}

	private static String getBoardName() {
		Method method = ReflectUtil.getDeclaredMethodSafely(Build.class, "getString", String.class);
		return (String) ReflectUtil.invokeMethodSafely(method, null, new Object[]{"ro.board.platform"});
	}
}
