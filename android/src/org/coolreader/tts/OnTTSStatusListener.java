/*
 * CoolReader for Android
 * Copyright (C) 2021 Aleksey Chernov <valexlin@gmail.com>
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

public interface OnTTSStatusListener {
	/**
	 * On utterance speech synthesis started.
	 */
	void onUtteranceStart();

	/**
	 * On utterance speech synthesis completed.
	 */
	void onUtteranceDone();

	/**
	 * On error occurred.
	 * @param errorCode TTS error code
	 */
	void onError(int errorCode);

	/**
	 * Playback state changed to state.
	 */
	void onStateChanged(TTSControlService.State state);

	/**
	 * On volume changed.
	 * @param currentVolume current volume
	 * @param maxVolume maximum volume
	 */
	void onVolumeChanged(int currentVolume, int maxVolume);

	/**
	 * On audio focus lost.
	 */
	void onAudioFocusLost();

	/**
	 * On audio focus restored.
	 */
	void onAudioFocusRestored();

	/**
	 * Current sentence requested.
	 * @param ttsbinder
	 */
	void onCurrentSentenceRequested(TTSControlBinder ttsbinder);

	/**
	 * Next sentence requested.
	 * @param ttsbinder
	 */
	void onNextSentenceRequested(TTSControlBinder ttsbinder);

	/**
	 * Previous sentence requested.
	 * @param ttsbinder
	 */
	void onPreviousSentenceRequested(TTSControlBinder ttsbinder);

	/**
	 * Stop requested.
	 * @param ttsbinder
	 */
	void onStopRequested(TTSControlBinder ttsbinder);
}
