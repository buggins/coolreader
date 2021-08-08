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
