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

package org.coolreader.sync2.googledrive;

import android.app.Activity;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.util.Pair;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;

import com.google.android.gms.auth.api.signin.GoogleSignIn;
import com.google.android.gms.auth.api.signin.GoogleSignInAccount;
import com.google.android.gms.auth.api.signin.GoogleSignInClient;
import com.google.android.gms.auth.api.signin.GoogleSignInOptions;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.common.api.CommonStatusCodes;
import com.google.android.gms.common.api.Scope;
import com.google.android.gms.tasks.OnFailureListener;
import com.google.android.gms.tasks.OnSuccessListener;
import com.google.android.gms.tasks.RuntimeExecutionException;
import com.google.android.gms.tasks.Task;
import com.google.android.gms.tasks.Tasks;
import com.google.api.client.extensions.android.http.AndroidHttp;
import com.google.api.client.googleapis.extensions.android.gms.auth.GoogleAccountCredential;
import com.google.api.client.http.ByteArrayContent;
import com.google.api.client.json.gson.GsonFactory;
import com.google.api.client.util.DateTime;
import com.google.api.services.drive.Drive;
import com.google.api.services.drive.DriveScopes;
import com.google.api.services.drive.model.File;
import com.google.api.services.drive.model.FileList;

import org.coolreader.sync2.FileMetadata;
import org.coolreader.sync2.FileMetadataList;
import org.coolreader.sync2.OnOperationCompleteListener;
import org.coolreader.sync2.OnSignInListener;
import org.coolreader.sync2.OnSignOutListener;
import org.coolreader.sync2.RemoteAccess;

import java.io.IOException;
import java.io.InputStream;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

@RequiresApi(api = Build.VERSION_CODES.ICE_CREAM_SANDWICH)
public class GoogleDriveRemoteAccess implements RemoteAccess {

	private final static String TAG = "GoogleDriveRemoteAccess";

	// Cache for list command
	private final class FolderListCache {
		// keep alive time (milliseconds).
		private long m_keepAlive;
		// key : full path to folder
		// value : pair with update time & folder listing
		private HashMap<String, Pair<Date, FileMetadataList>> m_list = new HashMap<String, Pair<Date, FileMetadataList>>();

		public FolderListCache() {
			m_keepAlive = 15000;   // ms
		}

		/**
		 * @param keepAlive keep alive time (seconds)
		 */
		public FolderListCache(long keepAlive) {
			m_list = new HashMap<String, Pair<Date, FileMetadataList>>();
			m_keepAlive = 1000 * keepAlive;
		}

		/**
		 * Set cache keep alive time
		 *
		 * @param keepAlive time length in seconds.
		 */
		public final void setKeepAlive(long keepAlive) {
			m_keepAlive = 1000 * keepAlive;
		}

		public final void update(String path, FileMetadataList listing) {
			Date current = new Date();
			if (null != listing)
				m_list.put(path, new Pair<Date, FileMetadataList>(current, listing));
			else
				m_list.remove(path);
		}

		public final FileMetadataList get(String path) {
			Date current = new Date();
			Pair<Date, FileMetadataList> metadata = m_list.get(path);
			FileMetadataList result = null;
			if (null != metadata) {
				if (current.getTime() - metadata.first.getTime() <= m_keepAlive) {
					result = metadata.second;
				} else {
					m_list.remove(path);
				}
			}
			return result;
		}

		public void clear() {
			m_list.clear();
		}
	}

	private int m_isServiceAvailabilityRetCode = ConnectionResult.UNKNOWN;

	// Assigned activity
	private Activity m_activity = null;
	private String m_savedAppName;
	// Saved request code
	private int m_savedSignInRequestCode;
	private OnSignInListener m_savedSignInOnCompleteListener;
	//private Pair<Integer, Scope[]> m_savedPermissionsRequest;
	//private OnPermissionsListener m_savedOnPermissionsListener;
	//private Pair<Integer, Scope[]> m_savedPermissionsRequest;

	// Google Play Services objects
	private GoogleApiAvailability m_gapiAvailability;
	private GoogleSignInOptions m_gso;
	private GoogleSignInClient m_googleSignInClient;
	private GoogleSignInAccount m_account;
	private Drive m_googleDriveService;
	private Executor m_executor;
	private FolderListCache m_folderListCache;
	private final Object m_cacheLocker = new Object();

	private static final char EMULATED_PATH_SEPARATOR = '/';
	private static final String EMULATED_ROOT_PATH = "/";

	public GoogleDriveRemoteAccess(Activity activity) {
		m_activity = activity;
		m_executor = Executors.newSingleThreadExecutor();
		//m_executor = Executors.newFixedThreadPool(1);
		m_folderListCache = new FolderListCache(15);    // 15 s.
	}

