package org.coolreader.plugins;

import org.coolreader.crengine.FileInfo;

public interface OnlineStorePlugin {
	String getPackageName();
	String getDescription();
	String getLogin();
	String getPassword();
	String getFirstAuthorNameLetters();
	void authenticate(AsyncOperationControl control, String login, String password, AuthenticationCallback callback);
	void fillGenres(AsyncOperationControl control, FileInfo dir, FileInfoCallback callback);
	void getBooksForGenre(AsyncOperationControl control, FileInfo dir, String genreId, FileInfoCallback callback);
	void getBooksByAuthor(AsyncOperationControl control, FileInfo dir, String authorId, FileInfoCallback callback);
	void getAuthorsByPrefix(AsyncOperationControl control, FileInfo dir, String prefix, FileInfoCallback callback);
}
