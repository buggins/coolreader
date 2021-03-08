package org.coolreader.crengine;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.drawable.BitmapDrawable;
import android.media.AudioManager;
import android.os.Build;
import android.os.Bundle;
import android.speech.tts.TextToSpeech;
import android.speech.tts.UtteranceProgressListener;
import android.speech.tts.Voice;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.PopupWindow;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;

import com.s_trace.motion_watchdog.HandlerThread;
import com.s_trace.motion_watchdog.MotionWatchdogHandler;

import org.coolreader.CoolReader;
import org.coolreader.R;
import org.coolreader.tts.TTSControlBinder;
import org.coolreader.tts.TTSControlService;
import org.coolreader.tts.TTSControlServiceAccessor;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

public class TTSToolbarDlg implements Settings {
	public static final Logger log = L.create("ttssrv");

	private static final String CR3_UTTERANCE_ID = "cr3UtteranceId";
	private static final int MAX_CONTINUOUS_ERRORS = 3;

	private final PopupWindow mWindow;
	private final CoolReader mCoolReader;
	private final ReaderView mReaderView;
	private String mBookTitle;
	private TextToSpeech mTTS;
	private TTSControlServiceAccessor mTTSControl;
	private ImageButton mPlayPauseButton;
	private SeekBar mSbSpeed;
	private SeekBar mSbVolume;
	private HandlerThread mMotionWatchdog;
	private boolean changedPageMode;
	private int mContinuousErrors = 0;
	private Runnable mOnCloseListener;
	private boolean mClosed;
	private Selection mCurrentSelection;
	private boolean isSpeaking;
	private Runnable mOnStopRunnable;
	private int mMotionTimeout;
	private boolean mAutoSetDocLang;
	private String mForcedLanguage;
	private String mForcedVoice;
	private int mTTSSpeedPercent = 50;		// 50% (normal)


