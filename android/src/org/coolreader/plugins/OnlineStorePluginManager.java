package org.coolreader.plugins;

import java.util.HashMap;
import java.util.Map;

import org.coolreader.crengine.FileInfo;
import org.coolreader.plugins.litres.LitresPlugin;

public class OnlineStorePluginManager {
	private static Map<String, OnlineStoreWrapper> pluginMap = new HashMap<String, OnlineStoreWrapper>();
	public static OnlineStoreWrapper getPlugin(String path) {
		if (!path.startsWith(FileInfo.ONLINE_CATALOG_PLUGIN_PREFIX))
			path = FileInfo.ONLINE_CATALOG_PLUGIN_PREFIX + path;
		int pos = path.indexOf(":");
		String packageName = path.substring(pos + 1);
		OnlineStoreWrapper wrapper = pluginMap.get(packageName);
		if (wrapper == null) {
			if (LitresPlugin.PACKAGE_NAME.equals(packageName))
				wrapper = new OnlineStoreWrapper(new LitresPlugin());
			if (wrapper != null)
				pluginMap.put(packageName, wrapper);
		}
		return wrapper;
	}
}
