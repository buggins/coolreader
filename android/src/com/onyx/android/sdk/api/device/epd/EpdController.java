package com.onyx.android.sdk.api.device.epd;

import android.content.Context;
import android.content.Intent;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.view.View;
import android.webkit.WebView;

import com.onyx.android.sdk.device.Device;

public abstract class EpdController {
	private static String TAG = EpdController.class.getSimpleName();
	public static int SCHEME_START = 1;
	public static int SCHEME_NORMAL = 1;
	public static int SCHEME_KEYBOARD = 2;
	public static int SCHEME_SCRIBBLE = 3;
	public static int SCHEME_APPLICATION_ANIMATION = 4;
	public static int SCHEME_SYSTEM_ANIMATION = 5;
	public static int SCHEME_END = SCHEME_SYSTEM_ANIMATION;
	private static final String ENABLE_SYSTEM_CTP_ACTION = "action.enable.all.tp.region";
	private static final String RESET_SYSTEM_CTP_ACTION = "action.disable.tp.exclude.bar.region";

	public static void invalidate(View view, UpdateMode mode) {
		Device.currentDevice().invalidate(view, mode);
	}

	public static void invalidate(View view, int left, int top, int right, int bottom, UpdateMode mode) {
		Device.currentDevice().invalidate(view, left, top, right, bottom, mode);
	}

	public static void postInvalidate(View view, UpdateMode mode) {
		Device.currentDevice().postInvalidate(view, mode);
	}

	public static boolean enableScreenUpdate(View view, boolean enable) {
		return Device.currentDevice().enableScreenUpdate(view, enable);
	}

	public static boolean setViewDefaultUpdateMode(View view, UpdateMode mode) {
		Device.currentDevice().setViewDefaultUpdateMode(view, mode);
		return true;
	}

	public static UpdateMode getViewDefaultUpdateMode(View view) {
		return Device.currentDevice().getViewDefaultUpdateMode(view);
	}

	public static void useFastScheme() {
		setDisplayScheme(SCHEME_SCRIBBLE);
	}

	public static void resetUpdateMode(View view) {
		resetViewUpdateMode(view);
		useFastScheme();
	}

	public static boolean resetViewUpdateMode(View view) {
		Device.currentDevice().resetViewUpdateMode(view);
		return true;
	}

	public static UpdateMode getSystemDefaultUpdateMode() {
		return Device.currentDevice().getSystemDefaultUpdateMode();
	}

	public static boolean setSystemDefaultUpdateMode(UpdateMode mode) {
		Device.currentDevice().setSystemDefaultUpdateMode(mode);
		return false;
	}

	public static boolean setSystemUpdateModeAndScheme(UpdateMode mode, UpdateScheme scheme, int count) {
		Device.currentDevice().setSystemUpdateModeAndScheme(mode, scheme, count);
		return false;
	}

	public static boolean clearSystemUpdateModeAndScheme() {
		Device.currentDevice().clearSystemUpdateModeAndScheme();
		return false;
	}

	public static boolean applyApplicationFastMode(String application, boolean enable, boolean clear) {
		Device.currentDevice().applyApplicationFastMode(application, enable, clear);
		return true;
	}

	public static boolean applyApplicationFastMode(String application, boolean enable, boolean clear, UpdateMode repeatMode, int repeatLimit) {
		Device.currentDevice().applyApplicationFastMode(application, enable, clear, repeatMode, repeatLimit);
		return true;
	}

	public static boolean setDisplayScheme(int scheme) {
		Device.currentDevice().setDisplayScheme(scheme);
		return true;
	}

	public static void waitForUpdateFinished() {
		Device.currentDevice().waitForUpdateFinished();
	}

	public static void refreshScreen(View view, UpdateMode mode) {
		Device.currentDevice().refreshScreen(view, mode);
	}

	public static void refreshScreenRegion(View view, int left, int top, int width, int height, UpdateMode mode) {
		Device.currentDevice().refreshScreenRegion(view, left, top, width, height, mode);
	}

