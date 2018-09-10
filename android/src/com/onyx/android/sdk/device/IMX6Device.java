package com.onyx.android.sdk.device;

import android.content.Context;
import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Build;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.webkit.WebView;

import com.onyx.android.sdk.api.device.epd.EPDMode;
import com.onyx.android.sdk.api.device.epd.UpdateMode;
import com.onyx.android.sdk.api.device.epd.UpdatePolicy;
import com.onyx.android.sdk.api.device.epd.UpdateScheme;
import com.onyx.android.sdk.utils.ReflectUtil;

import java.io.File;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.List;

public class IMX6Device
		extends BaseDevice {
	private static final String TAG = IMX6Device.class.getSimpleName();
	private static IMX6Device sInstance = null;
	private static int sPolicyAutomatic = 0;
	private static int sPolicyGUIntervally = 0;
	private static int sModeDW = 0;
	private static int sModeGU = 0;
	private static int sModeGC = 0;
	private static int sModeAnim = 0;
	private static int sModeAnimQuality = 0;
	private static int sModeGC4 = 0;
	private static int sModeReagl = 0;
	private static int sModeReagld = 0;
	private static final int sSchemeSNAPSHOT = 0;
	private static final int sSchemeQUEUE = 1;
	private static final int sSchemeQUEUE_AND_MERGE = 2;
	private static Method sMethodGetWindowRotation = null;
	private static Method sMethodSetWindowRotation = null;
	private static Method sMethodSetUpdatePolicy = null;
	private static Method sMethodRefreshScreen = null;
	private static Method sMethodRefreshScreenRegion = null;
	private static Method sMethodScreenshot = null;
	private static Method sMethodSupportRegal = null;
	private static Method sMethodEnableRegal = null;
	private static Method sMethodMoveTo = null;
	private static Method sMethodViewMoveTo = null;
	private static Method sMethodSetStrokeColor = null;
	private static Method sMethodSetStrokeStyle = null;
	private static Method sMethodSetStrokeWidth = null;
	private static Method sMethodSetPainterStyle = null;
	private static Method sMethodLineTo = null;
	private static Method sMethodViewLineTo = null;
	private static Method sMethodQuadTo = null;
	private static Method sMethodViewQuadTo = null;
	private static Method sMethodGetTouchWidth = null;
	private static Method sMethodGetTouchHeight = null;
	private static Method sMethodGetMaxTouchPressure = null;
	private static Method sMethodGetEpdWidth = null;
	private static Method sMethodGetEpdHeight = null;
	private static Method sMethodMapToView = null;
	private static Method sMethodMapToEpd = null;
	private static Method sMethodMapFromRawTouchPoint = null;
	private static Method sMethodMapToRawTouchPoint = null;
	private static Method sMethodEnablePost = null;
	private static Method sMethodResetEpdPost = null;
	private static Method sMethodSetScreenHandWritingPenState = null;
	private static Method sMethodSetScreenHandWritingRegionLimit = null;
	private static Method sMethodSetScreenHandWritingRegionExclude = null;
	private static Method sMethodApplyGammaCorrection = null;
	private static Method sMethodStartStroke = null;
	private static Method sMethodAddStrokePoint = null;
	private static Method sMethodFinishStroke = null;
	private static Method sMethodEnableA2;
	private static Method sMethodDisableA2;
	private static Method sWebViewMethodSetCssInjectEnabled;
	private static Method sMethodSetQRShowConfig;
	private static Method sMethodSetInfoShowConfig;
	private static Method sEnvMethodGetRemovableSDCardDirectory;
	private static Method sMethodPostInvalidate = null;
	private static Method sMethodInvalidate = null;
	private static Method sMethodInvalidateRegion = null;
	private static Method sMethodInSystemFastMode = null;
	private static Method sMethodGetViewDefaultUpdateMode = null;
	private static Method sMethodResetViewUpdateMode = null;
	private static Method sMethodSetViewDefaultUpdateMode = null;
	private static Method sMethodGetSystemDefaultUpdateMode = null;
	private static Method sMethodSetSystemDefaultUpdateMode = null;
	private static Method sMethodSetFirstDrawUpdateMode = null;
	private static Method sMethodSetSystemUpdateModeAndScheme = null;
	private static Method sMethodApplyApplicationFastMode = null;
	private static Method sMethodClearSystemUpdateModeAndScheme = null;
	private static Method sMethodEnableScreenUpdate = null;
	private static Method sMethodSetDisplayScheme = null;
	private static Method sMethodWaitForUpdateFinished = null;
	private static Method sDevMethodOpenFrontLight;
	private static Method sDevMethodCloseFrontLight;
	private static Method sDevMethodGetFrontLightValue;
	private static Method sDevMethodSetFrontLightValue;
	private static Method sDevMethodSetNaturalLightValue;
	private static Method sDevMethodGetFrontLightConfigValue;
	private static Method sDevMethodSetFrontLightConfigValue;
	private static Method sDevMethodLed;
	private static Method sDevMethodSetLedColor;
	private static Method sDevMethodSetVCom;
	private static Method sDevMethodGetVCom;
	private static Method sDevMethodUpdateWaveform;
	private static Method sDevMethodReadSystemConfig;
	private static Method sDevMethodSaveSystemConfig;
	private static Method sDevMethodUpdateMetadataDB;
	private static Method sDevMethodGotoSleep;
	private static Method sDevMethodUseBigPen;
	private static Method sDevMethodStopTpd;
	private static Method sDevMethodStartTpd;
	private static Method sMethodEnableOnyxTpd;
	private static Method sDevMethodHasWifi;
	private static Method sMethodApplyGCOnce;

	public static IMX6Device createDevice() {
		if (sInstance == null) {
			sInstance = new IMX6Device();

			Class cls = View.class;

			sMethodGetWindowRotation = ReflectUtil.getMethodSafely(cls, "getWindowRotation");
			sMethodSetWindowRotation = ReflectUtil.getMethodSafely(cls, "setWindowRotation", Integer.TYPE, Boolean.TYPE, Integer.TYPE);
			sMethodInSystemFastMode = ReflectUtil.getMethodSafely(cls, "inSystemFastMode");

			int value_auto_mask = ReflectUtil.getStaticIntFieldSafely(cls, "EINK_ONYX_AUTO_MASK");
			int value_gc_mask = ReflectUtil.getStaticIntFieldSafely(cls, "EINK_ONYX_GC_MASK");
			int value_mode_regional = ReflectUtil.getStaticIntFieldSafely(cls, "EINK_AUTO_MODE_REGIONAL");
			int value_mode_nowait = ReflectUtil.getStaticIntFieldSafely(cls, "EINK_WAIT_MODE_NOWAIT");
			int value_mode_wait = ReflectUtil.getStaticIntFieldSafely(cls, "EINK_WAIT_MODE_WAIT");
			int value_mode_waveform_du = ReflectUtil.getStaticIntFieldSafely(cls, "EINK_WAVEFORM_MODE_DU");
			int value_mode_waveform_anim = ReflectUtil.getStaticIntFieldSafely(cls, "EINK_WAVEFORM_MODE_ANIM");
			int value_mode_waveform_gc4 = ReflectUtil.getStaticIntFieldSafely(cls, "EINK_WAVEFORM_MODE_GC4");
			int value_mode_waveform_gc16 = ReflectUtil.getStaticIntFieldSafely(cls, "EINK_WAVEFORM_MODE_GC16");
			int value_mode_waveform_reagl = ReflectUtil.getStaticIntFieldSafely(cls, "EINK_WAVEFORM_MODE_REAGL");
			int value_mode_reagl_reagld = ReflectUtil.getStaticIntFieldSafely(cls, "EINK_REAGL_MODE_REAGLD");
			int value_mode_update_partial = ReflectUtil.getStaticIntFieldSafely(cls, "EINK_UPDATE_MODE_PARTIAL");
			int value_mode_update_full = ReflectUtil.getStaticIntFieldSafely(cls, "EINK_UPDATE_MODE_FULL");

			sPolicyAutomatic = value_auto_mask;
			sPolicyGUIntervally = value_gc_mask;

			sModeDW = value_mode_regional | value_mode_nowait | value_mode_waveform_du | value_mode_update_partial;
			sModeGU = value_mode_regional | value_mode_nowait | value_mode_waveform_gc16 | value_mode_update_partial;
			sModeGC = value_mode_regional | value_mode_wait | value_mode_waveform_gc16 | value_mode_update_full;
			sModeAnim = value_mode_regional | value_mode_nowait | value_mode_waveform_anim | value_mode_update_partial;
			sModeAnimQuality = ReflectUtil.getStaticIntFieldSafely(cls, "UI_A2_QUALITY_MODE");
			sModeGC4 = value_mode_regional | value_mode_nowait | value_mode_waveform_gc4 | value_mode_update_partial;
			sModeReagl = value_mode_regional | value_mode_nowait | value_mode_waveform_reagl | value_mode_update_partial;
			sModeReagld = value_mode_regional | value_mode_nowait | value_mode_reagl_reagld | value_mode_waveform_reagl | value_mode_update_partial;

			Class devCtrlCls = ReflectUtil.classForName("android.hardware.DeviceController");

			sDevMethodOpenFrontLight = ReflectUtil.getMethodSafely(devCtrlCls, "openFrontLight", Context.class);
			sDevMethodCloseFrontLight = ReflectUtil.getMethodSafely(devCtrlCls, "closeFrontLight", Context.class);
			sDevMethodGetFrontLightValue = ReflectUtil.getMethodSafely(devCtrlCls, "getFrontLightValue", Context.class);
			sDevMethodSetFrontLightValue = ReflectUtil.getMethodSafely(devCtrlCls, "setFrontLightValue", Context.class, Integer.TYPE);
			sDevMethodSetNaturalLightValue = ReflectUtil.getMethodSafely(devCtrlCls, "setNaturalLightValue", Context.class, Integer.TYPE);
			sDevMethodGetFrontLightConfigValue = ReflectUtil.getMethodSafely(devCtrlCls, "getFrontLightConfigValue", Context.class);
			sDevMethodSetFrontLightConfigValue = ReflectUtil.getMethodSafely(devCtrlCls, "setFrontLightConfigValue", Context.class, Integer.TYPE);
			sDevMethodUseBigPen = ReflectUtil.getMethodSafely(devCtrlCls, "useBigPen", Boolean.TYPE);
			sDevMethodStopTpd = ReflectUtil.getMethodSafely(devCtrlCls, "stopTpd");
			sDevMethodStartTpd = ReflectUtil.getMethodSafely(devCtrlCls, "startTpd");
			sDevMethodGotoSleep = ReflectUtil.getMethodSafely(devCtrlCls, "gotoSleep", Context.class, Long.TYPE);

			sDevMethodLed = ReflectUtil.getMethodSafely(devCtrlCls, "led", Boolean.TYPE);
			sDevMethodSetLedColor = ReflectUtil.getMethodSafely(devCtrlCls, "setLedColor", String.class, Integer.TYPE);

			sMethodEnableOnyxTpd = ReflectUtil.getMethodSafely(cls, "enableOnyxTpd", Integer.TYPE);
			sDevMethodHasWifi = ReflectUtil.getMethodSafely(devCtrlCls, "hasWifi");

			sMethodSetUpdatePolicy = ReflectUtil.getMethodSafely(cls, "setUpdatePolicy", Integer.TYPE, Integer.TYPE);

			sMethodPostInvalidate = ReflectUtil.getMethodSafely(cls, "postInvalidate", Integer.TYPE);

			sMethodRefreshScreen = ReflectUtil.getMethodSafely(cls, "refreshScreen", Integer.TYPE);
			sMethodRefreshScreenRegion = ReflectUtil.getMethodSafely(cls, "refreshScreen", Integer.TYPE, Integer.TYPE, Integer.TYPE, Integer.TYPE, Integer.TYPE);
			sMethodScreenshot = ReflectUtil.getMethodSafely(cls, "screenshot", Integer.TYPE, String.class);

			sMethodSetStrokeColor = ReflectUtil.getMethodSafely(cls, "setStrokeColor", Integer.TYPE);
			sMethodSetStrokeStyle = ReflectUtil.getMethodSafely(cls, "setStrokeStyle", Integer.TYPE);
			sMethodSetStrokeWidth = ReflectUtil.getMethodSafely(cls, "setStrokeWidth", Float.TYPE);
			sMethodSetPainterStyle = ReflectUtil.getMethodSafely(cls, "setPainterStyle", Boolean.TYPE, Paint.Style.class, Paint.Join.class, Paint.Cap.class);
			sMethodSupportRegal = ReflectUtil.getMethodSafely(cls, "supportRegal");
			sMethodEnableRegal = ReflectUtil.getMethodSafely(cls, "enableRegal", Boolean.TYPE);
			sMethodMoveTo = ReflectUtil.getMethodSafely(cls, "moveTo", Float.TYPE, Float.TYPE, Float.TYPE);
			sMethodLineTo = ReflectUtil.getMethodSafely(cls, "lineTo", Float.TYPE, Float.TYPE, Integer.TYPE);
			sMethodQuadTo = ReflectUtil.getMethodSafely(cls, "quadTo", Float.TYPE, Float.TYPE, Integer.TYPE);
			sMethodViewMoveTo = ReflectUtil.getMethodSafely(cls, "moveTo", View.class, Float.TYPE, Float.TYPE, Float.TYPE);
			sMethodViewLineTo = ReflectUtil.getMethodSafely(cls, "lineTo", View.class, Float.TYPE, Float.TYPE, Integer.TYPE);
			sMethodViewQuadTo = ReflectUtil.getMethodSafely(cls, "quadTo", View.class, Float.TYPE, Float.TYPE, Integer.TYPE);
			sMethodGetTouchWidth = ReflectUtil.getMethodSafely(cls, "getTouchWidth");
			sMethodGetTouchHeight = ReflectUtil.getMethodSafely(cls, "getTouchHeight");
			sMethodGetMaxTouchPressure = ReflectUtil.getMethodSafely(cls, "getMaxTouchPressure");
			sMethodGetEpdWidth = ReflectUtil.getMethodSafely(cls, "getEpdWidth");
			sMethodGetEpdHeight = ReflectUtil.getMethodSafely(cls, "getEpdHeight");
			sMethodMapToView = ReflectUtil.getMethodSafely(cls, "mapToView", View.class, float[].class, float[].class);
			sMethodMapToEpd = ReflectUtil.getMethodSafely(cls, "mapToEpd", View.class, float[].class, float[].class);
			sMethodMapFromRawTouchPoint = ReflectUtil.getMethodSafely(cls, "mapFromRawTouchPoint", View.class, float[].class, float[].class);
			sMethodMapToRawTouchPoint = ReflectUtil.getMethodSafely(cls, "mapToRawTouchPoint", View.class, float[].class, float[].class);
			sMethodEnablePost = ReflectUtil.getMethodSafely(cls, "enablePost", Integer.TYPE);
			sMethodResetEpdPost = ReflectUtil.getMethodSafely(cls, "resetEpdPost");
			sMethodSetScreenHandWritingPenState = ReflectUtil.getMethodSafely(cls, "setScreenHandWritingPenState", Integer.TYPE);
			sMethodSetScreenHandWritingRegionLimit = ReflectUtil.getMethodSafely(cls, "setScreenHandWritingRegionLimit", View.class, int[].class);
			sMethodSetScreenHandWritingRegionExclude = ReflectUtil.getMethodSafely(cls, "setScreenHandWritingRegionExclude", View.class, int[].class);
			sMethodApplyGammaCorrection = ReflectUtil.getMethodSafely(cls, "applyGammaCorrection", Boolean.TYPE, Integer.TYPE);

			sMethodStartStroke = ReflectUtil.getMethodSafely(cls, "startStroke", Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE);
			sMethodAddStrokePoint = ReflectUtil.getMethodSafely(cls, "addStrokePoint", Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE);
			sMethodFinishStroke = ReflectUtil.getMethodSafely(cls, "finishStroke", Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE);

			sMethodInvalidate = ReflectUtil.getMethodSafely(cls, "invalidate", Integer.TYPE);
			sMethodInvalidateRegion = ReflectUtil.getMethodSafely(cls, "invalidate", Integer.TYPE, Integer.TYPE, Integer.TYPE, Integer.TYPE, Integer.TYPE);

			sMethodSetViewDefaultUpdateMode = ReflectUtil.getMethodSafely(cls, "setDefaultUpdateMode", Integer.TYPE);

			sMethodGetViewDefaultUpdateMode = ReflectUtil.getMethodSafely(cls, "getDefaultUpdateMode");
			sMethodResetViewUpdateMode = ReflectUtil.getMethodSafely(cls, "resetUpdateMode");

			sMethodGetSystemDefaultUpdateMode = ReflectUtil.getMethodSafely(cls, "getGlobalUpdateMode");

			sMethodSetSystemDefaultUpdateMode = ReflectUtil.getMethodSafely(cls, "setGlobalUpdateMode", Integer.TYPE);

			sMethodSetFirstDrawUpdateMode = ReflectUtil.getMethodSafely(cls, "setFirstDrawUpdateMode", Integer.TYPE);

			sMethodSetSystemUpdateModeAndScheme = ReflectUtil.getMethodSafely(cls, "setWaveformAndScheme", Integer.TYPE, Integer.TYPE, Integer.TYPE);

			sMethodClearSystemUpdateModeAndScheme = ReflectUtil.getMethodSafely(cls, "resetWaveformAndScheme");
			sMethodApplyApplicationFastMode = ReflectUtil.getMethodSafely(cls, "applyApplicationFastMode", String.class, Boolean.TYPE, Boolean.TYPE);
			sMethodEnableScreenUpdate = ReflectUtil.getMethodSafely(cls, "enableScreenUpdate", Boolean.TYPE);
			sMethodSetDisplayScheme = ReflectUtil.getMethodSafely(cls, "setDisplayScheme", Integer.TYPE);
			sMethodWaitForUpdateFinished = ReflectUtil.getMethodSafely(cls, "waitForUpdateFinished");

			sDevMethodSetVCom = ReflectUtil.getMethodSafely(devCtrlCls, "setVCom", Context.class, Integer.TYPE, String.class);
			sDevMethodGetVCom = ReflectUtil.getMethodSafely(devCtrlCls, "getVCom", String.class);
			sDevMethodUpdateWaveform = ReflectUtil.getMethodSafely(devCtrlCls, "updateWaveform", String.class, String.class);
			sDevMethodReadSystemConfig = ReflectUtil.getMethodSafely(devCtrlCls, "readSystemConfig", String.class);
			sDevMethodSaveSystemConfig = ReflectUtil.getMethodSafely(devCtrlCls, "saveSystemConfig", String.class, String.class);
			sDevMethodUpdateMetadataDB = ReflectUtil.getMethodSafely(devCtrlCls, "updateMetadataDB", String.class, String.class);

			sMethodEnableA2 = ReflectUtil.getMethodSafely(cls, "enableA2");

			sMethodDisableA2 = ReflectUtil.getMethodSafely(cls, "disableA2");

			sWebViewMethodSetCssInjectEnabled = ReflectUtil.getMethodSafely(WebView.class, "setCssInjectEnabled", Boolean.TYPE);

			sMethodSetQRShowConfig = ReflectUtil.getMethodSafely(cls, "setQRShowConfig", Integer.TYPE, Integer.TYPE, Integer.TYPE);
			sMethodSetInfoShowConfig = ReflectUtil.getMethodSafely(cls, "setInfoShowConfig", Integer.TYPE, Integer.TYPE, Integer.TYPE);

			sEnvMethodGetRemovableSDCardDirectory = ReflectUtil.getMethodSafely(Environment.class, "getRemovableSDCardDirectory");
			sMethodApplyGCOnce = ReflectUtil.getMethodSafely(cls, "applyGCOnce");
			Log.d(TAG, "init device EINK_ONYX_GC_MASK.");
		}
		return sInstance;
	}

	public int getWindowRotation() {
		if (sMethodGetWindowRotation != null) {
			try {
				return ((Integer) sMethodGetWindowRotation.invoke(null)).intValue();
			} catch (IllegalArgumentException localIllegalArgumentException) {
				Log.w(TAG, localIllegalArgumentException);
			} catch (IllegalAccessException localIllegalAccessException) {
				Log.w(TAG, localIllegalAccessException);
			} catch (InvocationTargetException localInvocationTargetException) {
				Log.w(TAG, localInvocationTargetException);
			}
		}
		return 0;
	}

	public boolean setWindowRotation(int rotation) {
		if (sMethodSetWindowRotation != null) {
			try {
				int i1 = 1;
				sMethodSetWindowRotation.invoke(null, rotation, true, 1);
				return true;
			} catch (IllegalArgumentException e) {
				Log.w(TAG, e);
			} catch (IllegalAccessException e1) {
				Log.w(TAG, e1);
			} catch (InvocationTargetException e2) {
				Log.w(TAG, e2);
			}
		}
		return false;
	}

	public boolean setUpdatePolicy(View view, UpdatePolicy policy, int guInterval) {
		int dst_policy = getPolicyValue(policy);
		try {
			assert (sMethodSetUpdatePolicy != null);
			sMethodSetUpdatePolicy.invoke(view, dst_policy, guInterval);
			return true;
		} catch (IllegalArgumentException e) {
		} catch (IllegalAccessException e1) {
		} catch (InvocationTargetException e2) {
		}
		return false;
	}

	public File getStorageRootDirectory() {
		return new File("/mnt");
	}

	public File getExternalStorageDirectory() {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
			return Environment.getExternalStorageDirectory();
		}
		return new File("/mnt/sdcard");
	}

	public File getRemovableSDCardDirectory() {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
			File file = (File) ReflectUtil.invokeMethodSafely(sEnvMethodGetRemovableSDCardDirectory, null);
			if (file != null) {
				return file;
			}
		}
		return new File("/mnt/extsd");
	}

	public boolean isFileOnRemovableSDCard(File file) {
		return file.getAbsolutePath().startsWith(getRemovableSDCardDirectory().getAbsolutePath());
	}

	public EPDMode getEpdMode() {
		return EPDMode.AUTO;
	}

	public boolean setEpdMode(Context context, EPDMode mode) {
		setSystemUpdateModeAndScheme(updateModeFromEPD(mode), UpdateScheme.QUEUE_AND_MERGE, Integer.MAX_VALUE);
		return false;
	}

	public void invalidate(View view, UpdateMode mode) {
		int dst_mode_value = getUpdateMode(mode);
		try {
			assert (sMethodInvalidate != null);
			sMethodInvalidate.invoke(view, dst_mode_value);
		} catch (IllegalArgumentException e) {
		} catch (IllegalAccessException e1) {
		} catch (InvocationTargetException e2) {
		}
	}

	public void invalidate(View view, int left, int top, int right, int bottom, UpdateMode mode) {
		int dst_mode_value = getUpdateMode(mode);
		try {
			assert (sMethodInvalidateRegion != null);
			sMethodInvalidateRegion.invoke(view, left, top, right, bottom, dst_mode_value);
		} catch (IllegalArgumentException e) {
		} catch (IllegalAccessException e1) {
		} catch (InvocationTargetException e2) {
		}
	}

	public void postInvalidate(View view, UpdateMode mode) {
		int dst_mode_value = getUpdateMode(mode);
		try {
			assert (sMethodPostInvalidate != null);
			Log.d(TAG, "dst mode: " + dst_mode_value);
			sMethodPostInvalidate.invoke(view, dst_mode_value);
			return;
		} catch (IllegalArgumentException e) {
		} catch (IllegalAccessException e1) {
		} catch (InvocationTargetException e2) {
		}
	}

	public void refreshScreen(View view, UpdateMode mode) {
		int dst_mode_value = getUpdateMode(mode);
		try {
			assert (sMethodRefreshScreen != null);
			sMethodRefreshScreen.invoke(view, new Object[]{Integer.valueOf(dst_mode_value)});
		} catch (IllegalArgumentException e) {
		} catch (IllegalAccessException e1) {
		} catch (InvocationTargetException e2) {
		}
	}

	public void refreshScreenRegion(View view, int left, int top, int width, int height, UpdateMode mode) {
		int dst_mode_value = getUpdateMode(mode);
		try {
			assert (sMethodRefreshScreenRegion != null);
			sMethodRefreshScreenRegion.invoke(view, left, top, width, height, dst_mode_value);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void screenshot(View view, int rotation, String path) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodScreenshot, view, rotation, path);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void setStrokeColor(int color) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodSetStrokeColor, null, color);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void setStrokeStyle(int style) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodSetStrokeStyle, null, style);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void setPainterStyle(boolean antiAlias, Paint.Style strokeStyle, Paint.Join joinStyle, Paint.Cap capStyle) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodSetPainterStyle, null, antiAlias, strokeStyle, joinStyle, capStyle);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void setStrokeWidth(float width) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodSetStrokeWidth, null, width);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void moveTo(float x, float y, float width) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodMoveTo, null, x, y, width);
		} catch (Exception localException) {
			localException.printStackTrace();
		}
	}

	public void moveTo(View view, float x, float y, float width) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodViewMoveTo, null, view, x, y, width);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public boolean supportDFB() {
		return sMethodLineTo != null;
	}

	public boolean supportRegal() {
		if (sMethodSupportRegal == null) {
			return false;
		}
		Boolean res = (Boolean) ReflectUtil.invokeMethodSafely(sMethodSupportRegal, null);
		if (res == null) {
			return false;
		}
		return res.booleanValue();
	}

	public void enableRegal(boolean enable) {
		ReflectUtil.invokeMethodSafely(sMethodEnableRegal, null, enable);
	}

	public void lineTo(float x, float y, UpdateMode mode) {
		int dst_mode_value = getUpdateMode(mode);
		try {
			ReflectUtil.invokeMethodSafely(sMethodLineTo, null, x, y, dst_mode_value);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void lineTo(View view, float x, float y, UpdateMode mode) {
		int dst_mode_value = getUpdateMode(mode);
		try {
			ReflectUtil.invokeMethodSafely(sMethodViewLineTo, null, view, x, y, dst_mode_value);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void quadTo(float x, float y, UpdateMode mode) {
		int dst_mode_value = getUpdateMode(mode);
		try {
			ReflectUtil.invokeMethodSafely(sMethodQuadTo, null, x, y, dst_mode_value);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void quadTo(View view, float x, float y, UpdateMode mode) {
		int dst_mode_value = getUpdateMode(mode);
		try {
			ReflectUtil.invokeMethodSafely(sMethodViewQuadTo, null, view, x, y, dst_mode_value);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public float getTouchWidth() {
		try {
			Float value = (Float) ReflectUtil.invokeMethodSafely(sMethodGetTouchWidth, null);
			return value.floatValue();
		} catch (Exception e) {
			e.printStackTrace();
		}
		return 0.0F;
	}

	public float getTouchHeight() {
		try {
			Float value = (Float) ReflectUtil.invokeMethodSafely(sMethodGetTouchHeight, null);
			return value.floatValue();
		} catch (Exception e) {
			e.printStackTrace();
		}
		return 0.0F;
	}

	public float getMaxTouchPressure() {
		try {
			Float value = (Float) ReflectUtil.invokeMethodSafely(sMethodGetMaxTouchPressure, null);
			return value.floatValue();
		} catch (Exception e) {
		}
		return 1024.0F;
	}

	public float getEpdWidth() {
		try {
			Float value = (Float) ReflectUtil.invokeMethodSafely(sMethodGetEpdWidth, null);
			return value.floatValue();
		} catch (Exception e) {
			e.printStackTrace();
		}
		return 0.0F;
	}

	public float getEpdHeight() {
		try {
			Float value = (Float) ReflectUtil.invokeMethodSafely(sMethodGetEpdHeight, null);
			return value.floatValue();
		} catch (Exception e) {
			e.printStackTrace();
		}
		return 0.0F;
	}

	public void mapToView(View view, float[] src, float[] dst) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodMapToView, null, view, src, dst);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void mapToEpd(View view, float[] src, float[] dst) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodMapToEpd, null, view, src, dst);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void mapFromRawTouchPoint(View view, float[] src, float[] dst) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodMapFromRawTouchPoint, null, view, src, dst);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void mapToRawTouchPoint(View view, float[] src, float[] dst) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodMapToRawTouchPoint, null, view, src, dst);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public float startStroke(float baseWidth, float x, float y, float pressure, float size, float time) {
		try {
			Float value = (Float) ReflectUtil.invokeMethodSafely(sMethodStartStroke, null, baseWidth, x, y, pressure, size, time);
			return value.floatValue();
		} catch (Exception e) {
			e.printStackTrace();
		}
		return baseWidth;
	}

	public float addStrokePoint(float baseWidth, float x, float y, float pressure, float size, float time) {
		try {
			Float value = (Float) ReflectUtil.invokeMethodSafely(sMethodAddStrokePoint, null, baseWidth, x, y, pressure, size, time);
			return value.floatValue();
		} catch (Exception e) {
			e.printStackTrace();
		}
		return baseWidth;
	}

	public float finishStroke(float baseWidth, float x, float y, float pressure, float size, float time) {
		try {
			Float value = (Float) ReflectUtil.invokeMethodSafely(sMethodFinishStroke, null, baseWidth, x, y, pressure, size, time);
			return value.floatValue();
		} catch (Exception e) {
			e.printStackTrace();
		}
		return baseWidth;
	}

	public void enterScribbleMode(View view) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodEnablePost, view, 0);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void leaveScribbleMode(View view) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodEnablePost, view, 1);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void enablePost(View view, int enable) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodEnablePost, view, enable);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void resetEpdPost() {
		try {
			ReflectUtil.invokeMethodSafely(sMethodResetEpdPost, null);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public boolean supportScreenHandWriting() {
		return sMethodSetScreenHandWritingPenState != null;
	}

	public void setScreenHandWritingPenState(View view, int penState) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodSetScreenHandWritingPenState, view, penState);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void setScreenHandWritingRegionLimit(View view) {
		if (view == null) {
			return;
		}
		setScreenHandWritingRegionLimit(view, 0, 0, view.getRight(), view.getBottom());
	}

	public void setScreenHandWritingRegionLimit(View view, int left, int top, int right, int bottom) {
		setScreenHandWritingRegionLimit(view, new int[]{left, top, right, bottom});
	}

	public void setScreenHandWritingRegionLimit(View view, int[] array) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodSetScreenHandWritingRegionLimit, view, view, array);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void setScreenHandWritingRegionLimit(View view, Rect[] regions) {
		setScreenHandWritingRegionLimit(view, regionsToArray(regions));
	}

	public void setScreenHandWritingRegionExclude(View view, int[] array) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodSetScreenHandWritingRegionExclude, view, view, array);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void setScreenHandWritingRegionExclude(View view, Rect[] regions) {
		setScreenHandWritingRegionExclude(view, regionsToArray(regions));
	}

	public void applyGammaCorrection(boolean apply, int value) {
		ReflectUtil.invokeMethodSafely(sMethodApplyGammaCorrection, null, apply, value);
	}

	public boolean enableScreenUpdate(View view, boolean enable) {
		try {
			sMethodEnableScreenUpdate.invoke(view, enable);
		} catch (Exception e) {
			e.printStackTrace();
		}
		return true;
	}

	public boolean setDisplayScheme(int scheme) {
		ReflectUtil.invokeMethodSafely(sMethodSetDisplayScheme, null, scheme);
		return true;
	}

	public void waitForUpdateFinished() {
		ReflectUtil.invokeMethodSafely(sMethodWaitForUpdateFinished, null);
	}

	public void useBigPen(boolean use) {
		invokeDeviceControllerMethod(null, sDevMethodUseBigPen, use);
	}

	public void stopTpd() {
		invokeDeviceControllerMethod(null, sDevMethodStopTpd);
	}

	public void startTpd() {
		invokeDeviceControllerMethod(null, sDevMethodStartTpd);
	}

	public void enableTpd(boolean enable) {
		ReflectUtil.invokeMethodSafely(sMethodEnableOnyxTpd, null, (enable ? 1 : 0));
	}

	public UpdateMode getViewDefaultUpdateMode(View view) {
		Integer res = (Integer) ReflectUtil.invokeMethodSafely(sMethodGetViewDefaultUpdateMode, view);
		if (res == null) {
			return UpdateMode.GU;
		}
		return updateModeFromValue(res.intValue());
	}

	public void resetViewUpdateMode(View view) {
		ReflectUtil.invokeMethodSafely(sMethodResetViewUpdateMode, view);
	}

	public boolean setViewDefaultUpdateMode(View view, UpdateMode mode) {
		Object res = ReflectUtil.invokeMethodSafely(sMethodSetViewDefaultUpdateMode, view, getUpdateMode(mode));
		return res != null;
	}

	public UpdateMode getSystemDefaultUpdateMode() {
		Integer res = (Integer) ReflectUtil.invokeMethodSafely(sMethodGetSystemDefaultUpdateMode, null);
		if (res == null) {
			return UpdateMode.GU;
		}
		return updateModeFromValue(res.intValue());
	}

	public boolean setSystemDefaultUpdateMode(UpdateMode mode) {
		Object res = ReflectUtil.invokeMethodSafely(sMethodSetSystemDefaultUpdateMode, null, getUpdateMode(mode));
		return res != null;
	}

	public boolean setFirstDrawUpdateMode(UpdateMode mode) {
		Object res = ReflectUtil.invokeMethodSafely(sMethodSetFirstDrawUpdateMode, null, getUpdateMode(mode));
		return res != null;
	}

	public boolean setSystemUpdateModeAndScheme(UpdateMode mode, UpdateScheme scheme, int count) {
		Object res = ReflectUtil.invokeMethodSafely(sMethodSetSystemUpdateModeAndScheme, null, getUpdateMode(mode), getUpdateScheme(scheme), count);
		return res != null;
	}

	public boolean clearSystemUpdateModeAndScheme() {
		Object res = ReflectUtil.invokeMethodSafely(sMethodClearSystemUpdateModeAndScheme, null);
		return res != null;
	}

	public boolean applyApplicationFastMode(String application, boolean enable, boolean clear) {
		Object res = ReflectUtil.invokeMethodSafely(sMethodApplyApplicationFastMode, null, application, enable, clear);
		return res != null;
	}

	public int getFrontLightBrightnessMinimum(Context context) {
		return 0;
	}

	public int getFrontLightBrightnessMaximum(Context context) {
		return 255;
	}

	public boolean openFrontLight(Context context) {
		Boolean res = (Boolean) invokeDeviceControllerMethod(context, sDevMethodOpenFrontLight, context);
		if (res == null) {
			return false;
		}
		return res.booleanValue();
	}

	public boolean closeFrontLight(Context context) {
		Boolean res = (Boolean) invokeDeviceControllerMethod(context, sDevMethodCloseFrontLight, context);
		if (res == null) {
			return false;
		}
		return res.booleanValue();
	}

	public int getFrontLightDeviceValue(Context context) {
		Integer res = (Integer) invokeDeviceControllerMethod(context, sDevMethodGetFrontLightValue, context);
		if (res == null) {
			return 0;
		}
		return res.intValue();
	}

	public boolean setFrontLightDeviceValue(Context context, int value) {
		Object res = invokeDeviceControllerMethod(context, sDevMethodSetFrontLightValue, context, value);
		return res != null;
	}

	public boolean setNaturalLightConfigValue(Context context, int value) {
		Object res = invokeDeviceControllerMethod(context, sDevMethodSetNaturalLightValue, context, value);
		return res != null;
	}

	public int getFrontLightConfigValue(Context context) {
		Integer res = (Integer) invokeDeviceControllerMethod(context, sDevMethodGetFrontLightConfigValue, context);
		return res.intValue();
	}

	public boolean setFrontLightConfigValue(Context context, int value) {
		invokeDeviceControllerMethod(context, sDevMethodSetFrontLightConfigValue, context, value);
		return true;
	}

	public List<Integer> getFrontLightValueList(Context context) {
		Integer[] array = { 0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160};
		return Arrays.asList(array);
	}

	public void led(Context context, boolean on) {
		invokeDeviceControllerMethod(context, sDevMethodLed, on);
	}

	public boolean setLedColor(String color, int on) {
		invokeDeviceControllerMethod(null, sDevMethodSetLedColor, color, on);
		return true;
	}

	public void setVCom(Context context, int value, String path) {
		invokeDeviceControllerMethod(context, sDevMethodSetVCom, context, value, path);
	}

	public int getVCom(Context context, String path) {
		Integer res = (Integer) invokeDeviceControllerMethod(context, sDevMethodGetVCom, path);
		if (res == null) {
			return 0;
		}
		return res;
	}

	public void updateWaveform(Context context, String path, String target) {
		invokeDeviceControllerMethod(context, sDevMethodUpdateWaveform, path, target);
	}

	public String readSystemConfig(Context context, String key) {
		Object res = invokeDeviceControllerMethod(context, sDevMethodReadSystemConfig, key);
		if ((res == null) || (res.equals(""))) {
			return "";
		}
		return res.toString();
	}

	public boolean saveSystemConfig(Context context, String key, String value) {
		Object res = invokeDeviceControllerMethod(context, sDevMethodSaveSystemConfig, key, value);
		return (res != null) && (((Boolean) res).booleanValue());
	}

	public void updateMetadataDB(Context context, String path, String target) {
		invokeDeviceControllerMethod(context, sDevMethodUpdateMetadataDB, path, target);
	}

	public void disableA2ForSpecificView(View view) {
		ReflectUtil.invokeMethodSafely(sMethodDisableA2, view);
	}

	public void enableA2ForSpecificView(View view) {
		ReflectUtil.invokeMethodSafely(sMethodEnableA2, view);
	}

	public void setWebViewContrastOptimize(WebView view, boolean enabled) {
		ReflectUtil.invokeMethodSafely(sWebViewMethodSetCssInjectEnabled, view, enabled);
	}

	public void setQRShowConfig(int orientation, int startX, int startY) {
		ReflectUtil.invokeMethodSafely(sMethodSetQRShowConfig, null, orientation, startX, startY);
	}

	public void setInfoShowConfig(int orientation, int startX, int startY) {
		ReflectUtil.invokeMethodSafely(sMethodSetInfoShowConfig, null, orientation, startX, startY);
	}

	public void gotoSleep(Context context) {
		long millis = System.currentTimeMillis();
		ReflectUtil.invokeMethodSafely(sDevMethodGotoSleep, context, millis);
	}

	public boolean hasWifi(Context context) {
		Boolean res = (Boolean) invokeDeviceControllerMethod(context, sDevMethodHasWifi);
		if (res == null) {
			return false;
		}
		return res;
	}

	public boolean inSystemFastMode() {
		Boolean res = (Boolean) ReflectUtil.invokeMethodSafely(sMethodInSystemFastMode, null);
		if (res == null) {
			return false;
		}
		return res;
	}

	public void applyGCOnce() {
		ReflectUtil.invokeMethodSafely(sMethodApplyGCOnce, null);
	}

	private int[] regionsToArray(Rect[] regions) {
		int[] array = new int[regions.length * 4];
		for (int i = 0; i < regions.length; ++i) {
			Rect region = regions[i];
			int left = Math.min(region.left, region.right);
			int top = Math.min(region.top, region.bottom);
			int right = Math.max(region.left, region.right);
			int bottom = Math.max(region.top, region.bottom);
			array[4 * i] = left;
			array[4 * i + 1] = top;
			array[4 * i + 2] = right;
			array[4 * i + 3] = bottom;
		}
		return array;
	}

	private Object invokeDeviceControllerMethod(Context context, Method method, Object... args) {
		if (method == null) {
			return null;
		}
		return ReflectUtil.invokeMethodSafely(method, null, args);
	}

	private UpdateMode updateModeFromEPD(EPDMode epd) {
		switch (epd) {
			case FULL:
				return UpdateMode.GC;
			case AUTO:
			case TEXT:
			case AUTO_PART:
				return UpdateMode.GU;
		}
		return UpdateMode.DU;
	}

	private int getUpdateMode(UpdateMode mode) {
		int dst_mode = sModeGC;
		switch (mode) {
			case GU_FAST:
			case DU:
				dst_mode = sModeDW;
				break;
			case GU:
				dst_mode = sModeGU;
				break;
			case GC:
				dst_mode = sModeGC;
				break;
			case ANIMATION:
				dst_mode = sModeAnim;
				break;
			case ANIMATION_QUALITY:
				dst_mode = sModeAnimQuality;
				break;
			case GC4:
				dst_mode = sModeGC4;
				break;
			case REGAL:
				dst_mode = sModeReagl != 0 ? sModeReagl : sModeGU;
				break;
			case REGAL_D:
				dst_mode = sModeReagld != 0 ? sModeReagld : sModeGU;
				break;
			default:
				assert (false);
				break;
		}
		return dst_mode;
	}

	private int getUpdateScheme(UpdateScheme scheme) {
		int dst_scheme = sSchemeQUEUE;
		switch (scheme) {
			case SNAPSHOT:
				dst_scheme = sSchemeSNAPSHOT;
				break;
			case QUEUE:
				dst_scheme = sSchemeQUEUE;
				break;
			case QUEUE_AND_MERGE:
				dst_scheme = sSchemeQUEUE_AND_MERGE;
				break;
			default:
				assert (false);
				break;
		}
		return dst_scheme;
	}

	private UpdateMode updateModeFromValue(int value) {
		if (value == sModeDW) {
			return UpdateMode.DU;
		} else if (value == sModeGU) {
			return UpdateMode.GU;
		} else if (value == sModeGC) {
			return UpdateMode.GC;
		}
		return UpdateMode.GC;
	}

	private static int getPolicyValue(UpdatePolicy policy) {
		int dst_policy = sModeGU;
		switch (policy) {
			case Automatic:
				dst_policy |= sPolicyAutomatic;
				break;
			case GUIntervally:
				dst_policy |= sPolicyGUIntervally;
				break;
			default:
				assert (false);
				break;
		}
		return dst_policy;
	}
}
