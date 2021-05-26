package org.eink_onyx_reflections;

import android.content.Context;
import android.view.View;

import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.List;

class IMX6DeviceImpl implements OnyxEinkDeviceImpl {

	private static int sModeDU = 0;
	private static int sModeGCPartial = 0;
	private static int sModeGC = 0;
	private static int sModeAnim = 0;
	private static int sUI_A2_QUALITY_MODE = 0;
	private static int sModeGC4 = 0;
	private static int sModeReagl = 0;
	private static int sModeReagld = 0;

	private static Method sMethodSupportRegal = null;
	private static Method sMethodEnableA2;
	private static Method sMethodDisableA2;
	private static Method sMethodSetDefaultUpdateMode = null;
	private static Method sMethodApplyApplicationFastMode = null;
	private static Method sMethodEnableScreenUpdate = null;
	private static Method sMethodGetFrontLightValue;
	private static Method sMethodSetFrontLightValue;
	private static Method sMethodSetFrontLightConfigValue;
	private static Method sMethodSetNaturalLightValue;
	private static Method sMethodGetWarmLightConfigValue;
	private static Method sMethodGetColdLightConfigValue;

	public IMX6DeviceImpl() {
		Class<?> ViewClass = View.class;
		int eink_auto_mode_regional = Utils.getStaticIntField(ViewClass, "EINK_AUTO_MODE_REGIONAL");
		int eink_wait_mode_nowait = Utils.getStaticIntField(ViewClass, "EINK_WAIT_MODE_NOWAIT");
		int eink_wait_mode_wait = Utils.getStaticIntField(ViewClass, "EINK_WAIT_MODE_WAIT");
		int eink_waveform_mode_du = Utils.getStaticIntField(ViewClass, "EINK_WAVEFORM_MODE_DU");
		int eink_waveform_mode_anim = Utils.getStaticIntField(ViewClass, "EINK_WAVEFORM_MODE_ANIM");
		int eink_waveform_mode_gc4 = Utils.getStaticIntField(ViewClass, "EINK_WAVEFORM_MODE_GC4");
		int eink_waveform_mode_gc16 = Utils.getStaticIntField(ViewClass, "EINK_WAVEFORM_MODE_GC16");
		int eink_waveform_mode_reagl = Utils.getStaticIntField(ViewClass, "EINK_WAVEFORM_MODE_REAGL");
		int eink_reagl_mode_reagld = Utils.getStaticIntField(ViewClass, "EINK_REAGL_MODE_REAGLD");
		int eink_update_mode_partial = Utils.getStaticIntField(ViewClass, "EINK_UPDATE_MODE_PARTIAL");
		int eink_update_mode_full = Utils.getStaticIntField(ViewClass, "EINK_UPDATE_MODE_FULL");
		sUI_A2_QUALITY_MODE = Utils.getStaticIntField(ViewClass, "UI_A2_QUALITY_MODE");
		sModeDU = eink_wait_mode_nowait | eink_auto_mode_regional | eink_waveform_mode_du | eink_update_mode_partial;
		sModeGCPartial = eink_wait_mode_nowait | eink_auto_mode_regional | eink_waveform_mode_gc16 | eink_update_mode_partial;
		sModeGC = eink_auto_mode_regional | eink_wait_mode_wait | eink_waveform_mode_gc16 | eink_update_mode_full;
		sModeAnim = eink_wait_mode_nowait | eink_auto_mode_regional | eink_waveform_mode_anim | eink_update_mode_partial;
		sModeGC4 = eink_wait_mode_nowait | eink_auto_mode_regional | eink_waveform_mode_gc4 | eink_update_mode_partial;
		sModeReagl = eink_wait_mode_nowait | eink_auto_mode_regional | eink_waveform_mode_reagl | eink_update_mode_partial;
		sModeReagld = eink_wait_mode_nowait | eink_auto_mode_regional | eink_reagl_mode_reagld | eink_waveform_mode_reagl | eink_update_mode_partial;

		sMethodSupportRegal = Utils.getMethod(ViewClass, "supportRegal");
		sMethodSetDefaultUpdateMode = Utils.getMethod(ViewClass, "setDefaultUpdateMode", Integer.TYPE);
		sMethodApplyApplicationFastMode = Utils.getMethod(ViewClass, "applyApplicationFastMode", String.class, Boolean.TYPE, Boolean.TYPE);
		sMethodEnableScreenUpdate = Utils.getMethod(ViewClass, "enableScreenUpdate", Boolean.TYPE);
		sMethodEnableA2 = Utils.getMethod(ViewClass, "enableA2");
		sMethodDisableA2 = Utils.getMethod(ViewClass, "disableA2");

		Class<?> DeviceControllerClass = Utils.getClassForName("android.hardware.DeviceController");
		sMethodGetFrontLightValue = Utils.getMethod(DeviceControllerClass, "getFrontLightValue", Context.class);
		sMethodSetFrontLightValue = Utils.getMethod(DeviceControllerClass, "setFrontLightValue", Context.class, Integer.TYPE);
		sMethodSetFrontLightConfigValue = Utils.getMethod(DeviceControllerClass, "setFrontLightConfigValue", Context.class, Integer.TYPE);
		sMethodSetNaturalLightValue = Utils.getMethod(DeviceControllerClass, "setNaturalLightValue", Context.class, Integer.TYPE);
		sMethodGetWarmLightConfigValue = Utils.getMethod(DeviceControllerClass, "getWarmLightConfigValue", Context.class);
		sMethodGetColdLightConfigValue = Utils.getMethod(DeviceControllerClass, "getColdLightConfigValue", Context.class);
	}

