package org.coolreader.plugins.litres;

import org.coolreader.plugins.AsyncResponse;

public class ErrorResponse extends AsyncResponse {
	public int errorCode;
	public String errorMessage;
	public ErrorResponse(int errorCode, String errorMessage) {
		this.errorCode = errorCode;
		this.errorMessage = errorMessage;
	}
}
