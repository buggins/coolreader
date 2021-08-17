package org.coolreader.db;

import android.util.Log;

import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;
import org.coolreader.crengine.Utils;

public abstract class Task implements Runnable {
	public static final Logger log = L.create("task");
	public static final Logger vlog = L.create("task", Log.ASSERT);
	private final String name;

	public Task(String name) {
		this.name = name;
	}

	@Override
	public String toString() {
		return "Task[" + name + "]";
	}

	@Override
	public void run() {
		long ts = Utils.timeStamp();
		vlog.v(toString() + " started");
		try {
			work();
		} catch (Exception e) {
			log.e("Exception while running DB task in background", e);
		}
		vlog.v(toString() + " finished in " + Utils.timeInterval(ts) + " ms");
	}

	public abstract void work();
}
