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

package org.coolreader.crengine;

import android.os.Handler;
import android.util.Log;

/**
 * Schedulable cancelable replaceable task.
 * Can schedule execution of runnable.
 * When previously scheduled runnable was not yet executed, it's being canceled and being replaced with new one.
 */
public class DelayedExecutor {

	public static final Logger log = L.create("dt", Log.INFO);
	
	private boolean isBackground;
	private Handler handler;
	private Runnable currentTask;
	private String name;

	private Handler getHandler() {
		if (handler != null)
			return handler;
		if (isBackground)
			handler = BackgroundThread.getBackgroundHandler();
		else
			handler = BackgroundThread.getGUIHandler();
		if (handler == null)
			throw new RuntimeException("Cannot get handler");
		return handler;
	}
	
	public static DelayedExecutor createBackground(String name) {
		DelayedExecutor task = new DelayedExecutor(true, name);
		return task;
	}
	
	public static DelayedExecutor createGUI(String name) {
		DelayedExecutor task = new DelayedExecutor(false, name);
		return task;
	}
	
	/**
	 * Run ASAP.
	 * @param task is task to execute.
	 */
	public void post(final Runnable task) {
		postDelayed(task, 0L);
	}

	/**
	 * Run delayed.
	 * @param task is task to execute delayed.
	 * @param delay is delay, milliseconds.
	 */
	public void postDelayed(final Runnable task, final long delay) {
		Runnable myTask = new Runnable() {
			@Override
			public void run() {
				try {
					if (currentTask != null) {
						log.v("Running task " + toString());
						task.run();
						log.v("Done task " + toString());
					} else {
						log.w("Skipping probably canceled task " + toString());
					}
				} catch (Exception e) {
					log.e("Exception while executing task", e);
				}
			}

			@Override
			public String toString() {
				return name + " " + task.hashCode();
			}
		};
		synchronized(this) {
			if (currentTask != null) {
				log.d("Cancelling pending task " + currentTask);
				getHandler().removeCallbacks(currentTask); // cancel pending task, replace with new one
			}
			currentTask = myTask;
			if (delay > 0) {
				log.d("Posting delayed task " + currentTask + " delay=" + delay);
				getHandler().postDelayed(currentTask, delay);
			} else {
				log.d("Posting task " + currentTask);
				getHandler().post(currentTask);
			}
		}
	}

	public void cancel() {
		synchronized(this) {
			if (currentTask != null) {
				log.d("Cancelling pending task " + currentTask);
				getHandler().removeCallbacks(currentTask); // cancel pending task, replace with new one
				currentTask = null;
			}
		}
	}
	
	private DelayedExecutor(boolean isBackground, String name) {
		this.isBackground = isBackground;
		this.name = name;
	}
}