	public GoogleSignInAccount getGoogleAccount() {
		return m_account;
	}

	private String simplifyFilePath(String path) {
		String result = path;
		// 1. Remove redundant '/' at end of path
		while (result.length() > 1 && result.charAt(result.length() - 1) == EMULATED_PATH_SEPARATOR)
			result = result.substring(0, result.length() - 1);
		// 2. Remove all preceding '/' at begin of path of the path
		while (!result.isEmpty() && result.charAt(0) == EMULATED_PATH_SEPARATOR)
			result = result.substring(1);
		return result;
	}

	private String dirname(String path) {
		path = simplifyFilePath(path);
		String result = "";
		int pos = path.lastIndexOf(EMULATED_PATH_SEPARATOR);
		if (pos > 0) {
			result = path.substring(0, pos);
			result = simplifyFilePath(result);
		}
		return result;
	}

	private String basename(String path) {
		path = simplifyFilePath(path);
		String result = path;
		int pos = path.lastIndexOf(EMULATED_PATH_SEPARATOR);
		if (pos > 0) {
			result = path.substring(pos + 1);
			result = simplifyFilePath(result);
		}
		return result;
	}

	@Override
	public boolean signInQuietly(OnSignInListener completedListener) {
		m_account = GoogleSignIn.getLastSignedInAccount(m_activity);
		if (null != m_account) {
			GoogleAccountCredential credential = GoogleAccountCredential.usingOAuth2(m_activity, Collections.singleton(DriveScopes.DRIVE_FILE));
			credential.setSelectedAccount(m_account.getAccount());
			Drive.Builder driveBuilder = new Drive.Builder(AndroidHttp.newCompatibleTransport(), new GsonFactory(), credential);
			m_googleDriveService = driveBuilder.setApplicationName(m_savedAppName).build();
			if (null != completedListener) {
				completedListener.onSignInCompleted(m_account, CommonStatusCodes.SUCCESS);
			}
			return true;
		}
		return false;
	}

	@Override
	public void signIn(Bundle params, OnSignInListener completedListener) {
		int requestCode = params.getInt("requestCode");
		String appName = params.getString("appName");
		if (ConnectionResult.SUCCESS != m_isServiceAvailabilityRetCode)
			Log.d(TAG, "Google Play Services are not available!");
		// check if already signed
		m_account = GoogleSignIn.getLastSignedInAccount(m_activity);
		if (null != m_account) {
			GoogleAccountCredential credential = GoogleAccountCredential.usingOAuth2(m_activity, Collections.singleton(DriveScopes.DRIVE_FILE));
			credential.setSelectedAccount(m_account.getAccount());
			Drive.Builder driveBuilder = new Drive.Builder(AndroidHttp.newCompatibleTransport(), new GsonFactory(), credential);
			m_googleDriveService = driveBuilder.setApplicationName(m_savedAppName).build();
			if (null != completedListener) {
				completedListener.onSignInCompleted(m_account, CommonStatusCodes.SUCCESS);
			}
		} else {
			buildGoogleSignInClient();
			// Then, when the sign-in is requested, start the sign-in intent:
			Intent signInIntent = m_googleSignInClient.getSignInIntent();
			m_savedAppName = appName;
			m_savedSignInRequestCode = requestCode;
			m_savedSignInOnCompleteListener = completedListener;
			m_activity.startActivityForResult(signInIntent, requestCode);
			// Activity must reimplement onActivityResult() function
			// and filter given request code.
		}
	}

	@Override
	public void signOut(Bundle params, OnSignOutListener completedListener) {
		final OnSignOutListener finalListener = completedListener;
		// 1. Revoke access
		buildGoogleSignInClient();
		Task<Void> task = m_googleSignInClient.revokeAccess();
		task.addOnCompleteListener(task1 -> {
			// Ignore permissions revoke result
			/*
			int statusCode = CommonStatusCodes.SUCCESS;
			try {
				Void obj = task.getResult(ApiException.class);
			} catch (ApiException e) {
				statusCode = e.getStatusCode();
			} catch (RuntimeExecutionException e) {
				statusCode = CommonStatusCodes.ERROR;
			}
			*/
			// 2. Sign Out
			Task<Void> task2 = m_googleSignInClient.signOut();
			task2.addOnCompleteListener(task21 -> {
				int statusCode = CommonStatusCodes.SUCCESS;
				try {
					Void obj = task21.getResult(ApiException.class);
				} catch (ApiException e) {
					statusCode = e.getStatusCode();
				} catch (RuntimeExecutionException e) {
					statusCode = CommonStatusCodes.ERROR;
				}
				m_account = null;
				if (null != finalListener)
					finalListener.onSignOutCompleted(statusCode);
			});
		});
	}