	@Override
	public DeviceType deviceType() {
		return DeviceType.imx6;
	}

	@Override
	public boolean applyApplicationFastMode(String application, boolean enable, boolean clear) {
		return Utils.invokeMethod(sMethodApplyApplicationFastMode, null, application, enable, clear) != null;
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
		Utils.invokeMethod(sMethodDisableA2, view);
	}

	@Override
	public void enableA2ForSpecificView(View view) {
		Utils.invokeMethod(sMethodEnableA2, view);
	}

	@Override
	public boolean enableScreenUpdate(View view, boolean enable) {
		Utils.invokeMethod(sMethodEnableScreenUpdate, view, enable);
		return true;
	}

	@Override
	public boolean hasFLBrightness(Context context) {
		return false;
	}

	@Override
	public List<Integer> getFrontLightValueList(Context context) {
		return Arrays.asList(0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160);
	}

	@Override
	public int getFrontLightDeviceValue(Context context) {
		Object res = Utils.invokeMethod(sMethodGetFrontLightValue, null, context);
		if (res instanceof Integer)
			return (int) res;
		return 0;
	}

	@Override
	public boolean setFrontLightConfigValue(Context context, int value) {
		Utils.invokeMethod(sMethodSetFrontLightConfigValue, null, context, value);
		return true;
	}

	@Override
	public boolean setFrontLightDeviceValue(Context context, int value) {
		return Utils.invokeMethod(sMethodSetFrontLightValue, null, context, value) != null;
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
		Object res = Utils.invokeMethod(sMethodGetColdLightConfigValue, null, context);
		if (res instanceof Integer)
			return (int) res;
		return 0;
	}

	@Override
	public boolean setColdLightDeviceValue(Context context, int value) {
		return false;
	}

	@Override
	public int getWarmLightConfigValue(Context context) {
		Object res = Utils.invokeMethod(sMethodGetWarmLightConfigValue, null, context);
		if (res instanceof Integer)
			return (int) res;
		return 0;
	}

	@Override
	public boolean setWarmLightDeviceValue(Context context, int value) {
		return Utils.invokeMethod(sMethodSetNaturalLightValue, null, context, value) != null;
	}

	@Override
	public void repaintEveryThing(UpdateMode mode) {
	}

	@Override
	public boolean setViewDefaultUpdateMode(View view, UpdateMode mode) {
		return Utils.invokeMethod(sMethodSetDefaultUpdateMode, view, getUpdateModeValue(mode)) != null;
	}

	@Override
	public boolean supportRegal() {
		if (sMethodSupportRegal == null)
			return false;
		Object res = Utils.invokeMethod(sMethodSupportRegal, null);
		if (res instanceof Boolean)
			return (boolean) res;
		return false;
	}

	@Override
	public boolean isAppOptimizationEnabled() {
		return false;
	}

	// private methods
	private int getUpdateModeValue(UpdateMode mode) {
		int value = sModeGC;
		switch (mode) {
			case GU_FAST:
			case DU:
				value = sModeDU;
				break;
			case GU:
				value = sModeGCPartial;
				break;
			case GC:
				value = sModeGC;
				break;
			case ANIMATION:
				value = sModeAnim;
				break;
			case ANIMATION_QUALITY:
				value = sUI_A2_QUALITY_MODE;
				break;
			case GC4:
				value = sModeGC4;
				break;
			case REGAL:
				if (0 != sModeReagl)
					value = sModeReagl;
				else
					value = sModeGCPartial;
				break;
			case REGAL_D:
				if (0 != sModeReagld)
					value = sModeReagld;
				else
					value = sModeGCPartial;
				break;
		}
		return value;
	}
}
