package com.onyx.android.sdk.device;

import android.content.Context;
import android.graphics.Paint;
import android.os.Environment;
import android.util.Log;
import android.view.View;

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

public class IMX7Device extends BaseDevice {
	private static final String TAG = IMX7Device.class.getSimpleName();
	private static IMX7Device sInstance = null;
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
	private static Method sMethodMoveTo = null;
	private static Method sMethodSetStrokeColor = null;
	private static Method sMethodSetStrokeStyle = null;
	private static Method sMethodSetStrokeWidth = null;
	private static Method sMethodSetPainterStyle = null;
	private static Method sMethodLineTo = null;
	private static Method sMethodQuadTo = null;
	private static Method sMethodGetTouchWidth = null;
	private static Method sMethodGetTouchHeight = null;
	private static Method sMethodEnablePost = null;
	private static Method sMethodSetScreenHandWritingPenState = null;
	private static Method sMethodSetScreenHandWritingRegionLimit = null;
	private static Method sMethodApplyGammaCorrection = null;
	private static Method sMethodStartStroke = null;
	private static Method sMethodAddStrokePoint = null;
	private static Method sMethodFinishStroke = null;
	private static Method sMethodEnableA2;
	private static Method sMethodDisableA2;
	private static Method sEnvMethodGetStorageRootDirectory;
	private static Method sEnvMethodGetRemovableSDCardDirectory;
	private static Method sMethodPostInvalidate = null;
	private static Method sMethodInvalidate = null;
	private static Method sMethodInvalidateRegion = null;
	private static Method sMethodGetDefaultUpdateMode = null;
	private static Method sMethodResetUpdateMode = null;
	private static Method sMethodSetDefaultUpdateMode = null;
	private static Method sMethodGetGlobalUpdateMode = null;
	private static Method sMethodSetGlobalUpdateMode = null;
	private static Method sMethodSetFirstDrawUpdateMode = null;
	private static Method sMethodSetWaveformAndScheme = null;
	private static Method sMethodApplyApplicationFastMode = null;
	private static Method sMethodResetWaveformAndScheme = null;
	private static Method sMethodEnableScreenUpdate = null;
	private static Method sMethodSetDisplayScheme = null;
	private static Method sMethodWaitForUpdateFinished = null;
	private static Method sDevMethodOpenFrontLight;
	private static Method sDevMethodCloseFrontLight;
	private static Method sDevMethodGetFrontLightValue;
	private static Method sDevMethodSetFrontLightValue;
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

	private IMX7Device() {
	}

