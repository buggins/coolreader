package com.onyx.android.sdk.device;

import android.content.Context;
import android.content.Intent;
import android.graphics.Paint;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.Environment;
import android.os.PowerManager;
import android.provider.Settings;
import android.view.View;
import android.view.WindowManager;
import android.webkit.WebView;

import com.onyx.android.sdk.api.device.epd.EPDMode;
import com.onyx.android.sdk.api.device.epd.UpdateMode;
import com.onyx.android.sdk.api.device.epd.UpdateScheme;
import com.onyx.android.sdk.utils.FileUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class BaseDevice {
	private static final String TAG = "BaseDevice";
	private static final String SHOW_STATUS_BAR = "show_status_bar";
	private static final String HIDE_STATUS_BAR = "hide_status_bar";
	private static final String ENABLE_WIFI_CONNECT_STATUS_DETECTION_STATUS = "enable_wifi_connect_status_detection_status";
	private static final String ARGS_WIFI_CONNECT_DETECTION_FLAG = "args_wifi_connect_detection_flag";
	private static final String SWITCH_TO_PAGE_KEY = "switch_to_page_key";
	private static final String SWITCH_TO_VOLUME_KEY = "switch_to_volume_key";
	private static final String SWITCH_TO_HOME_BACK_KEY = "switch_to_home_back_key";
	private static final String SWITCH_KEY = "switch_key";

	public File getStorageRootDirectory() {
		return Environment.getExternalStorageDirectory();
	}

	public File getExternalStorageDirectory() {
		return Environment.getExternalStorageDirectory();
	}

	public File getRemovableSDCardDirectory() {
		File dir = getExternalStorageDirectory();

		String str = "extsd";

		File extsd = new File(dir, str);
		if (extsd.exists()) {
			return extsd;
		}
		return dir;
	}

	public File getBluetoothRootDirectory() {
		return new File(getExternalStorageDirectory().getPath() + File.separator + "bluetooth");
	}

	public boolean isFileOnRemovableSDCard(File file) {
		return file.getAbsolutePath().startsWith(getRemovableSDCardDirectory().getAbsolutePath());
	}

	public PowerManager.WakeLock newWakeLock(Context context, String tag) {
		PowerManager powerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
		return powerManager.newWakeLock(PowerManager.FULL_WAKE_LOCK, tag);
	}

	public PowerManager.WakeLock newWakeLockWithFlags(Context context, int flags, String tag) {
		PowerManager localPowerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
		return localPowerManager.newWakeLock(flags, tag);
	}

	public void useBigPen(boolean use) {
	}

	public void stopTpd() {
	}

	public void startTpd() {
	}

	public void enableTpd(boolean enable) {
	}

	public float getTouchWidth() {
		return 0.0F;
	}

	public float getTouchHeight() {
		return 0.0F;
	}

	public float getMaxTouchPressure() {
		return 1024.0F;
	}

	public float getEpdWidth() {
		return 0.0F;
	}

	public float getEpdHeight() {
		return 0.0F;
	}

	public void mapToView(View view, float[] src, float[] dst) {
	}

	public void mapToEpd(View view, float[] src, float[] dst) {
	}

	public Rect mapToEpd(View view, Rect srcRect) {
		float[] arrayOfFloat1 = {srcRect.left, srcRect.top};
		float[] arrayOfFloat2 = new float[2];
		float[] arrayOfFloat3 = new float[2];
		mapToEpd(view, arrayOfFloat1, arrayOfFloat2);
		arrayOfFloat1[0] = srcRect.right;
		arrayOfFloat1[1] = srcRect.bottom;
		mapToEpd(view, arrayOfFloat1, arrayOfFloat3);
		return new Rect(
				(int) Math.min(arrayOfFloat2[0], arrayOfFloat3[0]),
				(int) Math.min(arrayOfFloat2[1], arrayOfFloat3[1]),
				(int) Math.max(arrayOfFloat2[0], arrayOfFloat3[0]),
				(int) Math.max(arrayOfFloat2[1], arrayOfFloat3[1]));
	}

	public void mapFromRawTouchPoint(View view, float[] src, float[] dst) {
	}

	public void mapToRawTouchPoint(View view, float[] src, float[] dst) {
	}

	public RectF mapToRawTouchPoint(View view, RectF rect) {
		float[] arrayOfFloat1 = {rect.left, rect.top};
		float[] arrayOfFloat2 = new float[2];
		float[] arrayOfFloat3 = new float[2];
		mapToRawTouchPoint(view, arrayOfFloat1, arrayOfFloat2);

		arrayOfFloat1[0] = rect.right;
		arrayOfFloat1[1] = rect.bottom;
		mapToRawTouchPoint(view, arrayOfFloat1, arrayOfFloat3);
		return new RectF(
				Math.min(arrayOfFloat2[0], arrayOfFloat3[0]),
				Math.min(arrayOfFloat2[1], arrayOfFloat3[1]),
				Math.max(arrayOfFloat2[0], arrayOfFloat3[0]),
				Math.max(arrayOfFloat2[1], arrayOfFloat3[1]));
	}

	public int getFrontLightBrightnessMinimum(Context context) {
		return 0;
	}

	public int getFrontLightBrightnessMaximum(Context context) {
		return 0;
	}

	public int getFrontLightBrightnessDefault(Context context) {
		return 0;
	}

	public boolean openFrontLight(Context context) {
		return false;
	}

	public boolean closeFrontLight(Context context) {
		return false;
	}

	public boolean setLedColor(String ledColor, int on) {
		return false;
	}

	public int getFrontLightDeviceValue(Context context) {
		return 0;
	}

	public List<Integer> getFrontLightValueList(Context context) {
		return new ArrayList<Integer>();
	}

	public boolean setFrontLightDeviceValue(Context context, int value) {
		return false;
	}

	public boolean setNaturalLightConfigValue(Context context, int value) {
		return false;
	}

	public int getFrontLightConfigValue(Context context) {
		return 0;
	}

	public boolean setFrontLightConfigValue(Context context, int value) {
		return false;
	}

	public EPDMode getEpdMode() {
		return EPDMode.AUTO;
	}

	public boolean setEpdMode(Context context, EPDMode mode) {
		return false;
	}

	public boolean setEpdMode(View view, EPDMode mode) {
		return false;
	}

	public UpdateMode getViewDefaultUpdateMode(View view) {
		return UpdateMode.GU;
	}

	public boolean setViewDefaultUpdateMode(View view, UpdateMode mode) {
		return false;
	}

	public void resetViewUpdateMode(View view) {
	}

	public UpdateMode getSystemDefaultUpdateMode() {
		return UpdateMode.GU;
	}

	public boolean setSystemDefaultUpdateMode(UpdateMode mode) {
		return false;
	}

	public boolean applyApplicationFastMode(String application, boolean enable, boolean clear) {
		return false;
	}

	public boolean applyApplicationFastMode(String application, boolean enable, boolean clear, UpdateMode repeatMode, int repeatLimit) {
		return false;
	}

	public boolean setDisplayScheme(int scheme) {
		return false;
	}

	public void waitForUpdateFinished() {
	}

	public void invalidate(View view, UpdateMode mode) {
		view.invalidate();
	}

	public void invalidate(View view, int left, int top, int right, int bottom, UpdateMode mode) {
	}

	public boolean enableScreenUpdate(View view, boolean enable) {
		return false;
	}

	public void refreshScreen(View view, UpdateMode mode) {
	}

	public void refreshScreenRegion(View view, int left, int top, int width, int height, UpdateMode mode) {
	}

	public void screenshot(View view, int r, String path) {
	}

	public boolean supportDFB() {
		return false;
	}

	public boolean supportRegal() {
		return false;
	}

	public void holdDisplay(boolean hold, UpdateMode updateMode, int ignoreFrame) {
	}

	public void setStrokeColor(int color) {
	}

	public void setStrokeStyle(int style) {
	}

	public void setStrokeWidth(float width) {
	}

	public void setPainterStyle(boolean antiAlias, Paint.Style strokeStyle, Paint.Join joinStyle, Paint.Cap capStyle) {
	}

	public void moveTo(float x, float y, float width) {
	}

	public void moveTo(View view, float x, float y, float width) {
	}

	public void lineTo(float x, float y, UpdateMode mode) {
	}

	public void lineTo(View view, float x, float y, UpdateMode mode) {
	}

	public void quadTo(float x, float y, UpdateMode mode) {
	}

	public void quadTo(View view, float x, float y, UpdateMode mode) {
	}

	public float startStroke(float baseWidth, float x, float y, float pressure, float size, float time) {
		return baseWidth;
	}

	public float addStrokePoint(float baseWidth, float x, float y, float pressure, float size, float time) {
		return baseWidth;
	}

	public float finishStroke(float baseWidth, float x, float y, float pressure, float size, float time) {
		return baseWidth;
	}

	public void enterScribbleMode(View view) {
	}

	public void leaveScribbleMode(View view) {
	}

	public void enablePost(View view, int enable) {
	}

	public void resetEpdPost() {
	}

	public void setScreenHandWritingPenState(View view, int penState) {
	}

	public void setScreenHandWritingRegionLimit(View view) {
	}

	public void setScreenHandWritingRegionLimit(View view, int left, int top, int right, int bottom) {
	}

	public void setScreenHandWritingRegionLimit(View view, int[] array) {
	}

	public void setScreenHandWritingRegionLimit(View view, Rect[] regions) {
	}

	public void setScreenHandWritingRegionExclude(View view, int[] array) {
	}

	public void setScreenHandWritingRegionExclude(View view, Rect[] regions) {
	}

	public void postInvalidate(View view, UpdateMode mode) {
		view.postInvalidate();
	}

	public boolean setSystemUpdateModeAndScheme(UpdateMode mode, UpdateScheme scheme, int count) {
		return false;
	}

	public boolean clearSystemUpdateModeAndScheme() {
		return false;
	}

	public void wifiLock(Context context, String className) {
	}

	public void wifiUnlock(Context context, String className) {
	}

	public void wifiLockClear(Context context) {
	}

	public Map<String, Integer> getWifiLockMap(Context context) {
		return null;
	}

	public void setWifiLockTimeout(Context context, long ms) {
	}

	public String getEncryptedDeviceID() {
		return null;
	}

	public void led(Context context, boolean on) {
	}

	public void setVCom(Context context, int mv, String path) {
	}

	public void updateWaveform(Context context, String path, String target) {
	}

	public int getVCom(Context context, String path) {
		return 0;
	}

	public String readSystemConfig(Context context, String key) {
		return "";
	}

	public boolean saveSystemConfig(Context context, String key, String mv) {
		return false;
	}

	public void updateMetadataDB(Context context, String path, String target) {
	}

	public Point getWindowWidthAndHeight(Context context) {
		WindowManager windowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
		Point point = new Point();
		point.x = windowManager.getDefaultDisplay().getWidth();
		point.y = windowManager.getDefaultDisplay().getHeight();
		return point;
	}

	public void hideSystemStatusBar(Context context) {
		sendBroadcast(context, HIDE_STATUS_BAR);
	}

	public void showSystemStatusBar(Context context) {
		sendBroadcast(context, SHOW_STATUS_BAR);
	}

	public void mapSideKeyToVolumeKey(Context context) {
		sendBroadcast(context, SWITCH_TO_VOLUME_KEY);
	}

	public void mapSideKeyToHomeBackKey(Context context) {
		sendBroadcast(context, SWITCH_TO_HOME_BACK_KEY);
	}

	public void resetKeyMapping(Context context) {
		sendBroadcast(context, SWITCH_TO_PAGE_KEY);
	}

	public int getCurrentSideKeyMapping(Context context) {
		try {
			return Settings.System.getInt(context.getContentResolver(), SWITCH_KEY);
		} catch (Exception e) {
			if (!(e instanceof Settings.SettingNotFoundException)) {
				e.printStackTrace();
			}
		}
		return -1;
	}

	public void enableWifiDetect(Context context) {
		enableWifiDetect(context, true);
	}

	public void enableWifiDetect(Context context, boolean enableDetect) {
		Intent intent = new Intent(ENABLE_WIFI_CONNECT_STATUS_DETECTION_STATUS);
		intent.putExtra(ARGS_WIFI_CONNECT_DETECTION_FLAG, enableDetect);
		context.sendBroadcast(intent);
	}

	private void sendBroadcast(Context context, String param) {
		context.sendBroadcast(new Intent(param));
	}

	public void stopBootAnimation() {
	}

	public void disableA2ForSpecificView(View view) {
	}

	public void enableA2ForSpecificView(View view) {
	}

	public void setWebViewContrastOptimize(WebView view, boolean enabled) {
	}

	public boolean isLegalSystem(Context context) {
		return true;
	}

	public boolean isTouchable(Context context) {
		return true;
	}

	public void gotoSleep(Context context) {
	}

	public void enableRegal(boolean enable) {
	}

	public boolean hasWifi(Context context) {
		return true;
	}

	public void setQRShowConfig(int orientation, int startX, int startY) {
	}

	public void setInfoShowConfig(int orientation, int startX, int startY) {
	}

	public void setUpdListSize(int size) {
	}

	public boolean inSystemFastMode() {
		return false;
	}

	public String getUpgradePackageName() {
		return "update.zip";
	}

	public boolean shouldVerifyUpdateModel() {
		return true;
	}

	public void powerCTP(boolean on) {
	}

	public void powerEMTP(boolean on) {
	}

	public boolean isCTPPowerOn() {
		return true;
	}

	public boolean isEMTPPowerOn() {
		return true;
	}

	public void setAppCTPDisableRegion(Context context, int[] disableRegionArray, int[] excludeRegionArray) {
	}

	public void setAppCTPDisableRegion(Context context, Rect[] disableRegions, Rect[] excludeRegions) {
	}

	public boolean isCTPDisableRegion(Context context) {
		return false;
	}

	public void appResetCTPDisableRegion(Context context) {
	}

	public void updateMtpDb(Context context, String filePath) {
	}

	public void updateMtpDb(Context context, File file) {
	}

	public void updateEACAppExcludeList(String jsonString) {
	}

	public void setEACApp(String pkgName, String jsonString) {
	}

	public void switchToA2Mode() {
	}

	public void applyGammaCorrection(boolean apply, int value) {
	}

	public void applyGCOnce() {
	}

	public String getCTPInfo() {
		return FileUtils.readContentOfFile(new File("/sys/android_touch/vendor"));
	}

	public boolean hasAudio(Context context) {
		return true;
	}

	public boolean hasFrontLight(Context context) {
		return false;
	}

	public boolean hasNaturalLight(Context context) {
		return false;
	}

	public boolean hasBluetooth(Context context) {
		return false;
	}

	public boolean supportExternalSD(Context context) {
		return true;
	}

	public void setTrigger(int count) {
	}
}
