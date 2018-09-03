package com.onyx.android.sdk.api.device;

import android.content.Context;
import android.view.View;

import com.onyx.android.sdk.api.device.epd.EpdController;
import com.onyx.android.sdk.api.device.epd.UpdateMode;
import com.onyx.android.sdk.device.Device;
import com.onyx.android.sdk.device.IMX6Device;
import com.onyx.android.sdk.device.IMX7Device;
import com.onyx.android.sdk.device.RK3026Device;

public class EpdDeviceManager {
	private static final String TAG = EpdDeviceManager.class.getSimpleName();
	private static int gcInterval;
	private static int refreshCount;
	private static boolean inFastUpdateMode = false;
	private static final EpdDevice epdDevice;

	public EpdDeviceManager() {
	}

	public static void enterAnimationUpdate(boolean clear) {
		if (!inFastUpdateMode) {
			EpdController.applyApplicationFastMode(TAG, true, clear);
			inFastUpdateMode = true;
		}
	}

	public static void exitAnimationUpdate(boolean clear) {
		if (inFastUpdateMode) {
			EpdController.applyApplicationFastMode(TAG, false, clear);
			inFastUpdateMode = false;
		}
	}

	public static void startScreenHandWriting(View view) {
		EpdController.setScreenHandWritingPenState(view, 1);
	}

	public static void stopScreenHandWriting(View view) {
		EpdController.setScreenHandWritingPenState(view, 0);
	}

	public static void prepareInitialUpdate(int interval) {
		gcInterval = interval - 1;
		refreshCount = gcInterval;
	}

	public static int getGcInterval() {
		return gcInterval;
	}

	public static void setGcInterval(int interval) {
		gcInterval = interval - 1;
		refreshCount = 0;
	}

	public static void applyWithGCInterval(View view, boolean isTextPage) {
		if (isUsingRegal(view.getContext())) {
			applyWithGCIntervalWitRegal(view, isTextPage);
		} else {
			applyWithGCIntervalWithoutRegal(view);
		}
	}

	public static boolean isUsingRegal(Context context) {
		return EpdController.supportRegal();
	}

	public static void enableScreenUpdate(View view, boolean enable) {
		epdDevice.enableScreenUpdate(view, enable);
	}

	public static void refreshScreenWithGCInterval(View view, boolean isTextPage) {
		enableScreenUpdate(view, true);
		if (isTextPage && EpdController.supportRegal()) {
			refreshScreenWithGCIntervalWithRegal(view);
		} else {
			refreshScreenWithGCIntervalWithoutRegal(view);
		}

	}

	public static void refreshScreenWithGCIntervalWithRegal(View view) {
		if (refreshCount++ >= gcInterval) {
			refreshCount = 0;
			epdDevice.refreshScreen(view, UpdateMode.GC);
		} else {
			epdDevice.refreshScreen(view, UpdateMode.REGAL);
		}

	}

	public static void refreshScreenWithGCIntervalWithoutRegal(View view) {
		if (refreshCount++ >= gcInterval) {
			refreshCount = 0;
			epdDevice.refreshScreen(view, UpdateMode.GC);
		} else {
			epdDevice.refreshScreen(view, UpdateMode.GU);
		}

	}

	public static void applyWithGCIntervalWitRegal(View view, boolean textOnly) {
		if (refreshCount++ >= gcInterval) {
			refreshCount = 0;
			epdDevice.applyGCUpdate(view);
		} else {
			epdDevice.applyRegalUpdate(view);
		}

	}

	public static void applyWithGCIntervalWithoutRegal(View view) {
		if (refreshCount++ >= gcInterval) {
			refreshCount = 0;
			epdDevice.applyGCUpdate(view);
		} else {
			epdDevice.resetUpdate(view);
		}

	}

	public static void applyGCUpdate(View view) {
		epdDevice.applyGCUpdate(view);
	}

	public static void setUpdateMode(View view, UpdateMode mode) {
		epdDevice.setUpdateMode(view, mode);
	}

	public static void resetUpdateMode(View view) {
		epdDevice.resetUpdate(view);
	}

	public static void cleanUpdateMode(View view) {
		epdDevice.cleanUpdate(view);
	}

	static {
		if (Device.currentDevice() instanceof RK3026Device) {
			epdDevice = new EpdRk3026();
		} else if ((Device.currentDevice() instanceof IMX6Device) || (Device.currentDevice() instanceof IMX7Device)) {
			epdDevice = new EpdImx6();
		} else {
			epdDevice = new EpdDevice();
		}
	}
}