	public static IMX7Device createDevice() {
		if (sInstance == null) {
			sInstance = new IMX7Device();
			Class cls = View.class;
			sMethodGetWindowRotation = ReflectUtil.getMethodSafely(cls, "getWindowRotation");
			sMethodSetWindowRotation = ReflectUtil.getMethodSafely(cls, "setWindowRotation", Integer.TYPE, Boolean.TYPE, Integer.TYPE);

			Class viewUpdateHelperClass = ReflectUtil.classForName("android.onyx.ViewUpdateHelper");
			int value_auto_mask = ReflectUtil.getStaticIntFieldSafely(viewUpdateHelperClass, "EINK_ONYX_AUTO_MASK");
			int value_gc_mask = ReflectUtil.getStaticIntFieldSafely(viewUpdateHelperClass, "EINK_ONYX_GC_MASK");
			int value_mode_regional = ReflectUtil.getStaticIntFieldSafely(viewUpdateHelperClass, "EINK_AUTO_MODE_REGIONAL");
			int value_mode_nowait = ReflectUtil.getStaticIntFieldSafely(viewUpdateHelperClass, "EINK_WAIT_MODE_NOWAIT");
			int value_mode_wait = ReflectUtil.getStaticIntFieldSafely(viewUpdateHelperClass, "EINK_WAIT_MODE_WAIT");
			int value_mode_waveform_du = ReflectUtil.getStaticIntFieldSafely(viewUpdateHelperClass, "EINK_WAVEFORM_MODE_DU");
			int value_mode_waveform_anim = ReflectUtil.getStaticIntFieldSafely(viewUpdateHelperClass, "EINK_WAVEFORM_MODE_ANIM");
			int value_mode_waveform_gc4 = ReflectUtil.getStaticIntFieldSafely(viewUpdateHelperClass, "EINK_WAVEFORM_MODE_GC4");
			int value_mode_waveform_gc16 = ReflectUtil.getStaticIntFieldSafely(viewUpdateHelperClass, "EINK_WAVEFORM_MODE_GC16");
			int value_mode_waveform_reagl = ReflectUtil.getStaticIntFieldSafely(viewUpdateHelperClass, "EINK_WAVEFORM_MODE_REAGL");
			int value_mode_reagl_reagld = ReflectUtil.getStaticIntFieldSafely(viewUpdateHelperClass, "EINK_REAGL_MODE_REAGLD");
			int value_mode_update_partial = ReflectUtil.getStaticIntFieldSafely(viewUpdateHelperClass, "EINK_UPDATE_MODE_PARTIAL");
			int value_mode_update_full = ReflectUtil.getStaticIntFieldSafely(viewUpdateHelperClass, "EINK_UPDATE_MODE_FULL");
			sPolicyAutomatic = value_auto_mask;
			sPolicyGUIntervally = value_gc_mask;
			sModeDW = value_mode_regional | value_mode_nowait | value_mode_waveform_du | value_mode_update_partial;
			sModeGU = value_mode_regional | value_mode_nowait | value_mode_waveform_gc16 | value_mode_update_partial;
			sModeGC = value_mode_regional | value_mode_wait | value_mode_waveform_gc16 | value_mode_update_full;
			sModeAnim = value_mode_regional | value_mode_nowait | value_mode_waveform_anim | value_mode_update_partial;
			sModeAnimQuality = ReflectUtil.getStaticIntFieldSafely(viewUpdateHelperClass, "UI_A2_QUALITY_MODE");
			sModeGC4 = value_mode_regional | value_mode_nowait | value_mode_waveform_gc4 | value_mode_update_partial;
			sModeReagl = value_mode_regional | value_mode_nowait | value_mode_waveform_reagl | value_mode_update_partial;
			sModeReagld = value_mode_regional | value_mode_nowait | value_mode_reagl_reagld | value_mode_waveform_reagl | value_mode_update_partial;

			Class devCtrlCls = ReflectUtil.classForName("android.onyx.hardware.DeviceController");
			sDevMethodOpenFrontLight = ReflectUtil.getMethodSafely(devCtrlCls, "openFrontLight", Context.class);
			sDevMethodCloseFrontLight = ReflectUtil.getMethodSafely(devCtrlCls, "closeFrontLight", Context.class);
			sDevMethodGetFrontLightValue = ReflectUtil.getMethodSafely(devCtrlCls, "getFrontLightValue", Context.class);
			sDevMethodSetFrontLightValue = ReflectUtil.getMethodSafely(devCtrlCls, "setFrontLightValue", Context.class, Integer.TYPE);
			sDevMethodGetFrontLightConfigValue = ReflectUtil.getMethodSafely(devCtrlCls, "getFrontLightConfigValue", Context.class);
			sDevMethodSetFrontLightConfigValue = ReflectUtil.getMethodSafely(devCtrlCls, "setFrontLightConfigValue", Context.class, Integer.TYPE);
			sDevMethodUseBigPen = ReflectUtil.getMethodSafely(devCtrlCls, "useBigPen", Boolean.TYPE);
			sDevMethodStopTpd = ReflectUtil.getMethodSafely(devCtrlCls, "stopTpd");
			sDevMethodStartTpd = ReflectUtil.getMethodSafely(devCtrlCls, "startTpd");
			sDevMethodGotoSleep = ReflectUtil.getMethodSafely(devCtrlCls, "gotoSleep", Context.class, Long.TYPE);
			sDevMethodLed = ReflectUtil.getMethodSafely(devCtrlCls, "led", Boolean.TYPE);
			sDevMethodSetLedColor = ReflectUtil.getMethodSafely(devCtrlCls, "setLedColor", String.class, Integer.TYPE);
			sMethodEnableOnyxTpd = ReflectUtil.getMethodSafely(cls, "enableOnyxTpd", Integer.TYPE);
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
			sMethodMoveTo = ReflectUtil.getMethodSafely(cls, "moveTo", Float.TYPE, Float.TYPE, Float.TYPE);
			sMethodLineTo = ReflectUtil.getMethodSafely(cls, "lineTo", Float.TYPE, Float.TYPE, Integer.TYPE);
			sMethodQuadTo = ReflectUtil.getMethodSafely(cls, "quadTo", Float.TYPE, Float.TYPE, Integer.TYPE);
			sMethodGetTouchWidth = ReflectUtil.getMethodSafely(cls, "getTouchWidth");
			sMethodGetTouchHeight = ReflectUtil.getMethodSafely(cls, "getTouchHeight");
			sMethodEnablePost = ReflectUtil.getMethodSafely(cls, "enablePost", Integer.TYPE);
			sMethodSetScreenHandWritingPenState = ReflectUtil.getMethodSafely(cls, "setScreenHandWritingPenState", Integer.TYPE);
			sMethodSetScreenHandWritingRegionLimit = ReflectUtil.getMethodSafely(cls, "setScreenHandWritingRegionLimit", Integer.TYPE, Integer.TYPE, Integer.TYPE, Integer.TYPE);
			sMethodApplyGammaCorrection = ReflectUtil.getMethodSafely(cls, "applyGammaCorrection", Boolean.TYPE, Integer.TYPE);
			sMethodStartStroke = ReflectUtil.getMethodSafely(cls, "startStroke", Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE);
			sMethodAddStrokePoint = ReflectUtil.getMethodSafely(cls, "addStrokePoint", Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE);
			sMethodFinishStroke = ReflectUtil.getMethodSafely(cls, "finishStroke", Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE, Float.TYPE);
			sMethodInvalidate = ReflectUtil.getMethodSafely(cls, "invalidate", Integer.TYPE);
			sMethodInvalidateRegion = ReflectUtil.getMethodSafely(cls, "invalidate", Integer.TYPE, Integer.TYPE, Integer.TYPE, Integer.TYPE, Integer.TYPE);
			sMethodSetDefaultUpdateMode = ReflectUtil.getMethodSafely(cls, "setDefaultUpdateMode", Integer.TYPE);
			sMethodGetDefaultUpdateMode = ReflectUtil.getMethodSafely(cls, "getDefaultUpdateMode");
			sMethodResetUpdateMode = ReflectUtil.getMethodSafely(cls, "resetUpdateMode");
			sMethodGetGlobalUpdateMode = ReflectUtil.getMethodSafely(cls, "getGlobalUpdateMode");
			sMethodSetGlobalUpdateMode = ReflectUtil.getMethodSafely(cls, "setGlobalUpdateMode", Integer.TYPE);
			sMethodSetFirstDrawUpdateMode = ReflectUtil.getMethodSafely(cls, "setFirstDrawUpdateMode", Integer.TYPE);
			sMethodSetWaveformAndScheme = ReflectUtil.getMethodSafely(cls, "setWaveformAndScheme", Integer.TYPE, Integer.TYPE, Integer.TYPE);
			sMethodResetWaveformAndScheme = ReflectUtil.getMethodSafely(cls, "resetWaveformAndScheme");
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
			sEnvMethodGetStorageRootDirectory = ReflectUtil.getMethodSafely(Environment.class, "getStorageRootDirectory");
			sEnvMethodGetRemovableSDCardDirectory = ReflectUtil.getMethodSafely(Environment.class, "getRemovableSDCardDirectory");
			Log.d(TAG, "init device EINK_ONYX_GC_MASK.");
		}
		return sInstance;
	}

