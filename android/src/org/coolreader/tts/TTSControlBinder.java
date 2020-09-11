package org.coolreader.tts;

import android.os.Binder;
import android.os.Build;

public class TTSControlBinder extends Binder {

	public interface Callback {
		void run(TTSControlBinder tts);
	}

	private TTSControlService mService;

	public TTSControlBinder(TTSControlService service) {
		mService = service;
	}

	public void notifyPlay(String title, String sentence) {
		if (Build.VERSION.SDK_INT > Build.VERSION_CODES.ECLAIR)
			mService.notifyPlay(title, sentence);
	}

	public void notifyPause(String title) {
		if (Build.VERSION.SDK_INT > Build.VERSION_CODES.ECLAIR)
			mService.notifyPause(title);
	}

}
