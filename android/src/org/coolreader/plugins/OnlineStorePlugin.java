/*
 * CoolReader for Android
 * Copyright (C) 2012,2014 Vadim Lopatin <coolreader.org@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.coolreader.plugins;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;

import org.coolreader.crengine.FileInfo;

public interface OnlineStorePlugin {
	String getPackageName();
	String getName();
	String getUrl();
	String getDescription();
	String getLogin();
	String getPassword();
	String getFirstAuthorNameLetters();
	String getAccountRefillUrl();
	// return null if no new account creation is supported
	ArrayList<OnlineStoreRegistrationParam> getNewAccountParameters();
	void registerNewAccount(AsyncOperationControl control, HashMap<String, String> params, AuthenticationCallback callback);
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
