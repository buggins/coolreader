package org.coolreader.crengine;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

public class LogcatSaver {

	private static final Logger log = L.create("lc");
	private static final int WAIT_TIMEOUT = 1000;

	private static class IOProcessWithTimeout extends Thread {

		private final Process m_process;
		private int m_exitCode = Integer.MIN_VALUE;
		private final ByteArrayOutputStream m_outputStream;
		private final byte[] m_buffer;

		public IOProcessWithTimeout(Process process, int buffSize) {
			m_process = process;
			m_buffer = new byte[buffSize];
			m_outputStream = new ByteArrayOutputStream();
		}

		public int waitForProcess(long timeout) {
			start();
			try {
				join(timeout);
			} catch (InterruptedException e) {
				interrupt();
			}
			return m_exitCode;
		}

		public byte[] receivedData() {
			return m_outputStream.toByteArray();
		}

		@Override
		public void run() {
			try {
				InputStream inputStream = m_process.getInputStream();
				int rb;
				while ((rb = inputStream.read(m_buffer)) != -1) {
					m_outputStream.write(m_buffer, 0, rb);
				}
				// at this point the process should already be terminated
				// just get exit code
				m_exitCode = m_process.waitFor();
				inputStream.close();
			} catch (InterruptedException ignore) {
				// Do nothing
			} catch (IOException ignored) {
				// Do nothing
			} catch (Exception ex) {
				// Unexpected exception
			}
		}
	}

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
			IOProcessWithTimeout ioProcessWithTimeout = new IOProcessWithTimeout(process, 1024);
			int exitCode = ioProcessWithTimeout.waitForProcess(WAIT_TIMEOUT);
			if (Integer.MIN_VALUE == exitCode) {
				// timeout
				process.destroy();
				process = null;
				throw new RuntimeException("Timed out waiting for logcat command output!");
			}
			// copy process output stream to file
			ByteArrayInputStream inputStream = new ByteArrayInputStream(ioProcessWithTimeout.receivedData());
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
