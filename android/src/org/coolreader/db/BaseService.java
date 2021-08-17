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
