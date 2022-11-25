/*
 * CoolReader for Android
 * Copyright (C) 2012,2014 Vadim Lopatin <coolreader.org@gmail.com>
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

import java.util.HashMap;
import java.util.Map;

import org.coolreader.crengine.FileInfo;
import org.coolreader.plugins.litres.LitresPlugin;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;

public class OnlineStorePluginManager {
	private static Map<String, OnlineStoreWrapper> pluginMap = new HashMap<String, OnlineStoreWrapper>();
	public static OnlineStoreWrapper getPlugin(Activity activity, String path) {
		if (!path.startsWith(FileInfo.ONLINE_CATALOG_PLUGIN_PREFIX))
			path = FileInfo.ONLINE_CATALOG_PLUGIN_PREFIX + path;
		int pos = path.indexOf(":");
		String packageName = path.substring(pos + 1);
		OnlineStoreWrapper wrapper = pluginMap.get(packageName);
		if (wrapper == null) {
			SharedPreferences preferences = null;
			if (activity != null)
				preferences = activity.getSharedPreferences(FileInfo.ONLINE_CATALOG_PLUGIN_PREFIX, Context.MODE_PRIVATE);
			if (LitresPlugin.PACKAGE_NAME.equals(packageName))
				wrapper = new OnlineStoreWrapper(new LitresPlugin(activity, preferences));
			if (wrapper != null)
				pluginMap.put(packageName, wrapper);
		}
		return wrapper;
	}
	public static final String PLUGIN_PKG_LITRES = "org.coolreader.plugins.litres";
}
