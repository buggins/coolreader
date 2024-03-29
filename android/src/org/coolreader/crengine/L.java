/*
 * CoolReader for Android
 * Copyright (C) 2011,2012 Vadim Lopatin <coolreader.org@gmail.com>
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

import android.util.Log;

public class L {
	public static final String TAG = "cr3"; 
	public static String getThreadLabel() {
		return BackgroundThread.isGUIThread() ? "G|" : "B|";
	}
	public static void i(String msg) {
		Log.i(TAG, getThreadLabel() + msg);
	}
	public static void i(String msg, Exception e) {
		Log.i(TAG, getThreadLabel() + msg, e);
	}
	public static void w(String msg) {
		Log.w(TAG, getThreadLabel() + msg);
	}
	public static void w(String msg, Exception e) {
		Log.w(TAG, getThreadLabel() + msg, e);
	}
	public static void e(String msg) {
		Log.e(TAG, getThreadLabel() + msg);
	}
	public static void e(String msg, Exception e) {
		Log.e(TAG, getThreadLabel() + msg, e);
	}
	public static void d(String msg) {
		Log.d(TAG, getThreadLabel() + msg);
	}
	public static void d(String msg, Exception e) {
		Log.d(TAG, getThreadLabel() + msg, e);
	}
	public static void v(String msg) {
		Log.v(TAG, getThreadLabel() + msg);
	}
	public static void v(String msg, Exception e) {
		Log.v(TAG, getThreadLabel() + msg, e);
	}
	public static Logger create(String name) {
		return new LoggerImpl(name, Log.VERBOSE);
	}
	public static Logger create(String name, int level) {
		return new LoggerImpl(name, level);
	}
}

class LoggerImpl implements Logger {
	private final String name;
	private int level;
	public LoggerImpl( String name, int level ) {
		this.name = name;
		this.level = level;
	}
	public void setLevel( int level ) {
		this.level = level;
	}
	private String addName( String msg ) {
		return name + "| " + msg;
	}
	@Override
	public void i(String msg) {
		if (level <= Log.INFO)
			L.i(addName(msg));
	}
	@Override
	public void i(String msg, Exception e) {
		if (level <= Log.INFO)
			L.i(addName(msg), e);
	}
	@Override
	public void w(String msg) {
		if (level <= Log.WARN)
			L.w(addName(msg));
	}
	@Override
	public void w(String msg, Exception e) {
		if (level <= Log.WARN)
			L.w(addName(msg), e);
	}
	@Override
	public void e(String msg) {
		if (level <= Log.ERROR)
			L.e(addName(msg));
	}
	@Override
	public void e(String msg, Exception e) {
		if (level <= Log.ERROR)
			L.e(addName(msg), e);
	}
	@Override
	public void d(String msg) {
		if (level <= Log.DEBUG)
			L.d(addName(msg));
	}
	@Override
	public void d(String msg, Exception e) {
		if (level <= Log.DEBUG)
			L.d(addName(msg), e);
	}
	@Override
	public void v(String msg) {
		if (level <= Log.VERBOSE)
			L.v(addName(msg));
	}
	@Override
	public void v(String msg, Exception e) {
		if (level <= Log.VERBOSE)
			L.v(addName(msg), e);
	}
}
