package org.coolreader.crengine;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.drawable.BitmapDrawable;
import android.os.Build;
import android.os.Bundle;
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
import org.coolreader.tts.TTS;
import org.coolreader.tts.TTSControlBinder;
import org.coolreader.tts.TTSControlService;
import org.coolreader.tts.TTSControlServiceAccessor;

import java.util.HashMap;
import java.util.Locale;

public class TTSToolbarDlg implements TTS.OnUtteranceCompletedListener {
	public static final Logger log = L.create("ttssrv");

	PopupWindow mWindow;
	View mAnchor;
	CoolReader mCoolReader;
	ReaderView mReaderView;
	String mBookTitle;
	View mPanel;
	TTS mTTS;
	TTSControlServiceAccessor mTTSControl;
	ImageButton playPauseButton;
	SeekBar sbSpeed;
	SeekBar sbVolume;
	private HandlerThread mMotionWatchdog;
	private boolean changedPageMode;

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

	static public TTSToolbarDlg showDialog( CoolReader coolReader, ReaderView readerView, TTS tts)
	{
		TTSToolbarDlg dlg = new TTSToolbarDlg(coolReader, readerView, tts);
		//dlg.mWindow.update(dlg.mAnchor, width, height)
		log.d("popup: " + dlg.mWindow.getWidth() + "x" + dlg.mWindow.getHeight());
		//dlg.update();
		//dlg.showAtLocation(readerView, Gravity.LEFT|Gravity.TOP, readerView.getLeft()+50, readerView.getTop()+50);
		//dlg.showAsDropDown(readerView);
		//dlg.update();
		return dlg;
	}
	
	private Runnable onCloseListener;
	public void setOnCloseListener(Runnable handler) {
		onCloseListener = handler;
	}

