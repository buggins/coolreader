/*
 *   Copyright (C) 2020 by Chernov A.A.
 *   valexlin@gmail.com
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
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

import org.coolreader.crengine.BookInfo;
import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.Properties;

// Synchronization status listener interface
public interface OnSyncStatusListener {
	void onSyncStarted(Synchronizer.SyncDirection direction, boolean showProgress, boolean interactively);

	void OnSyncProgress(Synchronizer.SyncDirection direction, boolean showProgress, int current, int total, boolean interactively);

	void onSyncCompleted(Synchronizer.SyncDirection direction, boolean showProgress, boolean interactively);

	void onSyncError(Synchronizer.SyncDirection direction, String errorString);

	void onAborted(Synchronizer.SyncDirection direction);

	void onSettingsLoaded(Properties settings, boolean interactively);

	void onBookmarksLoaded(BookInfo bookInfo, boolean interactively);

	void onCurrentBookInfoLoaded(FileInfo fileInfo, boolean interactively);

	void onFileNotFound(FileInfo fileInfo);
}
