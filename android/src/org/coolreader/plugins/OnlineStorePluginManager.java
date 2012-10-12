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
				wrapper = new OnlineStoreWrapper(new LitresPlugin(preferences));
			if (wrapper != null)
				pluginMap.put(packageName, wrapper);
		}
		return wrapper;
	}
	public static final String PLUGIN_PKG_LITRES = "org.coolreader.plugins.litres";
}
