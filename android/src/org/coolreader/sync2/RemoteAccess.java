/*
 *   Copyright (C) 2020 by Chernov A.A.
 *   valexlin@gmail.com
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.coolreader.sync2;

import android.content.Intent;
import android.os.Bundle;
import android.util.Pair;

import java.io.InputStream;

public interface RemoteAccess {
	/**
	 * Sign in into account quietly (non interactively), for example, using cached previous successfully sign-in result.
	 *
	 * @param completedListener
	 * @return true if quietly sign-in operation successfull.
	 */
	boolean signInQuietly(OnSignInListener completedListener);

	/**
	 * Sign in into account, possible interactively, for example, using some activities or forms.
	 *
	 * @param params            parameters for sign-in operation, such as, application name, etc.
	 * @param completedListener
	 */
	void signIn(Bundle params, OnSignInListener completedListener);

	/**
	 * Sign out from account, possible interactively, for example, using some activities or forms.
	 *
	 * @param params
	 * @param completedListener
	 */
	void signOut(Bundle params, OnSignOutListener completedListener);

	/**
	 * Fetch list of child item in specified path.
	 *
	 * @param filePath         Full file path to folder content to get.
	 * @param completeListener
	 */
	void list(String filePath, OnOperationCompleteListener<FileMetadataList> completeListener);

	/**
	 * Create directory on remove service.
	 *
	 * @param filePath         full file path of created directory.
	 * @param completeListener
	 */
	void mkdir(String filePath, OnOperationCompleteListener<FileMetadata> completeListener);

	/**
	 * Create directory on remove service recursively, i.e. also created all parent directories if necessary.
	 *
	 * @param filePath         full file path of created directory.
	 * @param completeListener
	 */
	void mkdir_recursively(String filePath, OnOperationCompleteListener<FileMetadata> completeListener);

	/**
	 * Retrieve information about file or directory on remote service.
	 *
	 * @param filePath         full file path to file or directory.
	 * @param completeListener
	 */
	void stat(String filePath, OnOperationCompleteListener<FileMetadata> completeListener);

	/**
	 * Read file content from file on remote service.
	 *
	 * @param filePath         full file path to file.
	 * @param completeListener
	 */
	void readFile(String filePath, OnOperationCompleteListener<InputStream> completeListener);

	/**
	 * Write data to file on remote service.
	 *
	 * @param filePath         full file path to file.
	 * @param data             Data to write.
	 * @param completeListener
	 */
	void writeFile(String filePath, byte[] data, OnOperationCompleteListener<Boolean> completeListener);

	/**
	 * Move file or directory to trash.
	 *
	 * @param filePath         full file path to file or directory.
	 * @param completeListener
	 */
	void trash(String filePath, OnOperationCompleteListener<Boolean> completeListener);

	/**
	 * Permanently deletes file or directory.
	 *
	 * @param filePath         full file path to file or directory.
	 * @param completeListener
	 */
	void delete(String filePath, OnOperationCompleteListener<Boolean> completeListener);

	/**
	 * Get file metadata and its contents from remote service.
	 *
	 * @param filePath         full file path to file or directory.
	 * @param completeListener
	 */
	void getFile(String filePath, OnOperationCompleteListener<Pair<FileMetadata, InputStream>> completeListener);

	/**
	 * @param requestCode
	 * @param resultCode
	 * @param data
	 * @brief Helper function to handle some operation results from any remote service activity.
	 * Must be called from main activity in function onActivityResult().
	 */
	void onActivityResultHandler(int requestCode, int resultCode, Intent data);

}
