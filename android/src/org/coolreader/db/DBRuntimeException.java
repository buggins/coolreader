package org.coolreader.db;

@SuppressWarnings("serial")
public class DBRuntimeException extends RuntimeException {
	public DBRuntimeException(String msg) {
		super(msg);
	}
	public DBRuntimeException(String msg, Throwable e) {
		super(msg, e);
	}
}