	// RemoteAccess implementation

	@Override
	public void list(String filePath, OnOperationCompleteListener<FileMetadataList> completeListener) {
		list_wrapper(filePath, completeListener);
	}

	@Override
	public void mkdir(String filePath, OnOperationCompleteListener<FileMetadata> completeListener) {
		mkdir_wrapper(filePath, completeListener);
	}

	@Override
	public void mkdir_recursively(String filePath, OnOperationCompleteListener<FileMetadata> completeListener) {
		mkdir_recursively_wrapper(filePath, "root", completeListener);
	}

	@Override
	public void stat(String filePath, OnOperationCompleteListener<FileMetadata> completeListener) {
		stat_wrapper(filePath, completeListener);
	}

	@Override
	public void readFile(String filePath, OnOperationCompleteListener<InputStream> completeListener) {
		readFile_wrapper(filePath, completeListener);
	}

	@Override
	public void writeFile(String filePath, byte[] data, OnOperationCompleteListener<Boolean> completeListener) {
		writeFile_wrapper(filePath, data, completeListener);
	}

	@Override
	public void trash(String filePath, OnOperationCompleteListener<Boolean> completeListener) {
		trash_wrapper(filePath, completeListener);
	}

	@Override
	public void delete(String filePath, OnOperationCompleteListener<Boolean> completeListener) {
		delete_wrapper(filePath, completeListener);
	}

	@Override
	public void getFile(String filePath, OnOperationCompleteListener<Pair<FileMetadata, InputStream>> completeListener) {
		getFile_wrapper(filePath, completeListener);
	}

	// End of RemoteAccess implementation


	public final boolean isServicesAvailable() {
		if (null == m_gapiAvailability)
			m_gapiAvailability = GoogleApiAvailability.getInstance();
		m_isServiceAvailabilityRetCode = m_gapiAvailability.isGooglePlayServicesAvailable(m_activity);
		return ConnectionResult.SUCCESS == m_isServiceAvailabilityRetCode;
	}

	/**
	 * @brief If Google Play Services is not available this method trying to fix.
	 * Add listener with function addOnAvailabilityListener() to check function result.
	 */
	public final void fixServicesAvailability(final OnOperationCompleteListener<Boolean> listener) {
		if (ConnectionResult.UNKNOWN == m_isServiceAvailabilityRetCode) {
			// if isServicesAvailable() is not called yet, do it.
			isServicesAvailable();
		}
		if (ConnectionResult.SUCCESS == m_isServiceAvailabilityRetCode) {
			if (null != listener)
				listener.onCompleted(true);
			return;
		}
		if (null == m_gapiAvailability)
			m_gapiAvailability = GoogleApiAvailability.getInstance();
		Task<Void> task = m_gapiAvailability.makeGooglePlayServicesAvailable(m_activity);
		task.addOnCompleteListener(task1 -> {
			boolean available = false;
			if (task1.isSuccessful())
				available = true;
			if (null != listener)
				listener.onCompleted(available);
		});
	}

	/**
	 * @param requestCode
	 * @param resultCode
	 * @param data
	 * @brief Helper function to handle some operation results from Google Service activity.
	 * Must be called from main activity in function onActivityResult().
	 */
	public void onActivityResultHandler(int requestCode, int resultCode, Intent data) {
		if (m_savedSignInRequestCode == requestCode) {
			Task<GoogleSignInAccount> task = GoogleSignIn.getSignedInAccountFromIntent(data);
			int statusCode = CommonStatusCodes.SUCCESS;
			try {
				// As documented, we return a completed Task in this case and it's safe to directly call
				// getResult(Class<ExceptionType>) here (without need to worry about IllegalStateException).
				m_account = task.getResult(ApiException.class);
				// Use the authenticated account to sign in to the Drive service.
				GoogleAccountCredential credential = GoogleAccountCredential.usingOAuth2(m_activity, Collections.singleton(DriveScopes.DRIVE_FILE));
				credential.setSelectedAccount(m_account.getAccount());
				Drive.Builder driveBuilder = new Drive.Builder(AndroidHttp.newCompatibleTransport(), new GsonFactory(), credential);
				m_googleDriveService = driveBuilder.setApplicationName(m_savedAppName).build();
			} catch (ApiException e) {
				// The ApiException status code indicates the detailed failure reason.
				// Please refer to the GoogleSignInStatusCodes class reference for more information.
				statusCode = e.getStatusCode();
				m_account = null;
				m_googleDriveService = null;
			} catch (RuntimeExecutionException e) {
				statusCode = CommonStatusCodes.ERROR;
				m_account = null;
				m_googleDriveService = null;
			}
			if (null != m_savedSignInOnCompleteListener) {
				m_savedSignInOnCompleteListener.onSignInCompleted(m_account, statusCode);
				// call only once
				m_savedSignInOnCompleteListener = null;
			}
		} /* else if (m_savedPermissionsRequest != null && m_savedPermissionsRequest.first == requestCode) {
			// update account object
			m_account = GoogleSignIn.getLastSignedInAccount(m_activity);
			if (null != m_savedOnPermissionsListener) {
				if (resultCode == Activity.RESULT_OK)
					m_savedOnPermissionsListener.onPermissionsGranted(m_savedPermissionsRequest.second);
				else
					m_savedOnPermissionsListener.onPermissionDenied(m_savedPermissionsRequest.second);
				// call only once
				m_savedOnPermissionsListener = null;
			}
		}
		*/
	}