	public static boolean supportRegal() {
		return Device.currentDevice().supportRegal();
	}

	public static void holdDisplay(boolean hold, UpdateMode updateMode, int ignoreFrame) {
		Device.currentDevice().holdDisplay(hold, updateMode, ignoreFrame);
	}

	public static void setStrokeWidth(float width) {
		Device.currentDevice().setStrokeWidth(width);
	}

	public static void setStrokeStyle(int style) {
		Device.currentDevice().setStrokeStyle(style);
	}

	public static void setStrokeColor(int color) {
		Device.currentDevice().setStrokeColor(color);
	}

	public static void setScreenHandWritingPenState(View view, int penState) {
		Device.currentDevice().setScreenHandWritingPenState(view, penState);
	}

	public static void setScreenHandWritingRegionLimit(View view, int left, int top, int right, int bottom) {
		Device.currentDevice().setScreenHandWritingRegionLimit(view, left, top, right, bottom);
	}

	public static void setScreenHandWritingRegionLimit(View view) {
		Device.currentDevice().setScreenHandWritingRegionLimit(view);
	}

	public static void setScreenHandWritingRegionLimit(View view, int[] array) {
		Device.currentDevice().setScreenHandWritingRegionLimit(view, array);
	}

	public static void setScreenHandWritingRegionLimit(View view, Rect[] regions) {
		Device.currentDevice().setScreenHandWritingRegionLimit(view, regions);
	}

	public static void setScreenHandWritingRegionExclude(View view, Rect[] regions) {
		Device.currentDevice().setScreenHandWritingRegionExclude(view, regions);
	}

	public static float startStroke(float baseWidth, float x, float y, float pressure, float size, float time) {
		return Device.currentDevice().startStroke(baseWidth, x, y, pressure, size, time);
	}

	public static float addStrokePoint(float baseWidth, float x, float y, float pressure, float size, float time) {
		return Device.currentDevice().addStrokePoint(baseWidth, x, y, pressure, size, time);
	}

	public static float finishStroke(float baseWidth, float x, float y, float pressure, float size, float time) {
		return Device.currentDevice().finishStroke(baseWidth, x, y, pressure, size, time);
	}

	public static void enterScribbleMode(View view) {
		Device.currentDevice().enterScribbleMode(view);
	}

	public static void leaveScribbleMode(View view) {
		Device.currentDevice().leaveScribbleMode(view);
	}

	public static void enablePost(View view, int enable) {
		Device.currentDevice().enablePost(view, enable);
	}

	public static void resetEpdPost() {
		Device.currentDevice().resetEpdPost();
	}

	public static void setPainterStyle(boolean antiAlias, Paint.Style strokeStyle, Paint.Join joinStyle, Paint.Cap capStyle) {
		Device.currentDevice().setPainterStyle(antiAlias, strokeStyle, joinStyle, capStyle);
	}

	public static void moveTo(float x, float y, float width) {
		Device.currentDevice().moveTo(x, y, width);
	}

	public static void moveTo(View view, float x, float y, float width) {
		Device.currentDevice().moveTo(view, x, y, width);
	}

	public static void lineTo(float x, float y, UpdateMode mode) {
		Device.currentDevice().lineTo(x, y, mode);
	}

	public static void lineTo(View view, float x, float y, UpdateMode mode) {
		Device.currentDevice().lineTo(view, x, y, mode);
	}

	public static void quadTo(float x, float y, UpdateMode mode) {
		Device.currentDevice().quadTo(x, y, mode);
	}

	public static void quadTo(View view, float x, float y, UpdateMode mode) {
		Device.currentDevice().quadTo(view, x, y, mode);
	}

	public static void disableA2ForSpecificView(View view) {
		Device.currentDevice().disableA2ForSpecificView(view);
	}

	public static void enableA2ForSpecificView(View view) {
		Device.currentDevice().enableA2ForSpecificView(view);
	}

	public static void setWebViewContrastOptimize(WebView view, boolean enabled) {
		Device.currentDevice().setWebViewContrastOptimize(view, enabled);
	}

