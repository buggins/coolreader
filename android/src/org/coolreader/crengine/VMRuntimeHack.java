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

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import android.util.Log;

public class VMRuntimeHack {
	private Object runtime = null;
	private Method trackAllocation = null;
	private Method trackFree = null;
	private static int totalSize = 0;
	
	public boolean trackAlloc(long size) {
		if (runtime == null)
			return false;
		totalSize += size;
		L.v("trackAlloc(" + size + ")  total=" + totalSize);
		try {
			Object res = trackAllocation.invoke(runtime, Long.valueOf(size));
			return (res instanceof Boolean) ? (Boolean)res : true;
		} catch (IllegalArgumentException e) {
			return false;
		} catch (IllegalAccessException e) {
			return false;
		} catch (InvocationTargetException e) {
			return false;
		}
	}
	public boolean trackFree(long size) {
		if (runtime == null)
			return false;
		totalSize -= size;
		L.v("trackFree(" + size + ")  total=" + totalSize);
		try {
			Object res = trackFree.invoke(runtime, Long.valueOf(size));
			return (res instanceof Boolean) ? (Boolean)res : true;
		} catch (IllegalArgumentException e) {
			return false;
		} catch (IllegalAccessException e) {
			return false;
		} catch (InvocationTargetException e) {
			return false;
		}
	}
	public VMRuntimeHack() {
		if (!DeviceInfo.USE_BITMAP_MEMORY_HACK)
			return;
		boolean success = false;
		try {
			Class<?> cl = Class.forName("dalvik.system.VMRuntime");
			Method getRt = cl.getMethod("getRuntime", new Class[0]);
			runtime = getRt.invoke(null, new Object[0]);
			trackAllocation = cl.getMethod("trackExternalAllocation", new Class[] {long.class});
			trackFree = cl.getMethod("trackExternalFree", new Class[] {long.class});
			success = true;
		} catch (ClassNotFoundException e) {
		} catch (SecurityException e) {
		} catch (NoSuchMethodException e) {
		} catch (IllegalArgumentException e) {
		} catch (IllegalAccessException e) {
		} catch (InvocationTargetException e) {
		}
		if (!success) {
			Log.i("cr3", "VMRuntime hack does not work!");
			runtime = null;
			trackAllocation = null;
			trackFree = null;
		}
	}
}
