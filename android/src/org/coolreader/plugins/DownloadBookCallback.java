package org.coolreader.plugins;

import java.io.File;

public interface DownloadBookCallback extends ErrorCallback {
	void onBookDownloaded(OnlineStoreBook book, boolean trial, File savedFileName);
}
