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

import android.annotation.TargetApi;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Xml;

import org.coolreader.CoolReader;
import org.coolreader.R;
import org.coolreader.crengine.BackgroundThread;
import org.coolreader.crengine.BookInfo;
import org.coolreader.crengine.Bookmark;
import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;
import org.coolreader.crengine.Properties;
import org.coolreader.crengine.Settings;
import org.xml.sax.InputSource;
import org.xml.sax.XMLReader;
import org.xmlpull.v1.XmlSerializer;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Set;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;

import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

@TargetApi(Build.VERSION_CODES.GINGERBREAD)
public class Synchronizer {

	public static final Logger log = L.create("sync2");

	public enum SyncTarget {
		NONE,
		SETTINGS,
		BOOKMARKS,
		CURRENTBOOKINFO
	}

	public enum SyncDirection {
		None,
		SyncTo,
		SyncFrom
	}

	private RemoteAccess m_remoteAccess;
	private CoolReader m_coolReader;
	private int m_signInRequestCode;
	private String m_appName;
	private boolean m_isBusy;
	private boolean m_isAbortRequested;
	private SyncDirection m_syncDirection;
	private int m_currentOperationIndex;
	private int m_totalOperationsCount;
	private SyncOperation m_startOp;
	private SyncOperation m_lastOp;
	private OnSyncStatusListener m_onStatusListener;
	private Runnable m_onAbortedListener;
	private HashMap<SyncTarget, Boolean> m_syncTargets;
	private int m_dataKeepAlive = 14;
	private boolean m_forcedOperations;			// forced all sync operations regardless of specified sync targets

	private static final String[] ALLOWED_OPTIONS_PROP_NAMES = {
			Settings.PROP_FALLBACK_FONT_FACE,
			Settings.PROP_FALLBACK_FONT_FACES,
			Settings.PROP_FOOTNOTES,
			Settings.PROP_APP_HIGHLIGHT_BOOKMARKS,
			Settings.PROP_HYPHENATION_DICT,
			Settings.PROP_IMG_SCALING_ZOOMIN_INLINE_MODE,
			Settings.PROP_IMG_SCALING_ZOOMIN_INLINE_SCALE,
			Settings.PROP_IMG_SCALING_ZOOMOUT_INLINE_MODE,
			Settings.PROP_IMG_SCALING_ZOOMOUT_INLINE_SCALE,
			Settings.PROP_IMG_SCALING_ZOOMIN_BLOCK_MODE,
			Settings.PROP_IMG_SCALING_ZOOMIN_BLOCK_SCALE,
			Settings.PROP_IMG_SCALING_ZOOMOUT_BLOCK_MODE,
			Settings.PROP_IMG_SCALING_ZOOMOUT_BLOCK_SCALE,
			Settings.PROP_INTERLINE_SPACE,
			Settings.PROP_STATUS_CHAPTER_MARKS,
			Settings.PROP_PAGE_VIEW_MODE,
			Settings.PROP_PROGRESS_SHOW_FIRST_PAGE,
			Settings.PROP_RENDER_BLOCK_RENDERING_FLAGS,
			Settings.PROP_REQUESTED_DOM_VERSION,
			Settings.PROP_FLOATING_PUNCTUATION,
			Settings.PROP_FORMAT_MAX_ADDED_LETTER_SPACING_PERCENT,
			Settings.PROP_FORMAT_MIN_SPACE_CONDENSING_PERCENT,
			Settings.PROP_FORMAT_SPACE_WIDTH_SCALE_PERCENT,
			Settings.PROP_FORMAT_UNUSED_SPACE_THRESHOLD_PERCENT,
			Settings.PROP_TEXTLANG_EMBEDDED_LANGS_ENABLED,
			Settings.PROP_TEXTLANG_HYPHENATION_ENABLED,
			Settings.PROP_FONT_KERNING_ENABLED,
			Settings.PROP_FLOATING_PUNCTUATION,
			Settings.PROP_APP_TAP_ZONE_ACTIONS_TAP,
			// header settings
			Settings.PROP_SHOW_TITLE,
			Settings.PROP_SHOW_PAGE_NUMBER,
			Settings.PROP_SHOW_PAGE_COUNT,
			Settings.PROP_SHOW_POS_PERCENT,
			Settings.PROP_STATUS_CHAPTER_MARKS,
			Settings.PROP_SHOW_BATTERY_PERCENT,
			// all per-style definitions
			"styles."
	};

	private static final String REMOTE_FOLDER_PATH = "/.cr3";
	private static final String REMOTE_SETTINGS_FILE_PATH = REMOTE_FOLDER_PATH + "/cr3.ini.gz";

	private static final int BOOKMARKS_BUNDLE_VERSION = 3;
	private static final int CURRENTBOOKINFO_BUNDLE_VERSION = 3;


	public Synchronizer(CoolReader coolReader, RemoteAccess remoteAccess, String appName, int signInRequestCode) {
		m_coolReader = coolReader;
		m_remoteAccess = remoteAccess;
		m_syncTargets = new HashMap<>();
		m_signInRequestCode = signInRequestCode;
		m_appName = appName;
	}

	public void setTarget(SyncTarget target, boolean enable) {
		m_syncTargets.put(target, enable);
	}

	public boolean hasTarget(SyncTarget target) {
		return hasTarget(target, false);
	}

	public boolean hasTarget(SyncTarget target, boolean defValue) {
		Boolean value = m_syncTargets.get(target);
		if (null == value)
			return defValue;
		return value;
	}

	public void setBookmarksKeepAlive(int days) {
		if (days < 0)
			days = 0;
		else if (days > 365)
			days = 365;
		m_dataKeepAlive = days;
	}

	public void setSignInRequestCode(int requestCode) {
		m_signInRequestCode = requestCode;
	}

	public void setApplicationName(String appName) {
		m_appName = appName;
	}

	/**
	 * @param requestCode
	 * @param resultCode
	 * @param data
	 * @brief Helper function to handle some operation results from Google Service activity.
	 * Must be called from main activity in function onActivityResult().
	 */
	public void onActivityResultHandler(int requestCode, int resultCode, Intent data) {
		m_remoteAccess.onActivityResultHandler(requestCode, resultCode, data);
	}

	public boolean isBusy() {
		return m_isBusy;
	}

	public void abort() {
		abort(null);
	}

