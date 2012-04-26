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
	
	private Handler handler;
	private Runnable currentTask;
	private String name;

	public static DelayedExecutor createBackground(String name) {
		Handler handler = BackgroundThread.getBackgroundHandler();
		if (handler == null) {
			log.e("Cannot get background thread handler");
			return null;
		}
		DelayedExecutor task = new DelayedExecutor(handler, name);
		return task;
	}
	
	public static DelayedExecutor createGUI(String name) {
		Handler handler = BackgroundThread.getGUIHandler();
		if (handler == null) {
			log.e("Cannot get GUI thread handler");
			return null;
		}
		DelayedExecutor task = new DelayedExecutor(handler, name);
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
				handler.removeCallbacks(currentTask); // cancel pending task, replace with new one
			}
			currentTask = myTask;
			if (delay > 0) {
				log.d("Posting delayed task " + currentTask + " delay=" + delay);
				handler.postDelayed(currentTask, delay);
			} else {
				log.d("Posting task " + currentTask);
				handler.post(currentTask);
			}
		}
	}

	public void cancel() {
		synchronized(this) {
			if (currentTask != null) {
				log.d("Cancelling pending task " + currentTask);
				handler.removeCallbacks(currentTask); // cancel pending task, replace with new one
				currentTask = null;
			}
		}
	}
	
	private DelayedExecutor(Handler handler, String name) {
		this.handler = handler;
		this.name = name;
	}
}
