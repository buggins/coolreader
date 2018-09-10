package com.onyx.android.sdk.device;

import android.content.Context;
import android.content.Intent;
import android.os.Environment;
import android.provider.Settings;
import android.provider.Settings.Secure;
import android.telephony.TelephonyManager;
import android.util.Log;
import com.onyx.android.sdk.utils.FileUtils;
import com.onyx.android.sdk.utils.StringUtils;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.UnsupportedEncodingException;
import java.util.UUID;

public class EnvironmentUtil {
	private static final String TAG = "EnvironmentUtil";
	public static final File EXTERNAL_STORAGE_ANDROID_DATA_DIRECTORY = new File(new File(getExternalStorageDirectory(), "Android"), "data");

	public EnvironmentUtil() {
	}

	public static File getStorageRootDirectory() {
		return Device.currentDevice.getStorageRootDirectory();
	}

	public static File getExternalStorageDirectory() {
		return Device.currentDevice.getExternalStorageDirectory();
	}

	public static File getExternalStorageAndroidDataDir() {
		return EXTERNAL_STORAGE_ANDROID_DATA_DIRECTORY;
	}

	public static File getExternalStorageAppDataDirectory(String packageName) {
		return new File(EXTERNAL_STORAGE_ANDROID_DATA_DIRECTORY, packageName);
	}

	public static File getExternalStorageAppFilesDirectory(String packageName) {
		return new File(new File(EXTERNAL_STORAGE_ANDROID_DATA_DIRECTORY, packageName), "files");
	}

	public static File getExternalStorageAppCacheDirectory(String packageName) {
		return new File(new File(EXTERNAL_STORAGE_ANDROID_DATA_DIRECTORY, packageName), "cache");
	}

	public static File getRemovableSDCardDirectory() {
		return Device.currentDevice.getRemovableSDCardDirectory();
	}

	public static boolean isFileOnRemovableSDCard(File file) {
		return Device.currentDevice.isFileOnRemovableSDCard(file);
	}

	public static String getDeviceSerial(Context context) {
		UUID uuid = null;
		String androidId = Secure.getString(context.getContentResolver(), Settings.Secure.ANDROID_ID);
		try {
			if (!"9774d56d682e549c".equals(androidId)) {
				uuid = UUID.nameUUIDFromBytes(androidId.getBytes("utf8"));
			} else {
				String var3 = ((TelephonyManager)context.getSystemService(Context.TELEPHONY_SERVICE)).getDeviceId();
				uuid = var3 != null ? UUID.nameUUIDFromBytes(var3.getBytes("utf8")) : UUID.randomUUID();
			}
		} catch (UnsupportedEncodingException e) {
			Log.w(TAG, "exception", e);
			uuid = UUID.randomUUID();
		}
		return uuid.toString();
	}

	public static String getRemovableSDCardCid() {
		FileReader reader = null;
		String var1 = null;
		String tmpl = "/sys/block/mmcblk0/device/type";
		String[] names = new String[]{"mmcblk0", "mmcblk1", "mmcblk2"};

		for (String name : names) {
			var1 = getStorageDevice(tmpl.replaceFirst(names[0], name), "sd");
			if (StringUtils.isNotBlank(var1)) {
				break;
			}
		}
		if (StringUtils.isNullOrEmpty(var1)) {
			Log.w(TAG, "sdCard devicePath is null");
			return null;
		} else {
			String var13 = null;
			try {
				reader = new FileReader(var1 + "cid");
				var13 = (new BufferedReader(reader)).readLine();
				Log.i(TAG, "SDCard cid:" + var13);
			} catch (Exception e) {
			} finally {
				FileUtils.closeQuietly(reader);
			}
			return var13;
		}
	}

	public static String getStorageDevice(String devicePath, String deviceType) {
		FileReader fileReader = null;
		String path;
		try {
			fileReader = new FileReader(devicePath);
			BufferedReader bufferedReader = new BufferedReader(fileReader);
			if (bufferedReader.readLine().toLowerCase().contentEquals(deviceType)) {
				path = devicePath.replaceAll("type", "");
			} else {
				return null;
			}
		} catch (Exception e) {
			return null;
		} finally {
			FileUtils.closeQuietly(fileReader);
		}
		return path;
	}

	public static boolean isExternalStorageDirectory(String path) {
		return path.equals(getExternalStorageDirectory().getAbsolutePath());
	}

	public static boolean isExternalStorageDirectory(Context context, Intent intent) {
		String var2 = FileUtils.getRealFilePathFromUri(context, intent.getData());
		return isExternalStorageDirectory(var2);
	}

	public static boolean isRemovableSDDirectory(String path) {
		return path.equals(getRemovableSDCardDirectory().getAbsolutePath());
	}

	public static boolean isRemovableSDDirectory(Context context, Intent intent) {
		String var2 = FileUtils.getRealFilePathFromUri(context, intent.getData());
		return isRemovableSDDirectory(var2);
	}

	public static File getExternalStorageDownloadDirectory() {
		return new File(getExternalStorageDirectory(), Environment.DIRECTORY_DOWNLOADS);
	}

	public static boolean isStorageRootDirectory(String path) {
		return getStorageRootDirectory().getAbsolutePath().contains(path);
	}

	public static boolean isStorageSDCardDirectory(String path) {
		return isExternalStorageDirectory(path) || isRemovableSDDirectory(path);
	}
}
