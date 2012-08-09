package org.coolreader.plugins;

import org.coolreader.crengine.FileInfo;

public interface FileInfoCallback {
	void onFileInfoReady(FileInfo fileInfo);
	void onError(int errorCode, String description);
}
