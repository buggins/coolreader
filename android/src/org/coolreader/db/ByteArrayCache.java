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

import java.util.ArrayList;


public class ByteArrayCache {

	public ByteArrayCache(int maxSize) {
		this.maxSize = maxSize;
	}
	
	public void put(String id, byte[] data) {
		int index = find(id);
		if (index >= 0) {
			ByteArrayItem item = list.get(index);
			if (item.data != null)
				currentSize -= item.data.length;
			item.data = data;
			if (item.data != null)
				currentSize += item.data.length;
			moveOnTop(index);
		} else {
			ByteArrayItem item = new ByteArrayItem(id, data);
			list.add(item);
			if (item.data != null)
				currentSize += item.data.length;
		}
		checkSize();
	}
	
	public byte[] get(String id) {
		int index = find(id);
		if (index < 0)
			return null;
		ByteArrayItem item = list.get(index);
		moveOnTop(index);
		return item.data;
	}
	
	public void remove(String id) {
		int index = find(id);
		if (index < 0)
			return;
		list.remove(index);
	}
	
	public void clear() {
		list.clear();
	}

	private static class ByteArrayItem {
		public String id;
		public byte[] data;
		public ByteArrayItem(String id, byte[] data) {
			this.id = id;
			this.data = data;
		}
	}
	
	private int maxSize;
	private int currentSize;
	private ArrayList<ByteArrayItem> list = new ArrayList<ByteArrayItem>();

	private int find(String id) {
		for (int i=0; i<list.size(); i++)
			if (list.get(i).id.equals(id))
				return i;
		return -1;
	}

	private void moveOnTop(int index) {
		if (index >= list.size() - 1)
			return; // already on top
		ByteArrayItem item = list.remove(index);
		list.add(item);
	}

	private void checkSize() {
		int extraSize = currentSize - maxSize;
		if (extraSize < maxSize / 10)
			return;
		ArrayList<Integer> itemsToRemove = new ArrayList<Integer>(list.size() / 2);
		for (int i=0; i<list.size() - 1; i++) {
			ByteArrayItem item = list.get(i);
			if (item.data == null || item.data.length == 0)
				continue;
			extraSize -= item.data.length;
			itemsToRemove.add(i);
			if (extraSize < 0)
				break;
		}
		for (int i = itemsToRemove.size() - 1; i >= 0; i--) {
			ByteArrayItem item = list.get(itemsToRemove.get(i));
			list.remove(itemsToRemove.get(i));
			item.data = null; // faster GC
		}
	}
}
