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
