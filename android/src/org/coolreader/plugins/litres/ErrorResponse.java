package org.coolreader.plugins.litres;

public class ErrorResponse extends LitresResponse {
	public int errorCode;
	public String errorMessage;
	public ErrorResponse(int errorCode, String errorMessage) {
		this.errorCode = errorCode;
		this.errorMessage = errorMessage;
	}
}
