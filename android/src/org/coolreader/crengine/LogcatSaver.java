package org.coolreader.crengine;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

public class LogcatSaver {

	private static final Logger log = L.create("lc");
	private static final int WAIT_TIMEOUT = 1000;

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
		List<String> logcatCmdArgs = new ArrayList<>();
		logcatCmdArgs.add("logcat");			// command
		logcatCmdArgs.add("-t");				// argument 1
		logcatCmdArgs.add(dateString);			// argument 2
		logcatCmdArgs.add("-d");				// argument 3
		Process process = null;
		try {
			process = new ProcessBuilder().command(logcatCmdArgs).redirectErrorStream(true).start();
			ProcessIOWithTimeout processIOWithTimeout = new ProcessIOWithTimeout(process, 1024);
			int exitCode = processIOWithTimeout.waitForProcess(WAIT_TIMEOUT);
			if (ProcessIOWithTimeout.EXIT_CODE_TIMEOUT == exitCode) {
				// timeout
				process.destroy();
				log.e("Timed out waiting for logcat command output!");
			}
			// copy process output stream to file
			if (0 == exitCode) {
				ByteArrayInputStream inputStream = new ByteArrayInputStream(processIOWithTimeout.receivedData());
				byte[] buffer = new byte[1024];
				int rb;
				while ((rb = inputStream.read(buffer)) != -1) {
					outputStream.write(buffer, 0, rb);
				}
				inputStream.close();
				res = true;
			}
		} catch (Exception e) {
			log.e("saveLogcat(): " + e);
		}
		if (null != process)
			process.destroy();
		return res;
	}

}
