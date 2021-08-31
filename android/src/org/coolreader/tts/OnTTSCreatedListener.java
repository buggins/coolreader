package org.coolreader.tts;

public interface OnTTSCreatedListener {
	void onCreated();
	void onFailed();
	void onTimedOut();
}
