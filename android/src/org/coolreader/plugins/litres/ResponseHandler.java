package org.coolreader.plugins.litres;

import org.coolreader.plugins.AsyncResponse;
import org.xml.sax.helpers.DefaultHandler;

import android.util.Log;

public abstract class ResponseHandler extends DefaultHandler {
	private int errorCode = -1;
	private String errorMessage;
	public void onError(int errorCode, String errorMessage) {
		Log.e("litres", "error " + errorCode + ": " + errorMessage);
		this.errorCode = errorCode;
		this.errorMessage = errorMessage;
	}
	public AsyncResponse getResponse() {
		if (errorCode != -1)
			return new ErrorResponse(errorCode, errorMessage);
		return null;
	}
}
