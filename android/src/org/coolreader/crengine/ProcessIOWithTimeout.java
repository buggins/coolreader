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

	public ProcessIOWithTimeout(Process process) {
		m_process = process;
		m_outputStream = null;
		m_buffer = null;
	}

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
		if (null != m_outputStream)
			return m_outputStream.toByteArray();
		return null;
	}

	public String receivedText()  {
		if (null != m_outputStream)
			return m_outputStream.toString();
		return "";
	}

	@Override
	public void run() {
		InputStream inputStream = null;
		try {
			if (null != m_outputStream && null != m_buffer) {
				try { m_process.getOutputStream().close(); } catch (IOException ignored) {}
				try { m_process.getErrorStream().close(); } catch (IOException ignored) {}
				inputStream = m_process.getInputStream();
				int rb;
				while ((rb = inputStream.read(m_buffer)) > 0) {
					m_outputStream.write(m_buffer, 0, rb);
				}
				// at this point the process should already be terminated
				// just get exit code
				m_exitCode = m_process.waitFor();
			} else
				m_exitCode = m_process.waitFor();
		} catch (InterruptedException ignore) {
			// Do nothing
		} catch (IOException ignored) {
			// Do nothing
		} catch (Exception ex) {
			// Unexpected exception
		} finally {
			if (null != inputStream) {
				try {
					inputStream.close();
				} catch (Exception ignored) {
				}
			}
		}
	}
}