	public static void mapToView(View view, float[] src, float[] dst) {
		Device.currentDevice().mapToView(view, src, dst);
	}

	public static void mapToEpd(View view, float[] src, float[] dst) {
		Device.currentDevice().mapToEpd(view, src, dst);
	}

	public static Rect mapToEpd(View view, Rect src) {
		return Device.currentDevice().mapToEpd(view, src);
	}

	public static void mapFromRawTouchPoint(View view, float[] src, float[] dst) {
		Device.currentDevice().mapFromRawTouchPoint(view, src, dst);
	}

	public static void mapToRawTouchPoint(View view, float[] src, float[] dst) {
		Device.currentDevice().mapToRawTouchPoint(view, src, dst);
	}

	public static RectF mapToRawTouchPoint(View view, RectF rect) {
		return Device.currentDevice().mapToRawTouchPoint(view, rect);
	}

	public static float getTouchWidth() {
		return Device.currentDevice().getTouchWidth();
	}

	public static float getTouchHeight() {
		return Device.currentDevice().getTouchHeight();
	}

	public static float getMaxTouchPressure() {
		return Device.currentDevice().getMaxTouchPressure();
	}

	public static float getEpdWidth() {
		return Device.currentDevice().getEpdWidth();
	}

	public static float getEpdHeight() {
		return Device.currentDevice().getEpdHeight();
	}

	public static void enableRegal() {
		Device.currentDevice().enableRegal(true);
	}

	public static void disableRegal() {
		Device.currentDevice().enableRegal(false);
	}

	public static void setUpdListSize(int size) {
		Device.currentDevice().setUpdListSize(size);
	}

	public static void resetUpdListSize() {
		Device.currentDevice().setUpdListSize(-1);
	}

	public static boolean inSystemFastMode() {
		return Device.currentDevice().inSystemFastMode();
	}

	public static void setAppCTPDisableRegion(Context context, int[] disableRegionArray) {
		setAppCTPDisableRegion(context, disableRegionArray, null);
	}

	public static void setAppCTPDisableRegion(Context context, Rect[] disableRegions) {
		setAppCTPDisableRegion(context, disableRegions, null);
	}

	public static void setAppCTPDisableRegion(Context context, int[] disableRegionArray, int[] excludeRegionArray) {
		Device.currentDevice().setAppCTPDisableRegion(context, disableRegionArray, excludeRegionArray);
	}

	public static void setAppCTPDisableRegion(Context context, Rect[] disableRegions, Rect[] excludeRegions) {
		Device.currentDevice().setAppCTPDisableRegion(context, disableRegions, excludeRegions);
	}

	public static boolean isCTPDisableRegion(Context context) {
		return Device.currentDevice().isCTPDisableRegion(context);
	}

	public static void appResetCTPDisableRegion(Context context) {
		Device.currentDevice().appResetCTPDisableRegion(context);
	}

	public static void setSystemCTPDisableRegion(Context context) {
		context.sendBroadcast(new Intent(ENABLE_SYSTEM_CTP_ACTION));
	}

	public static void systemResetCTPDisableRegion(Context context) {
		context.sendBroadcast(new Intent(RESET_SYSTEM_CTP_ACTION));
	}

	public static boolean isCTPPowerOn() {
		return Device.currentDevice().isCTPPowerOn();
	}

	public static boolean isEMTPPowerOn() {
		return Device.currentDevice().isEMTPPowerOn();
	}

	public static void powerCTP(boolean enable) {
		Device.currentDevice().powerCTP(enable);
	}

	public static void powerEMTP(boolean enable) {
		Device.currentDevice().powerEMTP(enable);
	}

	public static void switchToA2Mode() {
		Device.currentDevice().switchToA2Mode();
	}

	public static void applyGammaCorrection(boolean apply, int value) {
		Device.currentDevice.applyGammaCorrection(apply, value);
	}

	public static void applyGCOnce() {
		Device.currentDevice().applyGCOnce();
	}

	public static void setTrigger(int count) {
		Device.currentDevice().setTrigger(count);
	}
}
