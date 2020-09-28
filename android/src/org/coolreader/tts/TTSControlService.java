package org.coolreader.tts;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.graphics.Color;
import android.graphics.drawable.Icon;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;

import androidx.annotation.RequiresApi;

import org.coolreader.CoolReader;
import org.coolreader.R;
import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;

/**
 * This service does not implement TTS!
 * This service was created to keep the application in the foreground (for the target API >= 26),
 * even if the main activity is in the background, so that the system understands that the application
 * does not need to be unloaded from memory while TTS is running.
 * It also adds TTS control buttons to the notification area and lock screen.
 */
public class TTSControlService extends Service {
	public static final Logger log = L.create("ttssrv");

	private final int NOTIFICATION_ID = 1;
	private final String NOTIFICATION_CHANNEL_ID = "CoolReader TTS";

	public static final String TTS_CONTROL_ACTION_PLAY_PAUSE = "org.coolreader.tts.tts_play_pause";
	public static final String TTS_CONTROL_ACTION_NEXT = "org.coolreader.tts.tts_next";
	public static final String TTS_CONTROL_ACTION_PREV = "org.coolreader.tts.tts_prev";
	public static final String TTS_CONTROL_ACTION_DONE = "org.coolreader.tts.tts_done";

	private boolean mChannelCreated = false;
	private IBinder mBinder = new TTSControlBinder(this);

	public enum TTSStatus {
		PLAYED,
		PAUSED
	}

	public TTSControlService() {
		super();
	}

	@Override
	public void onCreate() {
		log.d("onCreate");
	}

	@Override
	public void onStart(Intent intent, int startId) {
		// do nothing
	}

	@RequiresApi(api = Build.VERSION_CODES.ECLAIR)
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		log.d("Received start id " + startId + ": " + intent);

		String title = "";
		Bundle data = intent.getExtras();
		if (null != data)
			title = data.getString("bookTitle");

		// switch this service to foreground
		Notification notification = buildNotification(title, null, TTSStatus.PAUSED);
		if (null != notification) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
				startForeground(NOTIFICATION_ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK);
			else
				startForeground(NOTIFICATION_ID, notification);
		} else
			log.e("Failed to build notification!");

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
	}

	@Override
	public void onLowMemory() {
		log.d("onLowMemory");
	}

	private Notification buildNotification(String title, String sentence, TTSStatus status) {
		Notification notification;
		Intent notificationIntent = new Intent(this, CoolReader.class);
		PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
			Notification.Builder builder;
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
				builder = new Notification.Builder(this, NOTIFICATION_CHANNEL_ID);
				// create notification channel
				if (!mChannelCreated) {
					NotificationChannel channel = new NotificationChannel(NOTIFICATION_CHANNEL_ID, "TTS Control", NotificationManager.IMPORTANCE_NONE);
					channel.setDescription("CoolReader TTS control");
					// Register the channel with the system; you can't change the importance
					// or other notification behaviors after this
					NotificationManager notificationManager = getSystemService(NotificationManager.class);
					if (null != notificationManager) {
						notificationManager.createNotificationChannel(channel);
						mChannelCreated = true;
					}
				}
				if (mChannelCreated)
					builder = builder.setChannelId(NOTIFICATION_CHANNEL_ID);
				else
					return null;
			} else
				builder = new Notification.Builder(this);
			builder = builder.setSmallIcon(R.drawable.cr3_logo_button_hc);
			if (null != title && !title.isEmpty())
				builder = builder.setContentTitle(title);
			else
				builder = builder.setContentTitle("TTS");
			if (null != sentence && !sentence.isEmpty())
				builder = builder.setContentText(sentence);
			else
				builder = builder.setContentText("TTS Control");
			builder = builder.setOngoing(true);
			builder = builder.setAutoCancel(false);
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
				builder = builder.setShowWhen(false);
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT_WATCH) {
					// add actions
					// play/pause
					Intent intent1 = new Intent(TTS_CONTROL_ACTION_PLAY_PAUSE);
					PendingIntent pendingIntent1 = PendingIntent.getBroadcast(this, 0, intent1, 0);
					Notification.Action.Builder actionBld = new Notification.Action.Builder(status == TTSStatus.PAUSED ? R.drawable.ic_media_play : R.drawable.ic_media_pause, "", pendingIntent1);
					Notification.Action actionPlayPause = actionBld.build();
					builder = builder.addAction(actionPlayPause);
					// prev
					Intent intent2 = new Intent(TTS_CONTROL_ACTION_PREV);
					PendingIntent pendingIntent2 = PendingIntent.getBroadcast(this, 0, intent2, 0);
					actionBld = new Notification.Action.Builder(R.drawable.ic_media_rew, "", pendingIntent2);
					Notification.Action actionPrev = actionBld.build();
					builder = builder.addAction(actionPrev);
					// next
					Intent intent3 = new Intent(TTS_CONTROL_ACTION_NEXT);
					PendingIntent pendingIntent3 = PendingIntent.getBroadcast(this, 0, intent3, 0);
					actionBld = new Notification.Action.Builder(R.drawable.ic_media_ff, "", pendingIntent3);
					Notification.Action actionNext = actionBld.build();
					builder = builder.addAction(actionNext);
					// stop
					Intent intent4 = new Intent(TTS_CONTROL_ACTION_DONE);
					PendingIntent pendingIntent4 = PendingIntent.getBroadcast(this, 0, intent4, 0);
					actionBld = new Notification.Action.Builder(R.drawable.ic_media_stop, "", pendingIntent4);
					Notification.Action actionStop = actionBld.build();
					builder = builder.addAction(actionStop);
					//
					if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
						builder = builder.setStyle(new Notification.MediaStyle().setShowActionsInCompactView(0, 3));
						//if (null != sentence && !sentence.isEmpty())
						//	builder = builder.setStyle(new Notification.BigTextStyle().bigText(sentence));
						builder = builder.setColor(Color.GRAY);
						builder = builder.setVisibility(Notification.VISIBILITY_PUBLIC);
						if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
							builder = builder.setLargeIcon(Icon.createWithResource(this, R.drawable.cr3_logo_button));
					}
				}
			} else
				builder = builder.setWhen(System.currentTimeMillis());
			builder = builder.setContentIntent(pendingIntent);
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
				notification = builder.build();
			else
				notification = builder.getNotification();
		} else {
			notification = new Notification(R.drawable.cr3_logo_button, "TTS Control", System.currentTimeMillis());
			notification.contentIntent = pendingIntent;
		}
		return notification;
	}

	public void notifyPlay(String title, String sentence) {
		NotificationManager notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
		if (null != notificationManager) {
			Notification notification = buildNotification(title, sentence, TTSStatus.PLAYED);
			if (null != notification)
				notificationManager.notify(NOTIFICATION_ID, notification);
			else
				log.e("Failed to build notification!");
		}
	}

	public void notifyPause(String title) {
		NotificationManager notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
		if (null != notificationManager) {
			Notification notification = buildNotification(title, null, TTSStatus.PAUSED);
			if (null != notification)
				notificationManager.notify(NOTIFICATION_ID, notification);
			else
				log.e("Failed to build notification!");
		}
	}

}
