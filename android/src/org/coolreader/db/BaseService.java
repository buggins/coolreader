/*
 * CoolReader for Android
 * Copyright (C) 2012 Vadim Lopatin <coolreader.org@gmail.com>
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

package org.coolreader.db;

import android.app.Service;
import android.os.Handler;
import android.util.Log;

import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;

public abstract class BaseService extends Service {
	public static final Logger vlog = L.create("bssvc", Log.ASSERT);

	private ServiceThread mThread;
	private final String mName;

	public BaseService(String name) {
		super();
		mName = name;
	}

	@Override
	public void onCreate() {
		mThread = new ServiceThread(mName);
		mThread.start();
	}

	@Override
	public void onDestroy() {
		mThread.stop(5000);
	}

	/**
	 * Execute runnable in service background thread.
	 * Exceptions will be ignored, just dumped into log.
	 * @param task is Runnable to execute
	 */
	protected void execTask(final Task task) {
		vlog.v("Posting task " + task);
		mThread.post(task);
	}

	/**
	 * Execute runnable in service background thread, delayed.
	 * Exceptions will be ignored, just dumped into log.
	 * @param task is Runnable to execute
	 */
	protected void execTask(final Task task, long delay) {
		vlog.v("Posting task " + task + " with delay " + delay);
		mThread.postDelayed(task, delay);
	}

	/**
	 * Send task to handler, if specified, otherwise run immediately.
	 * Exceptions will be ignored, just dumped into log.
	 * @param handler is handler to send task to, null to run immediately
	 * @param task is Runnable to execute
	 */
	protected void sendTask(Handler handler, Runnable task) {
		try {
			if (handler != null) {
				vlog.v("Senging task to " + handler.toString());
				handler.post(task);
			} else {
				vlog.v("No Handler provided: executing task in current thread");
				task.run();
			}
		} catch (Exception e) {
			vlog.e("Exception in callback", e);
		}
	}

}
