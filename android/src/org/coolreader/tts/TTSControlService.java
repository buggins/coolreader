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

import android.annotation.TargetApi;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ServiceInfo;
import android.database.ContentObserver;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.Icon;
import android.media.AudioAttributes;
import android.media.AudioFocusRequest;
import android.media.AudioManager;
import android.media.MediaMetadata;
import android.media.MediaPlayer;
import android.media.session.MediaSession;
import android.media.session.PlaybackState;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.speech.tts.TextToSpeech;
import android.speech.tts.UtteranceProgressListener;
import android.speech.tts.Voice;

import org.coolreader.CoolReader;
import org.coolreader.R;
import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;
import org.coolreader.db.BaseService;
import org.coolreader.db.Task;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.Timer;
import java.util.TimerTask;

/**
 * This service implement TTS functionality.
 * This service keep the application in the foreground (for the target API >= 26),
 * even if the main activity is in the background, so that the system understands that the application
 * does not need to be unloaded from memory while TTS is running.
 * It also adds TTS control buttons to the notification area and lock screen.
 * It also declare media session and control audio focus.
 */
public class TTSControlService extends BaseService {

	public enum State {
		PLAYING,
		PAUSED,
		STOPPED
	}

	public static final Logger log = L.create("ttssvc");

	private static final String CR3_UTTERANCE_ID = "cr3UtteranceId";
	private static final int MAX_CONTINUOUS_ERRORS = 3;
	private static final long INIT_TTS_TIMEOUT = 10000;		// 10 sec.
	private static final int NOTIFICATION_ID = 1;
	private static final String NOTIFICATION_CHANNEL_ID = "CoolReader TTS C9";

	public static final String TTS_CONTROL_ACTION_PREPARE = "org.coolreader.tts.prepare";
	public static final String TTS_CONTROL_ACTION_PLAY_PAUSE = "org.coolreader.tts.tts_play_pause";
	public static final String TTS_CONTROL_ACTION_NEXT = "org.coolreader.tts.tts_next";
	public static final String TTS_CONTROL_ACTION_PREV = "org.coolreader.tts.tts_prev";
	public static final String TTS_CONTROL_ACTION_STOP = "org.coolreader.tts.tts_stop";

	private boolean mChannelCreated = false;
	private final TTSControlBinder mBinder = new TTSControlBinder(this);
	private NotificationManager mNotificationManager = null;

	private TextToSpeech mTTS;
	private boolean mTTSInitialized = false;
	private String mTTSEnginePackage;
	private Timer mInitTTSTimer;
	private String mAuthors;
	private String mTitle;
	private String mCurrentUtterance;
	private Bitmap mCoverBitmap;
	private Bundle mTTSParamsBundle = null;			// for API21+
	private HashMap<String, String> mTTSParamsMap;	// for API<21
	private Locale mLocale;
	private Voice mVoice;
	private Runnable mOnUtteranceStopOnce = null;
	private OnTTSStatusListener mStatusListener;
	private State mState = State.STOPPED;
	private State mPrevState = State.STOPPED;
	private float mSpeechRate;
	private int mContinuousErrors = 0;
	private AudioManager mAudioManager = null;
	private AudioFocusRequest mAudioFocusRequest = null;
	private boolean mPlaybackNowAuthorized = false;
	private boolean mPlaybackDelayed = false;
	private boolean mResumeOnFocusGain = false;
	private MediaSession mMediaSession;
	private MediaSession.Callback mMediaSessionCallback;
	private PlaybackState.Builder mPlaybackStateBuilder;
	private MediaPlayer mMediaPlayer;
	private VolumeSettingsContentObserver mVolumeSettingsContentObserver;
	private final Object mLocker = new Object();