	BroadcastReceiver mTTSControlButtonReceiver = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			log.d("received action: " + action);
			if (null != action) {
				switch (action) {
					case TTSControlService.TTS_CONTROL_ACTION_PLAY_PAUSE:
						toggleStartStop();
						break;
					case TTSControlService.TTS_CONTROL_ACTION_NEXT:
						if ( isSpeaking ) {
							stop(() -> {
								isSpeaking = true;
								moveSelection( ReaderCommand.DCMD_SELECT_NEXT_SENTENCE );
							});
						} else
							moveSelection( ReaderCommand.DCMD_SELECT_NEXT_SENTENCE );
						break;
					case TTSControlService.TTS_CONTROL_ACTION_PREV:
						if ( isSpeaking ) {
							stop(() -> {
								isSpeaking = true;
								moveSelection( ReaderCommand.DCMD_SELECT_PREV_SENTENCE );
							});
						} else
							moveSelection( ReaderCommand.DCMD_SELECT_PREV_SENTENCE );
						break;
					case TTSControlService.TTS_CONTROL_ACTION_DONE:
						stopAndClose();
						break;
				}
			}
		}
	};

	static public TTSToolbarDlg showDialog( CoolReader coolReader, ReaderView readerView, TextToSpeech tts) {
		TTSToolbarDlg dlg = new TTSToolbarDlg(coolReader, readerView, tts);
		log.d("popup: " + dlg.mWindow.getWidth() + "x" + dlg.mWindow.getHeight());
		return dlg;
	}

	public void setOnCloseListener(Runnable handler) {
		mOnCloseListener = handler;
	}

	public void stopAndClose() {
		if (mClosed)
			return;
		isSpeaking = false;
		mClosed = true;
		BackgroundThread.instance().executeGUI(() -> {
			stop();
			mCoolReader.unregisterReceiver(mTTSControlButtonReceiver);
			if (null != mTTSControl)
				mTTSControl.unbind();
			Intent intent = new Intent(mCoolReader, TTSControlService.class);
			mCoolReader.stopService(intent);
			restoreReaderMode();
			mReaderView.clearSelection();
			if (mOnCloseListener != null)
				mOnCloseListener.run();
			if ( mWindow.isShowing() )
				mWindow.dismiss();
			mReaderView.save();
		});
	}

	private void setReaderMode() {
		String oldViewSetting = mReaderView.getSetting( ReaderView.PROP_PAGE_VIEW_MODE );
		if ( "1".equals(oldViewSetting) ) {
			changedPageMode = true;
			mReaderView.setViewModeNonPermanent(ViewMode.SCROLL);
		}
		moveSelection( ReaderCommand.DCMD_SELECT_FIRST_SENTENCE );
	}

	private void restoreReaderMode() {
		if ( changedPageMode ) {
			mReaderView.setViewModeNonPermanent(ViewMode.PAGES);
		}
	}

	private void moveSelection( ReaderCommand cmd )
	{
		mReaderView.moveSelection(cmd, 0, new ReaderView.MoveSelectionCallback() {
			
			@Override
			public void onNewSelection(Selection selection) {
				log.d("onNewSelection: " + selection.text);
				mCurrentSelection = selection;
				if ( isSpeaking )
					say(mCurrentSelection);
			}
			
			@Override
			public void onFail() {
				log.e("fail()");
				stop();
				//mCurrentSelection = null;
			}
		});
	}

	private void say( Selection selection ) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
			Bundle bundle = new Bundle();
			bundle.putInt(TextToSpeech.Engine.KEY_PARAM_STREAM, AudioManager.STREAM_MUSIC);
			mTTS.speak(selection.text, TextToSpeech.QUEUE_ADD, bundle, CR3_UTTERANCE_ID);
		} else {
			HashMap<String, String> params = new HashMap<String, String>();
			params.put(TextToSpeech.Engine.KEY_PARAM_STREAM, String.valueOf(AudioManager.STREAM_MUSIC));
			params.put(TextToSpeech.Engine.KEY_PARAM_UTTERANCE_ID, CR3_UTTERANCE_ID);
			mTTS.speak(selection.text, TextToSpeech.QUEUE_ADD, params);
		}
		runInTTSControlService(tts -> tts.notifyPlay(mBookTitle, selection.text));
	}

	private void start() {
		if ( mCurrentSelection ==null )
			return;
		startMotionWatchdog();
		isSpeaking = true;
		say(mCurrentSelection);
	}

	private void startMotionWatchdog(){
		String TAG = "MotionWatchdog";
		log.d("startMotionWatchdog() enter");

		if (mMotionTimeout == 0) {
			Log.d(TAG, "startMotionWatchdog() early exit - timeout is 0");
			return;
		}

		mMotionWatchdog = new HandlerThread("MotionWatchdog");
		mMotionWatchdog.start();
		new MotionWatchdogHandler(this, mCoolReader, mMotionWatchdog, mMotionTimeout);
		Log.d(TAG, "startMotionWatchdog() exit");
	}

	private void stop() {
		stop(null);
	}

	private void stop(Runnable runnable) {
		isSpeaking = false;
		mOnStopRunnable = runnable;
		if ( mTTS.isSpeaking() ) {
			mTTS.stop();
		}
		if (mMotionWatchdog != null) {
			mMotionWatchdog.interrupt();
		}
	}

	public void pause() {
		if (isSpeaking)
			toggleStartStop();
	}

	private void toggleStartStop() {
		if ( isSpeaking ) {
			mPlayPauseButton.setImageResource(Utils.resolveResourceIdByAttr(mCoolReader, R.attr.ic_media_play_drawable, R.drawable.ic_media_play));
			runInTTSControlService(tts -> tts.notifyPause(mBookTitle));
			stop();
		} else {
			if (null != mCurrentSelection) {
				mPlayPauseButton.setImageResource(Utils.resolveResourceIdByAttr(mCoolReader, R.attr.ic_media_pause_drawable, R.drawable.ic_media_pause));
				runInTTSControlService(tts -> tts.notifyPlay(mBookTitle, mCurrentSelection.text));
				start();
			}
		}
	}

	private void runInTTSControlService(TTSControlBinder.Callback callback) {
		if (null == mTTSControl) {
			mTTSControl = new TTSControlServiceAccessor(mCoolReader);
		}
		mTTSControl.bind(callback);
	}

	public void changeTTS(TextToSpeech tts) {
		pause();
		mTTS = tts;
		setupTTSVoice();
		setupTTSHandlers();
	}

	/**
	 * Convert speech speed percentage to speech rate value.
	 * @param percent speech rate percentage
	 * @return speech rate value
	 *
	 * 0%  - 0.30
	 * 10% - 0.44
	 * 20% - 0.58
	 * 30% - 0.72
	 * 40% - 0.86
	 * 50% - 1.00
	 * 60% - 1.50
	 * 70% - 2.00
	 * 80% - 2.50
	 * 90% - 3.00
	 * 100%- 3.50
	 */
	private float speechRateFromPercent(int percent) {
		float rate;
		if ( percent < 50 )
			rate = 0.3f + 0.7f * percent / 50f;
		else
			rate = 1.0f + 2.5f * (percent - 50) / 50f;
		return rate;
	}

	public void setAppSettings(Properties newSettings, Properties oldSettings) {
		log.v("setAppSettings()");
		BackgroundThread.ensureGUI();
		if (oldSettings == null)
			oldSettings = new Properties();
		Properties changedSettings = newSettings.diff(oldSettings);
		for (Map.Entry<Object, Object> entry : changedSettings.entrySet()) {
			String key = (String) entry.getKey();
			String value = (String) entry.getValue();
			processAppSetting(key, value);
		}
		// Apply settings
		setupTTSVoice();
		mTTS.setSpeechRate(speechRateFromPercent(mTTSSpeedPercent));
		mSbSpeed.setProgress(mTTSSpeedPercent);
	}

	public void processAppSetting(String key, String value) {
		boolean flg = "1".equals(value);
		switch (key) {
			case PROP_APP_MOTION_TIMEOUT:
				mMotionTimeout = Utils.parseInt(value, 0, 0, 100);
				mMotionTimeout = mMotionTimeout * 60 * 1000; // Convert minutes to msecs
				break;
			case PROP_APP_TTS_SPEED:
				mTTSSpeedPercent = Utils.parseInt(value, 50, 0, 100);
				break;
			case PROP_APP_TTS_ENGINE:
				// handled in CoolReader
				break;
			case PROP_APP_TTS_USE_DOC_LANG:
				mAutoSetDocLang = flg;
				break;
			case PROP_APP_TTS_FORCE_LANGUAGE:
				mForcedLanguage = value;
				break;
			case PROP_APP_TTS_VOICE:
				mForcedVoice = value;
				break;
		}
	}

	private void setupTTSVoice() {
		if (mAutoSetDocLang) {
			// set language for TTS based on book's language
			log.d("Setting language according book's language");
			Locale locale = null;
			BookInfo bookInfo = mReaderView.getBookInfo();
			if (null != bookInfo) {
				FileInfo fileInfo = bookInfo.getFileInfo();
				if (null != fileInfo) {
					log.d("book language is \"" + fileInfo.language + "\"");
					if (null != fileInfo.language && fileInfo.language.length() > 0) {
						locale = new Locale(fileInfo.language);
					}
				}
			}
			if (null != locale) {
				log.d("trying to set TTS language to \"" + locale.getDisplayLanguage() + "\"");
				mTTS.setLanguage(locale);
			} else {
				log.e("Failed to detect book's language, using system default!");
			}
		} else {
			if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP) {
				// Update voices list
				Set<Voice> voices;
				if (null != mTTS)
					voices = mTTS.getVoices();
				else
					voices = null;
				// Filter voices for given language
				log.d("Trying to find voice for language \"" + mForcedLanguage + "\"");
				Voice sel_voice = null;
				if (null != voices && null != mForcedLanguage && mForcedLanguage.length() > 0) {
					ArrayList<Voice> acceptable_voices = new ArrayList<>();
					for (Voice voice : voices) {
						Locale locale = voice.getLocale();
						if (mForcedLanguage.toLowerCase().equals(locale.toString().toLowerCase())) {
							acceptable_voices.add(voice);
						}
					}
					if (acceptable_voices.size() > 0) {
						// Select one specific voice
						boolean found = false;
						for (Voice voice : acceptable_voices) {
							if (voice.getName().equals(mForcedVoice))
							{
								sel_voice = voice;
								found = true;
								break;
							}
						}
						if (found) {
							log.d("Voice \"" + mForcedVoice + "\" is found");
						} else {
							sel_voice = acceptable_voices.get(0);
							log.e("Voice \"" + mForcedVoice + "\" NOT found, using \"" + sel_voice.getName() + "\"");
						}
					}
				}
				if (sel_voice != null) {
					log.d("Setting voice: " + sel_voice.getName());
					mTTS.setVoice(sel_voice);
				} else {
					log.e("Failed to find voice for language \"" + mForcedLanguage + "\"!");
				}
			}
		}

	}

	private void setupTTSHandlers() {
		if (null != mTTS) {
			if (Build.VERSION.SDK_INT < Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1) {
				mTTS.setOnUtteranceCompletedListener(utteranceId -> {
					if (null != mOnStopRunnable) {
						mOnStopRunnable.run();
						mOnStopRunnable = null;
					} else {
						if ( isSpeaking )
							moveSelection( ReaderCommand.DCMD_SELECT_NEXT_SENTENCE );
					}
				});
			} else {
				mTTS.setOnUtteranceProgressListener(new UtteranceProgressListener() {
					@Override
					public void onStart(String utteranceId) {
						// nothing...
					}

					@Override
					public void onDone(String utteranceId) {
						if (null != mOnStopRunnable) {
							mOnStopRunnable.run();
							mOnStopRunnable = null;
						} else {
							if ( isSpeaking )
								moveSelection( ReaderCommand.DCMD_SELECT_NEXT_SENTENCE );
						}
						mContinuousErrors = 0;
					}

					@Override
					public void onError(String utteranceId) {
						log.e("TTS error");
						mContinuousErrors++;
						if (mContinuousErrors > MAX_CONTINUOUS_ERRORS) {
							BackgroundThread.instance().executeGUI(() -> {
								toggleStartStop();
								mCoolReader.showToast(R.string.tts_failed);
							});
						} else {
							if (null != mOnStopRunnable) {
								mOnStopRunnable.run();
								mOnStopRunnable = null;
							} else {
								if ( isSpeaking )
									moveSelection( ReaderCommand.DCMD_SELECT_NEXT_SENTENCE );
							}
						}
					}

					// API 21
					@Override
					public void onError(String utteranceId, int errorCode) {
						log.e("TTS error, code=" + errorCode);
						mContinuousErrors++;
						if (mContinuousErrors > MAX_CONTINUOUS_ERRORS) {
							BackgroundThread.instance().executeGUI(() -> {
								toggleStartStop();
								mCoolReader.showToast(R.string.tts_failed);
							});
						} else {
							if (null != mOnStopRunnable) {
								mOnStopRunnable.run();
								mOnStopRunnable = null;
							} else {
								if ( isSpeaking )
									moveSelection( ReaderCommand.DCMD_SELECT_NEXT_SENTENCE );
							}
						}
					}

					// API 23
					@Override
					public void onStop(String utteranceId, boolean interrupted) {
						if (null != mOnStopRunnable) {
							mOnStopRunnable.run();
							mOnStopRunnable = null;
						}
					}

					// API 24
					public void onAudioAvailable(String utteranceId, byte[] audio) {
						// nothing...
					}

					// API 24
					public void onBeginSynthesis(String utteranceId,
												 int sampleRateInHz,
												 int audioFormat,
												 int channelCount) {
						// nothing...
					}
				});
			}
		}
	}

	public TTSToolbarDlg(CoolReader coolReader, ReaderView readerView, TextToSpeech tts) {
		mCoolReader = coolReader;
		mReaderView = readerView;
		View anchor = readerView.getSurface();
		mTTS = tts;
		setupTTSHandlers();

		//Context context = mCoolReader.getApplicationContext();
		Context context = anchor.getContext();
		LayoutInflater inflater = LayoutInflater.from(context);
		View panel = inflater.inflate(R.layout.tts_toolbar, null);
		panel.measure(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);

		mPlayPauseButton = panel.findViewById(R.id.tts_play_pause);
		mPlayPauseButton.setImageResource(Utils.resolveResourceIdByAttr(mCoolReader, R.attr.ic_media_play_drawable, R.drawable.ic_media_play));
		ImageButton backButton = panel.findViewById(R.id.tts_back);
		ImageButton forwardButton = panel.findViewById(R.id.tts_forward);
		ImageButton stopButton = panel.findViewById(R.id.tts_stop);
		ImageButton optionsButton = panel.findViewById(R.id.tts_options);

		mWindow = new PopupWindow( context );
		mWindow.setBackgroundDrawable(new BitmapDrawable());
		mPlayPauseButton.setOnClickListener(v -> toggleStartStop());
		backButton.setOnClickListener(v -> {
			if ( isSpeaking ) {
				stop(() -> {
					isSpeaking = true;
					moveSelection( ReaderCommand.DCMD_SELECT_PREV_SENTENCE );
				});
			} else
				moveSelection( ReaderCommand.DCMD_SELECT_PREV_SENTENCE );
		});
		forwardButton.setOnClickListener(v -> {
			if ( isSpeaking ) {
				stop(() -> {
					isSpeaking = true;
					moveSelection( ReaderCommand.DCMD_SELECT_NEXT_SENTENCE );
				});
			} else
				moveSelection( ReaderCommand.DCMD_SELECT_NEXT_SENTENCE );
		});
		optionsButton.setOnClickListener(v -> {
			OptionsDialog dlg = new OptionsDialog(mCoolReader, OptionsDialog.Mode.TTS, null, null, mTTS);
			dlg.show();
		});
		stopButton.setOnClickListener(v -> stopAndClose());
		panel.setFocusable(true);
		panel.setEnabled(true);
		panel.setOnKeyListener((v, keyCode, event) -> {
			if ( event.getAction()==KeyEvent.ACTION_UP ) {
				switch ( keyCode ) {
				case KeyEvent.KEYCODE_VOLUME_DOWN:
				case KeyEvent.KEYCODE_VOLUME_UP:
					return true;
				case KeyEvent.KEYCODE_BACK:
					stopAndClose();
					return true;
//					case KeyEvent.KEYCODE_DPAD_LEFT:
//					case KeyEvent.KEYCODE_DPAD_UP:
//						//mReaderView.findNext(pattern, true, caseInsensitive);
//						return true;
//					case KeyEvent.KEYCODE_DPAD_RIGHT:
//					case KeyEvent.KEYCODE_DPAD_DOWN:
//						//mReaderView.findNext(pattern, false, caseInsensitive);
//						return true;
				}
			} else if ( event.getAction()==KeyEvent.ACTION_DOWN ) {
				switch ( keyCode ) {
				case KeyEvent.KEYCODE_VOLUME_DOWN: {
					int p = mSbVolume.getProgress() - 5;
					if ( p<0 )
						p = 0;
					mSbVolume.setProgress(p);
					return true;
				}
				case KeyEvent.KEYCODE_VOLUME_UP:
					int p = mSbVolume.getProgress() + 5;
					if ( p>100 )
						p = 100;
					mSbVolume.setProgress(p);
					return true;
				}
				if ( keyCode == KeyEvent.KEYCODE_BACK) {
					return true;
				}
			}
			return false;
		});

		mWindow.setOnDismissListener(() -> {
			if ( !mClosed)
				stopAndClose();
		});

		mWindow.setBackgroundDrawable(new BitmapDrawable());
		mWindow.setWidth(WindowManager.LayoutParams.FILL_PARENT);
		mWindow.setHeight(WindowManager.LayoutParams.WRAP_CONTENT);
		mWindow.setFocusable(true);
		mWindow.setTouchable(true);
		mWindow.setOutsideTouchable(true);
		mWindow.setContentView(panel);

		int [] location = new int[2];
		anchor.getLocationOnScreen(location);

		mWindow.showAtLocation(anchor, Gravity.TOP | Gravity.CENTER_HORIZONTAL, location[0], location[1] + anchor.getHeight() - panel.getHeight());

		setReaderMode();

		// setup speed && volume seek bars
		mSbSpeed = panel.findViewById(R.id.tts_sb_speed);
		mSbVolume = panel.findViewById(R.id.tts_sb_volume);

		mSbSpeed.setMax(100);
		mSbSpeed.setProgress(50);
		mSbVolume.setMax(100);
		mSbVolume.setProgress(mCoolReader.getVolume());
		mSbSpeed.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
			@Override
			public void onProgressChanged(SeekBar seekBar, int progress,
					boolean fromUser) {
				// round to a multiple of 10
				int roundedVal = 10*((progress + 5)/10);
				if (progress != roundedVal) {
					mSbSpeed.setProgress(roundedVal);
					return;
				}
				mTTSSpeedPercent = progress;
				mTTS.setSpeechRate(speechRateFromPercent(mTTSSpeedPercent));
			}

			@Override
			public void onStartTrackingTouch(SeekBar seekBar) {
			}

			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
				mCoolReader.setSetting(PROP_APP_TTS_SPEED, String.valueOf(mTTSSpeedPercent), false);
			}
		});
		mSbVolume.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
			@Override
			public void onProgressChanged(SeekBar seekBar, int progress,
					boolean fromUser) {
				mCoolReader.setVolume(progress);
			}

			@Override
			public void onStartTrackingTouch(SeekBar seekBar) {
			}

			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
			}
		});

		BookInfo bookInfo = mReaderView.getBookInfo();
		if (null != bookInfo) {
			FileInfo fileInfo = bookInfo.getFileInfo();
			if (null != fileInfo) {
				mBookTitle = fileInfo.title;
			}
		}
		if (null == mBookTitle)
			mBookTitle = "";

		// Start the foreground service to make this app also foreground,
		// even if the main activity is in the background.
		// https://developer.android.com/about/versions/oreo/background#services
		Intent intent = new Intent(coolReader, TTSControlService.class);
		Bundle data = new Bundle();
		data.putString("bookTitle", mBookTitle);
		intent.putExtras(data);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
			coolReader.startForegroundService(intent);
		else
			coolReader.startService(intent);
		IntentFilter filter = new IntentFilter();
		filter.addAction(TTSControlService.TTS_CONTROL_ACTION_PLAY_PAUSE);
		filter.addAction(TTSControlService.TTS_CONTROL_ACTION_NEXT);
		filter.addAction(TTSControlService.TTS_CONTROL_ACTION_PREV);
		filter.addAction(TTSControlService.TTS_CONTROL_ACTION_DONE);
		mCoolReader.registerReceiver(mTTSControlButtonReceiver, filter);

		panel.requestFocus();
	}

}
