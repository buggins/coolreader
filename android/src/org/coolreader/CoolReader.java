/*
 * CoolReader for Android
 * Copyright (C) 2010-2015,2020,2021 Vadim Lopatin <coolreader.org@gmail.com>
 * Copyright (C) 2012 Michael Berganovsky <mike0berg@gmail.com>
 * Copyright (C) 2012 klush
 * Copyright (C) 2012 Jeff Doozan <jeff@doozan.com>
 * Copyright (C) 2018,2020 Yuri Plotnikov <plotnikovya@gmail.com>
 * Copyright (C) 2018-2021 Aleksey Chernov <valexlin@gmail.com>
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

// Main Class
package org.coolreader;

import android.Manifest;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.net.Uri;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Debug;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;

import org.coolreader.Dictionaries.DictionaryException;
import org.coolreader.crengine.AboutDialog;
import org.coolreader.crengine.BackgroundThread;
import org.coolreader.crengine.BaseActivity;
import org.coolreader.crengine.BookInfo;
import org.coolreader.crengine.BookInfoEditDialog;
import org.coolreader.crengine.Bookmark;
import org.coolreader.crengine.BookmarksDlg;
import org.coolreader.crengine.BrowserViewLayout;
import org.coolreader.crengine.CRRootView;
import org.coolreader.crengine.CRToolBar;
import org.coolreader.crengine.DeviceInfo;
import org.coolreader.crengine.DocumentsContractWrapper;
import org.coolreader.crengine.Engine;
import org.coolreader.crengine.ErrorDialog;
import org.coolreader.crengine.FileBrowser;
import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.FileInfoOperationListener;
import org.coolreader.crengine.InterfaceTheme;
import org.coolreader.crengine.L;
import org.coolreader.crengine.LogcatSaver;
import org.coolreader.crengine.Logger;
import org.coolreader.crengine.N2EpdController;
import org.coolreader.crengine.OPDSCatalogEditDialog;
import org.coolreader.crengine.OptionsDialog;
import org.coolreader.crengine.PositionProperties;
import org.coolreader.crengine.Properties;
import org.coolreader.crengine.ReaderAction;
import org.coolreader.crengine.ReaderCommand;
import org.coolreader.crengine.ReaderView;
import org.coolreader.crengine.ReaderViewLayout;
import org.coolreader.crengine.Services;
import org.coolreader.crengine.Utils;
import org.coolreader.donations.CRDonationService;
import org.coolreader.tts.OnTTSCreatedListener;
import org.coolreader.tts.TTSControlServiceAccessor;
import org.koekak.android.ebookdownloader.SonyBookSelector;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Field;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;
import java.util.Map;

public class CoolReader extends BaseActivity {
	public static final Logger log = L.create("cr");

	private ReaderView mReaderView;
	private ReaderViewLayout mReaderFrame;
	private FileBrowser mBrowser;
	private View mBrowserTitleBar;
	private CRToolBar mBrowserToolBar;
	private BrowserViewLayout mBrowserFrame;
	private CRRootView mHomeFrame;
	private Engine mEngine;
	//View startupView;
	//CRDB mDB;
	private ViewGroup mCurrentFrame;
	private ViewGroup mPreviousFrame;

	/*
	  Commented until the appearance of free implementation of the binding to the Google Drive (R)
	private final SyncOptions mGoogleDriveSyncOpts = new SyncOptions();
	private boolean mSyncGoogleDriveEnabledPrev = false;
	private int mSyncGoogleDriveErrorsCount = 0;
	private Synchronizer mGoogleDriveSync;
	private OnSyncStatusListener mGoogleDriveSyncStatusListener;
	private Timer mGoogleDriveAutoSaveTimer = null;
	private SyncServiceAccessor syncServiceAccessor = null;
	// can be add more synchronizers
	private boolean mSuppressSettingsCopyToCloud;
	 */

	private String mOptionAppearance = "0";

	private String mFileToOpenFromExt = null;

	private int mOpenDocumentTreeCommand = ODT_CMD_NO_SPEC;
	private FileInfo mOpenDocumentTreeArg = null;

	private boolean phoneStateChangeHandlerInstalled = false;
	private int initialBatteryState = ReaderView.BATTERY_STATE_NO_BATTERY;
	private int initialBatteryChargeConn = ReaderView.BATTERY_CHARGER_NO;
	private int initialBatteryLevel = 0;

	private boolean isFirstStart = true;
	private boolean justCreated = false;
	private boolean activityIsRunning = false;
	private boolean isInterfaceCreated = false;

	private boolean dataDirIsRemoved = false;

	private String ttsEnginePackage = "";
	private TTSControlServiceAccessor ttsControlServiceAccessor = null;

	private static final int REQUEST_CODE_STORAGE_PERM = 1;
	private static final int REQUEST_CODE_READ_PHONE_STATE_PERM = 2;
	private static final int REQUEST_CODE_GOOGLE_DRIVE_SIGN_IN = 3;
	private static final int REQUEST_CODE_OPEN_DOCUMENT_TREE = 11;

	// open document tree activity commands
	private static final int ODT_CMD_NO_SPEC = -1;
	private static final int ODT_CMD_DEL_FILE = 1;
	private static final int ODT_CMD_DEL_FOLDER = 2;
	private static final int ODT_CMD_SAVE_LOGCAT = 3;

	private final BroadcastReceiver batteryChangeReceiver = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
			// TODO: When minSDK increases to 5 or higher replace string constants:
			//  "status" -> BatteryManager.EXTRA_STATUS
			//  "plugged" -> BatteryManager.EXTRA_PLUGGED
			//  "level" -> BatteryManager.EXTRA_LEVEL
			int status = intent.getIntExtra("status", 0);
			int plugged = intent.getIntExtra("plugged", 0);
			int level = intent.getIntExtra("level", 0);
			// Translate android values to cr3 values
			switch (plugged) {
				case BatteryManager.BATTERY_PLUGGED_AC:
					plugged = ReaderView.BATTERY_CHARGER_AC;
					break;
				case BatteryManager.BATTERY_PLUGGED_USB:
					plugged = ReaderView.BATTERY_CHARGER_USB;
					break;
				case BatteryManager.BATTERY_PLUGGED_WIRELESS:
					plugged = ReaderView.BATTERY_CHARGER_WIRELESS;
					break;
				default:
					plugged = ReaderView.BATTERY_CHARGER_NO;
			}
			switch (status) {
				case BatteryManager.BATTERY_STATUS_CHARGING:
					status = ReaderView.BATTERY_STATE_CHARGING;
					break;
				case BatteryManager.BATTERY_STATUS_DISCHARGING:
				default:
					status = ReaderView.BATTERY_STATE_DISCHARGING;
					break;
			}
			if (mReaderView != null)
				mReaderView.setBatteryState(status, plugged, level);
			else {
				initialBatteryState = status;
				initialBatteryChargeConn = plugged;
				initialBatteryLevel = level;
			}
		}
	};
	private BroadcastReceiver timeTickReceiver = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
			if (activityIsRunning && null != mReaderView) {
				mReaderView.onTimeTickReceived();
			}
		}
	};

	/**
	 * Called when the activity is first created.
	 */
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		startServices();

		log.i("CoolReader.onCreate() entered");
		super.onCreate(savedInstanceState);

		isFirstStart = true;
		justCreated = true;
		activityIsRunning = false;
		isInterfaceCreated = false;

		// Can request only one set of permissions at a time
		// Then request all permission at a time.
		requestStoragePermissions();

		// apply settings
		onSettingsChanged(settings(), null);

		mEngine = Engine.getInstance(this);

		//requestWindowFeature(Window.FEATURE_NO_TITLE);

		// Get battery level
		// ACTION_BATTERY_CHANGED is a sticky broadcast & we pass null instead of receiver, then
		// no receiver is registered -- the function simply returns the sticky Intent that matches filter.
		Intent intent = registerReceiver(null, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
		if (null != intent) {
			// and process this Intent: save received values
			batteryChangeReceiver.onReceive(null, intent);
		}

		// For TTS volume control
		//  See TTSControlService
		setVolumeControlStream(AudioManager.STREAM_MUSIC);

		//==========================================
		// Donations related code
		try {

			mDonationService = new CRDonationService(this);
			mDonationService.bind();
			SharedPreferences pref = getSharedPreferences(DONATIONS_PREF_FILE, 0);
			try {
				mTotalDonations = pref.getFloat(DONATIONS_PREF_TOTAL_AMOUNT, 0.0f);
			} catch (Exception e) {
				log.e("exception while reading total donations from preferences", e);
			}
		} catch (VerifyError e) {
			log.e("Exception while trying to initialize billing service for donations");
		}

		N2EpdController.n2MainActivity = this;

		showRootWindow();

		if (null != Engine.getExternalSettingsDirName()) {
			// if external data directory created or already exist.
			if (!Engine.DATADIR_IS_EXIST_AT_START && getExtDataDirCreateTime() > 0) {
				dataDirIsRemoved = true;
				log.e("DataDir removed by other application!");
			}
		}

		log.i("CoolReader.onCreate() exiting");
	}

	public final static boolean CLOSE_BOOK_ON_STOP = false;

	boolean mDestroyed = false;

	@Override
	protected void onDestroy() {

		log.i("CoolReader.onDestroy() entered");
		if (!CLOSE_BOOK_ON_STOP && mReaderView != null)
			mReaderView.close();

		// Shutdown TTS service if running
		if (null != ttsControlServiceAccessor) {
			ttsControlServiceAccessor.unbind();
			ttsControlServiceAccessor = null;
		}

		/*
		  Commented until the appearance of free implementation of the binding to the Google Drive (R)
		// Unbind from Cloud Sync service
		if (null != syncServiceAccessor) {
			syncServiceAccessor.unbind();
			syncServiceAccessor = null;
		}
		 */

		if (mHomeFrame != null)
			mHomeFrame.onClose();
		mDestroyed = true;

		//if ( mReaderView!=null )
		//	mReaderView.close();

		//if ( mHistory!=null && mDB!=null ) {
		//history.saveToDB();
		//}


//		if ( BackgroundThread.instance()!=null ) {
//			BackgroundThread.instance().quit();
//		}

		//mEngine = null;

		//===========================
		// Donations support code
		if (mDonationService != null)
			mDonationService.unbind();

		if (mReaderView != null) {
			mReaderView.destroy();
		}
		mReaderView = null;

		log.i("CoolReader.onDestroy() exiting");
		super.onDestroy();

		Services.stopServices();
	}

	public ReaderView getReaderView() {
		return mReaderView;
	}

	// Absolute screen rotation
	int screenRotation = Surface.ROTATION_0;

	@Override
	protected void onScreenRotationChanged(int rotation) {
		screenRotation = rotation;
		if (null != mReaderView) {
			mReaderView.doEngineCommand(ReaderCommand.DCMD_SET_ROTATION_INFO_FOR_AA, rotation);
		}
	}

	@Override
	public void applyAppSetting(String key, String value) {
		super.applyAppSetting(key, value);
		boolean flg = "1".equals(value);
		if (key.equals(PROP_APP_DICTIONARY)) {
			setDict(value);
		} else if (key.equals(PROP_APP_DICTIONARY_2)) {
			setDict2(value);
		} else if (key.equals(PROP_TOOLBAR_APPEARANCE)) {
			setToolbarAppearance(value);
		} else if (key.equals(PROP_APP_BOOK_SORT_ORDER)) {
			if (mBrowser != null)
				mBrowser.setSortOrder(value);
		} else if (key.equals(PROP_APP_SHOW_COVERPAGES)) {
			if (mBrowser != null)
				mBrowser.setCoverPagesEnabled(flg);
		} else if (key.equals(PROP_APP_BOOK_PROPERTY_SCAN_ENABLED)) {
			Services.getScanner().setDirScanEnabled(flg);
		} else if (key.equals(PROP_FONT_FACE)) {
			if (mBrowser != null)
				mBrowser.setCoverPageFontFace(value);
		} else if (key.equals(PROP_APP_COVERPAGE_SIZE)) {
			if (mBrowser != null)
				mBrowser.setCoverPageSizeOption(Utils.parseInt(value, 0, 0, 2));
		} else if (key.equals(PROP_APP_FILE_BROWSER_SIMPLE_MODE)) {
			if (mBrowser != null)
				mBrowser.setSimpleViewMode(flg);
		}
		/* See notes for buildGoogleDriveSynchronizer() function
		else if (key.equals(PROP_APP_CLOUDSYNC_GOOGLEDRIVE_ENABLED)) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				mSyncGoogleDriveEnabledPrev = mGoogleDriveSyncOpts.Enabled;
				mGoogleDriveSyncOpts.Enabled = flg;
				updateGoogleDriveSynchronizer();
			}
		} else if (key.equals(PROP_APP_CLOUDSYNC_CONFIRMATIONS)) {
			mGoogleDriveSyncOpts.AskConfirmations = flg;
		} else if (key.equals(PROP_APP_CLOUDSYNC_GOOGLEDRIVE_SETTINGS)) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				mGoogleDriveSyncOpts.SyncSettings = flg;
				updateGoogleDriveSynchronizer();
			}
		} else if (key.equals(PROP_APP_CLOUDSYNC_GOOGLEDRIVE_BOOKMARKS)) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				mGoogleDriveSyncOpts.SyncBookmarks = flg;
				updateGoogleDriveSynchronizer();
			}
		} else if (key.equals(PROP_APP_CLOUDSYNC_GOOGLEDRIVE_CURRENTBOOK_INFO)) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				mGoogleDriveSyncOpts.SyncCurrentBookInfo = flg;
				updateGoogleDriveSynchronizer();
			}
		} else if (key.equals(PROP_APP_CLOUDSYNC_GOOGLEDRIVE_CURRENTBOOK_BODY)) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				mGoogleDriveSyncOpts.SyncCurrentBookBody = flg;
				updateGoogleDriveSynchronizer();
			}
		} else if (key.equals(PROP_APP_CLOUDSYNC_GOOGLEDRIVE_AUTOSAVEPERIOD)) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				mGoogleDriveSyncOpts.AutoSavePeriod = Utils.parseInt(value, 0, 0, 30);
				updateGoogleDriveSynchronizer();
			}
		} else if (key.equals(PROP_APP_CLOUDSYNC_DATA_KEEPALIVE)) {
			mGoogleDriveSyncOpts.DataKeepAlive = Utils.parseInt(value, 14, 0, 365);
			updateGoogleDriveSynchronizer();
		} */
		else if (key.equals(PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS)) {
			// already in super method:
			// Services.getScanner().setHideEmptyDirs(flg);
			// Here only refresh the file browser
			if (null != mBrowser) {
				mBrowser.showLastDirectory();
			}
		} else if (key.equals(PROP_APP_FILE_BROWSER_HIDE_EMPTY_GENRES)) {
			if (null != mBrowser) {
				mBrowser.setHideEmptyGenres(flg);
			}
		} else if (key.equals(PROP_APP_TTS_ENGINE)) {
			ttsEnginePackage = value;
			if (null != mReaderView && mReaderView.isTTSActive()) {
				// Set new TTS engine if running
				initTTS(null);
			}
		}
		//
	}

	/*
	 * NOTE: Unfortunately, Services Google Play has a proprietary license,
	 * so we cannot use it in the program under GPL license.
	 * This code must be rewritten using free libraries compatible with
	 * the GPL license or write its implementation from scratch.
	private void buildGoogleDriveSynchronizer() {
		if (null != syncServiceAccessor && null != mGoogleDriveSync) {
			if (!syncServiceAccessor.isServiceBound()) {
				// lost connection to service, nullify sync instance
				mGoogleDriveSync = null;
			}
		}
		if (null != mGoogleDriveSync)
			return;
		// build synchronizer instance
		// DeviceInfo.getSDKLevel() not applicable here -> compile error about Android API compatibility
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			GoogleDriveRemoteAccess googleDriveRemoteAccess = new GoogleDriveRemoteAccess(this, 30);
			mGoogleDriveSync = new Synchronizer(this, googleDriveRemoteAccess, getString(R.string.app_name), REQUEST_CODE_GOOGLE_DRIVE_SIGN_IN);
			mGoogleDriveSyncStatusListener = new OnSyncStatusListener() {
				@Override
				public void onSyncStarted(Synchronizer.SyncDirection direction, boolean showProgress, boolean interactively) {
					if (Synchronizer.SyncDirection.SyncFrom == direction) {
						log.d("Starting synchronization from Google Drive");
					} else if (Synchronizer.SyncDirection.SyncTo == direction) {
						log.d("Starting synchronization to Google Drive");
					}
					if (null != mReaderView) {
						if (showProgress) {
							mReaderView.showCloudSyncProgress(100);
						}
					}
				}

				@Override
				public void OnSyncProgress(Synchronizer.SyncDirection direction, boolean showProgress, int current, int total, boolean interactively) {
					log.v("sync progress: current=" + current + "; total=" + total);
					if (null != mReaderView) {
						if (showProgress) {
							int total_ = total;
							if (current > total_)
								total_ = current;
							mReaderView.showCloudSyncProgress(10000 * current / total_);
						}
					}
				}

				@Override
				public void onSyncCompleted(Synchronizer.SyncDirection direction, boolean showProgress, boolean interactively) {
					if (Synchronizer.SyncDirection.SyncFrom == direction) {
						log.d("Google Drive SyncFrom successfully completed");
					} else if (Synchronizer.SyncDirection.SyncTo == direction) {
						log.d("Google Drive SyncTo successfully completed");
					}
					if (interactively)
						showToast(R.string.googledrive_sync_completed);
					if (showProgress) {
						if (null != mReaderView) {
							// Hide sync indicator
							mReaderView.hideCloudSyncProgress();
						}
					}
					if (mGoogleDriveSyncOpts.Enabled)
						mSyncGoogleDriveErrorsCount = 0;
				}

				@Override
				public void onSyncError(Synchronizer.SyncDirection direction, String errorString) {
					// Hide sync indicator
					if (null != mReaderView) {
						mReaderView.hideCloudSyncProgress();
					}
					if (null != errorString)
						showToast(R.string.googledrive_sync_failed_with, errorString);
					else
						showToast(R.string.googledrive_sync_failed);
					if (mGoogleDriveSyncOpts.Enabled) {
						mSyncGoogleDriveErrorsCount++;
						if (mSyncGoogleDriveErrorsCount >= 3) {
							showToast(R.string.googledrive_sync_failed_disabled);
							log.e("More than 3 sync failures in a row, auto sync disabled.");
							mGoogleDriveSyncOpts.Enabled = false;
						}
					}
				}

				@Override
				public void onAborted(Synchronizer.SyncDirection direction) {
					// Hide sync indicator
					if (null != mReaderView) {
						mReaderView.hideCloudSyncProgress();
					}
					showToast(R.string.googledrive_sync_aborted);
				}

				@Override
				public void onSettingsLoaded(Properties settings, boolean interactively) {
					// Apply downloaded (filtered) settings
					mSuppressSettingsCopyToCloud = true;
					mergeSettings(settings, true);
				}

				@Override
				public void onBookmarksLoaded(BookInfo bookInfo, boolean interactively) {
					waitForCRDBService(() -> {
						// TODO: ask the user whether to import new bookmarks.
						BookInfo currentBook = null;
						int currentPos = -1;
						if (null != mReaderView) {
							currentBook = mReaderView.getBookInfo();
							if (null != currentBook) {
								Bookmark lastPos = currentBook.getLastPosition();
								if (null != lastPos)
									currentPos = lastPos.getPercent();
							}
						}
						Services.getHistory().updateBookInfo(bookInfo);
						getDB().saveBookInfo(bookInfo);
						if (null != currentBook) {
							FileInfo currentFileInfo = currentBook.getFileInfo();
							if (null != currentFileInfo) {
								if (currentFileInfo.baseEquals((bookInfo.getFileInfo()))) {
									// if the book indicated by the bookInfo is currently open.
									Bookmark lastPos = bookInfo.getLastPosition();
									if (null != lastPos) {
										if (!interactively) {
											mReaderView.goToBookmark(lastPos);
										} else {
											if (Math.abs(currentPos - lastPos.getPercent()) > 10) {		// 0.1%
												askQuestion(R.string.cloud_synchronization_from_, R.string.sync_confirmation_new_reading_position,
														() -> mReaderView.goToBookmark(lastPos), null);
											}
										}
									}
								}
							}
						}
					});
				}

				@Override
				public void onCurrentBookInfoLoaded(FileInfo fileInfo, boolean interactively) {
					FileInfo current = null;
					if (null != mReaderView) {
						BookInfo bookInfo = mReaderView.getBookInfo();
						if (null != bookInfo)
							current = bookInfo.getFileInfo();
					}
					if (!fileInfo.baseEquals(current)) {
						if (!interactively) {
							loadDocument(fileInfo, false);
						} else {
							String shortBookInfo = "";
							if (null != fileInfo.authors && !fileInfo.authors.isEmpty())
								shortBookInfo = "\"" + fileInfo.authors + ", ";
							else
								shortBookInfo = "\"";
							shortBookInfo += fileInfo.title + "\"";
							String question = getString(R.string.sync_confirmation_other_book, shortBookInfo);
							askQuestion(getString(R.string.cloud_synchronization_from_), question, () -> loadDocument(fileInfo, false), null);
						}
					}
				}

				@Override
				public void onFileNotFound(FileInfo fileInfo) {
					if (null == fileInfo)
						return;
					String docInfo = "Unknown";
					if (null != fileInfo.title && !fileInfo.authors.isEmpty())
						docInfo = fileInfo.title;
					if (null != fileInfo.authors && !fileInfo.authors.isEmpty())
						docInfo = fileInfo.authors + ", " + docInfo;
					if (null != fileInfo.filename && !fileInfo.filename.isEmpty())
						docInfo += " (" + fileInfo.filename + ")";
					showToast(R.string.sync_info_no_such_document, docInfo);
				}
			};
			syncServiceAccessor = new SyncServiceAccessor(this);
		}
	}

	private void updateGoogleDriveSynchronizer() {
		// DeviceInfo.getSDKLevel() not applicable here -> lint error about Android API compatibility
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			if (mGoogleDriveSyncOpts.Enabled) {
				if (null == mGoogleDriveSync) {
					log.d("Google Drive sync is enabled.");
					buildGoogleDriveSynchronizer();
				}
				mGoogleDriveSync.setTarget(Synchronizer.SyncTarget.SETTINGS, mGoogleDriveSyncOpts.SyncSettings);
				mGoogleDriveSync.setTarget(Synchronizer.SyncTarget.BOOKMARKS, mGoogleDriveSyncOpts.SyncBookmarks);
				mGoogleDriveSync.setTarget(Synchronizer.SyncTarget.CURRENTBOOKINFO, mGoogleDriveSyncOpts.SyncCurrentBookInfo);
				mGoogleDriveSync.setTarget(Synchronizer.SyncTarget.CURRENTBOOKBODY, mGoogleDriveSyncOpts.SyncCurrentBookBody);
				mGoogleDriveSync.setBookmarksKeepAlive(mGoogleDriveSyncOpts.DataKeepAlive);
				if (null != mGoogleDriveAutoSaveTimer) {
					mGoogleDriveAutoSaveTimer.cancel();
					mGoogleDriveAutoSaveTimer = null;
				}
				if (mGoogleDriveSyncOpts.AutoSavePeriod > 0) {
					mGoogleDriveAutoSaveTimer = new Timer();
					mGoogleDriveAutoSaveTimer.schedule(new TimerTask() {
						@Override
						public void run() {
							if (activityIsRunning && null != mGoogleDriveSync) {
								//mGoogleDriveSync.startSyncTo(getCurrentBookInfo(), Synchronizer.SYNC_FLAG_QUIETLY);
								syncServiceAccessor.bind(sync -> {
									sync.setSynchronizer(mGoogleDriveSync);
									sync.setOnSyncStatusListener(mGoogleDriveSyncStatusListener);
									sync.startSyncTo(getCurrentBookInfo(), Synchronizer.SYNC_FLAG_QUIETLY);
								});
							}
						}
					}, mGoogleDriveSyncOpts.AutoSavePeriod * 60000L, mGoogleDriveSyncOpts.AutoSavePeriod * 60000L);
				}
			} else {
				if (null != mGoogleDriveAutoSaveTimer) {
					mGoogleDriveAutoSaveTimer.cancel();
					mGoogleDriveAutoSaveTimer = null;
				}
				if (mSyncGoogleDriveEnabledPrev && null != mGoogleDriveSync) {
					log.d("Google Drive autosync is disabled.");
					if (false) {
						// TODO: Don't remove authorization on Google Account here, move this into OptionsDialog
						// ask user: cleanup & sign out
						askConfirmation(R.string.googledrive_disabled_cleanup_question,
								() -> {
									if (null != mGoogleDriveSync) {
										mGoogleDriveSync.abort(() -> {
											if (null != mGoogleDriveSync) {
												mGoogleDriveSync.cleanupAndSignOut();
												mGoogleDriveSync = null;
											}
										});
									}
								},
								() -> {
									if (null != mGoogleDriveSync) {
										mGoogleDriveSync.abort(() -> {
											if (null != mGoogleDriveSync) {
												mGoogleDriveSync.signOut();
												mGoogleDriveSync = null;
											}
										});
									}
								}
						);
					}
				}
			}
		}
	}

	public void forceSyncToGoogleDrive() {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			if (null == mGoogleDriveSync)
				buildGoogleDriveSynchronizer();
			mGoogleDriveSync.setBookmarksKeepAlive(mGoogleDriveSyncOpts.DataKeepAlive);
			//mGoogleDriveSync.startSyncTo(getCurrentBookInfo(), Synchronizer.SYNC_FLAG_SHOW_SIGN_IN | Synchronizer.SYNC_FLAG_FORCE | Synchronizer.SYNC_FLAG_SHOW_PROGRESS | Synchronizer.SYNC_FLAG_ASK_CHANGED);
			syncServiceAccessor.bind(sync -> {
				sync.setSynchronizer(mGoogleDriveSync);
				sync.setOnSyncStatusListener(mGoogleDriveSyncStatusListener);
				sync.startSyncTo(getCurrentBookInfo(), Synchronizer.SYNC_FLAG_SHOW_SIGN_IN | Synchronizer.SYNC_FLAG_FORCE | Synchronizer.SYNC_FLAG_SHOW_PROGRESS | Synchronizer.SYNC_FLAG_ASK_CHANGED);
			});
		}
	}

	public void forceSyncFromGoogleDrive() {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			if (null == mGoogleDriveSync)
				buildGoogleDriveSynchronizer();
			mGoogleDriveSync.setBookmarksKeepAlive(mGoogleDriveSyncOpts.DataKeepAlive);
			//mGoogleDriveSync.startSyncFrom(Synchronizer.SYNC_FLAG_SHOW_SIGN_IN | Synchronizer.SYNC_FLAG_FORCE | Synchronizer.SYNC_FLAG_SHOW_PROGRESS | Synchronizer.SYNC_FLAG_ASK_CHANGED);
			syncServiceAccessor.bind(sync -> {
				sync.setSynchronizer(mGoogleDriveSync);
				sync.setOnSyncStatusListener(mGoogleDriveSyncStatusListener);
				sync.startSyncFrom(Synchronizer.SYNC_FLAG_SHOW_SIGN_IN | Synchronizer.SYNC_FLAG_FORCE | Synchronizer.SYNC_FLAG_SHOW_PROGRESS | Synchronizer.SYNC_FLAG_ASK_CHANGED);
			});
		}
	}
	*/

	private BookInfo getCurrentBookInfo() {
		BookInfo bookInfo = null;
		if (mReaderView != null) {
			bookInfo = mReaderView.getBookInfo();
			if (null != bookInfo && null == bookInfo.getFileInfo()) {
				// nullify if fileInfo is null
				bookInfo = null;
			}
		}
		return bookInfo;
	}

	@Override
	public void setFullscreen(boolean fullscreen) {
		super.setFullscreen(fullscreen);
		if (mReaderFrame != null)
			mReaderFrame.updateFullscreen(fullscreen);
	}

	@Override
	protected void onNewIntent(Intent intent) {
		log.i("onNewIntent : " + intent);
		if (mDestroyed) {
			log.e("engine is already destroyed");
			return;
		}
		processIntent(intent);
	}

	private boolean processIntent(Intent intent) {
		log.d("intent=" + intent);
		if (intent == null)
			return false;
		String fileToOpen = null;
		mFileToOpenFromExt = null;
		Uri uri = null;
		if (Intent.ACTION_VIEW.equals(intent.getAction())) {
			uri = intent.getData();
			intent.setData(null);
			if (uri != null) {
				fileToOpen = filePathFromUri(uri);
			}
		} else {
			for (ReaderAction ra: ReaderAction.AVAILABLE_ACTIONS) {
				String raIntentName = "org.coolreader.cmd." + ra.id;
				if (raIntentName.equals(intent.getAction())) {
					mReaderView.onCommand(ra.cmd, ra.param, null);
					return true;
				}
			}
		}

		if (fileToOpen == null && intent.getExtras() != null) {
			log.d("extras=" + intent.getExtras());
			fileToOpen = intent.getExtras().getString(OPEN_FILE_PARAM);
		}
		if (fileToOpen != null) {
			mFileToOpenFromExt = fileToOpen;
			log.d("FILE_TO_OPEN = " + fileToOpen);
			final String finalFileToOpen = fileToOpen;
			loadDocument(fileToOpen, null, () -> BackgroundThread.instance().postGUI(() -> {
				// if document not loaded show error & then root window
				ErrorDialog errDialog = new ErrorDialog(CoolReader.this, CoolReader.this.getString(R.string.error), CoolReader.this.getString(R.string.cant_open_file, finalFileToOpen));
				errDialog.setOnDismissListener(dialog -> showRootWindow());
				errDialog.show();
			}, 500), true);
			return true;
		} else if (null != uri) {
			log.d("URI_TO_OPEN = " + uri);
			final String uriString = uri.toString();
			mFileToOpenFromExt = uriString;
			loadDocumentFromUri(uri, null, () -> BackgroundThread.instance().postGUI(() -> {
				// if document not loaded show error & then root window
				ErrorDialog errDialog = new ErrorDialog(CoolReader.this, CoolReader.this.getString(R.string.error), CoolReader.this.getString(R.string.cant_open_file, uriString));
				errDialog.setOnDismissListener(dialog -> showRootWindow());
				errDialog.show();
			}, 500));
			return true;
		} else {
			log.d("No file to open");
			return false;
		}
	}

	private String filePathFromUri(Uri uri) {
		if (null == uri)
			return null;
		String filePath = null;
		String scheme = uri.getScheme();
		String host = uri.getHost();
		if ("file".equals(scheme)) {
			filePath = uri.getPath();
			// patch for opening of books from ReLaunch (under Nook Simple Touch)
			if (null != filePath) {
				if (filePath.contains("%2F"))
					filePath = filePath.replace("%2F", "/");
			}
		} else if ("content".equals(scheme)) {
			if (uri.getEncodedPath().contains("%00"))
				filePath = uri.getEncodedPath();
			else
				filePath = uri.getPath();
			if (null != filePath) {
				// parse uri from system filemanager
				if (filePath.contains("%00")) {
					// splitter between archive file name and inner file.
					filePath = filePath.replace("%00", "@/");
					filePath = Uri.decode(filePath);
				}
				if ("com.android.externalstorage.documents".equals(host)) {
					// application "Files" by Google, package="com.android.externalstorage.documents"
					if (filePath.matches("^/document/.*:.*$")) {
						// decode special uri form: /document/primary:<somebody>
						//                          /document/XXXX-XXXX:<somebody>
						String shortcut = filePath.replaceFirst("^/document/(.*):.*$", "$1");
						String mountRoot = Engine.getMountRootByShortcut(shortcut);
						if (mountRoot != null) {
							filePath = filePath.replaceFirst("^/document/.*:(.*)$", mountRoot + "/$1");
						}
					}
				} else if ("com.google.android.apps.nbu.files.provider".equals(host)) {
					// application "Files" by Google, package="com.google.android.apps.nbu.files"
					if (filePath.startsWith("/1////")) {
						// skip "/1///"
						filePath = filePath.substring(5);
						filePath = Uri.decode(filePath);
					} else if (filePath.startsWith("/1/file:///")) {
						// skip "/1/file://"
						filePath = filePath.substring(10);
						filePath = Uri.decode(filePath);
					}
				} else {
					// Try some common conversions...
					if (filePath.startsWith("/file%3A%2F%2F")) {
						filePath = filePath.substring(14);
						filePath = Uri.decode(filePath);
						if (filePath.contains("%20")) {
							filePath = filePath.replace("%20", " ");
						}
					}
				}
			}
		}
		if (null != filePath) {
			File file;
			int pos = filePath.indexOf("@/");
			if (pos > 0)
				file = new File(filePath.substring(0, pos));
			else
				file = new File(filePath);
			if (!file.exists())
				filePath = null;
		}
		return filePath;
	}

	@Override
	protected void onPause() {
		activityIsRunning = false;
		if (mReaderView != null) {
			mReaderView.onAppPause();
		}
		if (mBrowser != null) {
			mBrowser.stopCurrentScan();
		}
		try {
			unregisterReceiver(batteryChangeReceiver);
		} catch (IllegalArgumentException e) {
			log.e("Failed to unregister receiver: " + e.toString());
		}
		try {
			unregisterReceiver(timeTickReceiver);
		} catch (IllegalArgumentException e) {
			log.e("Failed to unregister receiver: " + e.toString());
		}
		Services.getCoverpageManager().removeCoverpageReadyListener(mHomeFrame);
		/*
		  Commented until the appearance of free implementation of the binding to the Google Drive (R)
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			if (mGoogleDriveSyncOpts.Enabled && mGoogleDriveSync != null) {
				//mGoogleDriveSync.startSyncTo(getCurrentBookInfo(), Synchronizer.SYNC_FLAG_QUIETLY | Synchronizer.SYNC_FLAG_SHOW_PROGRESS);
				syncServiceAccessor.bind(sync -> {
					sync.setSynchronizer(mGoogleDriveSync);
					sync.setOnSyncStatusListener(mGoogleDriveSyncStatusListener);
					sync.startSyncTo(getCurrentBookInfo(), Synchronizer.SYNC_FLAG_QUIETLY | Synchronizer.SYNC_FLAG_SHOW_PROGRESS);
				});
				try {
					// start SyncService to prevent service destroying on unbinding in onDestroy()
					Intent intent = new Intent(SyncService.SYNC_ACTION_NOOP, Uri.EMPTY, this, SyncService.class);
					startService(intent);
				} catch (Exception ignored) {}
			}
		}
		 */
		super.onPause();
	}

	@Override
	protected void onPostCreate(Bundle savedInstanceState) {
		log.i("CoolReader.onPostCreate()");
		super.onPostCreate(savedInstanceState);
	}

	@Override
	protected void onPostResume() {
		log.i("CoolReader.onPostResume()");
		super.onPostResume();
	}

	//	private boolean restarted = false;
	@Override
	protected void onRestart() {
		log.i("CoolReader.onRestart()");
		//restarted = true;
		super.onRestart();
	}

	@Override
	protected void onRestoreInstanceState(Bundle savedInstanceState) {
		log.i("CoolReader.onRestoreInstanceState()");
		super.onRestoreInstanceState(savedInstanceState);
	}

	@Override
	protected void onResume() {
		if (null == mFileToOpenFromExt)
			log.i("CoolReader.onResume()");
		else
			log.i("CoolReader.onResume(), mFileToOpenFromExt=" + mFileToOpenFromExt);
		super.onResume();
		//Properties props = SettingsManager.instance(this).get();

		if (mReaderView != null)
			mReaderView.onAppResume();
		// ACTION_BATTERY_CHANGED: This is a sticky broadcast containing the charging state, level, and other information about the battery.
		Intent intent = registerReceiver(batteryChangeReceiver, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
		if (null != intent) {
			// process this Intent
			batteryChangeReceiver.onReceive(null, intent);
		}
		// ACTION_TIME_TICK: The current time has changed. Sent every minute.
		registerReceiver(timeTickReceiver, new IntentFilter(Intent.ACTION_TIME_TICK));

		if (DeviceInfo.EINK_SCREEN) {
			if (DeviceInfo.EINK_SONY) {
				SharedPreferences pref = getSharedPreferences(PREF_FILE, 0);
				String res = pref.getString(PREF_LAST_BOOK, null);
				if (res != null && res.length() > 0) {
					SonyBookSelector selector = new SonyBookSelector(this);
					long l = selector.getContentId(res);
					if (l != 0) {
						selector.setReadingTime(l);
						selector.requestBookSelection(l);
					}
				}
			}
		}
		/*
		  Commented until the appearance of free implementation of the binding to the Google Drive (R)
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			if (mGoogleDriveSyncOpts.Enabled && mGoogleDriveSync != null) {
				// when the program starts, the local settings file is already updated, so the local file is always newer than the remote one
				// Therefore, the synchronization mode is quiet, i.e. without comparing modification times and without prompting the user for action.
				// If the file is opened from an external file manager, we must disable the "currently reading book" sync operation with google drive.
				if (null == mFileToOpenFromExt) {
					//mGoogleDriveSync.startSyncFrom(Synchronizer.SYNC_FLAG_SHOW_SIGN_IN | Synchronizer.SYNC_FLAG_QUIETLY | Synchronizer.SYNC_FLAG_SHOW_PROGRESS | (mGoogleDriveSyncOpts.AskConfirmations ? Synchronizer.SYNC_FLAG_ASK_CHANGED : 0));
					syncServiceAccessor.bind(sync -> {
						sync.setSynchronizer(mGoogleDriveSync);
						sync.setOnSyncStatusListener(mGoogleDriveSyncStatusListener);
						sync.startSyncFrom(Synchronizer.SYNC_FLAG_SHOW_SIGN_IN | Synchronizer.SYNC_FLAG_QUIETLY | Synchronizer.SYNC_FLAG_SHOW_PROGRESS | (mGoogleDriveSyncOpts.AskConfirmations ? Synchronizer.SYNC_FLAG_ASK_CHANGED : 0));
					});
				} else {
					//mGoogleDriveSync.startSyncFromOnly(Synchronizer.SYNC_FLAG_SHOW_SIGN_IN | Synchronizer.SYNC_FLAG_QUIETLY | Synchronizer.SYNC_FLAG_SHOW_PROGRESS | (mGoogleDriveSyncOpts.AskConfirmations ? Synchronizer.SYNC_FLAG_ASK_CHANGED : 0), Synchronizer.SyncTarget.SETTINGS, Synchronizer.SyncTarget.BOOKMARKS);
					syncServiceAccessor.bind(sync -> {
						sync.setSynchronizer(mGoogleDriveSync);
						sync.setOnSyncStatusListener(mGoogleDriveSyncStatusListener);
						sync.startSyncFromOnly(Synchronizer.SYNC_FLAG_SHOW_SIGN_IN | Synchronizer.SYNC_FLAG_QUIETLY | Synchronizer.SYNC_FLAG_SHOW_PROGRESS | (mGoogleDriveSyncOpts.AskConfirmations ? Synchronizer.SYNC_FLAG_ASK_CHANGED : 0), Synchronizer.SyncTarget.SETTINGS, Synchronizer.SyncTarget.BOOKMARKS);
					});
				}
			}
		}
		 */
		activityIsRunning = true;
	}

	@Override
	protected void onSaveInstanceState(Bundle outState) {
		log.i("CoolReader.onSaveInstanceState()");
		super.onSaveInstanceState(outState);
	}

	static final boolean LOAD_LAST_DOCUMENT_ON_START = true;

	@Override
	protected void onStart() {
		log.i("CoolReader.onStart() version=" + getVersion());
		super.onStart();

		//		BackgroundThread.instance().postGUI(new Runnable() {
//			public void run() {
//				// fixing font settings
//				Properties settings = mReaderView.getSettings();
//				if (SettingsManager.instance(CoolReader.this).fixFontSettings(settings)) {
//					log.i("Missing font settings were fixed");
//					mBrowser.setCoverPageFontFace(settings.getProperty(ReaderView.PROP_FONT_FACE, DeviceInfo.DEF_FONT_FACE));
//					mReaderView.setSettings(settings, null);
//				}
//			}
//		});

		if (mHomeFrame == null) {
			waitForCRDBService(() -> {
				Services.getHistory().loadFromDB(getDB(), 200);

				mHomeFrame = new CRRootView(CoolReader.this);
				Services.getCoverpageManager().addCoverpageReadyListener(mHomeFrame);
				mHomeFrame.requestFocus();

				showRootWindow();
				setSystemUiVisibility();

				notifySettingsChanged();

				showNotifications();

				isInterfaceCreated = true;
			});
		}

		if (isBookOpened()) {
			showOpenedBook();
			return;
		}

		if (!isFirstStart)
			return;
		isFirstStart = false;

		if (justCreated) {
			justCreated = false;
			if (!processIntent(getIntent()))
				showLastLocation();
		}
		if (dataDirIsRemoved) {
			// show message
			ErrorDialog dlg = new ErrorDialog(this, getString(R.string.error), getString(R.string.datadir_is_removed, Engine.getExternalSettingsDirName()));
			dlg.show();
		}
		if (Engine.getExternalSettingsDirName() != null) {
			setExtDataDirCreateTime(new Date());
		} else {
			setExtDataDirCreateTime(null);
		}
		stopped = false;

		log.i("CoolReader.onStart() exiting");
	}


	private boolean stopped = false;

	@Override
	protected void onStop() {
		log.i("CoolReader.onStop() entering");
		// Donations support code
		super.onStop();
		stopped = true;
		// will close book at onDestroy()
		if (CLOSE_BOOK_ON_STOP)
			mReaderView.close();

		log.i("CoolReader.onStop() exiting");
	}

	private void requestStoragePermissions() {
		// check or request permission for storage
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
			int readExtStoragePermissionCheck = checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE);
			int writeExtStoragePermissionCheck = checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE);
			ArrayList<String> needPerms = new ArrayList<>();
			if (PackageManager.PERMISSION_GRANTED != readExtStoragePermissionCheck) {
				needPerms.add(Manifest.permission.READ_EXTERNAL_STORAGE);
			} else {
				log.i("READ_EXTERNAL_STORAGE permission already granted.");
			}
			if (PackageManager.PERMISSION_GRANTED != writeExtStoragePermissionCheck) {
				needPerms.add(Manifest.permission.WRITE_EXTERNAL_STORAGE);
			} else {
				log.i("WRITE_EXTERNAL_STORAGE permission already granted.");
			}
			if (!needPerms.isEmpty()) {
				// TODO: Show an explanation to the user
				// Show an explanation to the user *asynchronously* -- don't block
				// this thread waiting for the user's response! After the user
				// sees the explanation, try again to request the permission.
				String[] templ = new String[0];
				log.i("Some permissions DENIED, requesting from user these permissions: " + needPerms.toString());
				// request permission from user
				requestPermissions(needPerms.toArray(templ), REQUEST_CODE_STORAGE_PERM);
			}
		}
	}

	private void requestReadPhoneStatePermissions() {
		// check or request permission to read phone state
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
			int phoneStatePermissionCheck = checkSelfPermission(Manifest.permission.READ_PHONE_STATE);
			if (PackageManager.PERMISSION_GRANTED != phoneStatePermissionCheck) {
				log.i("READ_PHONE_STATE permission DENIED, requesting from user");
				// TODO: Show an explanation to the user
				// Show an explanation to the user *asynchronously* -- don't block
				// this thread waiting for the user's response! After the user
				// sees the explanation, try again to request the permission.
				// request permission from user
				requestPermissions(new String[]{Manifest.permission.READ_PHONE_STATE}, REQUEST_CODE_READ_PHONE_STATE_PERM);
			} else {
				log.i("READ_PHONE_STATE permission already granted.");
			}
		}
	}

	public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
		log.i("CoolReader.onRequestPermissionsResult()");
		if (REQUEST_CODE_STORAGE_PERM == requestCode) {        // external storage read & write permissions
			int ext_sd_perm_count = 0;
			//boolean read_phone_state_granted = false;
			for (int i = 0; i < permissions.length; i++) {
				if (grantResults[i] == PackageManager.PERMISSION_GRANTED)
					log.i("Permission " + permissions[i] + " GRANTED");
				else
					log.i("Permission " + permissions[i] + " DENIED");
				if (permissions[i].compareTo(Manifest.permission.READ_EXTERNAL_STORAGE) == 0 && grantResults[i] == PackageManager.PERMISSION_GRANTED)
					ext_sd_perm_count++;
				else if (permissions[i].compareTo(Manifest.permission.WRITE_EXTERNAL_STORAGE) == 0 && grantResults[i] == PackageManager.PERMISSION_GRANTED)
					ext_sd_perm_count++;
			}
			if (2 == ext_sd_perm_count) {
				log.i("read&write to storage permissions GRANTED, adding sd card mount point...");
				Services.refreshServices(this);
				rebaseSettings();
				waitForCRDBService(() -> {
					getDBService().setPathCorrector(Engine.getInstance(CoolReader.this).getPathCorrector());
					getDB().reopenDatabase();
					Services.getHistory().loadFromDB(getDB(), 200);
				});
				mHomeFrame.refreshView();
			}
			if (Engine.getExternalSettingsDirName() != null) {
				setExtDataDirCreateTime(new Date());
			} else {
				setExtDataDirCreateTime(null);
			}
		} else if (REQUEST_CODE_READ_PHONE_STATE_PERM == requestCode) {
			if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
				log.i("read phone state permission is GRANTED, registering phone activity handler...");
				PhoneStateReceiver.setPhoneActivityHandler(() -> {
					if (mReaderView != null) {
						mReaderView.pauseTTS();
						mReaderView.save();
					}
				});
				phoneStateChangeHandlerInstalled = true;
			} else {
				log.i("Read phone state permission is DENIED!");
			}
		}
	}

	private static Debug.MemoryInfo info = new Debug.MemoryInfo();
	private static Field[] infoFields = Debug.MemoryInfo.class.getFields();

	private static String dumpFields(Field[] fields, Object obj) {
		StringBuilder buf = new StringBuilder();
		try {
			for (Field f : fields) {
				if (buf.length() > 0)
					buf.append(", ");
				buf.append(f.getName());
				buf.append("=");
				buf.append(f.get(obj));
			}
		} catch (Exception e) {

		}
		return buf.toString();
	}

	public static void dumpHeapAllocation() {
		Debug.getMemoryInfo(info);
		log.d("nativeHeapAlloc=" + Debug.getNativeHeapAllocatedSize() + ", nativeHeapSize=" + Debug.getNativeHeapSize() + ", info: " + dumpFields(infoFields, info));
	}


	@Override
	public void setCurrentTheme(InterfaceTheme theme) {
		super.setCurrentTheme(theme);
		if (mHomeFrame != null)
			mHomeFrame.onThemeChange(theme);
		if (mBrowser != null)
			mBrowser.onThemeChanged();
		if (mBrowserFrame != null)
			mBrowserFrame.onThemeChanged(theme);
		//getWindow().setBackgroundDrawable(theme.getActionBarBackgroundDrawableBrowser());
	}

	public void directoryUpdated(FileInfo dir, FileInfo selected) {
		if (dir.isOPDSRoot())
			mHomeFrame.refreshOnlineCatalogs();
		else if (dir.isRecentDir())
			mHomeFrame.refreshRecentBooks();
		if (mBrowser != null)
			mBrowser.refreshDirectory(dir, selected);
	}

	public void directoryUpdated(FileInfo dir) {
		directoryUpdated(dir, null);
	}

	@Override
	public void onSettingsChanged(Properties props, Properties oldProps) {
		Properties changedProps = oldProps != null ? props.diff(oldProps) : props;
		if (mHomeFrame != null) {
			mHomeFrame.refreshOnlineCatalogs();
		}
		if (mReaderFrame != null) {
			mReaderFrame.updateSettings(props);
			if (mReaderView != null)
				mReaderView.updateSettings(props);
		}
		for (Map.Entry<Object, Object> entry : changedProps.entrySet()) {
			String key = (String) entry.getKey();
			String value = (String) entry.getValue();
			applyAppSetting(key, value);
		}
		// Show/Hide soft navbar after OptionDialog is closed.
		applyFullscreen(getWindow());
		if (!justCreated && isInterfaceCreated) {
			// Only after onStart()!
			/*
			  Commented until the appearance of free implementation of the binding to the Google Drive (R)
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				if (mGoogleDriveSyncOpts.Enabled && !mSyncGoogleDriveEnabledPrev && null != mGoogleDriveSync) {
					// if cloud sync has just been enabled in options dialog
					//mGoogleDriveSync.startSyncFrom(Synchronizer.SYNC_FLAG_SHOW_SIGN_IN | Synchronizer.SYNC_FLAG_QUIETLY | Synchronizer.SYNC_FLAG_SHOW_PROGRESS | (mGoogleDriveSyncOpts.AskConfirmations ? Synchronizer.SYNC_FLAG_ASK_CHANGED : 0) );
					syncServiceAccessor.bind(sync -> {
						sync.setSynchronizer(mGoogleDriveSync);
						sync.setOnSyncStatusListener(mGoogleDriveSyncStatusListener);
						sync.startSyncFrom(Synchronizer.SYNC_FLAG_SHOW_SIGN_IN | Synchronizer.SYNC_FLAG_QUIETLY | Synchronizer.SYNC_FLAG_SHOW_PROGRESS | (mGoogleDriveSyncOpts.AskConfirmations ? Synchronizer.SYNC_FLAG_ASK_CHANGED : 0) );
					});
					mSyncGoogleDriveEnabledPrev = mGoogleDriveSyncOpts.Enabled;
					return;
				}
				if (changedProps.size() > 0) {
					// After options dialog is closed, sync new settings to the cloud with delay
					BackgroundThread.instance().postGUI(() -> {
						if (mGoogleDriveSyncOpts.Enabled && mGoogleDriveSyncOpts.SyncSettings && null != mGoogleDriveSync) {
							if (mSuppressSettingsCopyToCloud) {
								// Immediately after downloading settings from Google Drive
								// prevent uploading settings file
								mSuppressSettingsCopyToCloud = false;
							} else {
								// After setting changed in OptionsDialog
								log.d("Some settings is changed, uploading to cloud...");
								//mGoogleDriveSync.startSyncToOnly(null, Synchronizer.SYNC_FLAG_SHOW_SIGN_IN | Synchronizer.SYNC_FLAG_QUIETLY | Synchronizer.SYNC_FLAG_SHOW_PROGRESS, Synchronizer.SyncTarget.SETTINGS);
								syncServiceAccessor.bind(sync -> {
									sync.setSynchronizer(mGoogleDriveSync);
									sync.setOnSyncStatusListener(mGoogleDriveSyncStatusListener);
									sync.startSyncToOnly(null, Synchronizer.SYNC_FLAG_SHOW_SIGN_IN | Synchronizer.SYNC_FLAG_QUIETLY | Synchronizer.SYNC_FLAG_SHOW_PROGRESS, Synchronizer.SyncTarget.SETTINGS);
								});
							}
						}
					}, 500);
				}
			}
			 */
			validateSettings();
		}
	}

	protected boolean allowLowBrightness() {
		// override to force higher brightness in non-reading mode (to avoid black screen on some devices when brightness level set to small value)
		return mCurrentFrame == mReaderFrame;
	}


	public ViewGroup getPreviousFrame() {
		return mPreviousFrame;
	}

	public boolean isPreviousFrameHome() {
		return mPreviousFrame != null && mPreviousFrame == mHomeFrame;
	}

	private void setCurrentFrame(ViewGroup newFrame) {
		if (mCurrentFrame != newFrame) {
			mPreviousFrame = mCurrentFrame;
			log.i("New current frame: " + newFrame.getClass().toString());
			mCurrentFrame = newFrame;
			setContentView(mCurrentFrame);
			mCurrentFrame.requestFocus();
			if (mCurrentFrame != mReaderFrame)
				releaseBacklightControl();
			if (mCurrentFrame == mHomeFrame) {
				// update recent books
				mHomeFrame.refreshRecentBooks();
				setLastLocationRoot();
				mCurrentFrame.invalidate();
			}
			if (mCurrentFrame == mBrowserFrame) {
				// update recent books directory
				mBrowser.refreshDirectory(Services.getScanner().getRecentDir(), null);
			} else {
				if (null != mBrowser)
					mBrowser.stopCurrentScan();
			}
			onUserActivity();
		}
	}

	public void showReader() {
		runInReader(() -> {
			// do nothing
		});
	}

	public void showRootWindow() {
		if (null != mBrowser)
			mBrowser.stopCurrentScan();
		setCurrentFrame(mHomeFrame);
		if (isInterfaceCreated) {
			/*
			  Commented until the appearance of free implementation of the binding to the Google Drive (R)
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				// Save bookmarks and current reading position on the cloud
				if (mGoogleDriveSyncOpts.Enabled && null != mGoogleDriveSync) {
					//mGoogleDriveSync.startSyncToOnly(getCurrentBookInfo(), Synchronizer.SYNC_FLAG_QUIETLY, Synchronizer.SyncTarget.BOOKMARKS);
					syncServiceAccessor.bind(sync -> {
						sync.setSynchronizer(mGoogleDriveSync);
						sync.setOnSyncStatusListener(mGoogleDriveSyncStatusListener);
						sync.startSyncToOnly(getCurrentBookInfo(), Synchronizer.SYNC_FLAG_QUIETLY, Synchronizer.SyncTarget.BOOKMARKS);
					});
				}
			}
			 */
		}
	}

	private void runInReader(final Runnable task) {
		if (null != mBrowser)
			mBrowser.stopCurrentScan();
		waitForCRDBService(() -> {
			if (mReaderFrame != null) {
				task.run();
				setCurrentFrame(mReaderFrame);
				if (mReaderView != null && mReaderView.getSurface() != null) {
					mReaderView.getSurface().setFocusable(true);
					mReaderView.getSurface().setFocusableInTouchMode(true);
					mReaderView.getSurface().requestFocus();
				} else {
					log.w("runInReader: mReaderView or mReaderView.getSurface() is null");
				}
			} else {
				mReaderView = new ReaderView(CoolReader.this, mEngine, settings());
				mReaderFrame = new ReaderViewLayout(CoolReader.this, mReaderView);
				mReaderFrame.getToolBar().setOnActionHandler(item -> {
					if (mReaderView != null)
						mReaderView.onAction(item);
					return true;
				});
				task.run();
				setCurrentFrame(mReaderFrame);
				if (mReaderView.getSurface() != null) {
					mReaderView.getSurface().setFocusable(true);
					mReaderView.getSurface().setFocusableInTouchMode(true);
					mReaderView.getSurface().requestFocus();
				}
				mReaderView.setBatteryState(initialBatteryState, initialBatteryChargeConn, initialBatteryLevel);
				mReaderView.doEngineCommand(ReaderCommand.DCMD_SET_ROTATION_INFO_FOR_AA, screenRotation);
			}
		});
	}

	public boolean isBrowserCreated() {
		return mBrowserFrame != null;
	}

	private void runInBrowser(final Runnable task) {
		waitForCRDBService(() -> {
			if (mBrowserFrame == null) {
				mBrowser = new FileBrowser(CoolReader.this, Services.getEngine(), Services.getScanner(), Services.getHistory(), settings().getBool(PROP_APP_FILE_BROWSER_HIDE_EMPTY_GENRES, false));
				mBrowser.setCoverPagesEnabled(settings().getBool(ReaderView.PROP_APP_SHOW_COVERPAGES, true));
				mBrowser.setCoverPageFontFace(settings().getProperty(ReaderView.PROP_FONT_FACE, DeviceInfo.DEF_FONT_FACE));
				mBrowser.setCoverPageSizeOption(settings().getInt(ReaderView.PROP_APP_COVERPAGE_SIZE, 1));
				mBrowser.setSortOrder(settings().getProperty(ReaderView.PROP_APP_BOOK_SORT_ORDER));
				mBrowser.setSimpleViewMode(settings().getBool(ReaderView.PROP_APP_FILE_BROWSER_SIMPLE_MODE, false));
				mBrowser.init();

				LayoutInflater inflater = LayoutInflater.from(CoolReader.this);// activity.getLayoutInflater();

				mBrowserTitleBar = inflater.inflate(R.layout.browser_status_bar, null);
				setBrowserTitle("Cool Reader browser window");

				mBrowserToolBar = new CRToolBar(CoolReader.this, ReaderAction.createList(
						ReaderAction.FILE_BROWSER_UP,
						ReaderAction.CURRENT_BOOK,
						ReaderAction.OPTIONS,
						ReaderAction.FILE_BROWSER_ROOT,
						ReaderAction.RECENT_BOOKS,
						ReaderAction.CURRENT_BOOK_DIRECTORY,
						ReaderAction.OPDS_CATALOGS,
						ReaderAction.SEARCH,
						ReaderAction.SCAN_DIRECTORY_RECURSIVE,
						ReaderAction.FILE_BROWSER_SORT_ORDER,
						ReaderAction.SAVE_LOGCAT,
						ReaderAction.EXIT
				), false);
				mBrowserToolBar.setBackgroundResource(R.drawable.ui_status_background_browser_dark);
				mBrowserToolBar.setOnActionHandler(item -> {
					switch (item.cmd) {
						case DCMD_EXIT:
							//
							finish();
							break;
						case DCMD_FILE_BROWSER_ROOT:
							showRootWindow();
							break;
						case DCMD_FILE_BROWSER_UP:
							mBrowser.showParentDirectory();
							break;
						case DCMD_OPDS_CATALOGS:
							mBrowser.showOPDSRootDirectory();
							break;
						case DCMD_RECENT_BOOKS_LIST:
							mBrowser.showRecentBooks();
							break;
						case DCMD_SEARCH:
							mBrowser.showFindBookDialog();
							break;
						case DCMD_CURRENT_BOOK:
							showCurrentBook();
							break;
						case DCMD_OPTIONS_DIALOG:
							showBrowserOptionsDialog();
							break;
						case DCMD_SCAN_DIRECTORY_RECURSIVE:
							mBrowser.scanCurrentDirectoryRecursive();
							break;
						case DCMD_FILE_BROWSER_SORT_ORDER:
							mBrowser.showSortOrderMenu();
							break;
						case DCMD_SAVE_LOGCAT:
							createLogcatFile();
							break;
						default:
							// do nothing
							break;
					}
					return false;
				});
				mBrowserFrame = new BrowserViewLayout(CoolReader.this, mBrowser, mBrowserToolBar, mBrowserTitleBar);

				//					if (getIntent() == null)
//						mBrowser.showDirectory(Services.getScanner().getDownloadDirectory(), null);
			}
			task.run();
			setCurrentFrame(mBrowserFrame);
		});

	}

	public void showBrowser() {
		runInBrowser(() -> {
			// do nothing, browser is shown
		});
	}

	public void showManual() {
		loadDocument("@manual", null, null, false);
	}

	public static final String OPEN_FILE_PARAM = "FILE_TO_OPEN";

	public void loadDocument(final String item, final Runnable doneCallback, final Runnable errorCallback, final boolean forceSync) {
		runInReader(() -> mReaderView.loadDocument(item, forceSync ? () -> {
			if (null != doneCallback)
				doneCallback.run();
			/*
			  Commented until the appearance of free implementation of the binding to the Google Drive (R)
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				// Save last opened document on cloud
				if (mGoogleDriveSyncOpts.Enabled && null != mGoogleDriveSync) {
					ArrayList<Synchronizer.SyncTarget> targets = new ArrayList<Synchronizer.SyncTarget>();
					if (mGoogleDriveSyncOpts.SyncCurrentBookInfo)
						targets.add(Synchronizer.SyncTarget.CURRENTBOOKINFO);
					if (mGoogleDriveSyncOpts.SyncCurrentBookBody)
						targets.add(Synchronizer.SyncTarget.CURRENTBOOKBODY);
					if (!targets.isEmpty()) {
						//mGoogleDriveSync.startSyncToOnly(getCurrentBookInfo(), Synchronizer.SYNC_FLAG_SHOW_SIGN_IN | Synchronizer.SYNC_FLAG_QUIETLY | Synchronizer.SYNC_FLAG_SHOW_PROGRESS, targets.toArray(new Synchronizer.SyncTarget[0]));
						syncServiceAccessor.bind(sync -> {
							sync.setSynchronizer(mGoogleDriveSync);
							sync.setOnSyncStatusListener(mGoogleDriveSyncStatusListener);
							sync.startSyncToOnly(getCurrentBookInfo(), Synchronizer.SYNC_FLAG_SHOW_SIGN_IN | Synchronizer.SYNC_FLAG_QUIETLY | Synchronizer.SYNC_FLAG_SHOW_PROGRESS, targets.toArray(new Synchronizer.SyncTarget[0]));
						});
					}
				}
			}
			 */
		} : doneCallback, errorCallback));
	}

	public void loadDocumentFromUri(Uri uri, Runnable doneCallback, Runnable errorCallback) {
		runInReader(() -> {
			ContentResolver contentResolver = getContentResolver();
			InputStream inputStream;
			try {
				inputStream = contentResolver.openInputStream(uri);
				// TODO: Fix this
				// Don't save the last opened document from the stream in the cloud, since we still cannot open it later in this program.
				mReaderView.loadDocumentFromStream(inputStream, uri.getPath(), doneCallback, errorCallback);
			} catch (Exception e) {
				errorCallback.run();
			}
		});
	}

	public void loadDocument(FileInfo item, boolean forceSync) {
		loadDocument(item, null, null, forceSync);
	}

	public void loadDocument(FileInfo item, Runnable doneCallback, Runnable errorCallback, boolean forceSync) {
		log.d("Activities.loadDocument(" + item.pathname + ")");
		loadDocument(item.getPathName(), doneCallback, errorCallback, forceSync);
	}

	/**
	 * When current book is opened, switch to previous book.
	 *
	 * @param errorCallback
	 */
	public void loadPreviousDocument(Runnable errorCallback) {
		BookInfo bi = Services.getHistory().getPreviousBook();
		if (bi != null && bi.getFileInfo() != null) {
			log.i("loadPreviousDocument() is called, prevBookName = " + bi.getFileInfo().getPathName());
			loadDocument(bi.getFileInfo(), null, errorCallback, true);
			return;
		}
		errorCallback.run();
	}

	public void showOpenedBook() {
		showReader();
	}

	public static final String OPEN_DIR_PARAM = "DIR_TO_OPEN";

	public void showBrowser(final FileInfo dir) {
		runInBrowser(() -> mBrowser.showDirectory(dir, null));
	}

	public void showBrowser(final String dir) {
		runInBrowser(() -> mBrowser.showDirectory(Services.getScanner().pathToFileInfo(dir), null));
	}

	public void showRecentBooks() {
		log.d("Activities.showRecentBooks() is called");
		runInBrowser(() -> mBrowser.showRecentBooks());
	}

	public void showOnlineCatalogs() {
		log.d("Activities.showOnlineCatalogs() is called");
		runInBrowser(() -> mBrowser.showOPDSRootDirectory());
	}

	public void showDirectory(FileInfo path) {
		log.d("Activities.showDirectory(" + path + ") is called");
		showBrowser(path);
	}

	public void showCatalog(final FileInfo path) {
		log.d("Activities.showCatalog(" + path + ") is called");
		runInBrowser(() -> mBrowser.showDirectory(path, null));
	}


	public void setBrowserTitle(String title) {
		if (mBrowserFrame != null)
			mBrowserFrame.setBrowserTitle(title);
	}

	public void setBrowserProgressStatus(boolean enable) {
		if (mBrowserFrame != null)
			mBrowserFrame.setBrowserProgressStatus(enable);
	}


	// Dictionary support


	public void findInDictionary(String s) {
		if (s != null && s.length() != 0) {
			int start, end;

			// Skip over non-letter characters at the beginning and end of the search string
			for (start = 0; start < s.length(); start++)
				if (Character.isLetterOrDigit(s.charAt(start)))
					break;
			for (end = s.length() - 1; end >= start; end--)
				if (Character.isLetterOrDigit(s.charAt(end)))
					break;

			if (end > start) {
				final String pattern = s.substring(start, end + 1);

				BackgroundThread.instance().postBackground(() -> BackgroundThread.instance()
						.postGUI(() -> findInDictionaryInternal(pattern), 100));
			}
		}
	}

	private void findInDictionaryInternal(String s) {
		log.d("lookup in dictionary: " + s);
		try {
			mDictionaries.findInDictionary(s);
		} catch (DictionaryException e) {
			showToast(e.getMessage());
		}
	}

	public void showDictionary() {
		findInDictionaryInternal(null);
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent intent) {
		try {
			mDictionaries.onActivityResult(requestCode, resultCode, intent);
		} catch (DictionaryException e) {
			showToast(e.getMessage());
		}
		if (mDonationService != null) {
			mDonationService.onActivityResult(requestCode, resultCode, intent);
		}
		/*
		  Commented until the appearance of free implementation of the binding to the Google Drive (R)
		if (requestCode == REQUEST_CODE_GOOGLE_DRIVE_SIGN_IN) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				if (null != mGoogleDriveSync) {
					mGoogleDriveSync.onActivityResultHandler(requestCode, resultCode, intent);
				}
			}
		} else */ if (requestCode == REQUEST_CODE_OPEN_DOCUMENT_TREE) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
				if (resultCode == Activity.RESULT_OK) {
					switch (mOpenDocumentTreeCommand) {
						case ODT_CMD_DEL_FILE:
							if (mOpenDocumentTreeArg != null && !mOpenDocumentTreeArg.isDirectory) {
								Uri sdCardUri = intent.getData();
								Uri docUri = DocumentsContractWrapper.getDocumentUri(mOpenDocumentTreeArg, this, sdCardUri);
								if (null != docUri) {
									if (DocumentsContractWrapper.deleteFile(this, docUri)) {
										Services.getHistory().removeBookInfo(getDB(), mOpenDocumentTreeArg, true, true);
										FileInfo dirToUpdate = mOpenDocumentTreeArg.parent;
										if (null != dirToUpdate)
											BackgroundThread.instance().postGUI(() -> directoryUpdated(dirToUpdate), 700);
										updateExtSDURI(mOpenDocumentTreeArg, sdCardUri);
									} else {
										showToast(R.string.could_not_delete_file, mOpenDocumentTreeArg);
									}
								} else {
									showToast(R.string.could_not_delete_on_sd);
								}
							}
							break;
						case ODT_CMD_DEL_FOLDER:
							if (mOpenDocumentTreeArg != null && mOpenDocumentTreeArg.isDirectory) {
								Uri sdCardUri = intent.getData();
								Uri documentUri = null;
								if (null != sdCardUri)
									documentUri = DocumentsContractWrapper.getDocumentUri(mOpenDocumentTreeArg, this, sdCardUri);
								if (null != documentUri) {
									if (DocumentsContractWrapper.fileExists(this, documentUri)) {
										updateExtSDURI(mOpenDocumentTreeArg, sdCardUri);
										deleteFolder(mOpenDocumentTreeArg);
									}
								} else {
									showToast(R.string.could_not_delete_on_sd);
								}
							}
							break;
						case ODT_CMD_SAVE_LOGCAT:
							if (mOpenDocumentTreeArg != null) {
								Uri uri = intent.getData();
								if (null != uri) {
									Uri docFolderUri = DocumentsContractWrapper.buildDocumentUriUsingTree(uri);
									if (null != docFolderUri) {
										Uri fileUri = DocumentsContractWrapper.createFile(this, docFolderUri, "text/x-log", mOpenDocumentTreeArg.filename);
										if (null != fileUri) {
											try {
												OutputStream ostream = getContentResolver().openOutputStream(fileUri);
												if (null != ostream) {
													saveLogcat(mOpenDocumentTreeArg.filename, ostream);
													ostream.close();
												} else {
													log.e("logcat: failed to open stream!");
												}
											} catch (Exception e) {
												log.e("logcat: " + e);
											}
										} else {
											log.e("logcat: can't create file!");
										}
									}
								} else {
									log.d("logcat creation canceled by user");
								}
							}
							break;
					}
					mOpenDocumentTreeArg = null;
				}
			}
		} //if (requestCode == REQUEST_CODE_OPEN_DOCUMENT_TREE)
	}

	public void setDict(String id) {
		mDictionaries.setDict(id);
	}

	public void setDict2(String id) {
		mDictionaries.setDict2(id);
	}

	public void setToolbarAppearance(String id) {
		mOptionAppearance = id;
	}

	public String getToolbarAppearance() {
		return mOptionAppearance;
	}

	public void showAboutDialog() {
		AboutDialog dlg = new AboutDialog(this);
		dlg.show();
	}


	private CRDonationService mDonationService = null;
	private DonationListener mDonationListener = null;
	private double mTotalDonations = 0;

	public CRDonationService getDonationService() {
		return mDonationService;
	}

	public boolean isDonationSupported() {
		return mDonationService.isBillingSupported();
	}

	public void setDonationListener(DonationListener listener) {
		mDonationListener = listener;
	}

	public static interface DonationListener {
		void onDonationTotalChanged(double total);
	}

	public double getTotalDonations() {
		return mTotalDonations;
	}

	public boolean makeDonation(final double amount) {
		final String itemName = "donation" + (amount >= 1 ? String.valueOf((int) amount) : String.valueOf(amount));
		log.i("makeDonation is called, itemName=" + itemName);
		if (!mDonationService.isBillingSupported())
			return false;
		BackgroundThread.instance().postBackground(() -> mDonationService.purchase(itemName,
				(success, productId, totalDonations) -> BackgroundThread.instance().postGUI(() -> {
					try {
						if (success) {
							log.i("Donation purchased: " + productId + ", total amount: " + mTotalDonations);
							mTotalDonations += amount;
							SharedPreferences pref = getSharedPreferences(DONATIONS_PREF_FILE, 0);
							pref.edit().putString(DONATIONS_PREF_TOTAL_AMOUNT, String.valueOf(mTotalDonations)).commit();
						} else {
							showToast("Donation purchase failed");
						}
						if (mDonationListener != null)
							mDonationListener.onDonationTotalChanged(mTotalDonations);
					} catch (Exception e) {
						// ignore
					}
				})));
		return true;
	}

	private static String DONATIONS_PREF_FILE = "cr3donations";
	private static String DONATIONS_PREF_TOTAL_AMOUNT = "total";


	public void initTTS(TTSControlServiceAccessor.Callback callback) {
		if (!phoneStateChangeHandlerInstalled) {
			// TODO: Investigate the need to tracking state of the phone, while we already respect the audio focus.
			boolean readPhoneStateIsAvailable;
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
				readPhoneStateIsAvailable = checkSelfPermission(Manifest.permission.READ_PHONE_STATE) == PackageManager.PERMISSION_GRANTED;
			} else
				readPhoneStateIsAvailable = true;
			if (!readPhoneStateIsAvailable) {
				// assumed Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
				requestReadPhoneStatePermissions();
			} else {
				// On Android API less than 23 phone read state permission is granted
				// after application install (permission requested while application installing).
				log.i("read phone state permission already GRANTED, registering phone activity handler...");
				PhoneStateReceiver.setPhoneActivityHandler(() -> {
					if (mReaderView != null) {
						mReaderView.pauseTTS();
						mReaderView.save();
					}
				});
				phoneStateChangeHandlerInstalled = true;
			}
		}

		showToast("Initializing TTS");
		if (null == ttsControlServiceAccessor)
			ttsControlServiceAccessor = new TTSControlServiceAccessor(this);
		ttsControlServiceAccessor.bind(ttsbinder -> {
			ttsbinder.initTTS(ttsEnginePackage, new OnTTSCreatedListener() {
				@Override
				public void onCreated() {
					if (null != callback)
						callback.run(ttsControlServiceAccessor);
				}

				@Override
				public void onFailed() {
					BackgroundThread.instance().executeGUI(() -> showToast("Cannot initialize TTS"));
				}

				@Override
				public void onTimedOut() {
					// TTS engine init hangs, remove it from settings
					log.e("TTS engine \"" + ttsEnginePackage + "\" init failure, disabling!");
					BackgroundThread.instance().executeGUI(() -> {
						showToast(R.string.tts_init_failure, ttsEnginePackage);
						setSetting(PROP_APP_TTS_ENGINE, "", false);
						ttsEnginePackage = "";
						try {
							mReaderView.getTTSToolbar().stopAndClose();
						} catch (Exception ignored) {}
					});
				}
			});
		});
	}

	public void showOptionsDialog(final OptionsDialog.Mode mode) {
		BackgroundThread.instance().postBackground(() -> {
			final String[] mFontFaces = Engine.getFontFaceList();
			BackgroundThread.instance().executeGUI(() -> {
				OptionsDialog dlg = new OptionsDialog(CoolReader.this, mode, mReaderView, mFontFaces, null);
				dlg.show();
			});
		});
	}

	public void updateCurrentPositionStatus(FileInfo book, Bookmark position, PositionProperties props) {
		mReaderFrame.getStatusBar().updateCurrentPositionStatus(book, position, props);
	}


	@Override
	protected void setDimmingAlpha(int dimmingAlpha) {
		if (mReaderView != null)
			mReaderView.setDimmingAlpha(dimmingAlpha);
	}

	public void showReaderMenu() {
		//
		if (mReaderFrame != null) {
			mReaderFrame.showMenu();
		}
	}


	public void sendBookFragment(BookInfo bookInfo, String text) {
		final Intent emailIntent = new Intent(android.content.Intent.ACTION_SEND);
		emailIntent.setType("text/plain");
		emailIntent.putExtra(android.content.Intent.EXTRA_SUBJECT, bookInfo.getFileInfo().getAuthors() + " " + bookInfo.getFileInfo().getTitle());
		emailIntent.putExtra(android.content.Intent.EXTRA_TEXT, text);
		startActivity(Intent.createChooser(emailIntent, null));
	}

	public void showBookmarksDialog() {
		BackgroundThread.instance().executeGUI(() -> {
			BookmarksDlg dlg = new BookmarksDlg(CoolReader.this, mReaderView);
			dlg.show();
		});
	}

	public void openURL(String url) {
		try {
			Intent i = new Intent(Intent.ACTION_VIEW);
			i.setData(Uri.parse(url));
			startActivity(i);
		} catch (Exception e) {
			log.e("Exception " + e + " while trying to open URL " + url);
			showToast("Cannot open URL " + url);
		}
	}


	public boolean isBookOpened() {
		if (mReaderView == null)
			return false;
		return mReaderView.isBookLoaded();
	}

	public void closeBookIfOpened(FileInfo book) {
		if (mReaderView == null)
			return;
		mReaderView.closeIfOpened(book);
	}

	public void askDeleteBook(final FileInfo item) {
		askConfirmation(R.string.win_title_confirm_book_delete, () -> {
			closeBookIfOpened(item);
			FileInfo file = Services.getScanner().findFileInTree(item);
			if (file == null)
				file = item;
			final FileInfo finalFile = file;
			if (file.deleteFile()) {
				waitForCRDBService(() -> {
					Services.getHistory().removeBookInfo(getDB(), finalFile, true, true);
					BackgroundThread.instance().postGUI(() -> directoryUpdated(finalFile.parent, null), 700);
				});
			} else {
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
					Uri docUri = null;
					Uri sdCardUri = getExtSDURIByFileInfo(file);
					if (sdCardUri != null)
						docUri = DocumentsContractWrapper.getDocumentUri(file, this, sdCardUri);
					if (null != docUri) {
						if (DocumentsContractWrapper.deleteFile(this, docUri)) {
							waitForCRDBService(() -> {
								Services.getHistory().removeBookInfo(getDB(), finalFile, true, true);
								BackgroundThread.instance().postGUI(() -> directoryUpdated(finalFile.parent), 700);
							});
						} else {
							showToast(R.string.could_not_delete_file, file);
						}
					} else {
						showToast(R.string.choose_root_sd);
						mOpenDocumentTreeArg = file;
						mOpenDocumentTreeCommand = ODT_CMD_DEL_FILE;
						Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
						startActivityForResult(intent, REQUEST_CODE_OPEN_DOCUMENT_TREE);
					}
				} else {
					showToast(R.string.could_not_delete_file, file);
				}
			}
		});
	}

	public void askDeleteRecent(final FileInfo item) {
		askConfirmation(R.string.win_title_confirm_history_record_delete, () -> waitForCRDBService(() -> {
			Services.getHistory().removeBookInfo(getDB(), item, true, false);
			directoryUpdated(Services.getScanner().createRecentRoot());
		}));
	}

	public void askDeleteCatalog(final FileInfo item) {
		askConfirmation(R.string.win_title_confirm_catalog_delete, () -> {
			if (item != null && item.isOPDSDir()) {
				waitForCRDBService(() -> {
					getDB().removeOPDSCatalog(item.id);
					directoryUpdated(Services.getScanner().createOPDSRoot());
				});
			}
		});
	}

	int mFolderDeleteRetryCount = 0;
	public void askDeleteFolder(final FileInfo item) {
		askConfirmation(R.string.win_title_confirm_folder_delete, () -> {
			mFolderDeleteRetryCount = 0;
			deleteFolder(item);
		});
	}

	private void deleteFolder(final FileInfo item) {
		if (mFolderDeleteRetryCount > 3)
			return;
		if (item != null && item.isDirectory && !item.isOPDSDir() && !item.isOnlineCatalogPluginDir()) {
			FileInfoOperationListener bookDeleteCallback = (fileInfo, errorStatus) -> {
				if (0 == errorStatus && null != fileInfo.format) {
					BackgroundThread.instance().executeGUI(() -> {
						waitForCRDBService(() -> Services.getHistory().removeBookInfo(getDB(), fileInfo, true, true));
					});
				}
			};
			BackgroundThread.instance().postBackground(() -> Utils.deleteFolder(item, bookDeleteCallback, (fileInfo, errorStatus) -> {
				if (0 == errorStatus) {
					BackgroundThread.instance().executeGUI(() -> directoryUpdated(fileInfo.parent));
				} else {
					// Can't be deleted using standard Java I/O,
					// Try DocumentContract wrapper...
					if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
						Uri sdCardUri = getExtSDURIByFileInfo(item);
						if (null != sdCardUri) {
							Utils.deleteFolderDocTree(item, this, sdCardUri, bookDeleteCallback, (fileInfo2, errorStatus2) -> {
								BackgroundThread.instance().executeGUI(() -> {
									if (0 == errorStatus2) {
										directoryUpdated(fileInfo2.parent);
									} else {
										showToast(R.string.choose_root_sd);
										mFolderDeleteRetryCount++;
										mOpenDocumentTreeCommand = ODT_CMD_DEL_FOLDER;
										mOpenDocumentTreeArg = item;
										Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
										startActivityForResult(intent, REQUEST_CODE_OPEN_DOCUMENT_TREE);
									}
								});
							});
						} else {
							BackgroundThread.instance().executeGUI(() -> {
								showToast(R.string.choose_root_sd);
								mFolderDeleteRetryCount++;
								mOpenDocumentTreeCommand = ODT_CMD_DEL_FOLDER;
								mOpenDocumentTreeArg = item;
								Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
								startActivityForResult(intent, REQUEST_CODE_OPEN_DOCUMENT_TREE);
							});
						}
					}
				}
			}));
		}
	}

	public void createLogcatFile() {
		final SimpleDateFormat format = new SimpleDateFormat("'cr3-'yyyy-MM-dd_HH_mm_ss'.log'", Locale.US);
		FileInfo dir = Services.getScanner().getSharedDownloadDirectory();
		if (null == dir) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
				log.d("logcat: no access to download directory, opening document tree...");
				askConfirmation(R.string.confirmation_select_folder_for_log, () -> {
					mOpenDocumentTreeCommand = ODT_CMD_SAVE_LOGCAT;
					mOpenDocumentTreeArg = new FileInfo(format.format(new Date()));
					Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
					startActivityForResult(intent, REQUEST_CODE_OPEN_DOCUMENT_TREE);
				});
			} else {
				log.e("Can't create logcat file: no access to download directory!");
			}
		} else {
			try {
				File outputFile = new File(dir.pathname, format.format(new Date()));
				FileOutputStream ostream = new FileOutputStream(outputFile);
				saveLogcat(outputFile.getCanonicalPath(), ostream);
			} catch (Exception e) {
				log.e("createLogcatFile: " + e);
			}
		}
	}

	private void saveLogcat(String fileName, OutputStream ostream) {
		Date since = getLastLogcatDate();
		Date now = new Date();
		if (LogcatSaver.saveLogcat(since, ostream)) {
			setLastLogcatDate(now);
			log.i("logcat saved to file " + fileName);
			//showToast("Logcat saved to " + fileName);
			showMessage(getString(R.string.win_title_log), getString(R.string.notice_log_saved_to_, fileName));
		} else {
			log.e("Failed to save logcat to " + fileName);
			showToast("Failed to save logcat to " + fileName);
		}
	}

	public void saveSetting(String name, String value) {
		if (mReaderView != null)
			mReaderView.saveSetting(name, value);
	}

	public void editBookInfo(final FileInfo currDirectory, final FileInfo item) {
		waitForCRDBService(() -> Services.getHistory().getOrCreateBookInfo(getDB(), item, bookInfo -> {
			if (bookInfo == null)
				bookInfo = new BookInfo(item);
			BookInfoEditDialog dlg = new BookInfoEditDialog(CoolReader.this, currDirectory, bookInfo,
					currDirectory.isRecentDir());
			dlg.show();
		}));
	}

	public void editOPDSCatalog(FileInfo opds) {
		if (opds == null) {
			opds = new FileInfo();
			opds.isDirectory = true;
			opds.pathname = FileInfo.OPDS_DIR_PREFIX + "http://";
			opds.filename = "New Catalog";
			opds.isListed = true;
			opds.isScanned = true;
			opds.parent = Services.getScanner().getOPDSRoot();
		}
		OPDSCatalogEditDialog dlg = new OPDSCatalogEditDialog(CoolReader.this, opds,
				() -> refreshOPDSRootDirectory(true));
		dlg.show();
	}

	public void refreshOPDSRootDirectory(boolean showInBrowser) {
		if (mBrowser != null)
			mBrowser.refreshOPDSRootDirectory(showInBrowser);
		if (mHomeFrame != null)
			mHomeFrame.refreshOnlineCatalogs();
	}


	private SharedPreferences mPreferences;
	private final static String BOOK_LOCATION_PREFIX = "@book:";
	private final static String DIRECTORY_LOCATION_PREFIX = "@dir:";

	private SharedPreferences getPrefs() {
		if (mPreferences == null)
			mPreferences = getSharedPreferences(PREF_FILE, 0);
		return mPreferences;
	}

	public void setLastBook(String path) {
		setLastLocation(BOOK_LOCATION_PREFIX + path);
	}

	public void setLastDirectory(String path) {
		setLastLocation(DIRECTORY_LOCATION_PREFIX + path);
	}

	public void setLastLocationRoot() {
		setLastLocation(FileInfo.ROOT_DIR_TAG);
	}

	/**
	 * Store last location - to resume after program restart.
	 *
	 * @param location is file name, directory, or special folder tag
	 */
	public void setLastLocation(String location) {
		try {
			String oldLocation = getPrefs().getString(PREF_LAST_LOCATION, null);
			if (oldLocation != null && oldLocation.equals(location))
				return; // not changed
			SharedPreferences.Editor editor = getPrefs().edit();
			editor.putString(PREF_LAST_LOCATION, location);
			editor.commit();
		} catch (Exception e) {
			// ignore
		}
	}

	private static final int NOTIFICATION_READER_MENU_MASK = 0x01;
	private static final int NOTIFICATION_LOGCAT_MASK = 0x02;
	private static final int NOTIFICATION_MASK_ALL = NOTIFICATION_READER_MENU_MASK |
			NOTIFICATION_LOGCAT_MASK;

	public void setLastNotificationMask(int notificationId) {
		try {
			SharedPreferences.Editor editor = getPrefs().edit();
			editor.putInt(PREF_LAST_NOTIFICATION_MASK, notificationId);
			editor.commit();
		} catch (Exception e) {
			// ignore
		}
	}

	public int getLastNotificationMask() {
		int res = getPrefs().getInt(PREF_LAST_NOTIFICATION_MASK, 0);
		log.i("getLastNotification() = " + res);
		return res;
	}


	public void showNotifications() {
		int lastNoticeMask = getLastNotificationMask();
		if ((lastNoticeMask & NOTIFICATION_MASK_ALL) == NOTIFICATION_MASK_ALL)
			return;
		if (DeviceInfo.getSDKLevel() >= DeviceInfo.HONEYCOMB) {
			if ((lastNoticeMask & NOTIFICATION_READER_MENU_MASK) == 0) {
				notification1();
				return;
			}
		}
		if ((lastNoticeMask & NOTIFICATION_LOGCAT_MASK) == 0) {
			notification2();
		}
	}

	public void notification1() {
		if (hasHardwareMenuKey())
			return; // don't show notice if hard key present
		showNotice(R.string.note1_reader_menu,
				R.string.dlg_button_yes, () -> {
					setSetting(PROP_TOOLBAR_LOCATION, String.valueOf(VIEWER_TOOLBAR_SHORT_SIDE), false);
					setLastNotificationMask(getLastNotificationMask() | NOTIFICATION_READER_MENU_MASK);
					showNotifications();
				},
				R.string.dlg_button_no, () -> {
					setSetting(PROP_TOOLBAR_LOCATION, String.valueOf(VIEWER_TOOLBAR_NONE), false);
					setLastNotificationMask(getLastNotificationMask() | NOTIFICATION_READER_MENU_MASK);
					showNotifications();
				}
		);
	}

	public void notification2() {
		showNotice(R.string.note2_logcat,
				() -> {
					setLastNotificationMask(getLastNotificationMask() | NOTIFICATION_LOGCAT_MASK);
					showNotifications();
				}
		);
	}

	/**
	 * Get last stored location.
	 *
	 * @return
	 */
	private String getLastLocation() {
		String res = getPrefs().getString(PREF_LAST_LOCATION, null);
		if (res == null) {
			// import last book value from previous releases
			res = getPrefs().getString(PREF_LAST_BOOK, null);
			if (res != null) {
				res = BOOK_LOCATION_PREFIX + res;
				try {
					getPrefs().edit().remove(PREF_LAST_BOOK).commit();
				} catch (Exception e) {
					// ignore
				}
			}
		}
		log.i("getLastLocation() = " + res);
		return res;
	}

	/**
	 * Open location - book, root view, folder...
	 */
	public void showLastLocation() {
		String location = getLastLocation();
		if (location == null)
			location = FileInfo.ROOT_DIR_TAG;
		if (location.startsWith(BOOK_LOCATION_PREFIX)) {
			location = location.substring(BOOK_LOCATION_PREFIX.length());
			loadDocument(location, null, () -> BackgroundThread.instance().postGUI(() -> {
				// if document not loaded show error & then root window
				ErrorDialog errDialog = new ErrorDialog(CoolReader.this, "Error", "Can't open file!");
				errDialog.setOnDismissListener(dialog -> showRootWindow());
				errDialog.show();
			}, 1000), false);
			return;
		}
		if (location.startsWith(DIRECTORY_LOCATION_PREFIX)) {
			location = location.substring(DIRECTORY_LOCATION_PREFIX.length());
			showBrowser(location);
			return;
		}
		if (location.equals(FileInfo.RECENT_DIR_TAG)) {
			showBrowser(location);
			return;
		}
		// TODO: support other locations as well
		showRootWindow();
	}

	public void setExtDataDirCreateTime(Date d) {
		try {
			SharedPreferences.Editor editor = getPrefs().edit();
			editor.putLong(PREF_EXT_DATADIR_CREATETIME, (null != d) ? d.getTime() : 0);
			editor.commit();
		} catch (Exception e) {
			// ignore
		}
	}

	public long getExtDataDirCreateTime() {
		long res = getPrefs().getLong(PREF_EXT_DATADIR_CREATETIME, 0);
		log.i("getExtDataDirCreateTime() = " + res);
		return res;
	}

	private boolean updateExtSDURI(FileInfo fi, Uri extSDUri) {
		String prefKey = null;
		String filePath = null;
		if (fi.isArchive && fi.arcname != null) {
			filePath = fi.arcname;
		} else
			filePath = fi.pathname;
		if (null != filePath) {
			File f = new File(filePath);
			filePath = f.getAbsolutePath();
			String[] parts = filePath.split("\\/");
			if (parts.length >= 3) {
				// For example,
				// parts[0] = ""
				// parts[1] = "storage"
				// parts[2] = "1501-3F19"
				// then prefKey = "/storage/1501-3F19"
				prefKey = "uri_for_/" + parts[1] + "/" + parts[2];
			}
		}
		if (null != prefKey) {
			SharedPreferences prefs = getPrefs();
			return prefs.edit().putString(prefKey, extSDUri.toString()).commit();
		}
		return false;
	}

	private Uri getExtSDURIByFileInfo(FileInfo fi) {
		Uri uri = null;
		String prefKey = null;
		String filePath = null;
		if (fi.isArchive && fi.arcname != null) {
			filePath = fi.arcname;
		} else
			filePath = fi.pathname;
		if (null != filePath) {
			File f = new File(filePath);
			filePath = f.getAbsolutePath();
			String[] parts = filePath.split("\\/");
			if (parts.length >= 3) {
				prefKey = "uri_for_/" + parts[1] + "/" + parts[2];
			}
		}
		if (null != prefKey) {
			SharedPreferences prefs = getPrefs();
			String strUri = prefs.getString(prefKey, null);
			if (null != strUri)
				uri = Uri.parse(strUri);
		}
		return uri;
	}

	private Date getLastLogcatDate() {
		long dateMillis = getPrefs().getLong(PREF_LAST_LOGCAT, 0);
		return new Date(dateMillis);
	}

	private void setLastLogcatDate(Date date) {
		SharedPreferences.Editor editor = getPrefs().edit();
		editor.putLong(PREF_LAST_LOGCAT, date.getTime());
		editor.commit();
	}

	public void showCurrentBook() {
		BookInfo bi = Services.getHistory().getLastBook();
		if (bi != null)
			loadDocument(bi.getFileInfo(), false);
	}

}
