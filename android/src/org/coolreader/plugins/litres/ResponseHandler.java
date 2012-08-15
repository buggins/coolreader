package org.coolreader.plugins.litres;

import org.coolreader.plugins.AsyncResponse;
import org.coolreader.plugins.ErrorResponse;
import org.xml.sax.helpers.DefaultHandler;

import android.util.Log;

public abstract class ResponseHandler extends DefaultHandler implements ResponseCallback {
	private int errorCode = -1;
	private String errorMessage;
	@Override
	public void onError(int errorCode, String errorMessage) {
		Log.e("litres", "error " + errorCode + ": " + errorMessage);
		this.errorCode = errorCode;
		this.errorMessage = errorMessage;
	}
	@Override
	public AsyncResponse getResponse() {
		if (errorCode != -1)
			return new ErrorResponse(errorCode, errorMessage);
		return null;
	}
}
