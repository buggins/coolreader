package org.coolreader.crengine;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileOutputStream;
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
		Process process = null;
		try {
			SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.US);
			String dateString = dateFormat.format(since);
			List<String> logcatCmdArgs = new ArrayList<>();
			logcatCmdArgs.add("logcat");			// command
			logcatCmdArgs.add("-t");				// argument 1
			logcatCmdArgs.add(dateString);			// argument 2
			logcatCmdArgs.add("-d");				// argument 3
			process = new ProcessBuilder().command(logcatCmdArgs).redirectErrorStream(true).start();
			ProcessIOWithTimeout processIOWithTimeout = new ProcessIOWithTimeout(process, 1024);
			int exitCode = processIOWithTimeout.waitForProcess(WAIT_TIMEOUT);
			if (Integer.MIN_VALUE == exitCode) {
				// timeout
				process.destroy();
				process = null;
				throw new RuntimeException("Timed out waiting for logcat command output!");
			}
			// copy process output stream to file
			ByteArrayInputStream inputStream = new ByteArrayInputStream(processIOWithTimeout.receivedData());
			FileOutputStream outputStream = new FileOutputStream(outputFile);
			byte[] buffer = new byte[1024];
			int rb;
			while ((rb = inputStream.read(buffer)) != -1) {
				outputStream.write(buffer, 0, rb);
			}
			inputStream.close();
			outputStream.close();
			process.destroy();
			process = null;
			res = (0 == exitCode);
		} catch (Exception e) {
			if (null != process)
				process.destroy();
			log.e("Failed to save logcat: " + e.toString());
		}
		return res;
	}
}
