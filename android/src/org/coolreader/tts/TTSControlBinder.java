/*
 * CoolReader for Android
 * Copyright (C) 2020,2021 Aleksey Chernov <valexlin@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.coolreader.tts;

import android.graphics.Bitmap;
import android.os.Binder;
import android.os.Handler;
import android.speech.tts.Voice;

import java.util.Locale;

public class TTSControlBinder extends Binder {

	public interface Callback {
		void run(TTSControlBinder ttsbinder);
	}

	private final TTSControlService mService;

	public TTSControlBinder(TTSControlService service) {
		mService = service;
	}

	public void initTTS(String engine, OnTTSCreatedListener listener) {
		mService.initTTS(engine, listener);
	}

	public void setMediaItemInfo(String authors, String title, Bitmap cover) {
		mService.setMediaItemInfo(authors, title, cover);
	}

	public void getState(TTSControlService.RetrieveStateCallback callback) {
		mService.retrieveState(callback, new Handler());
	}

	public void say(String utterance, TTSControlService.BooleanResultCallback callback) {
		mService.say(utterance, callback, new Handler());
	}

	public void setCurrentUtterance(String utterance) {
		mService.setCurrentUtterance(utterance);
	}

	public void pause(TTSControlService.BooleanResultCallback callback) {
		mService.pause(callback, new Handler());
	}

	public void stopUtterance(TTSControlService.BooleanResultCallback callback) {
		mService.stopUtterance(callback, new Handler());
	}

	public void stop(TTSControlService.BooleanResultCallback callback) {
		mService.stop(callback, new Handler());
	}

	public void retrieveAvailableEngines(TTSControlService.RetrieveEnginesListCallback callback) {
		mService.retrieveAvailableEngines(callback, new Handler());
	}

	public void retrieveAvailableLocales(TTSControlService.RetrieveLocalesListCallback callback) {
		mService.retrieveAvailableLocales(callback, new Handler());
	}

	public void retrieveAvailableVoices(Locale locale, TTSControlService.RetrieveVoicesListCallback callback) {
		mService.retrieveAvailableVoices(locale, callback, new Handler());
	}

	public void retrieveLanguage(TTSControlService.StringResultCallback callback) {
		mService.retrieveLanguage(callback, new Handler());
	}

	public void setLanguage(String language, TTSControlService.BooleanResultCallback callback) {
		mService.setLanguage(language, callback, new Handler());
	}

	public void retrieveVoice(TTSControlService.RetrieveVoiceCallback callback) {
		mService.retrieveVoice(callback, new Handler());
	}

	public void setVoice(Voice voice, TTSControlService.BooleanResultCallback callback) {
		mService.setVoice(voice, callback, new Handler());
	}

	public void setVoice(String voiceName, TTSControlService.BooleanResultCallback callback) {
		mService.setVoice(voiceName, callback, new Handler());
	}

	public void retrieveSpeechRate(TTSControlService.FloatResultCallback callback) {
		mService.retrieveSpeechRate(callback, new Handler());
	}

	public void setSpeechRate(float rate, TTSControlService.BooleanResultCallback callback) {
		mService.setSpeechRate(rate, callback, new Handler());
	}

	public void retrieveVolume(TTSControlService.VolumeResultCallback callback) {
		mService.retrieveVolume(callback, new Handler());
	}

	public void setVolume(int volume) {
		mService.setVolume(volume);
	}

	public OnTTSStatusListener getStatusListener() {
		return mService.getStatusListener();
	}

	public void setStatusListener(OnTTSStatusListener listener) {
		mService.setStatusListener(listener);
	}

}
