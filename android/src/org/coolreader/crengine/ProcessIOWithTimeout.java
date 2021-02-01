package org.coolreader.crengine;

// By Muzikant@stackoverflow.com
// See https://stackoverflow.com/a/19272315 for details

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class ProcessIOWithTimeout extends Thread {

	public static final int EXIT_CODE_TIMEOUT = Integer.MIN_VALUE;

	final private Process m_process;
	private int m_exitCode = EXIT_CODE_TIMEOUT;
	private final ByteArrayOutputStream m_outputStream;
	private final byte[] m_buffer;

	public ProcessIOWithTimeout(Process process, int buffSize) {
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
