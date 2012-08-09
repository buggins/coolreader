package org.coolreader.plugins;

import org.coolreader.crengine.FileInfo;

public interface OnlineStorePlugin {
	String getPackageName();
	String getDescription();
	String getLogin();
	String getPassword();
	void authenticate(AsyncOperationControl control, String login, String password, AuthenticationCallback callback);
	void fillGenres(AsyncOperationControl control, FileInfo dir, FileInfoCallback callback);
	void getBooksForGenre(AsyncOperationControl control, FileInfo dir, String genreId, FileInfoCallback callback);
}
