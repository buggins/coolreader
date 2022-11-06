/*
 * CoolReader for Android
 * Copyright (C) 2021 Aleksey Chernov <valexlin@gmail.com>
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
