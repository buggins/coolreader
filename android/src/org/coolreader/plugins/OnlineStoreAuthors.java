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

package org.coolreader.plugins;

import org.coolreader.crengine.Utils;

import java.util.ArrayList;
import java.util.Collections;


public class OnlineStoreAuthors implements AsyncResponse {
	private ArrayList<OnlineStoreAuthor> list = new ArrayList<>();
	public void add(OnlineStoreAuthor author) {
		list.add(author);
	}
	public int size() {
		return list.size();
	}
	public OnlineStoreAuthor get(int index) {
		return list.get(index);
	}
	public void sortByName() {
		Collections.sort(list, (lhs, rhs) -> Utils.cmp(lhs.lastName, rhs.lastName));
	}
}