	final private BroadcastReceiver mTTSControlActionReceiver = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			log.d("received action: " + action);
			if (null != action) {
				switch (action) {
					case TTSControlService.TTS_CONTROL_ACTION_PLAY_PAUSE:
						if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
							if (State.PLAYING == mState)
								mMediaSessionCallback.onPause();
							else
								mMediaSessionCallback.onPlay();
						} else {
							if (State.PLAYING == mState)
								pauseWrapper_api_less_than_21();
							else
								playWrapper_api_less_than_21();
						}
						break;
					case TTSControlService.TTS_CONTROL_ACTION_NEXT:
						if (State.PLAYING == mState) {
							stopUtterance_impl(() -> {
								if (null != mStatusListener)
									mStatusListener.onNextSentenceRequested(mBinder);
							});
						} else {
							if (null != mStatusListener)
								mStatusListener.onNextSentenceRequested(mBinder);
						}
						break;
					case TTSControlService.TTS_CONTROL_ACTION_PREV:
						if (State.PLAYING == mState) {
							stopUtterance_impl(() -> {
								if (null != mStatusListener)
									mStatusListener.onPreviousSentenceRequested(mBinder);
							});
						} else {
							if (null != mStatusListener)
								mStatusListener.onPreviousSentenceRequested(mBinder);
						}
						break;
					case TTSControlService.TTS_CONTROL_ACTION_STOP:
						if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
							mMediaSessionCallback.onStop();
						else
							stopWrapper_api_less_than_21();
						if (null != mStatusListener)
							mStatusListener.onStopRequested(mBinder);
						break;
				}
			}
		}
	};

	final private BroadcastReceiver mBecomingNoisyReceiver = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
			// See https://developer.android.com/reference/android/media/AudioManager#ACTION_AUDIO_BECOMING_NOISY
			log.d("audio is about to become 'noisy' due to a change in audio outputs");
			if (AudioManager.ACTION_AUDIO_BECOMING_NOISY.equals(intent.getAction())) {
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
					mMediaSessionCallback.onPause();
				else
					pauseWrapper_api_less_than_21();
			}
		}
	};

	final private AudioManager.OnAudioFocusChangeListener mAudioFocusChangeListener = focusChange -> {
		switch (focusChange) {
			case AudioManager.AUDIOFOCUS_GAIN:
				log.d("audio focus gain");
				mPlaybackNowAuthorized = true;
				if (mPlaybackDelayed || mResumeOnFocusGain) {
					mPlaybackDelayed = false;
					mResumeOnFocusGain = false;
					if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
						mMediaSessionCallback.onPlay();
					else
						playWrapper_api_less_than_21();
					if (null != mStatusListener)
						mStatusListener.onAudioFocusRestored();
				}
				break;
			case AudioManager.AUDIOFOCUS_LOSS:
				log.d("audio focus is lost");
				mPlaybackNowAuthorized = false;
				mResumeOnFocusGain = false;
				mPlaybackDelayed = false;
				if (State.PLAYING == mState) {
					if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
						mMediaSessionCallback.onPause();
					} else {
						// Fallback for API<21, direct control
						pauseWrapper_api_less_than_21();
					}
					if (null != mStatusListener)
						mStatusListener.onAudioFocusLost();
				}
				break;
			case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
			case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
				log.d("audio focus temporary lost");
				// set to pause, and schedule to resume when focus gain again
				mPlaybackNowAuthorized = false;
				mResumeOnFocusGain = false;
				mPlaybackDelayed = false;
				if (State.PLAYING == mState) {
					if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
						mMediaSessionCallback.onPause();
					else
						pauseWrapper_api_less_than_21();
					mResumeOnFocusGain = true;
				}
				break;
		}
	};

	/**
	 * See https://stackoverflow.com/a/17398781 for notes
	 */
	public class VolumeSettingsContentObserver extends ContentObserver {
		int mPreviousVolume;
		int mMaxVolume;

		public VolumeSettingsContentObserver(Handler handler) {
			super(handler);
			mPreviousVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
			mMaxVolume = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
		}

		@Override
		public boolean deliverSelfNotifications() {
			return super.deliverSelfNotifications();
		}

		@Override
		public void onChange(boolean selfChange) {
			super.onChange(selfChange);
			mMaxVolume = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
			int currentVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
			if (currentVolume != mPreviousVolume) {
				if (null != mStatusListener)
					mStatusListener.onVolumeChanged(currentVolume, mMaxVolume);
			}
		}
	}

	public TTSControlService() {
		super("tts");
	}

	@Override
	public void onCreate() {
		log.d("onCreate");
		super.onCreate();
		mState = State.STOPPED;
		mNotificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
		mAudioManager = (AudioManager) getSystemService(AUDIO_SERVICE);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
				AudioAttributes.Builder aabuilder = new AudioAttributes.Builder();
				aabuilder = aabuilder.setUsage(AudioAttributes.USAGE_MEDIA);
				aabuilder = aabuilder.setContentType(AudioAttributes.CONTENT_TYPE_SPEECH);
				AudioFocusRequest.Builder afbuilder = new AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN);
				afbuilder = afbuilder.setAudioAttributes(aabuilder.build());
				afbuilder = afbuilder.setAcceptsDelayedFocusGain(false);
				afbuilder = afbuilder.setWillPauseWhenDucked(true);
				afbuilder = afbuilder.setOnAudioFocusChangeListener(mAudioFocusChangeListener);
				mAudioFocusRequest = afbuilder.build();
			}
			mMediaSessionCallback = new MediaSession.Callback() {
				/*
				@Override
				public boolean onMediaButtonEvent (Intent mediaButtonIntent) {
					log.e("onMediaButtonEvent: " + mediaButtonIntent);
					Bundle extras = mediaButtonIntent.getExtras();
					if (null != extras) {
						KeyEvent event = mediaButtonIntent.getParcelableExtra(Intent.EXTRA_KEY_EVENT);
						log.e("onMediaButtonEvent: event=" + event);
					}
					return super.onMediaButtonEvent(mediaButtonIntent);
				}
				*/

				@Override
				public void onPlay() {
					if (null == mCurrentUtterance) {
						if (null != mStatusListener)
							mStatusListener.onCurrentSentenceRequested(mBinder);
						return;
					}
					// check or request audio focus
					if (!mPlaybackNowAuthorized)
						requestAudioFocusWrapper();
					if (!mPlaybackNowAuthorized) {
						log.e("Can't say anything, audio focus is locked by other app.");
						return;
					}
					// start to speech
					if (say_impl(mCurrentUtterance)) {
						// update media session
						MediaMetadata.Builder builder = new MediaMetadata.Builder();
						if (null != mAuthors && mAuthors.length() > 0) {
							builder = builder.putString(MediaMetadata.METADATA_KEY_AUTHOR, mAuthors);
							builder = builder.putString(MediaMetadata.METADATA_KEY_ARTIST, mAuthors);
						}
						if (null != mTitle && mTitle.length() > 0)
							builder = builder.putString(MediaMetadata.METADATA_KEY_TITLE, mTitle);
						if (null != mCoverBitmap) {
							builder = builder.putBitmap(MediaMetadata.METADATA_KEY_ALBUM_ART, mCoverBitmap);
							builder = builder.putBitmap(MediaMetadata.METADATA_KEY_ART, mCoverBitmap);
						}
						builder = builder.putLong(MediaMetadata.METADATA_KEY_DURATION, -1);		// unknown duration
						mMediaSession.setMetadata(builder.build());
						mMediaSession.setActive(true);
						mMediaSession.setPlaybackState(
								mPlaybackStateBuilder.setState(PlaybackState.STATE_PLAYING,
										PlaybackState.PLAYBACK_POSITION_UNKNOWN, mSpeechRate).build());
						registerReceiver(mBecomingNoisyReceiver, new IntentFilter(AudioManager.ACTION_AUDIO_BECOMING_NOISY));
						synchronized (mLocker) {
							mState = State.PLAYING;
							if (!mState.equals(mPrevState)) {
								if (null != mStatusListener)
									mStatusListener.onStateChanged(mState);
								mPrevState = mState;
							}
						}
						// update notification
						Notification notification = buildNotification(mCurrentUtterance);
						if (null != notification) {
							if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
								startForeground(NOTIFICATION_ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK);
							else
								startForeground(NOTIFICATION_ID, notification);
						} else {
							log.e("Failed to build notification!");
						}
						if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
							// Workaround for API26+ bug:
							// https://stackoverflow.com/questions/45960265/android-o-oreo-8-and-higher-media-buttons-issue
							// https://issuetracker.google.com/issues/65344811
							synchronized (mLocker) {
								if (null == mMediaPlayer) {
									mMediaPlayer = MediaPlayer.create(TTSControlService.this, R.raw.silence_10_0);
									mMediaPlayer.setOnCompletionListener(player -> {
										synchronized (mLocker) {
											mMediaPlayer.release();
											mMediaPlayer = null;
										}
									});
									mMediaPlayer.start();
								}
							}
						}
					}
				}

				@Override
				public void onPause() {
					stopUtterance_impl(null);
					synchronized (mLocker) {
						mState = State.PAUSED;
						if (!mState.equals(mPrevState)) {
							if (null != mStatusListener)
								mStatusListener.onStateChanged(mState);
							mPrevState = mState;
						}
					}
					mMediaSession.setPlaybackState(
							mPlaybackStateBuilder.setState(PlaybackState.STATE_PAUSED,
									PlaybackState.PLAYBACK_POSITION_UNKNOWN, 0).build());
					Notification notification = buildNotification(null);
					if (null != notification) {
						stopForeground(false);
						mNotificationManager.notify(NOTIFICATION_ID, notification);
					} else {
						log.e("Failed to build notification!");
					}
					try {
						unregisterReceiver(mBecomingNoisyReceiver);
					} catch (Exception ignored) {
					}
				}

				@Override
				public void onStop() {
					stopUtterance_impl(null);
					synchronized (mLocker) {
						mState = State.STOPPED;
						if (!mState.equals(mPrevState)) {
							if (null != mStatusListener)
								mStatusListener.onStateChanged(mState);
							mPrevState = mState;
						}
					}
					mMediaSession.setPlaybackState(
							mPlaybackStateBuilder.setState(PlaybackState.STATE_STOPPED,
									PlaybackState.PLAYBACK_POSITION_UNKNOWN, 0).build());
					mNotificationManager.cancel(NOTIFICATION_ID);
					stopForeground(true);
					abandonAudioFocusRequestWrapper();
					//WORKAROUND TO PREVENT CRASH ON STOP
					//mMediaSession.setActive(false);
					try {
						unregisterReceiver(mBecomingNoisyReceiver);
					} catch (Exception ignored) {}
				}

				@Override
				public void onSkipToNext() {
					sendBroadcast(new Intent(TTSControlService.TTS_CONTROL_ACTION_NEXT));
				}

				@Override
				public void onSkipToPrevious() {
					sendBroadcast(new Intent(TTSControlService.TTS_CONTROL_ACTION_PREV));
				}
			};
			mMediaSession = new MediaSession(this, "CoolReader TTS");
			if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
				// This constant was deprecated in API level 26.
				// https://developer.android.com/reference/android/media/session/MediaSession#FLAG_HANDLES_MEDIA_BUTTONS
				// https://developer.android.com/reference/android/media/session/MediaSession#FLAG_HANDLES_TRANSPORT_CONTROLS
				mMediaSession.setFlags(MediaSession.FLAG_HANDLES_MEDIA_BUTTONS | MediaSession.FLAG_HANDLES_TRANSPORT_CONTROLS);
			} else {
				mMediaSession.setFlags(0);
			}
			mMediaSession.setCallback(mMediaSessionCallback);
			mPlaybackStateBuilder = new PlaybackState.Builder()
					.setActions(PlaybackState.ACTION_PLAY
							| PlaybackState.ACTION_STOP
							| PlaybackState.ACTION_PAUSE
							| PlaybackState.ACTION_PLAY_PAUSE
							| PlaybackState.ACTION_SKIP_TO_NEXT
							| PlaybackState.ACTION_SKIP_TO_PREVIOUS);
			mMediaSession.setPlaybackState(mPlaybackStateBuilder.setState(PlaybackState.STATE_PAUSED, PlaybackState.PLAYBACK_POSITION_UNKNOWN, 0).build());
			mMediaSession.setPlaybackToLocal(
					new AudioAttributes.Builder()
							.setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
							.setUsage(AudioAttributes.USAGE_MEDIA)
							.build()
			);
		}
		IntentFilter filter = new IntentFilter();
		filter.addAction(TTS_CONTROL_ACTION_PLAY_PAUSE);
		filter.addAction(TTS_CONTROL_ACTION_NEXT);
		filter.addAction(TTS_CONTROL_ACTION_PREV);
		filter.addAction(TTS_CONTROL_ACTION_STOP);
		registerReceiver(mTTSControlActionReceiver, filter);
		mVolumeSettingsContentObserver = new VolumeSettingsContentObserver(new Handler());
		getApplicationContext().getContentResolver().registerContentObserver(android.provider.Settings.System.CONTENT_URI, true, mVolumeSettingsContentObserver);
	}

	@Override
	public void onStart(Intent intent, int startId) {
		// do nothing
	}

	@TargetApi(Build.VERSION_CODES.ECLAIR)
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		log.d("Received start id " + startId + ": " + intent);
		String action = null;
		// According to the documentation, intent can be null:
		// https://developer.android.com/reference/android/app/Service#onStartCommand(android.content.Intent,%20int,%20int)
		// https://developer.android.com/reference/android/app/Service#START_STICKY
		if (null != intent)
			action = intent.getAction();
		if (TTS_CONTROL_ACTION_PREPARE.equals(action)) {
			mCoverBitmap = null;
			mAuthors = null;
			mTitle = null;
			Bundle data = intent.getExtras();
			if (null != data) {
				mTitle = data.getString("bookTitle");
				mAuthors = data.getString("bookAuthors");
			}
			synchronized (mLocker) {
				mState = State.PAUSED;
			}
			// switch this service to foreground
			Notification notification = buildNotification(null);
			if (null != notification) {
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
					startForeground(NOTIFICATION_ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK);
				else
					startForeground(NOTIFICATION_ID, notification);
			} else {
				log.e("Failed to build notification!");
			}
		}
		// TODO: process media buttons events (android.intent.action.MEDIA_BUTTON)
		//  API<21
		// We want this service to continue running until it is explicitly
		// stopped, so return sticky.
		return START_STICKY;
	}

	@Override
	public IBinder onBind(Intent intent) {
		log.i("onBind(): " + intent);
		return mBinder;
	}

	@Override
	public void onDestroy() {
		log.d("onDestroy");
		getApplicationContext().getContentResolver().unregisterContentObserver(mVolumeSettingsContentObserver);
		synchronized (mLocker) {
			if (null != mMediaPlayer) {
				mMediaPlayer.stop();
				mMediaPlayer.release();
				mMediaPlayer = null;
			}
		}
		mStatusListener = null;
		try {
			unregisterReceiver(mTTSControlActionReceiver);
		} catch (Exception ignored) {}
		try {
			unregisterReceiver(mBecomingNoisyReceiver);
		} catch (Exception ignored) {}
		stopUtterance_impl(null);
		abandonAudioFocusRequestWrapper();
		//WORKAROUND TO PREVENT RARE CRASH ON STOP
		//if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
		//	mMediaSession.setActive(false);
		//	mMediaSession.release();
		//}
		mNotificationManager.cancel(NOTIFICATION_ID);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
			mNotificationManager.deleteNotificationChannel(NOTIFICATION_CHANNEL_ID);
		}
		if (null != mTTS) {
			mTTSEnginePackage = null;
			try {
				mTTS.shutdown();
			} catch (Exception ignored) {}
			mTTS = null;
			mTTSInitialized = false;
		}
		super.onDestroy();
	}

	@Override
	public void onLowMemory() {
		log.d("onLowMemory");
	}

	// ======================================
	// this service thread operations

	public interface RetrieveStateCallback {
		void onResult(State state);
	}

	public interface BooleanResultCallback {
		void onResult(boolean result);
	}

	public interface VolumeResultCallback {
		void onResult(int current, int max);
	}

	public interface FloatResultCallback {
		void onResult(float result);
	}

	public interface StringResultCallback {
		void onResult(String result);
	}

	public interface RetrieveEnginesListCallback {
		void onResult(List<TextToSpeech.EngineInfo> list);
	}

	public interface RetrieveLocalesListCallback {
		void onResult(List<Locale> list);
	}

	public interface RetrieveVoiceCallback {
		void onResult(Voice voice);
	}

	public interface RetrieveVoicesListCallback {
		void onResult(List<Voice> list);
	}

	// ======================================
	// this service specific functions

	/**
	 * Create TTS instance. Or recreate (to change engine).
	 * @param engine engine package name
	 * @param listener
	 */
	private void initTTS_impl(String engine, OnTTSCreatedListener listener) {
		if (mTTSInitialized && null != mTTS && null != mTTSEnginePackage && mTTSEnginePackage.equals(engine)) {
			if (null != listener)
				listener.onCreated();
			return;
		}
		if (null != mTTS) {
			try {
				mTTS.stop();
				mTTS.shutdown();
			} catch (Exception ignored) {
			}
			mTTS = null;
		}
		mTTSInitialized = false;
		final boolean isSpeaking = State.PLAYING == mState;
		TextToSpeech.OnInitListener onInitListener = status -> {
			mInitTTSTimer.cancel();
			mInitTTSTimer = null;
			L.i("TTS init status: " + status);
			if (status == TextToSpeech.SUCCESS) {
				mTTSInitialized = true;
				mTTSEnginePackage = engine;
				setupTTSHandlers();
				if (null != listener)
					listener.onCreated();
				if (isSpeaking)
					say_impl(mCurrentUtterance);
			} else {
				try {
					mTTS.shutdown();
				} catch (Exception ignored) {}
				mTTS = null;
				if (null != listener)
					listener.onFailed();
			}
		};
		mInitTTSTimer = new Timer();
		mInitTTSTimer.schedule(new TimerTask() {
			@Override
			public void run() {
				// TTS engine init hangs, call listener
				log.e("TTS engine \"" + engine + "\" init failure with timeout!");
				if (null != listener)
					listener.onTimedOut();
				mInitTTSTimer.cancel();
				mInitTTSTimer = null;
				try {
					mTTS.shutdown();
				} catch (Exception ignored) {}
				mTTS = null;
			}
		}, INIT_TTS_TIMEOUT);
		if (Build.VERSION.SDK_INT > Build.VERSION_CODES.ICE_CREAM_SANDWICH && null != engine && engine.length() > 0)
			mTTS = new TextToSpeech(this, onInitListener, engine);
		else
			mTTS = new TextToSpeech(this, onInitListener);
	}

	/**
	 * Set media info: authors, title and cover.
	 * @param authors book's author(s)
	 * @param title book's title
	 * @param cover book's cover
	 */
	private void setMediaItemInfo_impl(String authors, String title, Bitmap cover) {
		synchronized (mLocker) {
			mAuthors = authors;
			mTitle = title;
			mCoverBitmap = cover;
			// update notification
			if (null != mNotificationManager) {
				Notification notification = buildNotification(mCurrentUtterance);
				if (null != notification) {
					if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ECLAIR) {
						boolean isSpeaking;
						synchronized (mLocker) {
							isSpeaking = State.PLAYING == mState;
						}
						if (isSpeaking) {
							if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
								startForeground(NOTIFICATION_ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK);
							else
								startForeground(NOTIFICATION_ID, notification);
						} else {
							stopForeground(false);
							mNotificationManager.notify(NOTIFICATION_ID, notification);
						}
					} else {
						mNotificationManager.notify(NOTIFICATION_ID, notification);
					}
				} else {
					log.e("Failed to build notification!");
				}
			}
			if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP) {
				MediaMetadata.Builder builder = new MediaMetadata.Builder();
				if (null != mAuthors && mAuthors.length() > 0)
					builder = builder.putString(MediaMetadata.METADATA_KEY_AUTHOR, mAuthors);
				if (null != mTitle && mTitle.length() > 0)
					builder = builder.putString(MediaMetadata.METADATA_KEY_TITLE, mTitle);
				if (null != cover)
					builder = builder.putBitmap(MediaMetadata.METADATA_KEY_ALBUM_ART, cover);
				mMediaSession.setMetadata(builder.build());
			}
		}
	}

	private State getState_impl() {
		synchronized (mLocker) {
			return mState;
		}
	}

	/**
	 * Start to speech specified text.
	 * @param utterance Utterance to speech.
	 */
	private boolean say_impl(String utterance) {
		if (null != mTTS) {
			int ret;
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
				if (null == mTTSParamsBundle) {
					mTTSParamsBundle = new Bundle();
					mTTSParamsBundle.putInt(TextToSpeech.Engine.KEY_PARAM_STREAM, AudioManager.STREAM_MUSIC);
				}
				ret = mTTS.speak(utterance, TextToSpeech.QUEUE_ADD, mTTSParamsBundle, CR3_UTTERANCE_ID);
			} else {
				if (null == mTTSParamsMap) {
					mTTSParamsMap = new HashMap<String, String>();
					mTTSParamsMap.put(TextToSpeech.Engine.KEY_PARAM_STREAM, String.valueOf(AudioManager.STREAM_MUSIC));
					mTTSParamsMap.put(TextToSpeech.Engine.KEY_PARAM_UTTERANCE_ID, CR3_UTTERANCE_ID);
				}
				ret = mTTS.speak(utterance, TextToSpeech.QUEUE_ADD, mTTSParamsMap);
			}
			return TextToSpeech.SUCCESS == ret;
		}
		return false;
	}

	/**
	 * Stop to speech current utterance. Optional callback can be used to implement sentence switch.
	 * @param callback runnable to executed after completion.
	 */
	private boolean stopUtterance_impl(Runnable callback) {
		if (null != mTTS) {
			mOnUtteranceStopOnce = callback;
			if (mTTS.isSpeaking()) {
				return mTTS.stop() == TextToSpeech.SUCCESS;
			}
		}
		return false;
	}

	private void playWrapper_api_less_than_21() {
		if (null != mCurrentUtterance) {
			if (!mPlaybackNowAuthorized)
				requestAudioFocusWrapper();
			if (mPlaybackNowAuthorized) {
				// start to speech
				if (say_impl(mCurrentUtterance)) {
					synchronized (mLocker) {
						mState = State.PLAYING;
						if (!mState.equals(mPrevState)) {
							if (null != mStatusListener)
								mStatusListener.onStateChanged(mState);
							mPrevState = mState;
						}
					}
					// update notification
					Notification notification = buildNotification(mCurrentUtterance);
					if (null != notification) {
						if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ECLAIR)
							startForeground(NOTIFICATION_ID, notification);
						else
							mNotificationManager.notify(NOTIFICATION_ID, notification);
					} else {
						log.e("Failed to build notification!");
					}
					registerReceiver(mBecomingNoisyReceiver, new IntentFilter(AudioManager.ACTION_AUDIO_BECOMING_NOISY));
				}
			} else {
				log.e("Can't say anything, audio focus is locked by other app.");
			}
		} else {
			if (null != mStatusListener)
				mStatusListener.onCurrentSentenceRequested(mBinder);
		}
	}

	private void pauseWrapper_api_less_than_21() {
		stopUtterance_impl(null);
		synchronized (mLocker) {
			mState = State.PAUSED;
			if (!mState.equals(mPrevState)) {
				if (null != mStatusListener)
					mStatusListener.onStateChanged(mState);
				mPrevState = mState;
			}
		}
		// update notification
		Notification notification = buildNotification(null);
		if (null != notification) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ECLAIR)
				stopForeground(false);
			mNotificationManager.notify(NOTIFICATION_ID, notification);
		} else {
			log.e("Failed to build notification!");
		}
		try {
			unregisterReceiver(mBecomingNoisyReceiver);
		} catch (Exception ignored) {}
	}

	private void stopWrapper_api_less_than_21() {
		stopUtterance_impl(null);
		synchronized (mLocker) {
			mState = State.STOPPED;
			if (!mState.equals(mPrevState)) {
				if (null != mStatusListener)
					mStatusListener.onStateChanged(mState);
				mPrevState = mState;
			}
		}
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ECLAIR)
			stopForeground(true);
		mNotificationManager.cancel(NOTIFICATION_ID);
		abandonAudioFocusRequestWrapper();
		try {
			unregisterReceiver(mBecomingNoisyReceiver);
		} catch (Exception ignored) {}
		mTitle = null;
		mCoverBitmap = null;
		mAuthors = null;
	}

	/**
	 * Gets list of available engines.
	 * @return list of available engines.
	 */
	@TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
	private List<TextToSpeech.EngineInfo> getAvailableEngines_impl() {
		if (null != mTTS)
			return mTTS.getEngines();
		return null;
	}

	/**
	 * Gets list of available locales in current engine
	 * @return list of available locales in current engine
	 */
	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	private List<Locale> getAvailableLocales_impl() {
		ArrayList<Locale> list = null;
		if (null != mTTS) {
			list = new ArrayList<Locale>();
			Set<Voice> voices = mTTS.getVoices();
			// Fill list without duplicates
			// ArraySet can't be using since API level restriction
			HashMap<Locale, String> languagesMap = new HashMap<>();
			for (Voice voice : voices) {
				Locale locale = voice.getLocale();
				String language = locale.getDisplayLanguage();
				String country = locale.getDisplayCountry();
				if (country.length() > 0)
					language += " (" + country + ")";
				languagesMap.put(locale, language);
			}
			for (Map.Entry<Locale, String> entry : languagesMap.entrySet()) {
				list.add(entry.getKey());
			}
			// Sort lexicographically
			Collections.sort(list, (o1, o2) -> o1.toString().compareToIgnoreCase(o2.toString()));
		}
		return list;
	}

	/**
	 * Gets list of available voices in current engine for specified locale.
	 * @param locale specific locale
	 * @return list of available voices in current engine for specified locale.
	 */
	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	private List<Voice> getAvailableVoices_impl(Locale locale) {
		ArrayList<Voice> list = null;
		if (null != mTTS) {
			list = new ArrayList<Voice>();
			Set<Voice> voices = mTTS.getVoices();
			for (Voice voice : voices) {
				if (voice.getLocale().toString().equalsIgnoreCase(locale.toString()))
					list.add(voice);
			}
			Collections.sort(list, (o1, o2) -> o1.getName().compareTo(o2.getName()));
		}
		return list;
	}

	/**
	 * Get current TTS language.
	 * @return current TTS language.
	 */
	private String getLanguage_impl() {
		synchronized (mLocker) {
			if (null != mLocale)
				return mLocale.getLanguage();
		}
		return null;
	}

	/**
	 * Set TTS language. Voice can't be set with this function.
	 * @param language specific language
	 * @return
	 */
	private boolean setLanguage_impl(String language) {
		// set language for TTS by locale name
		synchronized (mLocker) {
			Locale locale = new Locale(language);
			int ret = mTTS.setLanguage(locale);
			if ((ret == TextToSpeech.LANG_AVAILABLE) ||
					(ret == TextToSpeech.LANG_COUNTRY_AVAILABLE) ||
					(ret == TextToSpeech.LANG_COUNTRY_VAR_AVAILABLE))
			{
				mLocale = locale;
				return true;
			}
			mLocale = null;
		}
		return false;
	}

	/**
	 * Get current TTS voice.
	 * @return current TTS voice if set, otherwise null.
	 */
	private Voice getVoice_impl() {
		synchronized (mLocker) {
			return mVoice;
		}
	}

	/**
	 * Set TTS voice. Override setLanguage() call.
	 * @param voice specific voice
	 * @return true if operation is successful, false otherwise.
	 */
	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	private boolean setVoice_impl(Voice voice) {
		synchronized (mLocker) {
			if (null != mTTS) {
				if (TextToSpeech.SUCCESS == mTTS.setVoice(voice)) {
					mVoice = voice;
					mLocale = voice.getLocale();
					return true;
				}
			}
			mVoice = null;
			mLocale = null;
		}
		return false;
	}

	/**
	 * Set TTS voice. Override setLanguage() call.
	 * @param voiceName specific voice name
	 * @return true if operation is successful, false otherwise.
	 */
	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	private boolean setVoice_impl(String voiceName) {
		synchronized (mLocker) {
			if (null != mTTS) {
				Set<Voice> voices = mTTS.getVoices();
				for (Voice voice : voices) {
					if (voice.getName().equals(voiceName)) {
						if (TextToSpeech.SUCCESS == mTTS.setVoice(voice)) {
							mVoice = voice;
							mLocale = voice.getLocale();
							return true;
						}
					}
				}
			}
			mVoice = null;
			mLocale = null;
		}
		return false;
	}

	private float getSpeechRate_impl() {
		synchronized (mLocker) {
			return mSpeechRate;
		}
	}

	private boolean setSpeechRate_impl(float rate) {
		synchronized (mLocker) {
			if (null != mTTS) {
				if (TextToSpeech.SUCCESS == mTTS.setSpeechRate(rate)) {
					mSpeechRate = rate;
					return true;
				}
			}
		}
		return false;
	}

	private int getMaxVolume_impl() {
		return mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
	}

	private int getVolume_impl() {
		return mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
	}

	private void setVolume_impl(int volume) {
		mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, volume, 0);
	}

	// ======================================
	// this service access functions (wrappers)

	public void initTTS(String engine, OnTTSCreatedListener listener) {
		execTask(new Task("initTTS") {
			@Override
			public void work() {
				initTTS_impl(engine, listener);
			}
		});
	}

	public void setMediaItemInfo(String authors, String title, Bitmap cover) {
		execTask(new Task("initTTS") {
			@Override
			public void work() {
				setMediaItemInfo_impl(authors, title, cover);
			}
		});
	}

	public void retrieveState(RetrieveStateCallback callback, Handler handler) {
		execTask(new Task("retrieveState") {
			@Override
			public void work() {
				State result = getState_impl();
				if (null != callback)
					sendTask(handler, () -> callback.onResult(result));
			}
		});
	}

	public void say(String utterance, BooleanResultCallback callback, Handler handler) {
		execTask(new Task("say") {
			@Override
			public void work() {
				mCurrentUtterance = utterance;
				boolean result;
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
					mMediaSessionCallback.onPlay();
				else
					playWrapper_api_less_than_21();
				synchronized (mLocker) {
					result = State.PLAYING == mState;
				}
				if (null != callback) {
					sendTask(handler, () -> callback.onResult(result));
				}
			}
		});
	}

	public void setCurrentUtterance(String utterance) {
		mCurrentUtterance = utterance;
	}

	public void pause(BooleanResultCallback callback, Handler handler) {
		execTask(new Task("pause") {
			@Override
			public void work() {
				boolean result;
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
					mMediaSessionCallback.onPause();
				else
					pauseWrapper_api_less_than_21();
				synchronized (mLocker) {
					result = State.PAUSED == mState;
				}
				if (null != callback) {
					sendTask(handler, () -> callback.onResult(result));
				}
			}
		});
	}

	public void stopUtterance(BooleanResultCallback callback, Handler handler) {
		execTask(new Task("stopUtterance") {
			@Override
			public void work() {
				//boolean result = stopUtterance_impl(onStop);
				boolean result = stopUtterance_impl(null);
				if (null != callback)
					sendTask(handler, () -> callback.onResult(result));
			}
		});
	}

	public void stop(BooleanResultCallback callback, Handler handler) {
		execTask(new Task("stop") {
			@Override
			public void work() {
				boolean result;
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
					mMediaSessionCallback.onStop();
				else
					stopWrapper_api_less_than_21();
				synchronized (mLocker) {
					result = State.PAUSED == mState;
				}
				if (null != callback)
					sendTask(handler, () -> callback.onResult(result));
			}
		});
	}

	public void retrieveAvailableEngines(RetrieveEnginesListCallback callback, Handler handler) {
		execTask(new Task("retrieveAvailableEngines") {
			@Override
			public void work() {
				List<TextToSpeech.EngineInfo> result = getAvailableEngines_impl();
				if (null != callback)
					sendTask(handler, () -> callback.onResult(result));
			}
		});
	}

	public void retrieveAvailableLocales(RetrieveLocalesListCallback callback, Handler handler) {
		execTask(new Task("retrieveAvailableLocales") {
			@Override
			public void work() {
				List<Locale> result = getAvailableLocales_impl();
				if (null != callback)
					sendTask(handler, () -> callback.onResult(result));
			}
		});
	}

	public void retrieveAvailableVoices(Locale locale, RetrieveVoicesListCallback callback, Handler handler) {
		execTask(new Task("retrieveAvailableVoices") {
			@Override
			public void work() {
				List<Voice> result = getAvailableVoices_impl(locale);
				if (null != callback)
					sendTask(handler, () -> callback.onResult(result));
			}
		});
	}

	public void retrieveLanguage(StringResultCallback callback, Handler handler) {
		execTask(new Task("retrieveLanguage") {
			@Override
			public void work() {
				String result = getLanguage_impl();
				if (null != callback)
					sendTask(handler, () -> callback.onResult(result));
			}
		});
	}

	public void setLanguage(String language, BooleanResultCallback callback, Handler handler) {
		execTask(new Task("setLanguage") {
			@Override
			public void work() {
				boolean result = setLanguage_impl(language);
				if (null != callback)
					sendTask(handler, () -> callback.onResult(result));
			}
		});
	}

	public void retrieveVoice(RetrieveVoiceCallback callback, Handler handler) {
		execTask(new Task("retrieveLanguage") {
			@Override
			public void work() {
				Voice result = getVoice_impl();
				if (null != callback)
					sendTask(handler, () -> callback.onResult(result));
			}
		});
	}

	public void setVoice(Voice voice, BooleanResultCallback callback, Handler handler) {
		execTask(new Task("setVoice") {
			@Override
			public void work() {
				boolean result = setVoice_impl(voice);
				if (null != callback)
					sendTask(handler, () -> callback.onResult(result));
			}
		});
	}

	public void setVoice(String voiceName, BooleanResultCallback callback, Handler handler) {
		execTask(new Task("setVoice(name)") {
			@Override
			public void work() {
				boolean result = setVoice_impl(voiceName);
				if (null != callback)
					sendTask(handler, () -> callback.onResult(result));
			}
		});
	}

	public void retrieveSpeechRate(FloatResultCallback callback, Handler handler) {
		execTask(new Task("retrieveSpeechRate") {
			@Override
			public void work() {
				float result = getSpeechRate_impl();
				if (null != callback)
					sendTask(handler, () -> callback.onResult(result));
			}
		});
	}

	public void setSpeechRate(float rate, BooleanResultCallback callback, Handler handler) {
		execTask(new Task("setSpeechRate") {
			@Override
			public void work() {
				boolean result = setSpeechRate_impl(rate);
				if (null != callback)
					sendTask(handler, () -> callback.onResult(result));
			}
		});
	}

	public void retrieveVolume(VolumeResultCallback callback, Handler handler) {
		execTask(new Task("retrieveVolume") {
			@Override
			public void work() {
				int maxVolume = getMaxVolume_impl();
				int result = getVolume_impl();
				if (null != callback)
					sendTask(handler, () -> callback.onResult(result, maxVolume));
			}
		});
	}

	public void setVolume(int volume) {
		execTask(new Task("setVolume") {
			@Override
			public void work() {
				setVolume_impl(volume);
			}
		});
	}

	public OnTTSStatusListener getStatusListener() {
		synchronized (mLocker) {
			return mStatusListener;
		}
	}

	public void setStatusListener(OnTTSStatusListener listener) {
		synchronized (mLocker) {
			mStatusListener = listener;
		}
	}

	// ======================================
	// private implementation

	private Notification buildNotification(String utterance) {
		String title = "";
		if (null != mAuthors && mAuthors.length() > 0) {
			title = mAuthors;
		}
		if (mTitle != null && mTitle.length() > 0) {
			if (title.length() > 0) {
				title = title + " - " + mTitle;
			} else {
				title = mTitle;
			}
		}
		if (0 == title.length())
			title = "CoolReader";
		Notification notification;
		Intent notificationIntent = new Intent(this, CoolReader.class);
		PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
			Notification.Builder builder;
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
				builder = new Notification.Builder(this, NOTIFICATION_CHANNEL_ID);
				// create notification channel
				if (!mChannelCreated) {
					NotificationChannel channel = new NotificationChannel(NOTIFICATION_CHANNEL_ID, "CoolReader TTS", NotificationManager.IMPORTANCE_DEFAULT);
					channel.setDescription("CoolReader TTS control");
					channel.setSound(null, null);
					// Register the channel with the system; you can't change the importance
					// or other notification behaviors after this
					if (null != mNotificationManager) {
						mNotificationManager.createNotificationChannel(channel);
						mChannelCreated = true;
					}
				}
				if (mChannelCreated)
					builder = builder.setChannelId(NOTIFICATION_CHANNEL_ID);
				else
					return null;
			} else {
				builder = new Notification.Builder(this);
			}
			builder = builder.setDefaults(0);
			builder = builder.setSmallIcon(R.drawable.cr3_logo_button_hc);
			builder = builder.setContentTitle(title);
			if (null != utterance && !utterance.isEmpty())
				builder = builder.setContentText(utterance);
			else
				builder = builder.setContentText("...");
			builder = builder.setOngoing(true);
			builder = builder.setAutoCancel(false);
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
				builder = builder.setPriority(Notification.PRIORITY_DEFAULT);
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
				builder = builder.setShowWhen(false);
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT_WATCH) {
					builder = builder.setLocalOnly(true);
					// add actions
					// play/pause
					PendingIntent playPauseIntent = PendingIntent.getBroadcast(this, 0, new Intent(TTS_CONTROL_ACTION_PLAY_PAUSE), 0);
					Notification.Action.Builder actionBld = new Notification.Action.Builder(mState == State.PAUSED ? R.drawable.ic_media_play : R.drawable.ic_media_pause, "", playPauseIntent);
					Notification.Action actionPlayPause = actionBld.build();
					builder = builder.addAction(actionPlayPause);
					// prev
					PendingIntent prevIntent = PendingIntent.getBroadcast(this, 0, new Intent(TTS_CONTROL_ACTION_PREV), 0);
					actionBld = new Notification.Action.Builder(R.drawable.ic_media_rew, "", prevIntent);
					Notification.Action actionPrev = actionBld.build();
					builder = builder.addAction(actionPrev);
					// next
					PendingIntent nextIntent = PendingIntent.getBroadcast(this, 0, new Intent(TTS_CONTROL_ACTION_NEXT), 0);
					actionBld = new Notification.Action.Builder(R.drawable.ic_media_ff, "", nextIntent);
					Notification.Action actionNext = actionBld.build();
					builder = builder.addAction(actionNext);
					// stop
					PendingIntent stopIntent = PendingIntent.getBroadcast(this, 0, new Intent(TTS_CONTROL_ACTION_STOP), 0);
					actionBld = new Notification.Action.Builder(R.drawable.ic_media_stop, "", stopIntent);
					Notification.Action actionStop = actionBld.build();
					builder = builder.addAction(actionStop);
					//
					if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
						builder = builder.setSound(null, null);
						builder = builder.setStyle(new Notification.MediaStyle().setShowActionsInCompactView(0, 3).setMediaSession(mMediaSession.getSessionToken()));
						builder = builder.setColor(Color.GRAY);
						builder = builder.setVisibility(Notification.VISIBILITY_PUBLIC);
						if (null != mCoverBitmap)
							builder = builder.setLargeIcon(mCoverBitmap);
						else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
							builder = builder.setLargeIcon(Icon.createWithResource(this, R.drawable.cr3_logo_button));
					}
				}
			} else
				builder = builder.setWhen(System.currentTimeMillis());
			// delete intent
			PendingIntent delPendingIntent = PendingIntent.getBroadcast(this, 0, new Intent(TTS_CONTROL_ACTION_STOP), 0);
			builder = builder.setDeleteIntent(delPendingIntent);
			builder = builder.setContentIntent(pendingIntent);
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
				notification = builder.build();
			else
				notification = builder.getNotification();
		} else {
			notification = new Notification(R.drawable.cr3_logo_button, "CoolReader", System.currentTimeMillis());
			notification.contentIntent = pendingIntent;
		}
		return notification;
	}

	private void setupTTSHandlers() {
		if (null != mTTS) {
			if (Build.VERSION.SDK_INT < Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1) {
				mTTS.setOnUtteranceCompletedListener(utteranceId -> {
					if (null != mOnUtteranceStopOnce) {
						mOnUtteranceStopOnce.run();
						mOnUtteranceStopOnce = null;
					}
					if (null != mStatusListener) {
						mStatusListener.onUtteranceDone();
						mStatusListener.onNextSentenceRequested(mBinder);
					}
				});
			} else {
				mTTS.setOnUtteranceProgressListener(new UtteranceProgressListener() {
					@Override
					public void onStart(String utteranceId) {
					}

					@Override
					public void onDone(String utteranceId) {
						if (null != mOnUtteranceStopOnce) {
							mOnUtteranceStopOnce.run();
							mOnUtteranceStopOnce = null;
						}
						if (null != mStatusListener) {
							mStatusListener.onUtteranceDone();
							mStatusListener.onNextSentenceRequested(mBinder);
						}
						synchronized (mLocker) {
							if (null != mMediaPlayer) {
								mMediaPlayer.stop();
								mMediaPlayer.release();
								mMediaPlayer = null;
							}
						}
						mContinuousErrors = 0;
					}

					@Override
					public void onError(String utteranceId) {
						log.e("TTS error");
						mContinuousErrors++;
						if (mContinuousErrors >= MAX_CONTINUOUS_ERRORS) {
							if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
								mMediaSessionCallback.onStop();
							else
								stopWrapper_api_less_than_21();
							if (null != mStatusListener)
								mStatusListener.onError(0);
						} else {
							// If error count is low - process as 'onDone' event
							if (null != mOnUtteranceStopOnce) {
								mOnUtteranceStopOnce.run();
								mOnUtteranceStopOnce = null;
							}
							if (null != mStatusListener) {
								mStatusListener.onUtteranceDone();
								mStatusListener.onNextSentenceRequested(mBinder);
							}
							synchronized (mLocker) {
								if (null != mMediaPlayer) {
									mMediaPlayer.stop();
									mMediaPlayer.release();
									mMediaPlayer = null;
								}
							}
						}
					}

					// API 21
					@Override
					public void onError(String utteranceId, int errorCode) {
						log.e("TTS error, code=" + errorCode);
						mContinuousErrors++;
						if (mContinuousErrors >= MAX_CONTINUOUS_ERRORS) {
							if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
								mMediaSessionCallback.onStop();
							else
								stopWrapper_api_less_than_21();
							if (null != mStatusListener)
								mStatusListener.onError(errorCode);
						} else {
							// If error count is low - process as 'onDone' event
							if (null != mOnUtteranceStopOnce) {
								mOnUtteranceStopOnce.run();
								mOnUtteranceStopOnce = null;
							}
							if (null != mStatusListener) {
								mStatusListener.onUtteranceDone();
								mStatusListener.onNextSentenceRequested(mBinder);
							}
							synchronized (mLocker) {
								if (null != mMediaPlayer) {
									mMediaPlayer.stop();
									mMediaPlayer.release();
									mMediaPlayer = null;
								}
							}
						}
					}

					// API 23
					@Override
					public void onStop(String utteranceId, boolean interrupted) {
						if (null != mOnUtteranceStopOnce) {
							mOnUtteranceStopOnce.run();
							mOnUtteranceStopOnce = null;
						}
						synchronized (mLocker) {
							if (null != mMediaPlayer) {
								mMediaPlayer.stop();
								mMediaPlayer.release();
								mMediaPlayer = null;
							}
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

	private boolean requestAudioFocusWrapper() {
		if (null != mAudioManager) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
				if (null != mAudioFocusRequest) {
					int res = mAudioManager.requestAudioFocus(mAudioFocusRequest);
					switch (res) {
						case AudioManager.AUDIOFOCUS_REQUEST_GRANTED:
							mPlaybackNowAuthorized = true;
							mPlaybackDelayed = false;
							mResumeOnFocusGain = false;
							break;
						case AudioManager.AUDIOFOCUS_REQUEST_DELAYED:
							// Now this is dead code since we call
							//  builder.setAcceptsDelayedFocusGain(false);
							mPlaybackNowAuthorized = false;
							mPlaybackDelayed = true;
							mResumeOnFocusGain = false;
							break;
						case AudioManager.AUDIOFOCUS_REQUEST_FAILED:
						default:
							mPlaybackNowAuthorized = false;
							mPlaybackDelayed = false;
							mResumeOnFocusGain = false;
							break;
					}
				}
			} else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.FROYO) {
				int res = mAudioManager.requestAudioFocus(mAudioFocusChangeListener, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
				mPlaybackNowAuthorized = AudioManager.AUDIOFOCUS_REQUEST_GRANTED == res;
				mPlaybackDelayed = false;
				mResumeOnFocusGain = false;
			} else {
				mPlaybackNowAuthorized = true;
			}
		} else {
			mPlaybackNowAuthorized = true;
		}
		return mPlaybackNowAuthorized;
	}

	private void abandonAudioFocusRequestWrapper() {
		if (null != mAudioManager) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
				if (null != mAudioFocusRequest) {
					mAudioManager.abandonAudioFocusRequest(mAudioFocusRequest);
				}
			} else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.FROYO) {
				mAudioManager.abandonAudioFocus(mAudioFocusChangeListener);
			}
		}
	}

}
