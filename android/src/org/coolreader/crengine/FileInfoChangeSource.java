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

import java.util.HashSet;
import java.util.Set;

public class FileInfoChangeSource implements FileInfoChangeListener {
	Set<FileInfoChangeListener> listeners = new HashSet<FileInfoChangeListener>();
	void addListener(FileInfoChangeListener listener) {
		listeners.add(listener);
	}
	void removeListener(FileInfoChangeListener listener) {
		listeners.remove(listener);
	}
	@Override
	public void onChange(FileInfo object, boolean filePropsOnlyChange) {
		for (FileInfoChangeListener listener : listeners)
			listener.onChange(object, filePropsOnlyChange);
	}
}
