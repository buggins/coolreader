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

import java.util.HashMap;
import java.util.Locale;

public class TTSToolbarDlg {
	public static final Logger log = L.create("ttssrv");

	private static final String CR3_UTTERANCE_ID = "cr3UtteranceId";
	private static final int MAX_CONTINUOUS_ERRORS = 3;

	private final PopupWindow mWindow;
	private final CoolReader mCoolReader;
	private final ReaderView mReaderView;
	private String mBookTitle;
	private final TextToSpeech mTTS;
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

		Properties settings = mReaderView.getSettings();
		int timeout = settings.getInt(ReaderView.PROP_APP_MOTION_TIMEOUT, 0);
		if (timeout == 0) {
			Log.d(TAG, "startMotionWatchdog() early exit - timeout is 0");
			return;
		}
		timeout = timeout * 60 * 1000; // Convert minutes to msecs

		mMotionWatchdog = new HandlerThread("MotionWatchdog");
		mMotionWatchdog.start();
		new MotionWatchdogHandler(this, mCoolReader, mMotionWatchdog, timeout);
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

	public TTSToolbarDlg( CoolReader coolReader, ReaderView readerView, TextToSpeech tts ) {
		mCoolReader = coolReader;
		mReaderView = readerView;
		View anchor = readerView.getSurface();
		mTTS = tts;
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
		mSbSpeed.setProgress(mCoolReader.getTTSSpeed());
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
				mCoolReader.setTTSSpeed(progress);
			}

			@Override
			public void onStartTrackingTouch(SeekBar seekBar) {
			}

			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
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

		// set language for TTS based on book's language
		BookInfo bookInfo = mReaderView.getBookInfo();
		if (null != bookInfo) {
			FileInfo fileInfo = bookInfo.getFileInfo();
			if (null != fileInfo) {
				mBookTitle = fileInfo.title;
				log.d("book language is \"" + fileInfo.language + "\"");
				if (null != fileInfo.language && fileInfo.language.length() > 0) {
					Locale locale = new Locale(fileInfo.language);
					log.d("trying to set TTS language to \"" + locale.getDisplayLanguage() + "\"");
					mTTS.setLanguage(locale);
				}
			}
		}

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
