package org.coolreader.plugins;

import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.Scanner;

public class OnlineStoreWrapper {
	private OnlineStorePlugin plugin;
	public OnlineStoreWrapper(OnlineStorePlugin plugin) {
		this.plugin = plugin;
	}
	public FileInfo createRootDirectory() {
		final FileInfo root = Scanner.createOnlineLibraryPluginItem(plugin.getPackageName(), plugin.getDescription());
		root.addDir(Scanner.createOnlineLibraryPluginItem(plugin.getPackageName() + ":genres", "Books by genres"));
		root.addDir(Scanner.createOnlineLibraryPluginItem(plugin.getPackageName() + ":authors", "Books by authors"));
		root.addDir(Scanner.createOnlineLibraryPluginItem(plugin.getPackageName() + ":popular", "Popular"));
		root.addDir(Scanner.createOnlineLibraryPluginItem(plugin.getPackageName() + ":my", "My books"));
		return root;
	}
	public AsyncOperationControl openDirectory(final FileInfo dir, final FileInfoCallback callback) {
		AsyncOperationControl control = new AsyncOperationControl();
		if (!plugin.getPackageName().equals(dir.getOnlineCatalogPluginPackage())) {
			control.finished();
			callback.onError(0, "wrong plugin");
			return control;
		}
		String path = dir.getOnlineCatalogPluginPath();
		if (path == null) {
			control.finished();
			callback.onError(0, "wrong path");
			control.finished();
			return control;
		}
		if ("genres".equals(path)) {
			plugin.fillGenres(control, dir, callback);
			control.finished();
			callback.onFileInfoReady(dir);
			return control;
		} else if (path.startsWith("genres:")) {
			String genre = dir.getOnlineCatalogPluginId();
			plugin.fillGenres(control, dir, callback);
			control.finished();
			callback.onFileInfoReady(dir);
			return control;
		} else if ("authors".equals(path)) {
			// TODO
		} else {
			
		}
			
		control.finished();
		callback.onFileInfoReady(dir);
		return control;
	}
	public AsyncOperationControl authenticate(String login, String password, AuthenticationCallback callback) {
		AsyncOperationControl control = new AsyncOperationControl();
		plugin.authenticate(control, login, password, callback);
		return control;
	}
}
