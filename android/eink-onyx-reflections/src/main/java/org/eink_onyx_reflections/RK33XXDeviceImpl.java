package org.eink_onyx_reflections;

import android.content.Context;
import android.view.View;

import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.List;

class RK33XXDeviceImpl implements OnyxEinkDeviceImpl {

	private static int sModeDU = 0;
	private static int sModeGCPartial = 0;
	private static int sModeGC = 0;
	private static int sModeAnim = 0;
	private static int sUI_A2_QUALITY_MODE = 0;
	private static int sModeGC4 = 0;
	private static int sModeReagl = 0;
	private static int sModeReagld = 0;
	private static int sUI_DEFAULT_MODE = 0;

	private static Method sMethodSupportRegal = null;
	private static Method sMethodByPass = null;
	private static Method sMethodEnableA2;
	private static Method sMethodDisableA2;
	private static Method sMethodSetDefaultUpdateMode = null;
	private static Method sMethodApplyApplicationFastMode = null;
	private static Method sMethodApplyApplicationFastMode_2 = null;
	private static Method sMethodClearApplicationFastMode = null;
	private static Method sMethodEnableScreenUpdate = null;
	private static Method sMethodRepaintEverything = null;
	private static Method sMethodGetFrontLightValue;
	private static Method sMethodSetFrontLightValue;
	private static Method sMethodSetFrontLightConfigValue;
	private static Method sMethodHasFLBrightness;
	private static Method sMethodHasCTMBrightness;
	private static Method sMethodSetColdLightDeviceValue;
	private static Method sMethodSetWarmLightDeviceValue;
	private static Method sMethodGetBrightnessConfig;
	private static Method sMethodGetCTMBrightnessValues;

	public RK33XXDeviceImpl() {
		Class<?> ViewUpdateHelperClass = Utils.getClassForName("android.onyx.ViewUpdateHelper");
		int eink_auto_mode_regional = Utils.getStaticIntField(ViewUpdateHelperClass, "EINK_AUTO_MODE_REGIONAL");
		int eink_wait_mode_nowait = Utils.getStaticIntField(ViewUpdateHelperClass, "EINK_WAIT_MODE_NOWAIT");
		int eink_wait_mode_wait = Utils.getStaticIntField(ViewUpdateHelperClass, "EINK_WAIT_MODE_WAIT");
		int eink_waveform_mode_du = Utils.getStaticIntField(ViewUpdateHelperClass, "EINK_WAVEFORM_MODE_DU");
		int eink_waveform_mode_anim = Utils.getStaticIntField(ViewUpdateHelperClass, "EINK_WAVEFORM_MODE_ANIM");
		int eink_waveform_mode_gc4 = Utils.getStaticIntField(ViewUpdateHelperClass, "EINK_WAVEFORM_MODE_GC4");
		int eink_waveform_mode_gc16 = Utils.getStaticIntField(ViewUpdateHelperClass, "EINK_WAVEFORM_MODE_GC16");
		int eink_waveform_mode_reagl = Utils.getStaticIntField(ViewUpdateHelperClass, "EINK_WAVEFORM_MODE_REAGL");
		int eink_reagl_mode_reagld = Utils.getStaticIntField(ViewUpdateHelperClass, "EINK_REAGL_MODE_REAGLD");
		int eink_update_mode_partial = Utils.getStaticIntField(ViewUpdateHelperClass, "EINK_UPDATE_MODE_PARTIAL");
		int eink_update_mode_full = Utils.getStaticIntField(ViewUpdateHelperClass, "EINK_UPDATE_MODE_FULL");
		sUI_DEFAULT_MODE = Utils.getStaticIntField(ViewUpdateHelperClass, "UI_DEFAULT_MODE");
		sUI_A2_QUALITY_MODE = Utils.getStaticIntField(ViewUpdateHelperClass, "UI_A2_QUALITY_MODE");
		sModeDU = eink_auto_mode_regional | eink_wait_mode_nowait | eink_waveform_mode_du | eink_update_mode_partial;
		sModeGCPartial = eink_auto_mode_regional | eink_wait_mode_nowait | eink_waveform_mode_gc16 | eink_update_mode_partial;
		sModeGC = eink_auto_mode_regional | eink_wait_mode_wait | eink_waveform_mode_gc16 | eink_update_mode_full;
		sModeAnim = eink_auto_mode_regional | eink_wait_mode_nowait | eink_waveform_mode_anim | eink_update_mode_partial;
		sModeGC4 = eink_auto_mode_regional | eink_wait_mode_nowait | eink_waveform_mode_gc4 | eink_update_mode_partial;
		sModeReagl = eink_auto_mode_regional | eink_wait_mode_nowait | eink_waveform_mode_reagl | eink_update_mode_partial;
		sModeReagld = eink_auto_mode_regional | eink_wait_mode_nowait | eink_reagl_mode_reagld | eink_waveform_mode_reagl | eink_update_mode_partial;

		Class<?> DeviceControllerClass = Utils.getClassForName("android.onyx.hardware.DeviceController");
		sMethodGetFrontLightValue = Utils.getMethod(DeviceControllerClass, "getFrontLightValue", Context.class);
		sMethodSetFrontLightValue = Utils.getMethod(DeviceControllerClass, "setFrontLightValue", Context.class, Integer.TYPE);
		sMethodSetFrontLightConfigValue = Utils.getMethod(DeviceControllerClass, "setFrontLightConfigValue", Context.class, Integer.TYPE);
		sMethodHasFLBrightness = Utils.getMethod(DeviceControllerClass, "hasFLBrightness", Context.class);
		sMethodHasCTMBrightness = Utils.getMethod(DeviceControllerClass, "hasCTMBrightness", Context.class);
		sMethodGetCTMBrightnessValues = Utils.getMethod(DeviceControllerClass, "getCTMBrightnessValues", Context.class);
		sMethodSetWarmLightDeviceValue = Utils.getMethod(DeviceControllerClass, "setWarmLightDeviceValue", Context.class, Integer.TYPE);
		sMethodSetColdLightDeviceValue = Utils.getMethod(DeviceControllerClass, "setColdLightDeviceValue", Context.class, Integer.TYPE);
		sMethodGetBrightnessConfig = Utils.getMethod(DeviceControllerClass, "getBrightnessConfig", Context.class, Integer.TYPE);

		Class<?> viewClass = View.class;
		sMethodByPass = Utils.getMethod(viewClass, "byPass", Integer.TYPE);
		sMethodSupportRegal = Utils.getMethod(viewClass, "supportRegal");
		sMethodSetDefaultUpdateMode = Utils.getMethod(viewClass, "setDefaultUpdateMode", Integer.TYPE);
		sMethodApplyApplicationFastMode = Utils.getMethod(viewClass, "applyApplicationFastMode", String.class, Boolean.TYPE, Boolean.TYPE);
		sMethodApplyApplicationFastMode_2 = Utils.getMethod(viewClass, "applyApplicationFastMode", String.class, Boolean.TYPE, Boolean.TYPE, Integer.TYPE, Integer.TYPE);
		sMethodClearApplicationFastMode = Utils.getMethod(viewClass, "clearApplicationFastMode");
		sMethodEnableScreenUpdate = Utils.getMethod(viewClass, "enableScreenUpdate", Boolean.TYPE);
		sMethodRepaintEverything = Utils.getMethod(viewClass, "repaintEverything", Integer.TYPE);
		sMethodEnableA2 = Utils.getMethod(viewClass, "enableA2");
		sMethodDisableA2 = Utils.getMethod(viewClass, "disableA2");
	}

