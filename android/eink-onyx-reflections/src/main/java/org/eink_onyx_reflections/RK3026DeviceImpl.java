package org.eink_onyx_reflections;

import android.content.Context;
import android.view.View;

import java.lang.reflect.Method;
import java.util.List;

class RK3026DeviceImpl implements OnyxEinkDeviceImpl {

	private static Class<Enum> sEinkModeEnumClass = null;
	private static Method sMethodViewRequestEpdMode = null;

	private static Method sMethodHasFrontLight;
	private static Method sMethodGetFrontLightValue;
	private static Method sMethodSetFrontLightValue;
	private static Method sMethodSetFrontLightConfigValue;
	private static Method sMethodGetFrontLightValues;
	private static Method sMethodHasNaturalLight;
	private static Method sMethodGetWarmLightConfigValue;
	private static Method sMethodGetColdLightConfigValue;
	private static Method sMethodSetWarmLightConfigValue;
	private static Method sMethodSetColdLightConfigValue;
	private static Method sMethodSetWarmLightValue;
	private static Method sMethodSetColdLightValue;
	private static Method sMethodEnableA2;
	private static Method sMethodDisableA2;
	private static Method sMethodSupportRegal;

	public RK3026DeviceImpl() {
		Class<?> viewClass = View.class;
		sMethodSupportRegal = Utils.getMethod(viewClass, "supportRegal");
		sMethodEnableA2 = Utils.getMethod(viewClass, "enableA2");
		sMethodDisableA2 = Utils.getMethod(viewClass, "disableA2");

		sEinkModeEnumClass = (Class<Enum>) Utils.getClassForName("android.view.View$EINK_MODE");
		sMethodViewRequestEpdMode = Utils.getMethod(viewClass, "requestEpdMode", sEinkModeEnumClass);

		Class<?> ctrlClass = Utils.getClassForName("android.hardware.DeviceController");
		sMethodHasFrontLight = Utils.getMethod(ctrlClass, "hasFrontLight");
		sMethodHasNaturalLight = Utils.getMethod(ctrlClass, "hasNaturalLight");
		sMethodGetFrontLightValue = Utils.getMethod(ctrlClass, "getFrontLightValue", Context.class);
		sMethodSetFrontLightValue = Utils.getMethod(ctrlClass, "setFrontLightValue", Context.class, Integer.TYPE);
		sMethodSetFrontLightConfigValue = Utils.getMethod(ctrlClass, "setFrontLightConfigValue", Context.class, Integer.TYPE);
		sMethodGetFrontLightValues = Utils.getMethod(ctrlClass, "getFrontLightValues", Context.class);
		sMethodGetWarmLightConfigValue = Utils.getMethod(ctrlClass, "getWarmLightConfigValue", Context.class);
		sMethodGetColdLightConfigValue = Utils.getMethod(ctrlClass, "getColdLightConfigValue", Context.class);
		sMethodSetWarmLightConfigValue = Utils.getMethod(ctrlClass, "setWarmLightConfigValue", Context.class, Integer.TYPE);
		sMethodSetColdLightConfigValue = Utils.getMethod(ctrlClass, "setColdLightConfigValue", Context.class, Integer.TYPE);
		sMethodSetWarmLightValue = Utils.getMethod(ctrlClass, "setWarmLightValue", Context.class, Integer.TYPE);
		sMethodSetColdLightValue = Utils.getMethod(ctrlClass, "setColdLightValue", Context.class, Integer.TYPE);
	}

	@Override
	public DeviceType deviceType() {
		return DeviceType.rk3026;
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
		Utils.invokeMethod(sMethodDisableA2, view);
	}

	@Override
	public void enableA2ForSpecificView(View view) {
		Utils.invokeMethod(sMethodEnableA2, view);
	}

	@Override
	public boolean enableScreenUpdate(View view, boolean enable) {
		return false;
	}

	@Override
	public boolean hasFLBrightness(Context context) {
		Object result = Utils.invokeMethod(sMethodHasFrontLight, null);
		if (result instanceof Boolean)
			return (boolean) result;
		return false;
	}

	@Override
	public List<Integer> getFrontLightValueList(Context context) {
		Object result = Utils.invokeMethod(sMethodGetFrontLightValues, null, context);
		if (result instanceof List<?>)
			return (List<Integer>) result;
		return null;
	}

	@Override
	public int getFrontLightDeviceValue(Context context) {
		Object result = Utils.invokeMethod(sMethodGetFrontLightValue, null, context);
		if (result instanceof Integer)
			return (int) result;
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
		Object result = Utils.invokeMethod(sMethodHasNaturalLight, null);
		if (result instanceof Boolean)
			return (boolean) result;
		return false;
	}

	@Override
	public List<Integer> getColdLightValues(Context context) {
		return getFrontLightValueList(context);
	}

	@Override
	public List<Integer> getWarmLightValues(Context context) {
		return getFrontLightValueList(context);
	}

	@Override
	public int getColdLightConfigValue(Context context) {
		Object result = Utils.invokeMethod(sMethodGetColdLightConfigValue, null, context);
		if (result instanceof Integer)
			return (Integer) result;
		return 0;
	}

	@Override
	public boolean setColdLightDeviceValue(Context context, int value) {
		Utils.invokeMethod(sMethodSetColdLightConfigValue, null, context, value);
		Utils.invokeMethod(sMethodSetColdLightValue, null, context, value);
		return true;
	}

	@Override
	public int getWarmLightConfigValue(Context context) {
		Object result = Utils.invokeMethod(sMethodGetWarmLightConfigValue, null, context);
		if (result instanceof Integer)
			return (Integer) result;
		return 0;
	}

	@Override
	public boolean setWarmLightDeviceValue(Context context, int value) {
		Utils.invokeMethod(sMethodSetWarmLightConfigValue, null, context, value);
		Utils.invokeMethod(sMethodSetWarmLightValue, null, context, value);
		return true;
	}

	@Override
	public void repaintEveryThing(UpdateMode mode) {
	}

	@Override
	public boolean setViewDefaultUpdateMode(View view, UpdateMode mode) {
		return Utils.invokeMethod(sMethodViewRequestEpdMode, view, getEinkModeFromUpdateMode(mode)) != null;
	}

	@Override
	public boolean supportRegal() {
		if (sMethodSupportRegal == null)
			return false;
		Object result = Utils.invokeMethod(sMethodSupportRegal, null);
		if (result instanceof Boolean)
			return (Boolean) result;
		return false;
	}

	@Override
	public boolean isAppOptimizationEnabled() {
		return false;
	}

	// private methods
	private Object getEinkModeFromUpdateMode(UpdateMode mode) {
		String str = "EPD_NULL";
		switch (mode) {
			case GU:
			case GU_FAST:
				str = "EPD_PART";
				break;
			case GC:
				str = "EPD_FULL";
				break;
			case DU:
				str = "EPD_A2";
				break;
			case REGAL:
				str = "EPD_REGLA";
				break;
		}
		return Enum.valueOf(sEinkModeEnumClass, str);
	}

}
