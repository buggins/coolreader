package com.onyx.android.sdk.api.device;

import android.view.View;

import com.onyx.android.sdk.api.device.epd.EpdController;
import com.onyx.android.sdk.api.device.epd.UpdateMode;

public class EpdRk3026 extends EpdDevice {
	public EpdRk3026() {
	}

	public void applyGCUpdate(View view) {
		EpdController.setViewDefaultUpdateMode(view, UpdateMode.GC);
		view.invalidate();
	}

	public void setUpdateMode(View view, UpdateMode mode) {
		EpdController.setViewDefaultUpdateMode(view, mode);
	}

	public void resetUpdate(View view) {
		EpdController.setViewDefaultUpdateMode(view, UpdateMode.GU);
	}
}
