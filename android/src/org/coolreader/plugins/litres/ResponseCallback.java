package org.coolreader.plugins.litres;

import org.coolreader.plugins.AsyncResponse;

public interface ResponseCallback {
	void onError(int errorCode, String errorMessage);
	AsyncResponse getResponse();
}