	public void abort(Runnable onAborted) {
		if (m_isBusy) {
			m_isAbortRequested = true;
			m_onAbortedListener = onAborted;
		} else {
			if (null != onAborted) {
				onAborted.run();
			}
		}
	}

	public void setOnSyncStatusListener(OnSyncStatusListener listener) {
		m_onStatusListener = listener;
	}

	protected void doneSuccessfully() {
		if (null != m_onStatusListener)
			BackgroundThread.instance().executeGUI(() -> m_onStatusListener.onSyncCompleted(m_syncDirection, m_forcedOperations));
		m_isBusy = false;
	}

	protected void doneFailed(String error) {
		if (null != m_onStatusListener)
			BackgroundThread.instance().executeGUI(() -> m_onStatusListener.onSyncError(m_syncDirection, error));
		m_isBusy = false;
	}

	protected void doneAborted() {
		if (null != m_onStatusListener)
			BackgroundThread.instance().executeGUI(() -> m_onStatusListener.onAborted(m_syncDirection));
		m_isBusy = false;
	}

	protected void setSyncStarted(SyncDirection dir) {
		m_isBusy = true;
		m_syncDirection = dir;
		if (null != m_onStatusListener) {
			BackgroundThread.instance().executeGUI(() -> m_onStatusListener.onSyncStarted(m_syncDirection, m_forcedOperations));
		}
	}

	protected void setSyncProgress(int current, int total) {
		if (null != m_onStatusListener) {
			BackgroundThread.instance().executeGUI(() -> m_onStatusListener.OnSyncProgress(m_syncDirection, current, total, m_forcedOperations));
		}
	}

	protected boolean checkAbort() {
		if (m_isAbortRequested) {
			if (null != m_onAbortedListener) {
				m_onAbortedListener.run();
				m_onAbortedListener = null;
			}
			doneAborted();
		}
		return m_isAbortRequested;
	}

	protected void clearOperation() {
		m_startOp = null;
		m_lastOp = null;
		m_totalOperationsCount = 0;
		m_currentOperationIndex = 0;
	}

	protected void addOperation(SyncOperation op) {
		if (null == m_startOp) {
			m_startOp = op;
		} else {
			m_lastOp.setNext(op);
		}
		m_lastOp = op;
		m_totalOperationsCount++;
	}

	protected void replaceOperation(SyncOperation op, SyncOperation with) {
		SyncOperation operation = m_startOp;
		SyncOperation prevOp = null;
		while (null != operation && operation != op) {
			prevOp = operation;
			operation = operation.m_next;
		}
		if (null != operation) {
			// found
			with.setNext(operation.m_next);
			if (null != prevOp)
				prevOp.setNext(with);
			else
				m_startOp = with;
		} else {
			log.e("replaceOperation() failed, cannot find a replacement point!");
		}
	}

	protected void insertOperation(SyncOperation after, SyncOperation op) {
		SyncOperation operation = m_startOp;
		while (null != operation && operation != after) {
			operation = operation.m_next;
		}
		if (null != operation) {
			// found
			op.setNext(operation.m_next);
			operation.setNext(op);
			m_totalOperationsCount++;
		} else {
			log.e("insertOperation() failed, can't find the insertion point!");
		}
	}

	protected void startOperations() {
		if (null != m_startOp)
			m_startOp.exec();
	}

	// SignIn operation
	protected class SignInSyncOperation extends SyncOperation {
		@Override
		void call(Runnable onContinue) {
			log.d("Starting sign-in operation...");

			Bundle params = new Bundle();
			params.putInt("requestCode", m_signInRequestCode);
			params.putString("appName", m_appName);
			m_remoteAccess.signIn(params, (data, statusCode) -> {
				if (checkAbort())
					return;
				m_currentOperationIndex++;
				setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
				if (0 == statusCode) {
					log.d("SignInSyncOperation: SignIn successfully.");
					onContinue.run();
				} else {
					log.e("SignInSyncOperation: SignIn failed");
					doneFailed("SignIn failed");
				}
			});
		}
	}

	// Quietly SignIn operation
	protected class SignInQuietlySyncOperation extends SyncOperation {
		@Override
		void call(Runnable onContinue) {
			log.d("Starting sign-in (quietly) operation...");

			m_remoteAccess.signInQuietly((data, statusCode) -> {
				if (checkAbort())
					return;
				m_currentOperationIndex++;
				setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
				if (0 == statusCode) {
					log.d("SignInQuietlySyncOperation: signIn successfully.");
					onContinue.run();
				} else {
					log.e("SignInQuietlySyncOperation: signIn failed.");
					doneFailed("SignIn failed");
				}
			});
		}
	}

	// SignOut operation
	protected class SignOutSyncOperation extends SyncOperation {
		@Override
		void call(Runnable onContinue) {
			log.d("Starting sign-out operation...");

			Bundle params = new Bundle();
			m_remoteAccess.signOut(params, (statusCode) -> {
				if (checkAbort())
					return;
				m_currentOperationIndex++;
				setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
				if (0 == statusCode) {
					log.d("SignOutSyncOperation: SignOut successfully.");
					onContinue.run();
				} else {
					log.e("SignOutSyncOperation: SignOut failed");
					doneFailed("SignOut failed");
				}
			});
		}
	}

	// Check/create application folder operation
	protected class CheckAppFolderSyncOperation extends SyncOperation {
		@Override
		void call(Runnable onContinue) {
			log.d("Starting CheckAppFolderSyncOperation operation...");

			m_remoteAccess.mkdir(REMOTE_FOLDER_PATH, new OnOperationCompleteListener<FileMetadata>() {
				@Override
				public void onCompleted(FileMetadata meta, boolean ok) {
					if (!ok)
						return;		// onFailed() will be called
					if (checkAbort())
						return;
					m_currentOperationIndex++;
					setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
					if (null != meta) {
						onContinue.run();
					} else {
						log.e("CheckAppFolderSyncOperation: failed to create application folder");
						doneFailed("Failed to create application folder");
					}
				}

				@Override
				public void onFailed(Exception e) {
					log.e("CheckAppFolderSyncOperation: mkdir() failed: " + e.toString());
					doneFailed(e.toString());
				}
			});
		}
	}

