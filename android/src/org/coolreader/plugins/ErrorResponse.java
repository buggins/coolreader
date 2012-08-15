package org.coolreader.plugins;


public class ErrorResponse implements AsyncResponse {
	public int errorCode;
	public String errorMessage;
	public ErrorResponse(int errorCode, String errorMessage) {
		this.errorCode = errorCode;
		this.errorMessage = errorMessage;
	}
}
