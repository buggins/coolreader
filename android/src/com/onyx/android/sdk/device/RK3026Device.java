package com.onyx.android.sdk.device;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.PowerManager;
import android.util.Log;
import android.view.View;
import com.onyx.android.sdk.api.device.epd.EPDMode;
import com.onyx.android.sdk.api.device.epd.UpdateMode;
import com.onyx.android.sdk.utils.FileUtils;
import com.onyx.android.sdk.utils.ReflectUtil;
import com.onyx.android.sdk.utils.StringUtils;
import java.io.File;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.List;

public class RK3026Device
		extends BaseDevice
{
	private static final String TAG = "RK3026Device";
	private static RK3026Device sInstance;
	private static final int d = 1;
	private static final String e = "1";
	private static final String ANDROID_OS_SYSTEM_PROPERTIES = "android.os.SystemProperties";
	private static Class<Enum> sEinkModeEnumClass = null;
	private static Method sMethodViewRequestEpdMode = null;
	private static Method sMethodViewRequestEpdModeForce = null;
	private static int sViewNull = 1;
	private static int sViewFull = 1;
	private static int sViewA2 = 1;
	private static int sViewAuto = 1;
	private static int sViewPart = 1;
	private static int o = 1;
	private static final int p = 0;
	private static final int q = 1;
	private static final int r = 2;
	private static final int s = 3;
	private static final int t = 4;
	private static final int u = 16;
	private static final String EPD_NULL = "EPD_NULL";
	private static final String EPD_AUTO = "EPD_AUTO";
	private static final String EPD_FULL = "EPD_FULL";
	private static final String EPD_A2 = "EPD_A2";
	private static final String EPD_PART = "EPD_PART";
	private static final String EPD_REGLA = "EPD_REGLA";
	private Context mContext = null;
	private EPDMode mCurrentMode = EPDMode.AUTO;
	private UpdateMode mViewDefaultUpdateMode = UpdateMode.GU;
	private static Method sDevMethodIsTouchable;
	private static Method sDevMethodGetTouchType;
	private static Method sDevMethodHasWifi;
	private static Method sDevMethodHasAudio;
	private static Method sDevMethodHasFrontLight;
	private static Method sDevMethodHasBluetooth;
	private static Method sDevMethodOpenFrontLight;
	private static Method sDevMethodCloseFrontLight;
	private static Method sDevMethodGetFrontLightValue;
	private static Method sDevMethodSetFrontLightValue;
	private static Method sDevMethodGetFrontLightConfigValue;
	private static Method sDevMethodSetFrontLightConfigValue;
	private static Method sDevMethodGetFrontLightValues;
	private static Method sDevMethodReadSystemConfig;
	private static Method sDevMethodSaveSystemConfig;
	private static Method sMethodRequestStopBootAnimation;
	private static Method sMethodLed;
	private static Method sMethodEnableA2;
	private static Method sMethodDisableA2;
	private static Method sDevMethodSystemIntegrityCheck;
	private static Method sMethodSupportRegal;
	private static Method sMethodHoldDisplay;
	private static Method sMethodEnableRegal;
	private static final String UNKNOWN = "unknown";
	private static final String RO_DEVICEID = "ro.deviceid";

	@SuppressLint("PrivateApi")
	@SuppressWarnings("unchecked")
	public static RK3026Device createDevice()
	{
		if (sInstance == null)
		{
			sInstance = new RK3026Device();
			try
			{
				Class cls = View.class;

				sEinkModeEnumClass = (Class<Enum>) Class.forName("android.view.View$EINK_MODE");

				sMethodViewRequestEpdMode = cls.getMethod("requestEpdMode", sEinkModeEnumClass);
				sMethodViewRequestEpdModeForce = cls.getMethod("requestEpdMode", sEinkModeEnumClass, Boolean.TYPE);
				Object[] enumConstants = sEinkModeEnumClass.getEnumConstants();
				Class cls2 = enumConstants[0].getClass();
				Method method = cls2.getDeclaredMethod("getValue");
				if (enumConstants.length > 4) {
					sViewNull = ((Integer) method.invoke(enumConstants[0]));
					sViewAuto = ((Integer) method.invoke(enumConstants[1]));
					sViewFull = ((Integer) method.invoke(enumConstants[2]));
					sViewA2 = ((Integer) method.invoke(enumConstants[3]));
					sViewPart = ((Integer) method.invoke(enumConstants[4]));
				}
				if (enumConstants.length > 16) {
					o = ((Integer)method.invoke(enumConstants[16]));
				} else {
					o = sViewPart;
				}
				sMethodSupportRegal = ReflectUtil.getMethodSafely(cls, "supportRegal");
				sMethodHoldDisplay = ReflectUtil.getMethodSafely(cls, "holdDisplay", Boolean.TYPE, Integer.TYPE, Integer.TYPE);
				sMethodEnableRegal = ReflectUtil.getMethodSafely(cls, "enableRegal", Boolean.TYPE);

				Class devCtrlCls = Class.forName("android.hardware.DeviceController");

				sDevMethodIsTouchable = ReflectUtil.getMethodSafely(devCtrlCls, "isTouchable", Context.class);
				sDevMethodGetTouchType = ReflectUtil.getMethodSafely(devCtrlCls, "getTouchType", Context.class);
				sDevMethodHasWifi = ReflectUtil.getMethodSafely(devCtrlCls, "hasWifi", Context.class);
				sDevMethodHasAudio = ReflectUtil.getMethodSafely(devCtrlCls, "hasAudio", Context.class);
				sDevMethodHasFrontLight = ReflectUtil.getMethodSafely(devCtrlCls, "hasFrontLight", Context.class);
				sDevMethodHasBluetooth = ReflectUtil.getMethodSafely(devCtrlCls, "hasBluetooth", Context.class);

				sDevMethodOpenFrontLight = ReflectUtil.getMethodSafely(devCtrlCls, "openFrontLight", Context.class);
				sDevMethodCloseFrontLight = ReflectUtil.getMethodSafely(devCtrlCls, "closeFrontLight", Context.class);
				sDevMethodGetFrontLightValue = ReflectUtil.getMethodSafely(devCtrlCls, "getFrontLightValue", Context.class);
				sDevMethodSetFrontLightValue = ReflectUtil.getMethodSafely(devCtrlCls, "setFrontLightValue", Context.class, Integer.TYPE);
				sDevMethodGetFrontLightConfigValue = ReflectUtil.getMethodSafely(devCtrlCls, "getFrontLightConfigValue", Context.class);
				sDevMethodSetFrontLightConfigValue = ReflectUtil.getMethodSafely(devCtrlCls, "setFrontLightConfigValue", Context.class, Integer.TYPE);
				sDevMethodGetFrontLightValues = ReflectUtil.getMethodSafely(devCtrlCls, "getFrontLightValues", Context.class);
				sDevMethodReadSystemConfig = ReflectUtil.getMethodSafely(devCtrlCls, "readSystemConfig", String.class);
				sDevMethodSaveSystemConfig = ReflectUtil.getMethodSafely(devCtrlCls, "saveSystemConfig", String.class, String.class);
				sDevMethodSystemIntegrityCheck = ReflectUtil.getMethodSafely(devCtrlCls, "systemIntegrityCheck");

				sMethodRequestStopBootAnimation = ReflectUtil.getMethodSafely(cls, "requestStopBootAnimation");

				sMethodLed = ReflectUtil.getMethodSafely(devCtrlCls, "led", Boolean.TYPE);

				sMethodEnableA2 = ReflectUtil.getMethodSafely(cls, "enableA2");

				sMethodDisableA2 = ReflectUtil.getMethodSafely(cls, "disableA2");
				Log.d(TAG, "init device EINK_ONYX_GC_MASK.");
			}
			catch (ClassNotFoundException e)
			{
				Log.w(TAG, e);
			}
			catch (SecurityException e1)
			{
				Log.w(TAG, e1);
			}
			catch (NoSuchMethodException e2)
			{
				Log.w(TAG, e2);
			}
			catch (IllegalArgumentException e3)
			{
				Log.w(TAG, e3);
			}
			catch (IllegalAccessException e4)
			{
				Log.w(TAG, e4);
			}
			catch (InvocationTargetException e5)
			{
				Log.w(TAG, e5);
			}
		}
		return sInstance;
	}

	public File getStorageRootDirectory()
	{
		return new File("/mnt");
	}

	public File getExternalStorageDirectory()
	{
		return new File("/mnt/sdcard");
	}

	public File getRemovableSDCardDirectory()
	{
		return new File("/mnt/external_sd");
	}

	public boolean isFileOnRemovableSDCard(File file)
	{
		return file.getAbsolutePath().startsWith(getRemovableSDCardDirectory().getAbsolutePath());
	}

	public PowerManager.WakeLock newWakeLock(Context context, String tag)
	{
		PowerManager powerManager = (PowerManager)context.getSystemService(Context.POWER_SERVICE);
		return powerManager.newWakeLock(PowerManager.ON_AFTER_RELEASE | PowerManager.PARTIAL_WAKE_LOCK, tag);
	}

	public String readSystemConfig(Context context, String key)
	{
		Object res = invokeDeviceControllerMethod(context, sDevMethodReadSystemConfig, key);
		if (res == null || res.equals("")) {
			return "";
		}
		return res.toString();
	}

	public boolean saveSystemConfig(Context context, String key, String value)
	{
		return ((Boolean) invokeDeviceControllerMethod(context, sDevMethodSaveSystemConfig, key, value)).booleanValue();
	}

	public EPDMode getEpdMode()
	{
		return mCurrentMode;
	}

	public boolean setEpdMode(Context context, EPDMode epdMode)
	{
		return false;
	}

	public boolean setEpdMode(View view, EPDMode epdMode)
	{
		Object mode_value = getEinkMode(epdMode);
		try
		{
			sMethodViewRequestEpdMode.invoke(view, mode_value);
			return true;
		}
		catch (IllegalArgumentException e)
		{
			Log.e(TAG, "exception", e);
		}
		catch (IllegalAccessException e1)
		{
			Log.e(TAG, "exception", e1);
		}
		catch (InvocationTargetException e2)
		{
			Log.e(TAG, "exception", e2);
		}
		return false;
	}

	public UpdateMode getViewDefaultUpdateMode(View view)
	{
		return mViewDefaultUpdateMode;
	}

	public boolean setViewDefaultUpdateMode(View view, UpdateMode mode)
	{
		Object mode_value = getEinkModeFromUpdateMode(mode);
		if (ReflectUtil.invokeMethodSafely(sMethodViewRequestEpdMode, view, mode_value) != null)
		{
			mViewDefaultUpdateMode = mode;
			return true;
		}
		return false;
	}

	public int getFrontLightBrightnessMinimum(Context context)
	{
		return 0;
	}

	public int getFrontLightBrightnessMaximum(Context context)
	{
		return 255;
	}

	public boolean openFrontLight(Context context)
	{
		Boolean res = (Boolean) invokeDeviceControllerMethod(context, sDevMethodOpenFrontLight, context);
		if (res == null) {
			return false;
		}
		return res;
	}

	public boolean closeFrontLight(Context context)
	{
		Boolean res = (Boolean) invokeDeviceControllerMethod(context, sDevMethodCloseFrontLight, context);
		if (res == null) {
			return false;
		}
		return res;
	}

	public int getFrontLightDeviceValue(Context context)
	{
		Integer res = (Integer) invokeDeviceControllerMethod(context, sDevMethodGetFrontLightValue, context);
		if (res == null) {
			return 0;
		}
		return res;
	}

	public boolean setFrontLightDeviceValue(Context context, int value)
	{
		Object object = invokeDeviceControllerMethod(context, sDevMethodSetFrontLightValue, context, value);
		return object != null;
	}

	public int getFrontLightConfigValue(Context context)
	{
		Integer res = (Integer) invokeDeviceControllerMethod(context, sDevMethodGetFrontLightConfigValue, context);
		return res;
	}

	public boolean setFrontLightConfigValue(Context context, int value)
	{
		invokeDeviceControllerMethod(context, sDevMethodSetFrontLightConfigValue, context, value);
		return true;
	}

	@SuppressWarnings("unchecked")
	public List<Integer> getFrontLightValueList(Context context)
	{
		List<Integer> list = (List<Integer>) invokeDeviceControllerMethod(context, sDevMethodGetFrontLightValues, context);
		return list;
	}

	public UpdateMode getSystemDefaultUpdateMode()
	{
		return null;
	}

	public boolean setSystemDefaultUpdateMode(UpdateMode mode)
	{
		return false;
	}

	public void invalidate(View view, UpdateMode mode)
	{
		Object mode_value = getEinkModeFromUpdateMode(mode);
		try
		{
			sMethodViewRequestEpdMode.invoke(view, mode_value);
		}
		catch (IllegalArgumentException e)
		{
			Log.e(TAG, "exception", e);
		}
		catch (IllegalAccessException e1)
		{
			Log.e(TAG, "exception", e1);
		}
		catch (InvocationTargetException e2)
		{
			Log.e(TAG, "exception", e2);
		}
		view.invalidate();
	}

	public void postInvalidate(View view, UpdateMode mode)
	{
		Object mode_value = getEinkModeFromUpdateMode(mode);
		try
		{
			sMethodViewRequestEpdMode.invoke(view, mode_value);
		}
		catch (IllegalArgumentException e)
		{
			Log.e(TAG, "exception", e);
		}
		catch (IllegalAccessException e1)
		{
			Log.e(TAG, "exception", e1);
		}
		catch (InvocationTargetException e2)
		{
			Log.e(TAG, "exception", e2);
		}
		view.postInvalidate();
	}

	@SuppressWarnings("unchecked")
	public String getEncryptedDeviceID()
	{
		Class cls = null;
		try
		{
			cls = Class.forName(ANDROID_OS_SYSTEM_PROPERTIES);
			Log.i(TAG, "Class: android.os.SystemProperties found!");
		}
		catch (ClassNotFoundException e)
		{
			Log.w(TAG, "Class: android.os.SystemProperties not found!");
			return null;
		}
		Method method = null;
		String methodsig = "get";
		try
		{
			method = cls.getMethod(methodsig, String.class, String.class);
		}
		catch (NoSuchMethodException e)
		{
			Log.w(TAG, "Method: " + methodsig + " not found!");
			return null;
		}
		String res = null;
		try
		{
			res = (String)method.invoke(null, RO_DEVICEID, UNKNOWN);
		}
		catch (IllegalArgumentException e)
		{
			e.printStackTrace();
			Log.w(TAG, "invoke android.os.SystemProperties." + methodsig + " exception, illegal argument!");
			return null;
		}
		catch (IllegalAccessException e1)
		{
			e1.printStackTrace();
			Log.w(TAG, "invoke android.os.SystemProperties." + methodsig + " exception, illegal access!");
			return null;
		}
		catch (InvocationTargetException e2)
		{
			e2.printStackTrace();
			Log.w(TAG, "invoke android.os.SystemProperties." + methodsig + " exception, invocation target exception!");
			return null;
		}
		return res;
	}

	public void stopBootAnimation()
	{
		invokeDeviceControllerMethod(null, sMethodRequestStopBootAnimation);
	}

	public void led(Context context, boolean on)
	{
		invokeDeviceControllerMethod(context, sMethodLed, on);
	}

	public boolean supportRegal()
	{
		Boolean res = (Boolean) ReflectUtil.invokeMethodSafely(sMethodSupportRegal, null);
		if (res == null)
			return false;
		return res;
	}

	public void holdDisplay(boolean hold, UpdateMode updateMode, int ignoreFrame)
	{
		ReflectUtil.invokeMethodSafely(sMethodHoldDisplay, null, hold, Integer.valueOf(o), ignoreFrame);
	}

	public int getVCom(Context context, String path)
	{
		String str = FileUtils.readContentOfFile(new File(path));
		if (StringUtils.isNullOrEmpty(str)) {
			return Integer.MIN_VALUE;
		}
		return Integer.parseInt(str);
	}

	public void setVCom(Context context, int value, String path)
	{
		FileUtils.saveContentToFile(String.valueOf(value), new File(path));
	}

	public void disableA2ForSpecificView(View view)
	{
		ReflectUtil.invokeMethodSafely(sMethodDisableA2, view);
	}

	public void enableA2ForSpecificView(View view)
	{
		ReflectUtil.invokeMethodSafely(sMethodEnableA2, view);
	}

	public boolean isLegalSystem(Context context)
	{
		return ((Boolean) invokeDeviceControllerMethod(context, sDevMethodSystemIntegrityCheck));
	}

	public boolean hasWifi(Context context)
	{
		Boolean res = (Boolean) invokeDeviceControllerMethod(context, sDevMethodHasWifi, context);
		if (res == null)
			return false;
		return res;
	}

	public boolean hasAudio(Context context)
	{
		Boolean res = (Boolean) invokeDeviceControllerMethod(context, sDevMethodHasAudio, context);
		if (res == null)
			return false;
		return res;
	}

	public boolean hasFrontLight(Context context)
	{
		Boolean res = (Boolean) invokeDeviceControllerMethod(context, sDevMethodHasFrontLight, context);
		if (res == null)
			return false;
		return res;
	}

	public boolean hasBluetooth(Context context)
	{
		Boolean res = (Boolean) invokeDeviceControllerMethod(context, sDevMethodHasBluetooth, context);
		if (res == null)
			return false;
		return res;
	}

	public boolean isTouchable(Context context)
	{
		Boolean res = (Boolean) invokeDeviceControllerMethod(context, sDevMethodIsTouchable, context);
		if (res == null)
			return false;
		return res;
	}

	public void enableRegal(boolean enable)
	{
		ReflectUtil.invokeMethodSafely(sMethodEnableRegal, null, enable);
	}

	private Object invokeDeviceControllerMethod(Context context, Method method, Object... args)
	{
		if (method == null) {
			return null;
		}
		return ReflectUtil.invokeMethodSafely(method, null, args);
	}

	@SuppressWarnings("unchecked")
	private Object getValueOfEinkModeEnum(String name)
	{
		return Enum.valueOf(sEinkModeEnumClass, name);
	}

	private Object getEinkMode(EPDMode epd)
	{
		String str = EPD_NULL;
		switch (epd)
		{
			case FULL:
				str = EPD_FULL;
				break;
			case AUTO:
				str = EPD_PART;
				break;
			case TEXT:
				str = EPD_PART;
				break;
			case AUTO_PART:
				str = EPD_PART;
				break;
			case AUTO_BLACK_WHITE:
				str = EPD_A2;
				break;
			case AUTO_A2:
				str = EPD_A2;
				break;
			case EPD_REGLA:
				str = EPD_REGLA;
				break;
			default:
				assert(false);
				break;
		}
		return getValueOfEinkModeEnum(str);
	}

	private Object getEinkModeFromUpdateMode(UpdateMode mode)
	{
		String str = EPD_NULL;
		switch (mode)
		{
			case GU:
				str = EPD_PART;
				break;
			case GU_FAST:
				str = EPD_PART;
				break;
			case GC:
				str = EPD_FULL;
				break;
			case DU:
				str = EPD_A2;
				break;
			case REGAL:
				str = EPD_REGLA;
				break;
			default:
				assert(false);
				break;
		}
		return getValueOfEinkModeEnum(str);
	}
}