	// Check remote settings file modification operation
	protected class CheckUploadSettingsSyncOperation extends SyncOperation {
		private final String localFilePath;
		private final String remoteFilePath;

		CheckUploadSettingsSyncOperation(String localFilePath, String remoteFilePath) {
			this.localFilePath = localFilePath;
			this.remoteFilePath = remoteFilePath;
		}

		@Override
		void call(Runnable onContinue) {
			log.d("Starting CheckUploadSettingsSyncOperation operation...");

			// 1. Check remote file modification file
			m_remoteAccess.stat(remoteFilePath, new OnOperationCompleteListener<FileMetadata>() {
				@Override
				public void onCompleted(FileMetadata meta, boolean ok) {
					if (!ok)
						return;		// onFailed() will be called
					if (checkAbort())
						return;
					m_currentOperationIndex++;
					setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
					SyncOperation this_op = CheckUploadSettingsSyncOperation.this;
					if (null != meta) {
						// file found on remote device
						File localFile = new File(localFilePath);
						if (localFile.exists()) {
							// local file exist
							if (localFile.lastModified() >= meta.modifiedDate.getTime()) {
								// local file newer than remote
								insertOperation(this_op, new UploadSettingsSyncOperation(localFilePath, remoteFilePath));
								onContinue.run();
							} else {
								// remote file newer than local
								log.d("CheckUploadSettingsSyncOperation: remote file newer than local");
								// ask the user via dialog what to do with it
								SyncInfoDialog syncDirDialog = new SyncInfoDialog(m_coolReader, m_coolReader.getString(R.string.confirmation_title), m_coolReader.getString(R.string.googledrive_remotefile_is_newer_confirm, remoteFilePath));
								syncDirDialog.setPositiveButtonLabel(m_coolReader.getString(R.string.googledrive_upload_local));
								syncDirDialog.setNegativeButtonLabel(m_coolReader.getString(R.string.googledrive_load_remote));
								syncDirDialog.setOnPositiveClickListener( view -> {
									insertOperation(this_op, new UploadSettingsSyncOperation(localFilePath, remoteFilePath));
									onContinue.run();
								} );
								syncDirDialog.setOnNegativeClickListener( view -> {
									insertOperation(this_op, new DownloadSettingsSyncOperation(remoteFilePath));
									onContinue.run();
								} );
								syncDirDialog.setOnCancelListener(dialog -> {
									log.e("CheckUploadSettingsSyncOperation: canceled");
									doneFailed("canceled");
								} );
								syncDirDialog.show();
							}
						} else {
							// local file not exist
							log.e("CheckUploadSettingsSyncOperation: local file not exist!");
							doneFailed("local file not exist!");
						}
					} else {
						// file not found on remote service
						insertOperation(this_op, new UploadSettingsSyncOperation(localFilePath, remoteFilePath));
						onContinue.run();
					}
				}

				@Override
				public void onFailed(Exception e) {
					log.e("CheckUploadSettingsSyncOperation: stat failed: " + e.toString());
					doneFailed(e.toString());
				}
			});
		}
	}

	// Upload settings operation
	protected class UploadSettingsSyncOperation extends SyncOperation {
		private final String localFilePath;
		private final String remoteFilePath;

		UploadSettingsSyncOperation(String localFilePath, String remoteFilePath) {
			this.localFilePath = localFilePath;
			this.remoteFilePath = remoteFilePath;
		}

		@Override
		public void call(Runnable onContinue) {
			log.d("Starting UploadSettingsSyncOperation operation...");

			try {
				FileInputStream inputStream = new FileInputStream(localFilePath);
				ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
				GZIPOutputStream gzipOutputStream = new GZIPOutputStream(outputStream);
				byte[] buf = new byte[4096];
				int readBytes;
				while (true) {
					readBytes = inputStream.read(buf);
					if (readBytes > 0)
						gzipOutputStream.write(buf, 0, readBytes);
					else
						break;
				}
				gzipOutputStream.close();
				outputStream.close();
				m_remoteAccess.writeFile(remoteFilePath, outputStream.toByteArray(), new OnOperationCompleteListener<Boolean>() {
					@Override
					public void onCompleted(Boolean result, boolean ok) {
						if (!ok)
							return;		// onFailed() will be called
						if (checkAbort())
							return;
						m_currentOperationIndex++;
						setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
						if (null != result && result) {
							log.d("file created or updated.");
							onContinue.run();
						} else {
							log.e("UploadSettingsSyncOperation: file NOT created!");
							doneFailed("file NOT created");
						}
					}

					@Override
					public void onFailed(Exception e) {
						log.e("UploadSettingsSyncOperation: file creation failed: " + e.toString());
						doneFailed("file creation failed: " + e.toString());
					}
				});
			} catch (Exception e) {
				log.e("UploadSettingsSyncOperation: Can't read local file: " + e.toString());
				doneFailed("Can't read local file: " + e.toString());
			}
		}
	}

	// Check remote settings file modification operation
	protected class CheckDownloadSettingsSyncOperation extends SyncOperation {
		private final String localFilePath;
		private final String remoteFilePath;

		CheckDownloadSettingsSyncOperation(String localFilePath, String remoteFilePath) {
			this.localFilePath = localFilePath;
			this.remoteFilePath = remoteFilePath;
		}