	public int getWindowRotation() {
		if (sMethodGetWindowRotation != null) {
			try {
				return (Integer) sMethodGetWindowRotation.invoke(null);
			} catch (IllegalArgumentException e) {
			} catch (IllegalAccessException e1) {
			} catch (InvocationTargetException e2) {
			}
		}

		return 0;
	}

	public boolean setWindowRotation(int rotation) {
		if (sMethodSetWindowRotation != null) {
			try {
				sMethodSetWindowRotation.invoke((Object) null, rotation, true, 1);
				return true;
			} catch (IllegalArgumentException e) {
			} catch (IllegalAccessException e1) {
			} catch (InvocationTargetException e2) {
			}
		}
		return false;
	}

	public boolean setUpdatePolicy(View view, UpdatePolicy policy, int guInterval) {
		int dst_policy = getPolicyValue(policy);
		try {
			assert(sMethodSetUpdatePolicy != null);
			sMethodSetUpdatePolicy.invoke(view, dst_policy, guInterval);
			return true;
		} catch (IllegalArgumentException e) {
		} catch (IllegalAccessException e1) {
		} catch (InvocationTargetException e2) {
		}
		return false;
	}

	public File getStorageRootDirectory() {
		return (File) ReflectUtil.invokeMethodSafely(sEnvMethodGetStorageRootDirectory, null);
	}

