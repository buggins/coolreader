/*
 *   Copyright (C) 2021 by Chernov A.A.
 *   valexlin@gmail.com
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.coolreader.sync2;

import android.annotation.TargetApi;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;

import org.coolreader.CoolReader;
import org.coolreader.R;
import org.coolreader.crengine.BookInfo;
import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;
import org.coolreader.crengine.Properties;
import org.coolreader.db.BaseService;
import org.coolreader.db.Task;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class SyncService extends BaseService {

	public static final Logger log = L.create("sync2svc");

	private static final class SyncCommand {
		public String action;
		public Bundle params;
		public SyncCommand(String action, Bundle params) {
			this.action = action;
			this.params = params;
		}
	}

	private static final int NOTIFICATION_ID = 2;
	private static final String NOTIFICATION_CHANNEL_ID = "CoolReader Sync2 C1";

	public static final String SYNC_ACTION_SYNCTO = "org.coolreader.sync2.syncto";
	public static final String SYNC_ACTION_SYNCTO_ONLY = "org.coolreader.sync2.syncto.only";
	public static final String SYNC_ACTION_SYNCFROM = "org.coolreader.sync2.syncfrom";
	public static final String SYNC_ACTION_SYNCFROM_ONLY = "org.coolreader.sync2.syncfrom.only";
	public static final String SYNC_ACTION_CANCEL = "org.coolreader.sync2.cancel";
	public static final String SYNC_ACTION_NOOP = "org.coolreader.sync2.noop";

	private final List<SyncCommand> mSyncCommands = Collections.synchronizedList(new ArrayList<SyncCommand>());

	private final SyncServiceBinder mBinder = new SyncServiceBinder(this);
	private boolean mChannelCreated = false;
	private NotificationManager mNotificationManager = null;
	private Synchronizer mSynchronizer = null;
	private OnSyncStatusListener mStatusListener = null;
	private final OnSyncStatusListener mStatusListenerWrapper = new OnSyncStatusListener() {
		@Override
		public void onSyncStarted(Synchronizer.SyncDirection direction, boolean showProgress, boolean interactively) {
			Notification notification = buildNotification(direction, -1, -1);
			if (null != notification && null != mNotificationManager)
				mNotificationManager.notify(NOTIFICATION_ID, notification);
			synchronized (mLocker) {
				if (null != mStatusListener)
					mStatusListener.onSyncStarted(direction, showProgress, interactively);
			}
		}

		@Override
		public void OnSyncProgress(Synchronizer.SyncDirection direction, boolean showProgress, int current, int total, boolean interactively) {
			Notification notification = buildNotification(direction, current, total);
			if (null != notification && null != mNotificationManager)
				mNotificationManager.notify(NOTIFICATION_ID, notification);
			synchronized (mLocker) {
				if (null != mStatusListener)
					mStatusListener.OnSyncProgress(direction, showProgress, current, total, interactively);
			}
		}

		@Override
		public void onSyncCompleted(Synchronizer.SyncDirection direction, boolean showProgress, boolean interactively) {
			if (null != mNotificationManager)
				mNotificationManager.cancel(NOTIFICATION_ID);
			synchronized (mLocker) {
				if (null != mStatusListener)
					mStatusListener.onSyncCompleted(direction, showProgress, interactively);
			}
			if (!mSyncCommands.isEmpty()) {
				// process next command
				SyncCommand command = null;
				try {
					command = mSyncCommands.get(0);
					mSyncCommands.remove(0);
				} catch (Exception ignored) { }
				if (null != command) {
					log.d("Process next command: " + command.action);
					processSyncCommand(command.action, command.params);
				}
			} else {
				stopSelf();
			}
		}

		@Override
		public void onSyncError(Synchronizer.SyncDirection direction, String errorString) {
			if (null != mNotificationManager)
				mNotificationManager.cancel(NOTIFICATION_ID);
			synchronized (mLocker) {
				if (null != mStatusListener)
					mStatusListener.onSyncError(direction, errorString);
			}
			// forgot about unprocessed commands
			stopSelf();
		}

		@Override
		public void onAborted(Synchronizer.SyncDirection direction) {
			if (null != mNotificationManager)
				mNotificationManager.cancel(NOTIFICATION_ID);
			synchronized (mLocker) {
				if (null != mStatusListener)
					mStatusListener.onAborted(direction);
			}
			// forgot about unprocessed commands
			stopSelf();
		}

		@Override
		public void onSettingsLoaded(Properties settings, boolean interactively) {
			synchronized (mLocker) {
				if (null != mStatusListener)
					mStatusListener.onSettingsLoaded(settings, interactively);
			}
		}

		@Override
		public void onBookmarksLoaded(BookInfo bookInfo, boolean interactively) {
			synchronized (mLocker) {
				if (null != mStatusListener)
					mStatusListener.onBookmarksLoaded(bookInfo, interactively);
			}
		}

		@Override
		public void onCurrentBookInfoLoaded(FileInfo fileInfo, boolean interactively) {
			synchronized (mLocker) {
				if (null != mStatusListener)
					mStatusListener.onCurrentBookInfoLoaded(fileInfo, interactively);
			}
		}

		@Override
		public void onFileNotFound(FileInfo fileInfo) {
			synchronized (mLocker) {
				if (null != mStatusListener)
					mStatusListener.onFileNotFound(fileInfo);
			}
		}
	};
	private final Object mLocker = new Object();

	final private BroadcastReceiver mSyncActionReceiver = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			log.d("received action: " + action);
			if (SYNC_ACTION_CANCEL.equals(action)) {
				execTask(new Task("processSyncCommand(SYNC_ACTION_CANCEL)") {
					@Override
					public void work() {
						processSyncCommand(SYNC_ACTION_CANCEL, new Bundle());
					}
				});
			}
		}
	};

	public SyncService() {
		super("sync2");
	}

	@Override
	public void onCreate() {
		log.d("onCreate");
		super.onCreate();
		mNotificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
		IntentFilter filter = new IntentFilter();
		filter.addAction(SYNC_ACTION_CANCEL);
		registerReceiver(mSyncActionReceiver, filter);
	}

	@Override
	public void onStart(Intent intent, int startId) {
		// do nothing
	}

	@TargetApi(Build.VERSION_CODES.ECLAIR)
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		log.i("Received start id " + startId + ": " + intent);
		String action = "";
		if (null != intent) {
			action = intent.getAction();
		}
		boolean ready;
		synchronized (mLocker) {
			ready = null != mSynchronizer && null != mStatusListener && !mSynchronizer.isBusy();
		}
		if (ready) {
			// if Synchronizer instance ready - process command
			if (action.length() > 0) {
				log.d("ready: process command \"" + action + "\"");
				String finalAction = action;
				execTask(new Task("processSyncCommand") {
					@Override
					public void work() {
						processSyncCommand(finalAction, intent.getExtras());
					}
				});
			}
		} else {
			// otherwise add to list to process later
			log.d("adding command \"" + action + "\" to deferred list");
			if (action.length() > 0) {
				mSyncCommands.add(new SyncCommand(action, intent.getExtras()));
			}
		}
		return START_NOT_STICKY;
	}

	@Override
	public IBinder onBind(Intent intent) {
		log.i("onBind(): " + intent);
		return mBinder;
	}

	@Override
	public boolean onUnbind (Intent intent) {
		log.i("onUnbind(): " + intent);
		return false;
	}

	@Override
	public void onDestroy() {
		log.d("onDestroy");
		try {
			unregisterReceiver(mSyncActionReceiver);
		} catch (Exception ignored) {}
		synchronized (mLocker) {
			if (null != mSynchronizer) {
				if (mSynchronizer.isBusy())
					mSynchronizer.abort();
				mSynchronizer = null;
			}
		}
		if (null != mNotificationManager) {
			mNotificationManager.cancel(NOTIFICATION_ID);
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
				mNotificationManager.deleteNotificationChannel(NOTIFICATION_CHANNEL_ID);
			}
			mNotificationManager = null;
		}
		super.onDestroy();
	}

	@Override
	public void onLowMemory() {
		log.d("onLowMemory");
	}

	// ======================================
	// this service access functions (wrappers)

	public void setSynchronizer(Synchronizer synchronizer) {
		synchronized (mLocker) {
			mSynchronizer = synchronizer;
			mSynchronizer.setOnSyncStatusListener(mStatusListenerWrapper);
		}
		boolean ready;
		synchronized (mLocker) {
			ready = null != mSynchronizer && null != mStatusListener && !mSynchronizer.isBusy();
		}
		if (ready && !mSyncCommands.isEmpty()) {
			execTask(new Task("processSyncCommand") {
				@Override
				public void work() {
					if (null != mSynchronizer) {
						SyncCommand command = null;
						try {
							command = mSyncCommands.get(0);
							mSyncCommands.remove(0);
						} catch (Exception ignored) {}
						if (null != command) {
							log.d("Process deferred command: " + command.action);
							processSyncCommand(command.action, command.params);
						}
					}
				}
			});
		}
	}

	public void setOnSyncStatusListener(OnSyncStatusListener listener) {
		synchronized (mLocker) {
			mStatusListener = listener;
		}
		boolean ready;
		synchronized (mLocker) {
			ready = null != mSynchronizer && null != mStatusListener && !mSynchronizer.isBusy();
		}
		if (ready && !mSyncCommands.isEmpty()) {
			execTask(new Task("processSyncCommand") {
				@Override
				public void work() {
					if (null != mSynchronizer) {
						SyncCommand command = null;
						try {
							command = mSyncCommands.get(0);
							mSyncCommands.remove(0);
						} catch (Exception ignored) {}
						if (null != command) {
							log.d("Process deferred command: " + command.action);
							processSyncCommand(command.action, command.params);
						}
					}
				}
			});
		}
	}

	// private implementation

	private void processSyncCommand(String action, Bundle params) {
		synchronized (mLocker) {
			if (null != mSynchronizer) {
				switch (action) {
					case SYNC_ACTION_SYNCTO: {
						// args: BookInfo bookInfo, int flags
						BookInfo bookInfo = params.getParcelable("bookInfo");
						int flags = params.getInt("flags", 0);
						mSynchronizer.startSyncTo(bookInfo, flags);
						break;
					}
					case SYNC_ACTION_SYNCTO_ONLY: {
						// args: BookInfo bookInfo, int flags, Synchronizer.SyncTarget... targets
						BookInfo bookInfo = params.getParcelable("bookInfo");
						int flags = params.getInt("flags", 0);
						int[] targets_code = params.getIntArray("targets");
						Synchronizer.SyncTarget[] targets = null;
						if (null != targets_code) {
							int len = targets_code.length;
							targets = new Synchronizer.SyncTarget[len];
							for (int i = 0; i < len; i++) {
								targets[i] = Synchronizer.SyncTarget.fromOrdinal(targets_code[i]);
							}
						}
						mSynchronizer.startSyncToOnly(bookInfo, flags, targets);
						break;
					}
					case SYNC_ACTION_SYNCFROM: {
						// args: int flags
						int flags = params.getInt("flags", 0);
						mSynchronizer.startSyncFrom(flags);
						break;
					}
					case SYNC_ACTION_SYNCFROM_ONLY: {
						// args: int flags, Synchronizer.SyncTarget... targets
						int flags = params.getInt("flags", 0);
						int[] targets_code = params.getIntArray("targets");
						Synchronizer.SyncTarget[] targets = null;
						if (null != targets_code) {
							int len = targets_code.length;
							targets = new Synchronizer.SyncTarget[len];
							for (int i = 0; i < len; i++) {
								targets[i] = Synchronizer.SyncTarget.fromOrdinal(targets_code[i]);
							}
						}
						mSynchronizer.startSyncFromOnly(flags, targets);
						break;
					}
					case SYNC_ACTION_CANCEL:
						mSynchronizer.abort();
						break;
					case SYNC_ACTION_NOOP:
						// do nothing
						break;
				}
			}
		}
	}

	private Notification buildNotification(Synchronizer.SyncDirection direction, int current, int total) {
		String title = getString(R.string.app_name);
		Notification notification;
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
			Notification.Builder builder;
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
				builder = new Notification.Builder(this, NOTIFICATION_CHANNEL_ID);
				// create notification channel
				if (!mChannelCreated) {
					NotificationChannel channel = new NotificationChannel(NOTIFICATION_CHANNEL_ID, "CoolReader TTS", NotificationManager.IMPORTANCE_LOW);
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
			switch (direction) {
				case SyncFrom:
					builder = builder.setContentText(getString(R.string.cloud_synchronization_from_));
					break;
				case SyncTo:
					builder = builder.setContentText(getString(R.string.cloud_synchronization_to_));
					break;
				default:
					builder = builder.setContentText("Synchronization...");
					break;
			}
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				if (current >= 0 && total > 0)
					builder = builder.setProgress(total, current, false);
				else
					builder = builder.setProgress(total, current, true);
			}
			builder = builder.setOngoing(true);
			builder = builder.setAutoCancel(false);
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
				builder = builder.setPriority(Notification.PRIORITY_LOW);
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
				builder = builder.setShowWhen(false);
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT_WATCH) {
					builder = builder.setLocalOnly(true);
					// add actions
					// cancel
					PendingIntent cancelIntent = PendingIntent.getBroadcast(this, 0, new Intent(SYNC_ACTION_CANCEL), 0);
					Notification.Action.Builder actionBld = new Notification.Action.Builder(android.R.drawable.ic_menu_close_clear_cancel, getString(R.string.dlg_button_cancel), cancelIntent);
					Notification.Action actionCancel = actionBld.build();
					builder = builder.addAction(actionCancel);
					if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
						builder = builder.setSound(null, null);
						builder = builder.setColor(Color.GRAY);
						builder = builder.setVisibility(Notification.VISIBILITY_PUBLIC);
					}
				}
			} else
				builder = builder.setWhen(System.currentTimeMillis());
			// delete intent (no-op)
			PendingIntent delPendingIntent = PendingIntent.getBroadcast(this, 0, new Intent(SYNC_ACTION_NOOP), 0);
			builder = builder.setDeleteIntent(delPendingIntent);
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
				notification = builder.build();
			else
				notification = builder.getNotification();
		} else {
			notification = new Notification(R.drawable.cr3_logo_button, title, System.currentTimeMillis());
		}
		return notification;
	}

}