		@Override
		void call(Runnable onContinue) {
			log.d("Starting CheckDownloadSettingsSyncOperation operation...");

			// 1. Check remote file modification file
			m_remoteAccess.stat(remoteFilePath, new OnOperationCompleteListener<FileMetadata>() {
				@Override
				public void onCompleted(FileMetadata meta, boolean ok) {
					if (!ok)
						return;		// onFailed() will be called
					if (checkAbort())
						return;
					m_currentOperationIndex++;
					setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
					SyncOperation this_op = CheckDownloadSettingsSyncOperation.this;
					if (null != meta) {
						// file found on remote device
						File localFile = new File(localFilePath);
						if (localFile.exists()) {
							// local file exist
							if (meta.modifiedDate.getTime() + 10000 >= localFile.lastModified()) {	// up to 10 s. difference allowed
								// remote file newer than remote
								insertOperation(this_op, new DownloadSettingsSyncOperation(remoteFilePath));
								onContinue.run();
							} else {
								// local file newer than remote
								log.d("CheckDownloadSettingsSyncOperation: local file (" + new Date(localFile.lastModified()).toString() + ") newer than remote (" + meta.modifiedDate.toString() + ")");
								// ask the user via dialog what to do with it
								SyncInfoDialog syncDirDialog = new SyncInfoDialog(m_coolReader, m_coolReader.getString(R.string.confirmation_title), m_coolReader.getString(R.string.googledrive_localfile_is_newer_confirm, localFilePath));
								syncDirDialog.setPositiveButtonLabel(m_coolReader.getString(R.string.googledrive_load_remote));
								syncDirDialog.setNegativeButtonLabel(m_coolReader.getString(R.string.googledrive_upload_local));
								syncDirDialog.setOnPositiveClickListener( view -> {
									insertOperation(this_op, new DownloadSettingsSyncOperation(remoteFilePath));
									onContinue.run();
								} );
								syncDirDialog.setOnNegativeClickListener( view -> {
									insertOperation(this_op, new UploadSettingsSyncOperation(localFilePath, remoteFilePath));
									onContinue.run();
								} );
								syncDirDialog.setOnCancelListener(dialog -> {
									log.e("CheckDownloadSettingsSyncOperation: canceled");
									doneFailed("canceled");
								} );
								syncDirDialog.show();
							}
						} else {
							// local file not exist, just copy from remote
							insertOperation(this_op, new DownloadSettingsSyncOperation(remoteFilePath));
							onContinue.run();
						}
					} else {
						// file not found on remote service
						log.e("CheckDownloadSettingsSyncOperation: remote file not exist!");
						doneFailed("remote file not exist!");
					}
				}

				@Override
				public void onFailed(Exception e) {
					log.e("CheckDownloadSettingsSyncOperation: stat failed: " + e.toString());
					doneFailed(e.toString());
				}
			});
		}
	}

	// Download settings operation
	protected class DownloadSettingsSyncOperation extends SyncOperation {
		private final String remoteFilePath;

		DownloadSettingsSyncOperation(String remoteFilePath) {
			this.remoteFilePath = remoteFilePath;
		}

		@Override
		void call(Runnable onContinue) {
			log.d("Starting DownloadSettingsSyncOperation operation...");

			m_remoteAccess.readFile(remoteFilePath, new OnOperationCompleteListener<InputStream>() {
				@Override
				public void onCompleted(InputStream inputStream, boolean ok) {
					if (!ok)
						return;		// onFailed() will be called
					if (checkAbort())
						return;
					m_currentOperationIndex++;
					setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
					if (null != inputStream) {
						log.d("Reading settings from remote service...");
						try {
							GZIPInputStream gzipInputStream = new GZIPInputStream(inputStream);
							Properties allProps = new Properties();
							allProps.load(gzipInputStream);
							inputStream.close();
							// filter not allowed options
							Set<String> keys = allProps.stringPropertyNames();
							boolean allowed;
							Properties props = new Properties();
							for (String key : keys) {
								allowed = false;
								for (String allowedPropName : ALLOWED_OPTIONS_PROP_NAMES) {
									if (key.startsWith(allowedPropName)) {
										allowed = true;
										break;
									}
								}
								if (allowed)
									props.put(key, allProps.get(key));
							}
							if (null != m_onStatusListener) {
								BackgroundThread.instance().executeGUI(() -> m_onStatusListener.onSettingsLoaded(props, m_forcedOperations));
							}
							log.d(" ... done.");
						} catch (Exception e) {
							log.e("DownloadSettingsSyncOperation: file opened, but failed to read or write: " + e.toString());
							// don't mark as failure, may be next operation will be successfully
						}
						// call next operation regardless of this operation result
						onContinue.run();
					} else {
						log.e("DownloadSettingsSyncOperation: read remote file: return null stream!");
						doneFailed("read remote file: return null stream!");
					}
				}

				@Override
				public void onFailed(Exception e) {
					log.e("DownloadSettingsSyncOperation: Can't read remote file: " + e.toString());
					doneFailed(e.toString());
				}
			});
		}
	}

	// upload bookmarks for currently opened book
	protected class UploadBookmarksSyncOperation extends SyncOperation {
		private final BookInfo bookInfo;

		UploadBookmarksSyncOperation(BookInfo bookInfo) {
			this.bookInfo = new BookInfo(bookInfo);		// make a copy
		}

		@Override
		void call(Runnable onContinue) {
			log.d("Starting UploadBookmarksSyncOperation operation...");
			FileInfo fileInfo = bookInfo.getFileInfo();
			byte[] data = getCurrentBookBookmarksData(bookInfo);
			if (null != data) {
				// TODO: replace crc32 with sha512 and remove filename from this
				String fileName = fileInfo.filename + "_" + fileInfo.crc32 + ".bmk.xml.gz";
				m_remoteAccess.writeFile(REMOTE_FOLDER_PATH + "/" + fileName, data, new OnOperationCompleteListener<Boolean>() {
					@Override
					public void onCompleted(Boolean result, boolean ok) {
						if (!ok)
							return;        // onFailed() will be called
						if (checkAbort())
							return;
						m_currentOperationIndex++;
						setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
						if (null != result && result) {
							log.d("file created or updated.");
							onContinue.run();
						} else {
							log.e("UploadBookmarksSyncOperation: file NOT created!");
							doneFailed("file NOT created");
						}
					}

					@Override
					public void onFailed(Exception e) {
						log.e("UploadBookmarksSyncOperation: write failed: " + e.toString());
						doneFailed(e.toString());
					}
				});
			} else {
				// bookmarks data is null, continue with next operation
				log.d("bookmarks data is null, continue with next operation");
				m_currentOperationIndex++;
				setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
				onContinue.run();
			}
		}
	}

	// download bookmarks for one specified book
	protected class DownloadBookmarksSyncOperation extends SyncOperation {
		private final String fileName;

		public DownloadBookmarksSyncOperation(String fileName) {
			this.fileName = fileName;
		}

		@Override
		void call(Runnable onContinue) {
			log.d("Starting DownloadBookmarksSyncOperation operation...");

			m_remoteAccess.readFile(fileName, new OnOperationCompleteListener<InputStream>() {
				@Override
				public void onCompleted(InputStream inputStream, boolean ok) {
					if (!ok)
						return;		// onFailed() will be called
					if (checkAbort())
						return;
					m_currentOperationIndex++;
					setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
					if (null != inputStream) {
						syncBookmarks(inputStream);
						onContinue.run();
					} else {
						log.e("DownloadBookmarksSyncOperation: can't read bookmarks bundle");
						doneFailed("Can't read bookmarks bundle");
					}
				}

				@Override
				public void onFailed(Exception e) {
					log.e("DownloadBookmarksSyncOperation: readFile failed: " + e.toString());
					doneFailed(e.toString());
				}
			});
		}
	}