	// private helper functions

	private void buildGoogleSignInClient() {
		if (null == m_googleSignInClient) {
			if (null == m_gso) {
				// Configure sign-in to request the user'm_stack ID and basic
				// profile. ID and basic profile are included in DEFAULT_SIGN_IN.
				GoogleSignInOptions.Builder builder = new GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_SIGN_IN);
				m_gso = builder.requestEmail().requestScopes(new Scope(DriveScopes.DRIVE_FILE)).build();
				// without requestScopes(new Scope(DriveScopes.DRIVE_FILE)) all drive operation failed even if drive permission requested separately!
				//m_gso = builder.requestEmail().build();
			}
			// Build a GoogleSignInClient with the options specified by gso.
			m_googleSignInClient = GoogleSignIn.getClient(m_activity, m_gso);
		}
	}

	private FileMetadata getFileMetadata(File file) {
		if (null != file) {
			FileMetadata metadata = new FileMetadata();
			metadata.id = file.getId();
			metadata.fileName = file.getName();
			metadata.description = file.getDescription();
			metadata.fileSize = null != file.getSize() ? file.getSize() : 0;
			metadata.createdDate = new Date(null != file.getCreatedTime() ? file.getCreatedTime().getValue() : 0);
			metadata.modifiedDate = new Date(null != file.getModifiedTime() ? file.getModifiedTime().getValue() : 0);
			metadata.mimeType = file.getMimeType();
			metadata.isFolder = "application/vnd.google-apps.folder".equals(metadata.mimeType);
			metadata.isTrashed = null != file.getTrashed() && file.getTrashed();
			metadata.isShared = null != file.getShared() && file.getShared();
			//java.util.List<String> parents = file.getParents();
			return metadata;
		}
		return null;
	}

	private File createGoogleDriveMetadata(FileMetadata meta) {
		File file = new File();
		file.setId(meta.id);
		file.setName(meta.fileName);
		file.setDescription(meta.description);
		if (meta.isFolder) {
			file.setMimeType("application/vnd.google-apps.folder");
		} else {
			file.setMimeType(meta.mimeType);
			file.setSize(meta.fileSize);
		}
		file.setCreatedTime(new DateTime(meta.createdDate));
		file.setModifiedTime(new DateTime(meta.modifiedDate));
		file.setTrashed(meta.isTrashed);
		file.setShared(meta.isShared);
		return file;
	}

	private FileMetadataList getFileMetadataList(FileList fileList) {
		FileMetadataList result = new FileMetadataList();
		if (null != fileList) {
			java.util.List<File> files = fileList.getFiles();
			if (null != files) {
				for (File file : files) {
					FileMetadata meta = getFileMetadata(file);
					result.add(meta);
				}
			}
		}
		return result;
	}

	private String guessMimeTypeByExt(String fileName) {
		String result = "application/octet-stream";
		if (null == fileName)
			return result;
		int pos = fileName.lastIndexOf('.');
		if (pos > 0 && pos < fileName.length() - 1) {
			String ext = fileName.substring(pos + 1);
			switch (ext) {
				case "txt":
				case "ini":
					result = "text/plain";
					break;
				case "xml":
					result = "application/xml";
					break;
				case "gz":
					result = "application/gzip";
					break;
			}
		}
		return result;
	}

	// private implementations

	private void stat_wrapper(String filePath, final OnOperationCompleteListener<FileMetadata> completeListener) {
		filePath = simplifyFilePath(filePath);
		if (filePath.isEmpty()) {
			Log.d(TAG, "Attempting to stat root folder, skipping.");
			if (null != completeListener)
				completeListener.onCompleted(new FileMetadata("root", "/", true));
			return;
		}
		String dirPath = dirname(filePath);
		final String fileName = basename(filePath);
		list_wrapper(dirPath, new OnOperationCompleteListener<FileMetadataList>() {
			@Override
			public void onCompleted(FileMetadataList metalist) {
				FileMetadata result = null;
				if (null != metalist) {
					for (FileMetadata meta : metalist) {
						if (fileName.equals(meta.fileName)) {
							result = meta;
							break;
						}
					}
				}
				if (null != completeListener)
					completeListener.onCompleted(result);
			}

			@Override
			public void onFailed(Exception e) {
				if (null != completeListener)
					completeListener.onFailed(e);
			}
		});
	}

	private void mkdir_impl(final String fileName, final String parentPath, String parentId, final OnOperationCompleteListener<FileMetadata> completeListener) {
		if (null == parentId || parentId.isEmpty())
			parentId = "root";
		final String finalParentId = parentId;

		// 2. Create folder
		Tasks.call(m_executor, () -> {
			Log.d(TAG, "Starting to create folder...");
			File metadata = new File()
					.setParents(Collections.singletonList(finalParentId))
					.setMimeType("application/vnd.google-apps.folder")
					.setName(fileName);
			return m_googleDriveService.files().create(metadata).execute();
		}).addOnSuccessListener(file -> {
			Log.d(TAG, "mkdir_impl() ok");
			FileMetadata meta = null;
			if (null != file) {
				Log.d(TAG, "dir " + file.getName() + " created!");
				meta = getFileMetadata(file);
			} else
				Log.d(TAG, "dir is NULL.");
			if (null != completeListener)
				completeListener.onCompleted(meta);
			synchronized (m_cacheLocker) {
				// clear cache
				m_folderListCache.update(parentPath, null);
			}
		}).addOnFailureListener(e -> {
			Log.d(TAG, "mkdir_impl() failed, e=" + e.toString());
			if (null != completeListener) {
				completeListener.onCompleted(null);
				completeListener.onFailed(e);
			}
		});
	}

	private void mkdir_wrapper(String filePath, final OnOperationCompleteListener<FileMetadata> completeListener) {
		final String finalFilePath = simplifyFilePath(filePath);
		if (finalFilePath.isEmpty()) {
			Log.d(TAG, "Attempting to create root folder, skipping.");
			if (null != completeListener)
				completeListener.onCompleted(new FileMetadata("root", "/", true));
			return;
		}
		// 1. Check if this folder already exist.
		stat_wrapper(finalFilePath, new OnOperationCompleteListener<FileMetadata>() {
			@Override
			public void onCompleted(FileMetadata meta) {
				if (null != meta && meta.isFolder) {
					// folder already exist
					Log.d(TAG, "Folder '" + finalFilePath + "' already exist.");
					if (null != completeListener)
						completeListener.onCompleted(meta);
				} else {
					// 2. Check if parent folder is exist
					final String dirPath = dirname(finalFilePath);
					final String fileName = basename(finalFilePath);
					stat_wrapper(dirPath, new OnOperationCompleteListener<FileMetadata>() {
						@Override
						public void onCompleted(FileMetadata meta) {
							if (null != meta && meta.isFolder) {
								// parent folder is exist, creating subfolder in parent folder
								mkdir_impl(fileName, dirPath, meta.id, completeListener);
							} else {
								if (null != completeListener) {
									completeListener.onCompleted(null);
									completeListener.onFailed(new IOException("parent folder '" + dirPath + "' not exist!"));
								}
							}
						}

						@Override
						public void onFailed(Exception e) {
							if (null != completeListener) {
								completeListener.onCompleted(null);
								completeListener.onFailed(e);
							}
						}
					});
				}
			}

			@Override
			public void onFailed(Exception e) {
				if (null != completeListener) {
					completeListener.onCompleted(null);
					completeListener.onFailed(e);
				}
			}
		});
	}

	private void mkdir_recursively_wrapper(final String filePath, final String parentId, final OnOperationCompleteListener<FileMetadata> completeListener) {
		final String finalFilePath = simplifyFilePath(filePath);
		if (finalFilePath.isEmpty()) {
			Log.d(TAG, "Attempting to create root folder, skipping.");
			if (null != completeListener)
				completeListener.onCompleted(new FileMetadata("root", "/", true));
			return;
		}
		// 1. Check if this folder already exist.
		stat_wrapper(finalFilePath, new OnOperationCompleteListener<FileMetadata>() {
			@Override
			public void onCompleted(FileMetadata meta) {
				if (null != meta && meta.isFolder) {
					// folder already exist
					Log.d(TAG, "Folder '" + finalFilePath + "' already exist.");
					if (null != completeListener)
						completeListener.onCompleted(meta);
				} else {
					// 2. Check if all parent preceding folders is exist
					final String dirPath = dirname(finalFilePath);
					final String fileName = basename(finalFilePath);
					if (dirPath.isEmpty()) {
						// head parent folder creation
						mkdir_impl(fileName, dirPath, parentId, completeListener);
					} else {
						mkdir_recursively_wrapper(dirPath, parentId, new OnOperationCompleteListener<FileMetadata>() {
							@Override
							public void onCompleted(FileMetadata meta) {
								if (null != meta) {
									// all parent folder created
									mkdir_impl(fileName, dirPath, meta.id, completeListener);
								} else {
									// any of parent folders not created
									completeListener.onCompleted(null);
									completeListener.onFailed(new IOException("Failed to create parent folder '" + dirPath + "'!"));
								}
							}

							@Override
							public void onFailed(Exception e) {
								if (null != completeListener) {
									completeListener.onCompleted(null);
									completeListener.onFailed(e);
								}
							}
						});
					}
				}
			}

			@Override
			public void onFailed(Exception e) {
				if (null != completeListener) {
					completeListener.onCompleted(null);
					completeListener.onFailed(e);
				}
			}
		});
	}

	private void list_impl(String parentId, final OnOperationCompleteListener<FileMetadataList> completeListener) {
		if (null == parentId || parentId.isEmpty())
			parentId = "root";
		final String finalParentId = parentId;
		Tasks.call(m_executor, () -> {
			Log.d(TAG, "Starting to list in " + finalParentId + " ...");
			// TODO: restrict retrieved fields
			Drive.Files.List list = m_googleDriveService.files().list().setSpaces("drive")
					.setQ("'" + finalParentId + "' in parents and trashed=false")
					.setFields("*");
			return list.execute();
		}).addOnSuccessListener(fileList -> {
			FileMetadataList metalist = getFileMetadataList(fileList);
			if (null != completeListener)
				completeListener.onCompleted(metalist);
		}).addOnFailureListener(new OnFailureListener() {
			@Override
			public void onFailure(@NonNull Exception e) {
				if (null != completeListener) {
					completeListener.onCompleted(null);
					completeListener.onFailed(e);
				}
			}
		});
	}

	private void list_wrapper(String filePath, final OnOperationCompleteListener<FileMetadataList> completeListener) {
		filePath = simplifyFilePath(filePath);
		FileMetadataList cachedList;
		synchronized (m_cacheLocker) {
			cachedList = m_folderListCache.get(filePath);
		}
		if (null != cachedList) {
			if (null != completeListener)
				completeListener.onCompleted(cachedList);
			return;
		}
		final String finalFilePath = filePath;
		if (finalFilePath.isEmpty()) {
			list_impl("root", new OnOperationCompleteListener<FileMetadataList>() {
				@Override
				public void onCompleted(FileMetadataList metalist) {
					if (null != metalist) {
						synchronized (m_cacheLocker) {
							m_folderListCache.update(finalFilePath, metalist);
						}
					}
					if (null != completeListener)
						completeListener.onCompleted(metalist);
				}

				@Override
				public void onFailed(Exception e) {
					if (null != completeListener)
						completeListener.onFailed(e);
				}
			});
		} else {
			// 1. firstly get requested folder id
			stat_wrapper(filePath, new OnOperationCompleteListener<FileMetadata>() {
				@Override
				public void onCompleted(FileMetadata meta) {
					if (null != meta) {
						// 2. folder exist, list it
						list_impl(meta.id, new OnOperationCompleteListener<FileMetadataList>() {
							@Override
							public void onCompleted(FileMetadataList metalist) {
								synchronized (m_cacheLocker) {
									m_folderListCache.update(finalFilePath, metalist);
								}
								if (null != completeListener)
									completeListener.onCompleted(metalist);
							}

							@Override
							public void onFailed(Exception e) {
								if (null != completeListener)
									completeListener.onFailed(e);
							}
						});
					} else {
						// folder not found, done
						if (null != completeListener) {
							completeListener.onCompleted(null);
							//completeListener.onFailed(new IOException("folder '" + finalFilePath + "' not exist!"));
						}
					}
				}

				@Override
				public void onFailed(Exception e) {
					Log.e(TAG, "list_wrapper() failed: e=" + e.toString());
					if (null != completeListener)
						completeListener.onFailed(e);
				}
			});
		}
	}

	private void createFile_impl(final String fileName, final String parentPath, String parentId, final byte[] data, final OnOperationCompleteListener<Boolean> completeListener) {
		if (null == parentId || parentId.isEmpty())
			parentId = "root";
		final String finalParentId = parentId;
		Tasks.call(m_executor, () -> {
			// metadata can be 'application/octet-stream', 'application/vnd.google-apps.file'
			// guess mime-type by file extension or even by content...
			String mimeType = guessMimeTypeByExt(fileName);
			File file = new File()
					.setParents(Collections.singletonList(finalParentId))
					.setName(fileName)
					.setMimeType(mimeType);
			ByteArrayContent content = null;
			if (null != data)
				content = new ByteArrayContent(null, data);
			File googleFile = m_googleDriveService.files().create(file, content).setFields("id").execute();
			if (null == googleFile)
				throw new IOException("Null result when requesting file creation.");
			return googleFile;
		}).addOnSuccessListener(file -> {
			boolean result = null != file;
			if (null != completeListener)
				completeListener.onCompleted(result);
			synchronized (m_cacheLocker) {
				m_folderListCache.update(parentPath, null);
			}
		}).addOnFailureListener(e -> {
			if (null != completeListener) {
				completeListener.onCompleted(false);
				completeListener.onFailed(e);
			}
		});
	}

	private void updateFile_impl(final FileMetadata meta, final String parentPath, final byte[] data, final OnOperationCompleteListener<Boolean> completeListener) {
		Tasks.call(m_executor, () -> {
			// metadata can be 'application/octet-stream', 'application/vnd.google-apps.file'
			ByteArrayContent content = new ByteArrayContent(null, data);
			File googleFile = m_googleDriveService.files().update(meta.id, null, content).execute();
			if (null == googleFile)
				throw new IOException("Null result when requesting file creation.");
			return googleFile;
		}).addOnSuccessListener(file -> {
			boolean result = null != file;
			if (null != completeListener)
				completeListener.onCompleted(result);
			synchronized (m_cacheLocker) {
				m_folderListCache.update(parentPath, null);
			}
		}).addOnFailureListener(e -> {
			if (null != completeListener) {
				completeListener.onCompleted(false);
				completeListener.onFailed(e);
			}
		});
	}

	private void writeFile_wrapper(final String filePath, final byte[] data, final OnOperationCompleteListener<Boolean> completeListener) {
		// 1. Check if file already exist
		final String finalFilePath = simplifyFilePath(filePath);
		stat_wrapper(finalFilePath, new OnOperationCompleteListener<FileMetadata>() {
			@Override
			public void onCompleted(final FileMetadata meta) {
				if (null != meta) {
					// 2. file exist, update content
					updateFile_impl(meta, dirname(filePath), data, completeListener);
				} else {
					// 3. file don't exist, create file with data if parent folder is exist
					final String dirPath = dirname(finalFilePath);
					final String fileName = basename(finalFilePath);
					if (!dirPath.isEmpty()) {
						stat(dirPath, new OnOperationCompleteListener<FileMetadata>() {
							@Override
							public void onCompleted(FileMetadata meta) {
								if (null != meta) {
									// 4. parent folder exist, create file inside they
									createFile_impl(fileName, dirPath, meta.id, data, completeListener);
								} else {
									if (null != completeListener) {
										// 4.1 parent folder not exist, done.
										completeListener.onCompleted(false);
										completeListener.onFailed(new IOException("Parent folder '" + dirPath + "' not exist!"));
									}
								}
							}

							@Override
							public void onFailed(Exception e) {
								if (null != completeListener)
									completeListener.onFailed(e);
							}
						});
					} else {
						// 5. In file path no parent folders, create file explicitly
						createFile_impl(finalFilePath, dirPath, "root", data, completeListener);
					}
				}
			}

			@Override
			public void onFailed(Exception e) {
				if (null != completeListener)
					completeListener.onFailed(e);
			}
		});
	}

	private void readFile_wrapper(final String filePath, final OnOperationCompleteListener<InputStream> completeListener) {
		stat_wrapper(filePath, new OnOperationCompleteListener<FileMetadata>() {
			@Override
			public void onCompleted(final FileMetadata meta) {
				if (null != meta) {
					// File found, get his content
					Tasks.call(m_executor, () -> m_googleDriveService.files().get(meta.id).executeMediaAsInputStream()).addOnSuccessListener(m_executor, new OnSuccessListener<InputStream>() {
						@Override
						public void onSuccess(InputStream inputStream) {
							if (null != completeListener) {
								completeListener.onCompleted(inputStream);
							}
						}
					}).addOnFailureListener(e -> {
						if (null != completeListener) {
							completeListener.onCompleted(null);
							completeListener.onFailed(new IOException("file '" + filePath + "' not exist!"));
						}
					});
				} else {
					if (null != completeListener) {
						completeListener.onCompleted(null);
						completeListener.onFailed(new IOException("file '" + filePath + "' not exist!"));
					}
				}
			}

			@Override
			public void onFailed(Exception e) {
				if (null != completeListener)
					completeListener.onFailed(e);
			}
		});
	}

	private void trash_impl(final FileMetadata meta, final String parentPath, final OnOperationCompleteListener<Boolean> completeListener) {
		Tasks.call(m_executor, () -> {
			File file = new File().setTrashed(true);
			File googleFile = m_googleDriveService.files().update(meta.id, file).setFields("id, trashed").execute();
			if (null == googleFile)
				throw new IOException("Null result when requesting file creation.");
			return googleFile;
		}).addOnSuccessListener(file -> {
			boolean result = null != file;
			synchronized (m_cacheLocker) {
				m_folderListCache.update(parentPath, null);
			}
			if (null != completeListener)
				completeListener.onCompleted(result);
		}).addOnFailureListener(e -> {
			if (null != completeListener) {
				completeListener.onCompleted(false);
				completeListener.onFailed(e);
			}
		});
	}

	private void trash_wrapper(final String filePath, final OnOperationCompleteListener<Boolean> completeListener) {
		// Find file or folder
		stat_wrapper(filePath, new OnOperationCompleteListener<FileMetadata>() {
			@Override
			public void onCompleted(FileMetadata meta) {
				if (null != meta) {
					// file or folder found, trashing
					trash_impl(meta, dirname(filePath), completeListener);
				} else {
					// File or folder not exist, done
					if (null != completeListener) {
						completeListener.onCompleted(false);
						completeListener.onFailed(new IOException("File '" + filePath + "' not exist!"));
					}
				}
			}

			@Override
			public void onFailed(Exception e) {
				if (null != completeListener)
					completeListener.onFailed(e);
			}
		});
	}

	private void delete_impl(final FileMetadata meta, final String parentPath, final OnOperationCompleteListener<Boolean> completeListener) {
		Tasks.call(m_executor, () -> m_googleDriveService.files().delete(meta.id).execute()).addOnSuccessListener(new OnSuccessListener<Void>() {
			@Override
			public void onSuccess(Void res) {
				synchronized (m_cacheLocker) {
					m_folderListCache.update(parentPath, null);
				}
				completeListener.onCompleted(true);
			}
		}).addOnFailureListener(e -> {
			if (null != completeListener) {
				completeListener.onCompleted(false);
				completeListener.onFailed(e);
			}
		});
	}

	private void delete_wrapper(final String filePath, final OnOperationCompleteListener<Boolean> completeListener) {
		// Find file or folder
		stat_wrapper(filePath, new OnOperationCompleteListener<FileMetadata>() {
			@Override
			public void onCompleted(FileMetadata meta) {
				if (null != meta) {
					// file or folder found, deleting
					delete_impl(meta, dirname(filePath), completeListener);
				} else {
					// File or folder not exist, done
					if (null != completeListener) {
						completeListener.onCompleted(false);
						completeListener.onFailed(new IOException("File '" + filePath + "' not exist!"));
					}
				}
			}

			@Override
			public void onFailed(Exception e) {
				if (null != completeListener)
					completeListener.onFailed(e);
			}
		});
	}

	public void getFile_wrapper(final String filePath, final OnOperationCompleteListener<Pair<FileMetadata, InputStream>> completeListener) {
		stat_wrapper(filePath, new OnOperationCompleteListener<FileMetadata>() {
			@Override
			public void onCompleted(final FileMetadata meta) {
				if (null != meta) {
					// File found, get his content
					Tasks.call(m_executor, () -> {
						InputStream stream = m_googleDriveService.files().get(meta.id).executeMediaAsInputStream();
						if (null == stream)
							throw new IOException("returned stream is null!");
						return new Pair<FileMetadata, InputStream>(meta, stream);
					}).addOnSuccessListener(pair -> {
						if (null != completeListener) {
							completeListener.onCompleted(pair);
						}
					}).addOnFailureListener(e -> {
						if (null != completeListener) {
							completeListener.onCompleted(null);
							completeListener.onFailed(new IOException("file '" + filePath + "' not exist!"));
						}
					});
				} else {
					if (null != completeListener) {
						completeListener.onCompleted(null);
						completeListener.onFailed(new IOException("file '" + filePath + "' not exist!"));
					}
				}
			}

			@Override
			public void onFailed(Exception e) {
				if (null != completeListener)
					completeListener.onFailed(e);
			}
		});
	}
}
