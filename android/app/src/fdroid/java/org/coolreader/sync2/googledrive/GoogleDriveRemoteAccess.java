/*
 *   Copyright (C) 2020-2021 by Chernov A.A.
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

package org.coolreader.sync2.googledrive;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Pair;

import org.coolreader.sync2.FileMetadata;
import org.coolreader.sync2.FileMetadataList;
import org.coolreader.sync2.OnOperationCompleteListener;
import org.coolreader.sync2.OnSignInListener;
import org.coolreader.sync2.OnSignOutListener;
import org.coolreader.sync2.RemoteAccess;

import java.io.InputStream;
import java.util.Map;

@TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
public class GoogleDriveRemoteAccess implements RemoteAccess {

	public static final class NotImplementedException extends RuntimeException {
		public NotImplementedException() {
			super("Not implemented");
		}
	}

	// Assigned activity
	private Activity m_activity = null;

	public GoogleDriveRemoteAccess(Activity activity) {
		m_activity = activity;
	}

	public GoogleDriveRemoteAccess(Activity activity, int keepAlive) {
		m_activity = activity;
	}

	@Override
	public boolean signInQuietly(OnSignInListener completedListener) {
		return false;
	}

	@Override
	public void signIn(Bundle params, OnSignInListener completedListener) {
		if (null != completedListener)
			completedListener.onSignInCompleted(null, 255);
	}

	@Override
	public void signOut(Bundle params, OnSignOutListener completedListener) {
		if (null != completedListener)
			completedListener.onSignOutCompleted(255);
	}


	// RemoteAccess implementation

	@Override
	public void discardDirCache() {
	}

	@Override
	public void list(String filePath, boolean useCache, OnOperationCompleteListener<FileMetadataList> completeListener) {
		if (null != completeListener) {
			completeListener.onCompleted(null, false);
			completeListener.onFailed(new NotImplementedException());
		}
	}

	@Override
	public void mkdir(String filePath, OnOperationCompleteListener<FileMetadata> completeListener) {
		if (null != completeListener) {
			completeListener.onCompleted(null, false);
			completeListener.onFailed(new NotImplementedException());
		}
	}

	@Override
	public void mkdir_recursively(String filePath, OnOperationCompleteListener<FileMetadata> completeListener) {
		if (null != completeListener) {
			completeListener.onCompleted(null, false);
			completeListener.onFailed(new NotImplementedException());
		}
	}

	@Override
	public void stat(String filePath, boolean useCache, OnOperationCompleteListener<FileMetadata> completeListener) {
		if (null != completeListener) {
			completeListener.onCompleted(null, false);
			completeListener.onFailed(new NotImplementedException());
		}
	}

	@Override
	public void readFile(String filePath, OnOperationCompleteListener<InputStream> completeListener) {
		if (null != completeListener) {
			completeListener.onCompleted(null, false);
			completeListener.onFailed(new NotImplementedException());
		}
	}

	@Override
	public void writeFile(String filePath, byte[] data, Map<String, String> customProps, OnOperationCompleteListener<Boolean> completeListener) {
		if (null != completeListener) {
			completeListener.onCompleted(null, false);
			completeListener.onFailed(new NotImplementedException());
		}
	}

	@Override
	public void trash(String filePath, OnOperationCompleteListener<Boolean> completeListener) {
		if (null != completeListener) {
			completeListener.onCompleted(null, false);
			completeListener.onFailed(new NotImplementedException());
		}
	}

	@Override
	public void delete(String filePath, OnOperationCompleteListener<Boolean> completeListener) {
		if (null != completeListener) {
			completeListener.onCompleted(null, false);
			completeListener.onFailed(new NotImplementedException());
		}
	}

	@Override
	public void getFile(String filePath, OnOperationCompleteListener<Pair<FileMetadata, InputStream>> completeListener) {
		if (null != completeListener) {
			completeListener.onCompleted(null, false);
			completeListener.onFailed(new NotImplementedException());
		}
	}

	// End of RemoteAccess implementation


	/**
	 * @param requestCode
	 * @param resultCode
	 * @param data
	 * @brief Helper function to handle some operation results from Google Service activity.
	 * Must be called from main activity in function onActivityResult().
	 */
	public void onActivityResultHandler(int requestCode, int resultCode, Intent data) {
	}

	@Override
	public boolean needSignInRepeat() {
		return false;
	}

}