	protected class DownloadAllBookmarksSyncOperation extends SyncOperation {
		@Override
		void call(Runnable onContinue) {
			log.d("Starting DownloadAllBookmarksSyncOperation operation...");

			m_remoteAccess.list(REMOTE_FOLDER_PATH, new OnOperationCompleteListener<FileMetadataList>() {
				@Override
				public void onCompleted(FileMetadataList metalist, boolean ok) {
					if (!ok)
						return;		// onFailed() will be called
					if (checkAbort())
						return;
					m_currentOperationIndex++;
					setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
					if (null != metalist) {
						SyncOperation op = DownloadAllBookmarksSyncOperation.this;
						for (FileMetadata meta : metalist) {
							if (meta.fileName.endsWith(".bmk.xml.gz")) {
								log.d("scheduling bookmark loading from file " + meta.fileName);
								String fileName = REMOTE_FOLDER_PATH + "/" + meta.fileName;
								SyncOperation downloadBookmark = new DownloadBookmarksSyncOperation(fileName);
								insertOperation(op, downloadBookmark);
								op = downloadBookmark;
							}
						}
						onContinue.run();
					} else {
						log.e("DownloadAllBookmarksSyncOperation: list return null");
						doneFailed("list return null");
					}
				}

				@Override
				public void onFailed(Exception e) {
					log.e("DownloadAllBookmarksSyncOperation: list failed: " + e.toString());
					doneFailed(e.toString());
				}
			});
		}
	}

	protected class UploadCurrentBookInfoSyncOperation extends SyncOperation {
		private final BookInfo bookInfo;

		UploadCurrentBookInfoSyncOperation(BookInfo bookInfo) {
			this.bookInfo = new BookInfo(bookInfo);		// make a copy
		}

		@Override
		void call(Runnable onContinue) {
			log.d("Starting UploadCurrentBookInfoSyncOperation operation...");

			FileInfo fileInfo = bookInfo.getFileInfo();
			try {
				ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
				GZIPOutputStream gzipOutputStream = new GZIPOutputStream(outputStream);
				Properties props = new Properties();
				props.setInt("version", CURRENTBOOKINFO_BUNDLE_VERSION);
				props.setProperty("filename", fileInfo.filename);
				props.setProperty("authors", fileInfo.authors);
				props.setProperty("title", fileInfo.title);
				props.setProperty("series", fileInfo.series);
				props.setInt("seriesNumber", fileInfo.seriesNumber);
				props.setInt("size", fileInfo.size);
				props.setLong("crc32", fileInfo.crc32);
				props.storeToXML(gzipOutputStream, "CoolReader current document info");
				gzipOutputStream.close();
				outputStream.close();
				m_remoteAccess.writeFile(REMOTE_FOLDER_PATH + "/current.xml.gz", outputStream.toByteArray(), new OnOperationCompleteListener<Boolean>() {
					@Override
					public void onCompleted(Boolean result, boolean ok) {
						if (!ok)
							return;        // onFailed() will be called
						if (checkAbort())
							return;
						m_currentOperationIndex++;
						setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
						if (null != result && result) {
							log.d("file created or updated.");
							onContinue.run();
						} else {
							log.e("UploadCurrentBookInfoSyncOperation: failed to save current book info");
							doneFailed("Failed to save current book info");
						}
					}

					@Override
					public void onFailed(Exception e) {
						log.e("UploadCurrentBookInfoSyncOperation: write failed: " + e.toString());
						doneFailed(e.toString());
					}
				});
			} catch (Exception e) {
				log.e("UploadCurrentBookInfoSyncOperation: " + e.toString());
				doneFailed(e.toString());
			}
		}
	}

	protected class DownloadCurrentBookInfoSyncOperation extends SyncOperation {
		@Override
		void call(Runnable onContinue) {
			log.d("Starting DownloadCurrentBookInfoSyncOperation operation...");

			m_remoteAccess.readFile(REMOTE_FOLDER_PATH + "/current.xml.gz", new OnOperationCompleteListener<InputStream>() {
				@Override
				public void onCompleted(InputStream inputStream, boolean ok) {
					if (!ok)
						return;		// onFailed() will be called
					if (checkAbort())
						return;
					m_currentOperationIndex++;
					setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
					if (null != inputStream) {
						try {
							Properties props = new Properties();
							GZIPInputStream gzipInputStream = new GZIPInputStream(inputStream);
							props.loadFromXML(gzipInputStream);
							int version = props.getInt("version", -1);
							if (CURRENTBOOKINFO_BUNDLE_VERSION == version) {
								FileInfo fileInfo = new FileInfo();
								fileInfo.filename = props.getProperty("filename");
								fileInfo.authors = props.getProperty("authors");
								fileInfo.title = props.getProperty("title");
								fileInfo.series = props.getProperty("series");
								fileInfo.seriesNumber = props.getInt("seriesNumber", 0);
								fileInfo.size = props.getInt("size", 0);
								fileInfo.crc32 = props.getLong("crc32", 0);
								syncSetCurrentBook(fileInfo);
								onContinue.run();
							} else {
								throw new RuntimeException("Incompatible file info version " + version);
							}
						} catch (Exception e) {
							log.e("DownloadCurrentBookInfoSyncOperation: " + e.toString());
							doneFailed(e.toString());
						}
					} else {
						log.e("DownloadCurrentBookInfoSyncOperation: input stream is null");
						doneFailed("Can't read bookmarks bundle");
					}
				}

				@Override
				public void onFailed(Exception e) {
					log.e("DownloadCurrentBookInfoSyncOperation: read failed: " + e.toString());
					doneFailed(e.toString());
				}
			});
		}
	}

