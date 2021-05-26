package org.eink_onyx_reflections;

import android.content.Context;
import android.view.View;

import java.util.List;

class DeviceStubImpl implements OnyxEinkDeviceImpl {

	@Override
	public DeviceType deviceType() {
		return DeviceType.none;
	}

	@Override
	public boolean applyApplicationFastMode(String application, boolean enable, boolean clear) {
		return false;
	}

	@Override
	public boolean applyApplicationFastMode(String application, boolean enable, boolean clear, UpdateMode repeatMode, int repeatLimit) {
		return false;
	}

	@Override
	public void byPass(int count) {
	}

	@Override
	public boolean clearApplicationFastMode() {
		return false;
	}

	@Override
	public void disableA2ForSpecificView(View view) {
	}

	@Override
	public void enableA2ForSpecificView(View view) {
	}

	@Override
	public boolean enableScreenUpdate(View view, boolean enable) {
		return false;
	}

	@Override
	public boolean hasFLBrightness(Context context) {
		return false;
	}

	@Override
	public List<Integer> getFrontLightValueList(Context context) {
		return null;
	}

	@Override
	public int getFrontLightDeviceValue(Context context) {
		return 0;
	}

	@Override
	public boolean setFrontLightConfigValue(Context context, int value) {
		return false;
	}

	@Override
	public boolean setFrontLightDeviceValue(Context context, int value) {
		return false;
	}

	@Override
	public boolean hasCTMBrightness(Context context) {
		return false;
	}

	@Override
	public List<Integer> getColdLightValues(Context context) {
		return null;
	}

	@Override
	public List<Integer> getWarmLightValues(Context context) {
		return null;
	}

	@Override
	public int getColdLightConfigValue(Context context) {
		return 0;
	}

	@Override
	public boolean setColdLightDeviceValue(Context context, int value) {
		return false;
	}

	@Override
	public int getWarmLightConfigValue(Context context) {
		return 0;
	}

	@Override
	public boolean setWarmLightDeviceValue(Context context, int value) {
		return false;
	}

	@Override
	public void repaintEveryThing(UpdateMode mode) {
	}

	@Override
	public boolean setViewDefaultUpdateMode(View view, UpdateMode mode) {
		return false;
	}

	@Override
	public boolean supportRegal() {
		return false;
	}

	@Override
	public boolean isAppOptimizationEnabled() {
		return false;
	}
}
