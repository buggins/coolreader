package org.coolreader.plugins;

import java.io.File;

import org.coolreader.plugins.litres.ResponseCallback;

import android.util.Log;

public class FileResponse implements ResponseCallback, AsyncResponse {
	private int errorCode = -1;
	private String errorMessage;
	public OnlineStoreBook book;
	public File fileToSave;
	public boolean isTrial;

	public FileResponse(OnlineStoreBook book, File fileToSave, boolean isTrial) {
		this.book = book;
		this.fileToSave = fileToSave;
		this.isTrial = isTrial;
	}
	
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
		return this;
	}
}