	protected class DeleteAllAppDataSyncOperation extends SyncOperation {
		@Override
		void call(Runnable onContinue) {
			log.d("Starting DeleteAllAppDataSyncOperation operation...");

			// use trash() instead of delete() so that the user can recover the data later.
			m_remoteAccess.trash(REMOTE_FOLDER_PATH, new OnOperationCompleteListener<Boolean>() {
				@Override
				public void onCompleted(Boolean result, boolean ok) {
					if (!ok)
						return;		// onFailed() will be called
					if (checkAbort())
						return;
					m_currentOperationIndex++;
					setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
					if (null != result && result) {
						log.d("data removed.");
						onContinue.run();
					} else {
						log.e("DeleteAllAppDataSyncOperation: failed to remove");
						doneFailed("Failed to remove application data");
					}
				}

				@Override
				public void onFailed(Exception e) {
					log.e("DeleteAllAppDataSyncOperation: delete failed: " + e.toString());
					doneFailed(e.toString());
				}
			});
		}
	}

	protected class DeleteOldDataSyncOperation extends SyncOperation {
		@Override
		void call(Runnable onContinue) {
			log.d("Starting DeleteOldDataSyncOperation operation...");
			m_remoteAccess.list(REMOTE_FOLDER_PATH, new OnOperationCompleteListener<FileMetadataList>() {
				@Override
				public void onCompleted(FileMetadataList metalist, boolean ok) {
					if (!ok)
						return;		// onFailed() will be called
					if (checkAbort())
						return;
					m_currentOperationIndex++;
					setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
					if (null == metalist) {
						// `metalist` can be null, which means that the folder you are looking for was not found.
						log.d(REMOTE_FOLDER_PATH + " don't exist yet...");
					} else {
						Date now = new Date();
						SyncOperation op = DeleteOldDataSyncOperation.this;
						for (FileMetadata meta : metalist) {
							if (meta.fileName.endsWith(".bmk.xml.gz") ||
									meta.fileName.endsWith(".data.gz")) {
								if (meta.modifiedDate.getTime() + 86400000 * (long) m_dataKeepAlive < now.getTime()) {
									log.d("scheduling to remove file \"" + meta.fileName + "\".");
									String fileName = REMOTE_FOLDER_PATH + "/" + meta.fileName;
									SyncOperation deleteFileOp = new DeleteFileSyncOperation(fileName);
									insertOperation(op, deleteFileOp);
									op = deleteFileOp;
								}
							}
						}
					}
					onContinue.run();
				}

				@Override
				public void onFailed(Exception e) {
					log.e("DeleteOldBookmarksOperation: list failed: " + e.toString());
					doneFailed(e.toString());
				}
			});
		}
	}

	protected class DeleteFileSyncOperation extends SyncOperation {
		private String m_fileName;

		public DeleteFileSyncOperation(String fileName) {
			m_fileName = fileName;
		}

		@Override
		void call(Runnable onContinue) {
			log.d("Starting DeleteFileSyncOperation operation...");
			m_remoteAccess.trash(m_fileName, new OnOperationCompleteListener<Boolean>() {
				@Override
				public void onCompleted(Boolean result, boolean ok) {
					if (!ok)
						return;		// onFailed() will be called
					if (checkAbort())
						return;
					m_currentOperationIndex++;
					setSyncProgress(m_currentOperationIndex, m_totalOperationsCount);
					if (result)
						log.d("File \"" + m_fileName + "\" trashed.");
					onContinue.run();
				}

				@Override
				public void onFailed(Exception e) {
					log.e("DeleteFileSyncOperation: trash failed: " + e.toString());
					doneFailed(e.toString());
				}
			});
		}
	}

	protected SyncOperation m_doneOp = new SyncOperation() {
		@Override
		void call(Runnable onContinue) {
			log.d("All operation completed.");
			doneSuccessfully();
		}
	};


	/**
	 * Starts the process of data synchronization - downloading from a remote service.
	 * @param showSignIn if true - when required show activity to log into account.
	 * @param quietly    if true - quiet mode, do not ask the user for anything.
	 * @param force      if true - force all operation perform everything, even disabled operations.
	 */
	public void startSyncFrom(boolean showSignIn, boolean quietly, boolean force) {
		if (m_isBusy)
			return;
		// make "Sync From" operations chain and run it
		m_isAbortRequested = false;
		m_forcedOperations = force;
		setSyncStarted(SyncDirection.SyncFrom);

		clearOperation();
		if (showSignIn || m_remoteAccess.needSignInRepeat())
			addOperation(new SignInSyncOperation());
		else
			addOperation(new SignInQuietlySyncOperation());
		addOperation(new CheckAppFolderSyncOperation());
		if (force || hasTarget(SyncTarget.SETTINGS)) {
			if (quietly)
				addOperation(new DownloadSettingsSyncOperation(REMOTE_SETTINGS_FILE_PATH));
			else
				addOperation(new CheckDownloadSettingsSyncOperation(m_coolReader.getSettingsFile(0), REMOTE_SETTINGS_FILE_PATH));
		}
		if (force || hasTarget(SyncTarget.BOOKMARKS))
			addOperation(new DownloadAllBookmarksSyncOperation());
		if (m_dataKeepAlive > 0)		// if equals 0 -> disabled
			addOperation(new DeleteOldDataSyncOperation());
		if (force || hasTarget(SyncTarget.CURRENTBOOKINFO))
			addOperation(new DownloadCurrentBookInfoSyncOperation());
		addOperation(m_doneOp);
		startOperations();
	}

	/**
	 * Starts the process of data synchronization - uploading to a remote service.
	 * @param showSignIn if true - when required show activity to log into account
	 * @param quietly    if true - quiet mode, do not ask the user for anything.
	 * @param force      if true - force all operation perform everything, even disabled operations.
	 */
	public void startSyncTo(BookInfo bookInfo, boolean showSignIn, boolean quietly, boolean force) {
		if (m_isBusy)
			return;
		// make "Sync To" operations chain and run it
		m_isAbortRequested = false;
		m_forcedOperations = force;
		setSyncStarted(SyncDirection.SyncTo);

		clearOperation();
		if (showSignIn || m_remoteAccess.needSignInRepeat())
			addOperation(new SignInSyncOperation());
		else
			addOperation(new SignInQuietlySyncOperation());
		addOperation(new CheckAppFolderSyncOperation());
		if (force || hasTarget(SyncTarget.SETTINGS)) {
			if (quietly) {
				addOperation(new UploadSettingsSyncOperation(m_coolReader.getSettingsFile(0), REMOTE_SETTINGS_FILE_PATH));
			} else {
				addOperation(new CheckUploadSettingsSyncOperation(m_coolReader.getSettingsFile(0), REMOTE_SETTINGS_FILE_PATH));
			}
		}
		if (null != bookInfo && null != bookInfo.getFileInfo()) {
			if (force || hasTarget(SyncTarget.BOOKMARKS))
				addOperation(new UploadBookmarksSyncOperation(bookInfo));
			if (force || hasTarget(SyncTarget.CURRENTBOOKINFO))
				addOperation(new UploadCurrentBookInfoSyncOperation(bookInfo));
		} else {
			// bookInfo of fileInfo is null, skipping all operations related to the current book
			log.d("bookInfo or fileInfo is null, skipping all operations related to the current book");
		}
		addOperation(m_doneOp);
		startOperations();
	}

