package org.coolreader.plugins;

import java.io.File;

import org.coolreader.crengine.FileInfo;

public interface OnlineStorePlugin {
	String getPackageName();
	String getDescription();
	String getLogin();
	String getPassword();
	String getFirstAuthorNameLetters();
	void authenticate(AsyncOperationControl control, String login, String password, AuthenticationCallback callback);
	void fillGenres(AsyncOperationControl control, FileInfo dir, FileInfoCallback callback);
	void getBookInfo(AsyncOperationControl control, String bookId, boolean myOnly, BookInfoCallback callback);
	void getBooksForGenre(AsyncOperationControl control, FileInfo dir, String genreId, FileInfoCallback callback);
	void getPurchasedBooks(AsyncOperationControl control, FileInfo dir, FileInfoCallback callback);
	void getPopularBooks(AsyncOperationControl control, FileInfo dir, FileInfoCallback callback);
	void getNewBooks(AsyncOperationControl control, FileInfo dir, FileInfoCallback callback);
	void getBooksByAuthor(AsyncOperationControl control, FileInfo dir, String authorId, FileInfoCallback callback);
	void getAuthorsByPrefix(AsyncOperationControl control, FileInfo dir, String prefix, FileInfoCallback callback);
	void purchaseBook(AsyncOperationControl control, String bookId, PurchaseBookCallback callback);
	void downloadBook(AsyncOperationControl control, OnlineStoreBook book, boolean trial, File fileToSave, DownloadBookCallback callback);
}
