package com.onyx.android.sdk.api.device;

import android.view.View;

import com.onyx.android.sdk.api.device.epd.EpdController;
import com.onyx.android.sdk.api.device.epd.UpdateMode;

public class EpdImx6 extends EpdDevice {
	public EpdImx6() {
	}

	private void useFastScheme() {
		EpdController.setDisplayScheme(EpdController.SCHEME_SCRIBBLE);
	}

	public void applyGCUpdate(View view) {
		EpdController.setViewDefaultUpdateMode(view, UpdateMode.GC);
	}

	public void applyRegalUpdate(View view) {
		EpdController.setViewDefaultUpdateMode(view, UpdateMode.REGAL);
	}

	public void applyRegalClearUpdate(View view) {
		EpdController.setViewDefaultUpdateMode(view, UpdateMode.REGAL_D);
	}

	public void setUpdateMode(View view, UpdateMode mode) {
		EpdController.setViewDefaultUpdateMode(view, mode);
		useFastScheme();
	}

	public void resetUpdate(View view) {
		EpdController.resetUpdateMode(view);
		useFastScheme();
	}

	public void cleanUpdate(View view) {
		resetUpdate(view);
	}
}