	public File getExternalStorageDirectory() {
		return Environment.getExternalStorageDirectory();
	}

	public File getRemovableSDCardDirectory() {
		return (File) ReflectUtil.invokeMethodSafely(sEnvMethodGetRemovableSDCardDirectory, null);
	}

	public boolean isFileOnRemovableSDCard(File file) {
		return file.getAbsolutePath().startsWith(this.getRemovableSDCardDirectory().getAbsolutePath());
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
			assert(sMethodInvalidate != null);
			sMethodInvalidate.invoke(view, dst_mode_value);
			return;
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
			return;
		} catch (IllegalArgumentException e) {
		} catch (IllegalAccessException e1) {
		} catch (InvocationTargetException e2) {
		}
	}

	public void postInvalidate(View view, UpdateMode mode) {
		int dst_mode_value = getUpdateMode(mode);
		try {
			assert(sMethodPostInvalidate != null);
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
			assert(sMethodRefreshScreen != null);
			sMethodRefreshScreen.invoke(view, dst_mode_value);
			return;
		} catch (IllegalArgumentException e) {
		} catch (IllegalAccessException e1) {
		} catch (InvocationTargetException e2) {
		}
	}

	public void refreshScreenRegion(View view, int left, int top, int width, int height, UpdateMode mode) {
		int dst_mode_value = getUpdateMode(mode);
		try {
			assert(sMethodRefreshScreenRegion != null);
			sMethodRefreshScreenRegion.invoke(view, left, top, width, height, dst_mode_value);
		} catch (Exception e) {
		}
	}

	public void screenshot(View view, int rotation, String path) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodScreenshot, view, rotation, path);
		} catch (Exception e) {
		}
	}

	public void setStrokeColor(int color) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodSetStrokeColor, null, color);
		} catch (Exception e) {
		}

	}

	public void setStrokeStyle(int style) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodSetStrokeStyle, null, style);
		} catch (Exception e) {
		}
	}

	public void setPainterStyle(boolean antiAlias, Paint.Style strokeStyle, Paint.Join joinStyle, Paint.Cap capStyle) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodSetPainterStyle, null, antiAlias, strokeStyle, joinStyle, capStyle);
		} catch (Exception e) {
		}
	}

	public void setStrokeWidth(float width) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodSetStrokeWidth, null, width);
		} catch (Exception e) {
		}
	}

	public void moveTo(float x, float y, float width) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodMoveTo, null, x, y, width);
		} catch (Exception e) {
		}
	}

	public boolean supportDFB() {
		return sMethodLineTo != null;
	}

	public boolean supportRegal() {
		if (sMethodSupportRegal == null) {
			return false;
		} else {
			Boolean res = (Boolean) ReflectUtil.invokeMethodSafely(sMethodSupportRegal, null);
			return res == null ? false : res;
		}
	}

	public void lineTo(float x, float y, UpdateMode mode) {
		int dst_mode_value = getUpdateMode(mode);
		try {
			ReflectUtil.invokeMethodSafely(sMethodLineTo, null, x, y, dst_mode_value);
		} catch (Exception e) {
		}
	}

	public void quadTo(float x, float y, UpdateMode mode) {
		int dst_mode_value = getUpdateMode(mode);
		try {
			ReflectUtil.invokeMethodSafely(sMethodQuadTo, null, x, y, dst_mode_value);
		} catch (Exception e) {
		}
	}

	public float getTouchWidth() {
		try {
			Float res = (Float) ReflectUtil.invokeMethodSafely(sMethodGetTouchWidth, null);
			return res;
		} catch (Exception e) {
			return 0.0F;
		}
	}

	public float getTouchHeight() {
		try {
			Float res = (Float) ReflectUtil.invokeMethodSafely(sMethodGetTouchHeight, null);
			return res;
		} catch (Exception e) {
			return 0.0F;
		}
	}

	public float startStroke(float baseWidth, float x, float y, float pressure, float size, float time) {
		try {
			Float res = (Float) ReflectUtil.invokeMethodSafely(sMethodStartStroke, null, baseWidth, x, y, pressure, size, time);
			return res;
		} catch (Exception e) {
			return baseWidth;
		}
	}

	public float addStrokePoint(float baseWidth, float x, float y, float pressure, float size, float time) {
		try {
			Float res = (Float) ReflectUtil.invokeMethodSafely(sMethodAddStrokePoint, null, baseWidth, x, y, pressure, size, time);
			return res;
		} catch (Exception e) {
			return baseWidth;
		}
	}

	public float finishStroke(float baseWidth, float x, float y, float pressure, float size, float time) {
		try {
			Float res = (Float) ReflectUtil.invokeMethodSafely(sMethodFinishStroke, null, baseWidth, x, y, pressure, size, time);
			return res;
		} catch (Exception e) {
			return baseWidth;
		}
	}

	public void enterScribbleMode(View view) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodEnablePost, view, 0);
		} catch (Exception e) {
		}
	}

	public void leaveScribbleMode(View view) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodEnablePost, view, 1);
		} catch (Exception e) {
		}

	}

	public void enablePost(View view, int enable) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodEnablePost, view, enable);
		} catch (Exception e) {
		}
	}

	public boolean supportScreenHandWriting() {
		return sMethodSetScreenHandWritingPenState != null;
	}

	public void setScreenHandWritingPenState(View view, int penState) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodSetScreenHandWritingPenState, view, penState);
		} catch (Exception e) {
		}
	}

	public void setScreenHandWritingRegionLimit(View view, int left, int top, int right, int bottom) {
		try {
			ReflectUtil.invokeMethodSafely(sMethodSetScreenHandWritingRegionLimit, view, left, top, right, bottom);
		} catch (Exception e) {
		}
	}

	public void applyGammaCorrection(boolean apply, int value) {
		ReflectUtil.invokeMethodSafely(sMethodApplyGammaCorrection, null, apply, value);
	}

	public boolean enableScreenUpdate(View view, boolean enable) {
		try {
			sMethodEnableScreenUpdate.invoke(view, enable);
		} catch (Exception e) {
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
		invokeDeviceControllerMethod((Context) null, sDevMethodUseBigPen, use);
	}

	public void stopTpd() {
		invokeDeviceControllerMethod((Context) null, sDevMethodStopTpd);
	}

	public void startTpd() {
		invokeDeviceControllerMethod((Context) null, sDevMethodStartTpd);
	}

	public void enableTpd(boolean enable) {
		ReflectUtil.invokeMethodSafely(sMethodEnableOnyxTpd, null, (enable ? 1 : 0));
	}

	public UpdateMode getViewDefaultUpdateMode(View view) {
		Integer res = (Integer) ReflectUtil.invokeMethodSafely(sMethodGetDefaultUpdateMode, view);
		if (res == null) {
			return UpdateMode.GU;
		}
		return updateModeFromValue(res);
	}

	public void resetViewUpdateMode(View view) {
		ReflectUtil.invokeMethodSafely(sMethodResetUpdateMode, view);
	}

	public boolean setViewDefaultUpdateMode(View view, UpdateMode mode) {
		Object res = ReflectUtil.invokeMethodSafely(sMethodSetDefaultUpdateMode, view, getUpdateMode(mode));
		return res != null;
	}

	public UpdateMode getSystemDefaultUpdateMode() {
		Integer res = (Integer) ReflectUtil.invokeMethodSafely(sMethodGetGlobalUpdateMode, null);
		if (res == null) {
			return UpdateMode.GU;
		}
		return this.updateModeFromValue(res);
	}

	public boolean setSystemDefaultUpdateMode(UpdateMode mode) {
		Object res = ReflectUtil.invokeMethodSafely(sMethodSetGlobalUpdateMode, null, getUpdateMode(mode));
		return res != null;
	}

	public boolean setFirstDrawUpdateMode(UpdateMode mode) {
		Object res = ReflectUtil.invokeMethodSafely(sMethodSetFirstDrawUpdateMode, null, getUpdateMode(mode));
		return res != null;
	}

	public boolean setSystemUpdateModeAndScheme(UpdateMode mode, UpdateScheme scheme, int count) {
		Object res = ReflectUtil.invokeMethodSafely(sMethodSetWaveformAndScheme, null, getUpdateMode(mode), getUpdateScheme(scheme), count);
		return res != null;
	}

	public boolean clearSystemUpdateModeAndScheme() {
		Object res = ReflectUtil.invokeMethodSafely(sMethodResetWaveformAndScheme, null);
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
		return res;
	}

	public boolean closeFrontLight(Context context) {
		Boolean res = (Boolean) invokeDeviceControllerMethod(context, sDevMethodCloseFrontLight, context);
		if (res == null) {
			return false;
		}
		return res;
	}

	public int getFrontLightDeviceValue(Context context) {
		Integer res = (Integer) invokeDeviceControllerMethod(context, sDevMethodGetFrontLightValue, context);
		if (res == null) {
			return 0;
		}
		return res;
	}

	public boolean setFrontLightDeviceValue(Context context, int value) {
		Object res = invokeDeviceControllerMethod(context, sDevMethodSetFrontLightValue, context, value);
		return res != null;
	}

	public int getFrontLightConfigValue(Context context) {
		Integer res = (Integer) invokeDeviceControllerMethod(context, sDevMethodGetFrontLightConfigValue, context);
		return res;
	}

	public boolean setFrontLightConfigValue(Context context, int value) {
		invokeDeviceControllerMethod(context, sDevMethodSetFrontLightConfigValue, context, value);
		return true;
	}

	public List<Integer> getFrontLightValueList(Context context) {
		Integer[] array = new Integer[]{0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160};
		return Arrays.asList(array);
	}

	public void led(Context context, boolean on) {
		invokeDeviceControllerMethod(context, sDevMethodLed, on);
	}

	public boolean setLedColor(String color, int on) {
		invokeDeviceControllerMethod((Context) null, sDevMethodSetLedColor, color, on);
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
		return (Boolean) invokeDeviceControllerMethod(context, sDevMethodSaveSystemConfig, key, value);
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

	public void gotoSleep(Context context) {
		long millis = System.currentTimeMillis();
		ReflectUtil.invokeMethodSafely(sDevMethodGotoSleep, context, millis);
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
			default:
				return UpdateMode.DU;
		}
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
				assert(false);
		}
		return dst_mode;
	}

	private int getUpdateScheme(UpdateScheme scheme) {
		int dst_scheme = 1;
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
				assert(false);
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
		}
		return dst_policy;
	}
}