	public void startSyncFromOnly(boolean quietly, SyncTarget... targets) {
		if (m_isBusy)
			return;
		// check target
		boolean all_disabled = true;
		for (SyncTarget target : targets) {
			if (hasTarget(target)) {
				all_disabled = false;
				break;
			}
		}
		if (all_disabled)
			return;
		// make "Sync From" operations chain and run it
		m_isAbortRequested = false;
		setSyncStarted(SyncDirection.SyncFrom);

		clearOperation();
		if (!quietly || m_remoteAccess.needSignInRepeat())
			addOperation(new SignInSyncOperation());
		else
			addOperation(new SignInQuietlySyncOperation());
		addOperation(new CheckAppFolderSyncOperation());
		for (SyncTarget target : targets) {
			switch (target) {
				case SETTINGS:
					if (quietly)
						addOperation(new DownloadSettingsSyncOperation(REMOTE_SETTINGS_FILE_PATH));
					else
						addOperation(new CheckDownloadSettingsSyncOperation(m_coolReader.getSettingsFile(0), REMOTE_SETTINGS_FILE_PATH));
					break;
				case BOOKMARKS:
					addOperation(new DownloadAllBookmarksSyncOperation());
					break;
				case CURRENTBOOKINFO:
					addOperation(new DownloadCurrentBookInfoSyncOperation());
					break;
			}
		}
		addOperation(m_doneOp);
		startOperations();
	}

	public void startSyncToOnly(BookInfo bookInfo, boolean quietly, SyncTarget... targets) {
		if (m_isBusy)
			return;
		// check target
		boolean all_disabled = true;
		for (SyncTarget target : targets) {
			if (hasTarget(target)) {
				all_disabled = false;
				break;
			}
		}
		if (all_disabled)
			return;
		// make "Sync To" operations chain and run it
		m_isAbortRequested = false;
		setSyncStarted(SyncDirection.SyncTo);

		clearOperation();
		if (!quietly || m_remoteAccess.needSignInRepeat())
			addOperation(new SignInSyncOperation());
		else
			addOperation(new SignInQuietlySyncOperation());
		addOperation(new CheckAppFolderSyncOperation());
		for (SyncTarget target : targets) {
			switch (target) {
				case SETTINGS:
					if (quietly)
						addOperation(new UploadSettingsSyncOperation(m_coolReader.getSettingsFile(0), REMOTE_SETTINGS_FILE_PATH));
					else
						addOperation(new CheckUploadSettingsSyncOperation(m_coolReader.getSettingsFile(0), REMOTE_SETTINGS_FILE_PATH));
					break;
				case BOOKMARKS:
					if (null != bookInfo && null != bookInfo.getFileInfo())
						addOperation(new UploadBookmarksSyncOperation(bookInfo));
					break;
				case CURRENTBOOKINFO:
					if (null != bookInfo && null != bookInfo.getFileInfo())
						addOperation(new UploadCurrentBookInfoSyncOperation(bookInfo));
					break;
			}
		}
		addOperation(m_doneOp);
		startOperations();
	}

	public void cleanupAndSignOut() {
		if (m_isBusy)
			return;
		// make "Cleanup & Sign out" operations chain and run it
		m_isAbortRequested = false;
		setSyncStarted(SyncDirection.SyncTo);

		clearOperation();
		addOperation(new SignInSyncOperation());
		addOperation(new DeleteAllAppDataSyncOperation());
		addOperation(new SignOutSyncOperation());
		addOperation(m_doneOp);
		startOperations();
	}

	public void signOut() {
		if (m_isBusy)
			return;
		// make "Sign Out" operations chain and run it
		m_isAbortRequested = false;
		setSyncStarted(SyncDirection.SyncTo);

		clearOperation();
		// don't add SignIn operation
		addOperation(new SignOutSyncOperation());
		addOperation(m_doneOp);
		startOperations();
	}

