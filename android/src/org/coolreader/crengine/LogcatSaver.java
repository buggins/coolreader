/*
 * CoolReader for Android
 * Copyright (C) 2021 Aleksey Chernov <valexlin@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.coolreader.crengine;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class LogcatSaver {

	private static final Logger log = L.create("lc");
	private static final int WAIT_TIMEOUT_MAIN = 10000;
	private static final int WAIT_TIMEOUT_PRUNE = 1000;

	/*
	// May be called in Engine static initialization
	public static boolean logcatPruneSetup() {
		if (DeviceInfo.getSDKLevel() < Build.VERSION_CODES.M)
			// logcat prune list is available since Android 6.0 (API 23)
			return true;
		// Add this application (pid) to logcat white prune list
		// https://developer.android.com/studio/command-line/logcat.html#options
		// Usefully on rooted device.
		// On non rooted device logcat return error "failed to set the prune list"
		// Reason: the logcat daemon return "Permission Denied" instead of "success"
		// in response to the "setPruneList" command.
		boolean res = false;
		int pid = android.os.Process.myPid();
		Process process = null;
		try {
			process = new ProcessBuilder().command(
					//"strace",
					"logcat", "-P", "/" + pid)
					.redirectErrorStream(true).start();
			ProcessIOWithTimeout processIOWithTimeout = new ProcessIOWithTimeout(process);
			int exitCode = processIOWithTimeout.waitForProcess(WAIT_TIMEOUT_MAIN);
			if (0 == exitCode) {
				log.d("Process " + pid + " successfully added to whitelist");
				res = true;
			} else if (ProcessIOWithTimeout.EXIT_CODE_TIMEOUT == exitCode) {
				// timeout
				log.e("Timed out waiting for logcat command output!");
			} else {
				log.e("logcat setPruneList exit code: " + exitCode);
			}
		} catch (Exception e) {
			log.e("Error running logcat: " + e);
		} finally {
			if (null != process)
				process.destroy();
		}
		return res;
	}

	// May be called in Services.stopServices()
	public static boolean logcatPruneDefault() {
		// Set default logcat prune list
		if (DeviceInfo.getSDKLevel() < Build.VERSION_CODES.M)
			// logcat prune list is available since Android 6.0 (API 23)
			return true;
		boolean res = false;
		Process process = null;
		try {
			process = new ProcessBuilder().command(
					"logcat", "-P", "default")
					.redirectErrorStream(true).start();
			ProcessIOWithTimeout processIOWithTimeout = new ProcessIOWithTimeout(process);
			int exitCode = processIOWithTimeout.waitForProcess(WAIT_TIMEOUT_PRUNE);
			if (0 == exitCode) {
				log.d("logcat prune list successfully set to default");
				res = true;
			} else if (ProcessIOWithTimeout.EXIT_CODE_TIMEOUT == exitCode) {
				// timeout
				log.e("Timed out waiting for logcat command output!");
			} else {
				log.e("logcat setPruneList exit code: " + exitCode);
			}
		} catch (Exception e) {
			log.e("Error running logcat: " + e);
		} finally {
			if (null != process)
				process.destroy();
		}
		return res;
	}

	 */

	public static boolean saveLogcat(Date since, File outputFile) {
		boolean res = false;
		try {
			res = saveLogcat(since, new FileOutputStream(outputFile));
		} catch (Exception e) {
			log.e("Failed to save logcat: " + e.toString());
		}
		return res;
	}

	public static boolean saveLogcat(Date since, OutputStream outputStream) {
		boolean res = saveLogcat_impl(since, "yyyy-MM-dd HH:mm:ss.SSS", outputStream);
		if (!res)
			res = saveLogcat_impl(since, "MM-dd HH:mm:ss.SSS", outputStream);
		return res;
	}

	private static boolean saveLogcat_impl(Date since, String dateFmt, OutputStream outputStream) {
		boolean res = false;
		SimpleDateFormat dateFormat = new SimpleDateFormat(dateFmt, Locale.US);
		String dateString = dateFormat.format(since);
		Process process = null;
		try {
			// Do not specify the "-b" and "-v" switches, as they do not work on older versions of Android.
			// "-v", "threadtime",
			// "-b", "events", "-b", "system", "-b", "main", "-b", "crash",
			process = new ProcessBuilder().command(
					"logcat", "-t", dateString, "-d")
					.redirectErrorStream(true).start();
			ProcessIOWithTimeout processIOWithTimeout = new ProcessIOWithTimeout(process, 1024);
			int exitCode = processIOWithTimeout.waitForProcess(WAIT_TIMEOUT_MAIN);
			if (ProcessIOWithTimeout.EXIT_CODE_TIMEOUT == exitCode) {
				// timeout
				log.e("Timed out waiting for logcat command output!");
			} else {
				// Copy process output stream to file
				// If exits code != 0 then in outputStream saved logcat error message
				byte [] data = processIOWithTimeout.receivedData();
				if (null != data) {
					ByteArrayInputStream inputStream = new ByteArrayInputStream(data);
					byte[] buffer = new byte[1024];
					int rb;
					while ((rb = inputStream.read(buffer)) > 0) {
						outputStream.write(buffer, 0, rb);
					}
					inputStream.close();
					res = (0 == exitCode);
				}
			}
		} catch (Exception e) {
			log.e("Error running logcat: " + e);
		} finally {
			if (null != process)
				process.destroy();
		}
		return res;
	}

}