	@Override
	public DeviceType deviceType() {
		return DeviceType.rk33xx;
	}

	@Override
	public boolean applyApplicationFastMode(String application, boolean enable, boolean clear) {
		return Utils.invokeMethod(sMethodApplyApplicationFastMode, null, application, enable, clear) != null;
	}

	@Override
	public boolean applyApplicationFastMode(String application, boolean enable, boolean clear, UpdateMode repeatMode, int repeatLimit) {
		return Utils.invokeMethod(sMethodApplyApplicationFastMode_2, null, application, enable, clear, getUpdateModeValue(repeatMode), repeatLimit) != null;
	}

	@Override
	public void byPass(int count) {
		Utils.invokeMethod(sMethodByPass, null, count);
	}

	@Override
	public boolean clearApplicationFastMode() {
		return Utils.invokeMethod(sMethodClearApplicationFastMode, null) != null;
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
		Object res = Utils.invokeMethod(sMethodHasFLBrightness, null, context);
		if (res instanceof Boolean)
			return (boolean) res;
		return false;
	}

	@Override
	public List<Integer> getFrontLightValueList(Context context) {
		return getColdLightValues(context);
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
		Object res = Utils.invokeMethod(sMethodHasCTMBrightness, null, context);
		if (res instanceof Boolean)
			return (boolean) res;
		return false;
	}

	@Override
	public List<Integer> getColdLightValues(Context context) {
		Object res = Utils.invokeMethod(sMethodGetCTMBrightnessValues, null, context);
		if (res instanceof Integer[][]) {
			Integer[][] array = (Integer[][]) res;
			if (array != null && array.length > 1)
				return Arrays.asList(array[1]);
		}
		return null;
	}

	@Override
	public List<Integer> getWarmLightValues(Context context) {
		Object res = Utils.invokeMethod(sMethodGetCTMBrightnessValues, null, context);
		if (res instanceof Integer[][]) {
			Integer[][] array = (Integer[][]) res;
			if (array != null && array.length > 0)
				return Arrays.asList(array[0]);
		}
		return null;
	}

	@Override
	public int getColdLightConfigValue(Context context) {
		Object res = Utils.invokeMethod(sMethodGetBrightnessConfig, null, context, 3);
		if (res != null)
			return (Integer) res;
		return 0;
	}

	@Override
	public boolean setColdLightDeviceValue(Context context, int value) {
		return Utils.invokeMethod(sMethodSetColdLightDeviceValue, null, context, value) != null;
	}

	@Override
	public int getWarmLightConfigValue(Context context) {
		Object res = Utils.invokeMethod(sMethodGetBrightnessConfig, null, context, 2);
		if (res instanceof Integer)
			return (Integer) res;
		return 0;
	}

	@Override
	public boolean setWarmLightDeviceValue(Context context, int value) {
		return Utils.invokeMethod(sMethodSetWarmLightDeviceValue, null, context, value) != null;
	}

	@Override
	public void repaintEveryThing(UpdateMode mode) {
		Utils.invokeMethod(sMethodRepaintEverything, null, getUpdateModeValue(mode));
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
		boolean enabled = false;
		try {
			Class<?> einkHelperClass = Utils.getClassForName("android.onyx.optimization.EInkHelper");
			Method method = Utils.getDeclaredMethod(einkHelperClass, "getApplicationDPI");
			if (null != method) {
				Integer appDPI = (Integer) method.invoke(null);
				if (null != appDPI && appDPI > 0) {
					// application optimizations is enabled
					enabled = true;
				}
			}
		} catch (Exception ignored) {
		}
		return enabled;
	}

	// private methods
	private int getUpdateModeValue(UpdateMode mode) {
		int value;
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
			default:
				value = sUI_DEFAULT_MODE;
		}
		return value;
	}
}
