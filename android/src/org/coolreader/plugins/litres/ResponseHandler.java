package org.coolreader.plugins.litres;

import org.xml.sax.helpers.DefaultHandler;

public abstract class ResponseHandler extends DefaultHandler {
	private int errorCode = -1;
	private String errorMessage;
	public void onError(int errorCode, String errorMessage) {
		this.errorCode = errorCode;
		this.errorMessage = errorMessage;
	}
	public LitresResponse getResponse() {
		if (errorCode != -1)
			return new ErrorResponse(errorCode, errorMessage);
		return null;
	}
}
