/*
 * CoolReader for Android
 * Copyright (C) 2012 Vadim Lopatin <coolreader.org@gmail.com>
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

import java.util.LinkedList;

import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;

/**
 * Thread to run background tasks inside.
 * @author Vadim Lopatin
 */
public class ServiceThread extends Thread {

	public static final Logger log = L.create("st");
	
	public ServiceThread(String name) {
		super(name);
	}

	/**
	 * Post task for execution. 
	 * @param task is runnable to call
	 */
	public void post(Runnable task) {
		synchronized (mQueue) {
			if (mHandler == null || mStopped) {
				log.w("Thread is not yet started, just adding to queue " + task);
				mQueue.addLast(task);
			} else {
				postQueuedTasks();
				mHandler.post(task);
			}
		}
	}
	
	/**
	 * Post task for execution at front of queue. 
	 * @param task is runnable to call
	 */
	public void postAtFrontOfQueue(Runnable task) {
		synchronized (mQueue) {
			if (mHandler == null || mStopped)
				mQueue.addLast(task);
			else {
				postQueuedTasks();
				mHandler.postAtFrontOfQueue(task);
			}
		}
	}
	
	/**
	 * Post task for execution with delay. 
	 * @param task is runnable to call
	 */
	public void postDelayed(Runnable task, long delayMillis) {
		synchronized (mQueue) {
			if (mHandler == null || mStopped)
				mQueue.addLast(task);
			else {
				postQueuedTasks();
				mHandler.postDelayed(task, delayMillis);
			}
		}
	}
	
	private void postQueuedTasks() {
		while (mQueue.size() > 0) {
			Runnable t = mQueue.removeFirst();
			log.w("Executing queued task " + t);
			mHandler.post(t);
		}
	}
	
	public boolean waitForCompletion(long timeout) {
		final Object lock = new Object();
		synchronized (lock) {
			mHandler.post(() -> {
				synchronized (lock) {
					lock.notify();
				}
			});
			try {
				lock.wait(timeout);
				return true;
			} catch (InterruptedException e) {
				L.i("Waiting is interrupted");
			}
		}
		return false;
	}
	
	public void stop(final long timeout) {
		L.i("Stop is called. Not supported.");
		waitForCompletion(timeout);
		mHandler.getLooper().quit();
	}
	
	public boolean isStopped() {
		return mStopped;
	}

	private LinkedList<Runnable> mQueue = new LinkedList<Runnable>();

	@Override
	public void run() {
		log.i("Running service thread");
		Looper.prepare();
		mHandler = new Handler() {
			public void handleMessage( Message message )
			{
				log.d("message: " + message);
			}
		};
		log.i("Service thread handler is created");
		synchronized (mQueue) {
			postQueuedTasks();
			mStopped = false;
		}
		Looper.loop();
		mHandler = null;
		log.i("Exiting background thread");
	}
	private Handler mHandler;
	private boolean mStopped = true;
}
