package org.coolreader.sync2;

import android.os.Binder;

import org.coolreader.crengine.BookInfo;

/**
 * SyncService is used only by the local application and does not need to work across processes.
 * So just extending android.os.Binder class to work with service.
 * https://developer.android.com/guide/components/bound-services#Binder
 */

public class SyncServiceBinder extends Binder {

	public interface Callback {
		void run(SyncServiceBinder binder);
	}

	private final SyncService mService;

	public SyncServiceBinder(SyncService service) {
		mService = service;
	}

	public void setSynchronizer(Synchronizer synchronizer) {
		mService.setSynchronizer(synchronizer);
	}

	public void setOnSyncStatusListener(OnSyncStatusListener listener) {
		mService.setOnSyncStatusListener(listener);
	}

	public void startSyncTo(BookInfo bookInfo, int flags) {
		mService.startSyncTo(bookInfo, flags);
	}

	public void startSyncFrom(int flags) {
		mService.startSyncFrom(flags);
	}

	public void startSyncFromOnly(int flags, Synchronizer.SyncTarget... targets) {
		mService.startSyncFromOnly(flags, targets);
	}

	public void startSyncToOnly(BookInfo bookInfo, int flags, Synchronizer.SyncTarget... targets) {
		mService.startSyncToOnly(bookInfo, flags, targets);
	}

}
