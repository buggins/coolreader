package org.coolreader.plugins;

import org.coolreader.crengine.FileInfo;

public interface FileInfoCallback extends ErrorCallback {
	void onFileInfoReady(FileInfo fileInfo);
}
