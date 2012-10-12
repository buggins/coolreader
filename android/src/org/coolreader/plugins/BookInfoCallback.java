package org.coolreader.plugins;

public interface BookInfoCallback extends ErrorCallback {
	void onBookInfoReady(OnlineStoreBookInfo bookInfo);
}