	private byte[] getCurrentBookBookmarksData(BookInfo bookInfo) {
		byte[] data = null;
		FileInfo fileInfo = bookInfo.getFileInfo();
		if (null != fileInfo) {
			try {
				ByteArrayOutputStream ostream = new ByteArrayOutputStream();
				GZIPOutputStream gzipOutputStream = new GZIPOutputStream(ostream);
				XmlSerializer serializer = Xml.newSerializer();
				serializer.setOutput(gzipOutputStream, "utf-8");
				serializer.startDocument("UTF-8", true);
				// root tag
				serializer.startTag("", "root");
				serializer.attribute("", "version", Integer.toString(BOOKMARKS_BUNDLE_VERSION));
				// Write file info
				serializer.startTag("", "fileinfo");
				// fileName
				serializer.startTag("", "filename");
				serializer.text(fileInfo.filename);
				serializer.endTag("", "filename");
				// Authors
				serializer.startTag("", "authors");
				serializer.text(fileInfo.authors);
				serializer.endTag("", "authors");
				// Title
				serializer.startTag("", "title");
				serializer.text(fileInfo.title);
				serializer.endTag("", "title");
				// Series
				serializer.startTag("", "series");
				serializer.text(fileInfo.series);
				serializer.endTag("", "series");
				// Series Number
				serializer.startTag("", "seriesNumber");
				serializer.text(Integer.toString(fileInfo.seriesNumber, 10));
				serializer.endTag("", "seriesNumber");
				// File Size
				serializer.startTag("", "size");
				serializer.text(Integer.toString(fileInfo.size, 10));
				serializer.endTag("", "size");
				// File CRC32
				serializer.startTag("", "crc32");
				serializer.text(Long.toString(fileInfo.crc32, 10));
				serializer.endTag("", "crc32");
				serializer.endTag("", "fileinfo");
				// Write bookmarks
				serializer.startTag("", "bookmarks");
				for (Bookmark bk : bookInfo.getAllBookmarks()) {
					serializer.startTag("", "bookmark");
					// Id
					if (null == bk.getId())
						serializer.attribute("", "id", "null");
					else
						serializer.attribute("", "id", bk.getId().toString());
					// Type
					serializer.attribute("", "type", Integer.toString(bk.getType(), 10));
					// Percent
					serializer.attribute("", "percent", Integer.toString(bk.getPercent(), 10));
					// Shortcut
					serializer.attribute("", "shortcut", Integer.toString(bk.getShortcut(), 10));
					// Start Position
					serializer.startTag("", "startpos");
					serializer.text(bk.getStartPos());
					serializer.endTag("", "startpos");
					// End Position
					serializer.startTag("", "endpos");
					serializer.text(bk.getEndPos());
					serializer.endTag("", "endpos");
					// Title Text
					serializer.startTag("", "title");
					serializer.text(bk.getTitleText());
					serializer.endTag("", "title");
					// Position Text
					serializer.startTag("", "pos");
					serializer.text(bk.getPosText());
					serializer.endTag("", "pos");
					// Comment Text
					serializer.startTag("", "comment");
					serializer.text(bk.getCommentText());
					serializer.endTag("", "comment");
					// Timestamp
					serializer.startTag("", "timestamp");
					serializer.text(Long.toString(bk.getTimeStamp(), 10));
					serializer.endTag("", "timestamp");
					// Time elapsed
					serializer.startTag("", "elapsed");
					serializer.text(Long.toString(bk.getTimeElapsed(), 10));
					serializer.endTag("", "elapsed");

					serializer.endTag("", "bookmark");
				}
				serializer.endTag("", "bookmarks");
				serializer.endTag("", "root");
				serializer.endDocument();
				serializer.flush();
				gzipOutputStream.close();
				ostream.close();
				data = ostream.toByteArray();
			} catch (Exception e) {
				log.e("getCurrentBookBookmarksData() failed: " + e.toString());
			}
		}
		return data;
	}

	private void syncBookmarks(InputStream inputStream) {
		log.v("syncBookmarks()");
		// 1. Read & parse bookmarks from stream
		FileInfo fileInfo = null;
		List<Bookmark> bookmarks = null;
		try {
			SAXParserFactory saxParserFactory = SAXParserFactory.newInstance();
			SAXParser saxParser = saxParserFactory.newSAXParser();
			XMLReader xmlReader = saxParser.getXMLReader();
			BookmarksContentHandler contentHandler = new BookmarksContentHandler();
			xmlReader.setContentHandler(contentHandler);
			GZIPInputStream gzipInputStream = new GZIPInputStream(inputStream);
			xmlReader.parse(new InputSource(gzipInputStream));
			int version = contentHandler.getVersion();
			if (BOOKMARKS_BUNDLE_VERSION == version) {
				fileInfo = contentHandler.getFileInfo();
				bookmarks = contentHandler.getBookmarks();
			} else {
				throw new RuntimeException("incompatible bookmarks version " + version);
			}
		} catch (Exception e) {
			log.e("syncBookmarks() failed: " + e.toString());
		}
		// 2. Sync with ReaderView
		if (null != fileInfo && null != bookmarks) {
			log.v("fileInfo & bookmarks decoded.");
			// Sync with ReaderView and DB
			final List<Bookmark> finalBookmarks = bookmarks;
			final FileInfo finalFileInfo = fileInfo;
			BackgroundThread.instance().executeGUI(() -> m_coolReader.waitForCRDBService(() -> {
				//m_coolReader.getDB().findByFingerprint(2, finalFileInfo.filename, finalFileInfo.crc32,
				m_coolReader.getDB().findByPatterns(2, finalFileInfo.authors, finalFileInfo.title, finalFileInfo.series, finalFileInfo.filename,
						fileList -> {
							if (0 == fileList.size()) {
								// this book not found in db
								// find in filesystem?
								log.e("file \"" + finalFileInfo.filename + "\" not found in database!");
							} else {
								if (fileList.size() > 1) {
									// multiple files found that matches this fileInfo
									// select first or nothing?
									log.e("multiple files with name \"" + finalFileInfo.filename + "\" found, using first.");
									// TODO: show message
								}
								FileInfo dbFileInfo = fileList.get(0);
								BookInfo bookInfo = new BookInfo(dbFileInfo);
								for (Bookmark bk : finalBookmarks) {
									bookInfo.addBookmark(bk);
								}
								log.d("Book \"" + dbFileInfo + "\" found, syncing...");
								if (null != m_onStatusListener)
									m_onStatusListener.onBookmarksLoaded(bookInfo, m_forcedOperations);
							}
						});

			}));
		}
	}

	private void syncSetCurrentBook(FileInfo fileInfo) {
		log.v("syncSetCurrentBook()");
		BackgroundThread.instance().executeGUI(() -> m_coolReader.waitForCRDBService(() -> {
			//m_coolReader.getDB().findByFingerprint(2, fileName, crc32,
			m_coolReader.getDB().findByPatterns(2, fileInfo.authors, fileInfo.title, fileInfo.series, fileInfo.filename,
					fileList -> {
						if (0 == fileList.size()) {
							// this book not found in db
							// find in filesystem?
							log.e("file \"" + fileInfo.filename + "\" not found in database!");
							if (null != m_onStatusListener) {
								m_onStatusListener.onFileNotFound(fileInfo);
							}
						} else {
							if (fileList.size() > 1) {
								// multiple files found that matches this fileInfo
								// select first or nothing?
								log.e("multiple files with name \"" + fileInfo.filename + "\" found, using first.");
								// TODO: show message
							}
							FileInfo dbFileInfo = fileList.get(0);
							if (null != m_onStatusListener) {
								log.d("Book \"" + dbFileInfo + "\" found, call listener to load this book...");
								m_onStatusListener.onCurrentBookInfoLoaded(fileList.get(0), m_forcedOperations);
							}
						}
					});
		}));
	}

}
