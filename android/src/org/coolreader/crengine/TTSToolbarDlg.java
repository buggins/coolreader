/*
 * CoolReader for Android
 * Copyright (C) 2010-2014 Vadim Lopatin <coolreader.org@gmail.com>
 * Copyright (C) 2018 S-trace <S-trace@list.ru>
 * Copyright (C) 2020-2022 Aleksey Chernov <valexlin@gmail.com>
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

package org.coolreader.crengine;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.HandlerThread;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import com.s_trace.motion_watchdog.MotionWatchdogHandler;

import org.coolreader.CoolReader;
import org.coolreader.R;
import org.coolreader.tts.OnTTSStatusListener;
import org.coolreader.tts.TTSControlBinder;
import org.coolreader.tts.TTSControlService;
import org.coolreader.tts.TTSControlServiceAccessor;

import java.util.Locale;
import java.util.Map;

public class TTSToolbarDlg implements Settings {
	public static final Logger log = L.create("ttsdlg");

	public static final int MEDIA_COVER_WIDTH = 300;
	public static final int MEDIA_COVER_HEIGHT = 400;

	private final PopupWindow mWindow;
	private final CoolReader mCoolReader;
	private final ReaderView mReaderView;
	private final LinearLayout glassPanel;
	private final LinearLayout toolbarBody;
	private final TTSControlServiceAccessor mTTSControl;
	private final ImageButton mPlayPauseButton;
	private final TextView mVolumeTextView;
	private final TextView mSpeedTextView;
	private final SeekBar mSbSpeed;
	private final SeekBar mSbVolume;
	private HandlerThread mMotionWatchdog;
	private boolean changedPageMode;
	private Runnable mOnCloseListener;
	private boolean mClosed;
	private Selection mCurrentSelection;
	private boolean isSpeaking;
	private boolean isToolbarHidden;
	private int mMotionTimeout;
	private boolean mAutoSetDocLang;
	private String mBookAuthors;
	private String mBookTitle;
	private Bitmap mBookCover;
	private String mBookLanguage;
	private String mForcedLanguage;
	private String mForcedVoice;
	private String mCurrentLanguage;
	private String mCurrentVoiceName;
	private boolean mGoogleTTSAbbreviationWorkaround;
	private int mTTSSpeedPercent = 50;		// 50% (normal)


	static public TTSToolbarDlg showDialog( CoolReader coolReader, ReaderView readerView, TTSControlServiceAccessor ttsacc) {
		TTSToolbarDlg dlg = new TTSToolbarDlg(coolReader, readerView, ttsacc);
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
		mTTSControl.bind(ttsbinder -> {
			ttsbinder.stop(result -> {
				BackgroundThread.instance().postGUI(() -> {
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
			});
		});
	}

	public void pause() {
		mTTSControl.bind(ttsbinder -> {
			ttsbinder.pause(null);
		});
	}

	private void setReaderMode() {
		String oldViewSetting = mReaderView.getSetting( ReaderView.PROP_PAGE_VIEW_MODE );
		if ( "1".equals(oldViewSetting) ) {
			changedPageMode = true;
			mReaderView.setViewModeNonPermanent(ViewMode.SCROLL);
		}
		moveSelection(ReaderCommand.DCMD_SELECT_FIRST_SENTENCE, null);
	}

	private void restoreReaderMode() {
		if ( changedPageMode ) {
			mReaderView.setViewModeNonPermanent(ViewMode.PAGES);
		}
	}

	/**
	 * Select next or previous sentence. ONLY the selection changes and the specified callback is called!
	 * Not affected to speech synthesis process.
	 * @param cmd move command. DCMD_SELECT_NEXT_SENTENCE, DCMD_SELECT_PREV_SENTENCE, DCMD_SELECT_FIRST_SENTENCE.
	 * @param callback optional completion callback
	 */
	private void moveSelection( ReaderCommand cmd, ReaderView.MoveSelectionCallback callback )
	{
		mReaderView.moveSelection(cmd, 0, new ReaderView.MoveSelectionCallback() {

			@Override
			public void onNewSelection(Selection selection) {
				log.d("onNewSelection: " + selection.text);
				mCurrentSelection = selection;
				if (null != callback)
					callback.onNewSelection(mCurrentSelection);
			}

			@Override
			public void onFail() {
				log.e("fail()");
				if (isSpeaking) {
					mTTSControl.bind(ttsbinder ->
							ttsbinder.stop(result ->
									log.e("speech synthesis process stopped!")));
				}
				if (null != callback)
					callback.onFail();
			}
		});
	}

	private String preprocessUtterance(String utterance) {
		String newUtterance = utterance;
		if (mGoogleTTSAbbreviationWorkaround) {
			// Add space before last char if it's dot.
			int len = newUtterance.length();
			if (len > 1) {
				if (newUtterance.charAt(len - 1) == '.') {
					newUtterance = newUtterance.substring(0, len - 1);
					newUtterance += " .";
				}
			}
		}
		return newUtterance;
	}

	@TargetApi(Build.VERSION_CODES.ECLAIR)
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
		int oldTTSSpeed = mTTSSpeedPercent;
		Properties changedSettings = newSettings.diff(oldSettings);
		for (Map.Entry<Object, Object> entry : changedSettings.entrySet()) {
			String key = (String) entry.getKey();
			String value = (String) entry.getValue();
			processAppSetting(key, value);
		}
		// Apply settings
		setupTTSVoice();
		if (oldTTSSpeed != mTTSSpeedPercent) {
			mTTSControl.bind(ttsbinder -> {
				ttsbinder.setSpeechRate(speechRateFromPercent(mTTSSpeedPercent), result -> {
					if (result)
						BackgroundThread.instance().postGUI(() -> mSbSpeed.setProgress(mTTSSpeedPercent));
				});
			});
		}
	}

	private void processAppSetting(String key, String value) {
		boolean flg = "1".equals(value);
		switch (key) {
			case PROP_APP_MOTION_TIMEOUT:
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ECLAIR) {
					mMotionTimeout = Utils.parseInt(value, 0, 0, 100);
					mMotionTimeout = mMotionTimeout * 60 * 1000; // Convert minutes to msecs
				}
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
			case PROP_APP_TTS_GOOGLE_END_OF_SENTENCE_ABBR:
				mGoogleTTSAbbreviationWorkaround = flg;
		}
	}

	private void setupTTSVoice() {
		if (mAutoSetDocLang) {
			// set language for TTS based on book's language
			if (null != mBookLanguage && mBookLanguage.length() > 0 && !mBookLanguage.equals(mCurrentLanguage)) {
				log.d("trying to set TTS language to \"" + mBookLanguage + "\"");
				mTTSControl.bind(ttsbinder -> {
					ttsbinder.setLanguage(mBookLanguage, result -> {
						mCurrentLanguage = mBookLanguage;
						if (result)
							log.d("setting TTS language to \"" + mBookLanguage + "\" successful.");
						else
							log.d("Failed to set TTS language to \"" + mBookLanguage + "\".");
					});
				});
			} else {
				log.e("Failed to detect book's language, will be used system default!");
			}
		} else {
			if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP) {
				if (null != mForcedVoice && mForcedVoice.length() > 0 && !mForcedVoice.equals(mCurrentVoiceName)) {
					mTTSControl.bind(ttsbinder -> {
						ttsbinder.setVoice(mForcedVoice, result -> {
							mCurrentVoiceName = mForcedVoice;
							if (result) {
								log.d("Set voice \"" + mForcedVoice + "\" successful");
							} else {
								log.e("Failed to set voice \"" + mForcedVoice + "\"!");
							}
						});
					});
				}
			}
		}
	}

	private void setupSpeechStatusHandler() {
		mTTSControl.bind(ttsbinder -> {
			ttsbinder.setStatusListener(new OnTTSStatusListener() {
				@Override
				public void onUtteranceStart() {
					isSpeaking = true;
				}

				@Override
				public void onUtteranceDone() {
				}

				@Override
				public void onError(int errorCode) {
					BackgroundThread.instance().postGUI(() -> mCoolReader.showToast(R.string.tts_failed));
				}

				@Override
				public void onStateChanged(TTSControlService.State state) {
					switch (state) {
						case PLAYING:
							isSpeaking = true;
							BackgroundThread.instance().postGUI(() -> mPlayPauseButton.setImageResource(Utils.resolveResourceIdByAttr(mCoolReader, R.attr.ic_media_pause_drawable, R.drawable.ic_media_pause)));
							if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ECLAIR && mMotionTimeout > 0)
								startMotionWatchdog();
							break;
						case PAUSED:
						case STOPPED:
							isSpeaking = false;
							BackgroundThread.instance().postGUI(() -> mPlayPauseButton.setImageResource(Utils.resolveResourceIdByAttr(mCoolReader, R.attr.ic_media_play_drawable, R.drawable.ic_media_play)));
							if (mMotionWatchdog != null)
								mMotionWatchdog.interrupt();
							break;
					}
				}

				@Override
				public void onVolumeChanged(int currentVolume, int maxVolume) {
					BackgroundThread.instance().postGUI(() -> {
						mSbVolume.setMax(maxVolume);
						mSbVolume.setProgress(currentVolume);
					});
				}

				@Override
				public void onAudioFocusLost() {
				}

				@Override
				public void onAudioFocusRestored() {
				}

				@Override
				public void onCurrentSentenceRequested(TTSControlBinder ttsbinder) {
					if (null != mCurrentSelection) {
						ttsbinder.say(preprocessUtterance(mCurrentSelection.text), null);
					}
				}

				@Override
				public void onNextSentenceRequested(TTSControlBinder ttsbinder) {
					if (isSpeaking) {
						moveSelection(ReaderCommand.DCMD_SELECT_NEXT_SENTENCE, new ReaderView.MoveSelectionCallback() {
							@Override
							public void onNewSelection(Selection selection) {
								ttsbinder.say(preprocessUtterance(selection.text), null);
							}

							@Override
							public void onFail() {
							}
						});
					} else {
						moveSelection(ReaderCommand.DCMD_SELECT_NEXT_SENTENCE, null);
					}
				}

				@Override
				public void onPreviousSentenceRequested(TTSControlBinder ttsbinder) {
					if (isSpeaking) {
						moveSelection(ReaderCommand.DCMD_SELECT_PREV_SENTENCE, new ReaderView.MoveSelectionCallback() {
							@Override
							public void onNewSelection(Selection selection) {
								ttsbinder.say(preprocessUtterance(selection.text), null);
							}

							@Override
							public void onFail() {
							}
						});
					} else {
						moveSelection(ReaderCommand.DCMD_SELECT_PREV_SENTENCE, null);
					}
				}

				@Override
				public void onStopRequested(TTSControlBinder ttsbinder) {
					stopAndClose();
				}
			});
		});
	}

	@SuppressLint("ClickableViewAccessibility")
	public TTSToolbarDlg(CoolReader coolReader, ReaderView readerView, TTSControlServiceAccessor ttsacc) {
		mCoolReader = coolReader;
		mReaderView = readerView;
		mTTSControl = ttsacc;
		View anchor = readerView.getSurface();

		//Context context = mCoolReader.getApplicationContext();
		Context context = anchor.getContext();
		LayoutInflater inflater = LayoutInflater.from(context);
		View panel = inflater.inflate(R.layout.tts_toolbar, null);

		glassPanel = panel.findViewById(R.id.tts_glass_panel);
		toolbarBody = panel.findViewById(R.id.tts_toolbar_body);

                glassPanel.setOnClickListener(v -> {
                    isToolbarHidden = !isToolbarHidden;
                    toolbarBody.setVisibility(isToolbarHidden ? View.INVISIBLE: View.VISIBLE);
                });

		mPlayPauseButton = panel.findViewById(R.id.tts_play_pause);
		mPlayPauseButton.setImageResource(Utils.resolveResourceIdByAttr(mCoolReader, R.attr.ic_media_play_drawable, R.drawable.ic_media_play));
		ImageButton backButton = panel.findViewById(R.id.tts_back);
		ImageButton forwardButton = panel.findViewById(R.id.tts_forward);
		ImageButton stopButton = panel.findViewById(R.id.tts_stop);
		ImageButton optionsButton = panel.findViewById(R.id.tts_options);

		mWindow = new PopupWindow( context );
		mWindow.setBackgroundDrawable(new BitmapDrawable());
		mPlayPauseButton.setOnClickListener(
				v -> mCoolReader.sendBroadcast(new Intent(TTSControlService.TTS_CONTROL_ACTION_PLAY_PAUSE)));
		backButton.setOnClickListener(
				v -> mCoolReader.sendBroadcast(new Intent(TTSControlService.TTS_CONTROL_ACTION_PREV)));
		forwardButton.setOnClickListener(
				v -> mCoolReader.sendBroadcast(new Intent(TTSControlService.TTS_CONTROL_ACTION_NEXT)));
		optionsButton.setOnClickListener(v -> mTTSControl.bind(ttsbinder -> {
			OptionsDialog dlg = new OptionsDialog(mCoolReader, OptionsDialog.Mode.TTS, null, null, ttsbinder);
			dlg.show();
		}));
		stopButton.setOnClickListener(v -> stopAndClose());

		// setup speed && volume seek bars
		mVolumeTextView = panel.findViewById(R.id.tts_lbl_volume);
		mSpeedTextView = panel.findViewById(R.id.tts_lbl_speed);
		mSpeedTextView.setText(String.format(Locale.getDefault(), "%s (x%.2f)", context.getString(R.string.tts_rate), speechRateFromPercent(50)));

		mSbSpeed = panel.findViewById(R.id.tts_sb_speed);
		mSbVolume = panel.findViewById(R.id.tts_sb_volume);

		mSbSpeed.setMax(100);
		mSbSpeed.setProgress(50);
		mSbVolume.setMax(100);
		mSbVolume.setProgress(0);
		mSbSpeed.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
			int mProgress;
			@Override
			public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
				mProgress = progress;
				float rate = speechRateFromPercent(progress);
				mSpeedTextView.setText(String.format(Locale.getDefault(), "%s (x%.2f)", context.getString(R.string.tts_rate), rate));
			}

			@Override
			public void onStartTrackingTouch(SeekBar seekBar) {
			}

			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
				mCoolReader.setSetting(PROP_APP_TTS_SPEED, String.valueOf(mProgress), true);
			}
		});
		ImageButton btnDecVolume = panel.findViewById(R.id.btn_dec_volume);
		btnDecVolume.setOnTouchListener(new RepeatOnTouchListener(500, 150,
				view -> mSbVolume.setProgress(mSbVolume.getProgress() - 1)));
		ImageButton btnIncVolume = panel.findViewById(R.id.btn_inc_volume);
		btnIncVolume.setOnTouchListener(new RepeatOnTouchListener(500, 150, view -> mSbVolume.setProgress(mSbVolume.getProgress() + 1)));

		ImageButton btnDecSpeed = panel.findViewById(R.id.btn_dec_speed);
		btnDecSpeed.setOnTouchListener(new RepeatOnTouchListener(500, 150, view -> {
			mSbSpeed.setProgress(mSbSpeed.getProgress() - 1);
			mCoolReader.setSetting(PROP_APP_TTS_SPEED, String.valueOf(mSbSpeed.getProgress()), true);
		}));
		ImageButton btnIncSpeed = panel.findViewById(R.id.btn_inc_speed);
		btnIncSpeed.setOnTouchListener(new RepeatOnTouchListener(500, 150, view -> {
			mSbSpeed.setProgress(mSbSpeed.getProgress() + 1);
			mCoolReader.setSetting(PROP_APP_TTS_SPEED, String.valueOf(mSbSpeed.getProgress()), true);
		}));

		panel.measure(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
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
					int p = mSbVolume.getProgress() - 1;
					if ( p<0 )
						p = 0;
					mSbVolume.setProgress(p);
					return true;
				}
				case KeyEvent.KEYCODE_VOLUME_UP:
					int p = mSbVolume.getProgress() + 1;
					if ( p > mSbVolume.getMax() )
						p = mSbVolume.getMax();
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
		mWindow.setHeight(WindowManager.LayoutParams.FILL_PARENT);
		mWindow.setFocusable(true);
		mWindow.setTouchable(true);
		mWindow.setOutsideTouchable(true);
		mWindow.setContentView(panel);

		int [] location = new int[2];
		anchor.getLocationOnScreen(location);

		mWindow.showAtLocation(anchor, Gravity.TOP | Gravity.CENTER_HORIZONTAL, location[0], location[1] + anchor.getHeight() - panel.getHeight());

		setReaderMode();

		if (null == mBookTitle)
			mBookTitle = "";
		if (null == mBookAuthors)
			mBookAuthors = "";
		if (null == mBookLanguage) {
			log.e("Failed to detect book's language!");
		}

		// Start the foreground service to make this app also foreground,
		// even if the main activity is in the background.
		// https://developer.android.com/about/versions/oreo/background#services
		Intent intent = new Intent(TTSControlService.TTS_CONTROL_ACTION_PREPARE, Uri.EMPTY, coolReader, TTSControlService.class);
		Bundle data = new Bundle();
		data.putString("bookAuthors", mBookAuthors);
		data.putString("bookTitle", mBookTitle);
		intent.putExtras(data);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
			coolReader.startForegroundService(intent);
		else
			coolReader.startService(intent);

		panel.requestFocus();

		// All tasks bellow after service start
		// Fetch book's metadata
		BookInfo bookInfo = mReaderView.getBookInfo();
		if (null != bookInfo) {
			FileInfo fileInfo = bookInfo.getFileInfo();
			if (null != fileInfo) {
				mBookAuthors = fileInfo.authors;
				mBookTitle = fileInfo.title;
				mBookLanguage = fileInfo.language;
				mBookCover = Bitmap.createBitmap(MEDIA_COVER_WIDTH, MEDIA_COVER_HEIGHT, Bitmap.Config.RGB_565);
				Services.getCoverpageManager().drawCoverpageFor(mCoolReader.getDB(), fileInfo, mBookCover, true,
						(file, bitmap) -> mTTSControl.bind(ttsbinder -> ttsbinder.setMediaItemInfo(mBookAuthors, mBookTitle, bitmap)));
			}
		}
		// Show volume
		mTTSControl.bind(ttsbinder -> ttsbinder.retrieveVolume((current, max) -> {
			mSbVolume.setMax(max);
			mSbVolume.setProgress(current);
		}));
		mSbVolume.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
			@Override
			public void onProgressChanged(SeekBar seekBar, int progress,
										  boolean fromUser) {
				if (mSbVolume.getMax() < 1)
					return;
				mTTSControl.bind(ttsbinder -> ttsbinder.setVolume(progress));
				mVolumeTextView.setText(String.format(Locale.getDefault(), "%s (%d%%)", context.getString(R.string.tts_volume), 100*progress/seekBar.getMax()));
			}

			@Override
			public void onStartTrackingTouch(SeekBar seekBar) {
			}

			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
			}
		});
		// And finally, setup status change handler
		setupSpeechStatusHandler();
	}
}
