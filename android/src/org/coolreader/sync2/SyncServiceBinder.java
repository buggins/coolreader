package org.coolreader.sync2;

import android.os.Binder;

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

}
