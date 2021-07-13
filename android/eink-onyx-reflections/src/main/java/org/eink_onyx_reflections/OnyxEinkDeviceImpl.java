package org.eink_onyx_reflections;

import android.content.Context;
import android.view.View;

import java.util.List;

public interface OnyxEinkDeviceImpl {

	/**
	 * Return this device type
	 * @return
	 */
	DeviceType deviceType();

	/**
	 * Enter/leave fast mode
	 *
	 * @param application
	 * @param enable      If true - enter fast mode, false - leave fast mode
	 * @param clear       clear screen before animation. By using clear = true, device will clear device to white before animation with less ghosting
	 * @return
	 */
	boolean applyApplicationFastMode(String application, boolean enable, boolean clear);

	boolean applyApplicationFastMode(String application, boolean enable, boolean clear, UpdateMode repeatMode, int repeatLimit);

	/**
	 * Skip some unwanted coming screen updates.
	 *
	 * @param count count of skipped updates
	 */
	void byPass(int count);

	boolean clearApplicationFastMode();

	void disableA2ForSpecificView(View view);

	void enableA2ForSpecificView(View view);

	boolean enableScreenUpdate(View view, boolean enable);

	boolean hasFLBrightness(Context context);

	List<Integer> getFrontLightValueList(Context context);

	int getFrontLightDeviceValue(Context context);

	boolean setFrontLightConfigValue(Context context, int value);

	boolean setFrontLightDeviceValue(Context context, int value);

	boolean hasCTMBrightness(Context context);

	List<Integer> getColdLightValues(Context context);

	List<Integer> getWarmLightValues(Context context);

	int getColdLightConfigValue(Context context);

	boolean setColdLightDeviceValue(Context context, int value);

	int getWarmLightConfigValue(Context context);

	boolean setWarmLightDeviceValue(Context context, int value);

	void repaintEveryThing(UpdateMode mode);

	/**
	 * Sets the update mode of the view for future updates, so it should be called before update the view.
	 *
	 * @param view View to apply new update mode
	 * @param mode update mode
	 * @return
	 */
	boolean setViewDefaultUpdateMode(View view, UpdateMode mode);

	/**
	 * Get Regal support by device.
	 * @return
	 */
	boolean supportRegal();

	boolean isAppOptimizationEnabled();
}