	private boolean closed; 
	public void stopAndClose() {
		if (closed)
			return;
		isSpeaking = false;
		closed = true;
		BackgroundThread.instance().executeGUI(() -> {
			stop();
			mCoolReader.unregisterReceiver(mTTSControlButtonReceiver);
			if (null != mTTSControl)
				mTTSControl.unbind();
			Intent intent = new Intent(mCoolReader, TTSControlService.class);
			mCoolReader.stopService(intent);
			restoreReaderMode();
			mReaderView.clearSelection();
			if (onCloseListener != null)
				onCloseListener.run();
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
	
	private Selection currentSelection;
	
	private void moveSelection( ReaderCommand cmd )
	{
		mReaderView.moveSelection(cmd, 0, new ReaderView.MoveSelectionCallback() {
			
			@Override
			public void onNewSelection(Selection selection) {
				log.d("onNewSelection: " + selection.text);
				currentSelection = selection;
				if ( isSpeaking )
					say( currentSelection );
			}
			
			@Override
			public void onFail() {
				log.e("fail()");
				stop();
				//currentSelection = null;
			}
		});
	}
	
	private void say( Selection selection ) {
		HashMap<String, String> params = new HashMap<String, String>();
		params.put(TTS.KEY_PARAM_UTTERANCE_ID, "cr3UtteranceId");
		mTTS.speak(selection.text, TTS.QUEUE_ADD, params);
		runInTTSControlService(tts -> tts.notifyPlay(mBookTitle, selection.text));
	}
	
	private void start() {
		if ( currentSelection==null )
			return;
		startMotionWatchdog();
		isSpeaking = true;
		say( currentSelection );
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
	
	private boolean isSpeaking;
	private Runnable mOnStopRunnable;

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
			playPauseButton.setImageResource(R.drawable.ic_media_play);
			runInTTSControlService(tts -> tts.notifyPause(mBookTitle));
			stop();
		} else {
			if (null != currentSelection) {
				playPauseButton.setImageResource(R.drawable.ic_media_pause);
				runInTTSControlService(tts -> tts.notifyPlay(mBookTitle, currentSelection.text));
				start();
			}
		}
	}
	
	@Override
	public void onUtteranceCompleted(String utteranceId) {
		if (null != mOnStopRunnable) {
			mOnStopRunnable.run();
			mOnStopRunnable = null;
		} else {
			if ( isSpeaking )
				moveSelection( ReaderCommand.DCMD_SELECT_NEXT_SENTENCE );
		}
	}

	private void runInTTSControlService(TTSControlBinder.Callback callback) {
		if (null == mTTSControl) {
			mTTSControl = new TTSControlServiceAccessor(mCoolReader);
		}
		mTTSControl.bind(callback);
	}

	public TTSToolbarDlg( CoolReader coolReader, ReaderView readerView, TTS tts )
	{
		mCoolReader = coolReader;
		mReaderView = readerView;
		mAnchor = readerView.getSurface();
		mTTS = tts;
		mTTS.setOnUtteranceCompletedListener(this);

		View panel = (LayoutInflater.from(coolReader.getApplicationContext()).inflate(R.layout.tts_toolbar, null));
		playPauseButton = panel.findViewById(R.id.tts_play_pause);
		playPauseButton.setImageResource(R.drawable.ic_media_play);
		//panel.measure(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
		panel.measure(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
		
		//mReaderView.getS
		
		mWindow = new PopupWindow( mAnchor.getContext() );
//		mWindow.setFocusable(true);
//		mWindow.setTouchable(true);
//		mWindow.setOutsideTouchable(true);
		mWindow.setBackgroundDrawable(new BitmapDrawable());
//		mWindow.setTouchInterceptor(new OnTouchListener() {
//			@Override
//			public boolean onTouch(View v, MotionEvent event) {
////				if ( event.getAction()==MotionEvent.ACTION_OUTSIDE ) {
////					stopAndClose();
////					return true;
////				}
//				return true;
//			}
//		});
		//super(panel);
		mPanel = panel;
		mPanel.findViewById(R.id.tts_play_pause).setOnClickListener(v -> toggleStartStop());
		mPanel.findViewById(R.id.tts_back).setOnClickListener(v -> {
			if ( isSpeaking ) {
				stop(() -> {
					isSpeaking = true;
					moveSelection( ReaderCommand.DCMD_SELECT_PREV_SENTENCE );
				});
			} else
				moveSelection( ReaderCommand.DCMD_SELECT_PREV_SENTENCE );
		});
		mPanel.findViewById(R.id.tts_forward).setOnClickListener(v -> {
			if ( isSpeaking ) {
				stop(() -> {
					isSpeaking = true;
					moveSelection( ReaderCommand.DCMD_SELECT_NEXT_SENTENCE );
				});
			} else
				moveSelection( ReaderCommand.DCMD_SELECT_NEXT_SENTENCE );
		});
		mPanel.findViewById(R.id.tts_stop).setOnClickListener(v -> stopAndClose());
		mPanel.setFocusable(true);
		mPanel.setEnabled(true);
		mPanel.setOnKeyListener((v, keyCode, event) -> {
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
					int p = sbVolume.getProgress() - 5;
					if ( p<0 )
						p = 0;
					sbVolume.setProgress(p);
					return true;
				}
				case KeyEvent.KEYCODE_VOLUME_UP:
					int p = sbVolume.getProgress() + 5;
					if ( p>100 )
						p = 100;
					sbVolume.setProgress(p);
					return true;
				}
				if ( keyCode == KeyEvent.KEYCODE_BACK) {
					return true;
				}
			}
			return false;
		});

		mWindow.setOnDismissListener(() -> {
			if ( !closed )
				stopAndClose();
		});
		
		mWindow.setBackgroundDrawable(new BitmapDrawable());
		//mWindow.setAnimationStyle(android.R.style.Animation_Toast);
		//mWindow.setWidth(WindowManager.LayoutParams.WRAP_CONTENT);
		mWindow.setWidth(WindowManager.LayoutParams.FILL_PARENT);
		mWindow.setHeight(WindowManager.LayoutParams.WRAP_CONTENT);
//		setWidth(panel.getWidth());
//		setHeight(panel.getHeight());
		
		mWindow.setFocusable(true);
		mWindow.setTouchable(true);
		mWindow.setOutsideTouchable(true);
		mWindow.setContentView(panel);
		
		
		int [] location = new int[2];
		mAnchor.getLocationOnScreen(location);
		//mWindow.update(location[0], location[1], mPanel.getWidth(), mPanel.getHeight() );
		//mWindow.setWidth(mPanel.getWidth());
		//mWindow.setHeight(mPanel.getHeight());

		mWindow.showAtLocation(mAnchor, Gravity.TOP | Gravity.CENTER_HORIZONTAL, location[0], location[1] + mAnchor.getHeight() - mPanel.getHeight());
//		if ( mWindow.isShowing() )
//			mWindow.update(mAnchor, 50, 50);
		//dlg.mWindow.showAsDropDown(dlg.mAnchor);
		
		setReaderMode();

		// setup speed && volume seek bars
		sbSpeed = mPanel.findViewById(R.id.tts_sb_speed);
		sbVolume = mPanel.findViewById(R.id.tts_sb_volume);
		
		sbSpeed.setMax(100);
		sbSpeed.setProgress(50);
		sbVolume.setMax(100);
		sbVolume.setProgress(mCoolReader.getVolume());
		sbSpeed.setOnSeekBarChangeListener( new OnSeekBarChangeListener() {
			@Override
			public void onProgressChanged(SeekBar seekBar, int progress,
					boolean fromUser) {
				float rate = 1.0f;
				if ( progress<50 )
					rate = 0.3f + 0.7f * progress / 50f;
				else
					rate = 1.0f + 2.5f * (progress-50) / 50f;
				mTTS.setSpeechRate(rate);
			}

			@Override
			public void onStartTrackingTouch(SeekBar seekBar) {
			}

			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
			}
		});

		sbVolume.setOnSeekBarChangeListener( new OnSeekBarChangeListener() {
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

		mPanel.requestFocus();

		BookInfo bookInfo = mReaderView.getBookInfo();
		if (null != bookInfo) {
			FileInfo fileInfo = bookInfo.getFileInfo();
			if (null != fileInfo) {
				mBookTitle = fileInfo.title;
				// set language for TTS based on book's language
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
	}

}
