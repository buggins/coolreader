// Main Class
package org.coolreader;

import android.Manifest;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Debug;
import android.view.LayoutInflater;
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
import org.coolreader.crengine.Engine;
import org.coolreader.crengine.ErrorDialog;
import org.coolreader.crengine.FileBrowser;
import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.InterfaceTheme;
import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;
import org.coolreader.crengine.N2EpdController;
import org.coolreader.crengine.OPDSCatalogEditDialog;
import org.coolreader.crengine.OptionsDialog;
import org.coolreader.crengine.PositionProperties;
import org.coolreader.crengine.Properties;
import org.coolreader.crengine.ReaderAction;
import org.coolreader.crengine.ReaderView;
import org.coolreader.crengine.ReaderViewLayout;
import org.coolreader.crengine.Services;
import org.coolreader.crengine.TTS;
import org.coolreader.crengine.TTS.OnTTSCreatedListener;
import org.coolreader.donations.CRDonationService;
import org.coolreader.sync2.OnSyncStatusListener;
import org.coolreader.sync2.Synchronizer;
import org.coolreader.sync2.googledrive.GoogleDriveRemoteAccess;
import org.koekak.android.ebookdownloader.SonyBookSelector;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Date;
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

	private BookInfo mBookInfoToSync;
	private boolean mSyncGoogleDriveEnabled = false;
	private boolean mSyncGoogleDriveEnabledSettings = false;
	private boolean mSyncGoogleDriveEnabledBookmarks = false;
	private boolean mSyncGoogleDriveEnabledCurrentBooks = false;
	private Synchronizer mGoogleDriveSync;
	// can be add more synchronizers
	private boolean mSuppressSettingsCopyToCloud;

	private String mOptionAppearance = "0";

	private String fileToLoadOnStart = null;

	private boolean isFirstStart = true;
	private boolean phoneStateChangeHandlerInstalled = false;
	private int initialBatteryState = -1;
	private BroadcastReceiver intentReceiver;

	private boolean justCreated = false;

	private boolean dataDirIsRemoved = false;

	private static final int REQUEST_CODE_STORAGE_PERM = 1;
	private static final int REQUEST_CODE_READ_PHONE_STATE_PERM = 2;
	private static final int REQUEST_CODE_GOOGLE_DRIVE_SIGN_IN = 3;

	/**
	 * Called when the activity is first created.
	 */
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		startServices();

		log.i("CoolReader.onCreate() entered");
		super.onCreate(savedInstanceState);

		// Can request only one set of permissions at a time
		// Then request all permission at a time.
		requestStoragePermissions();

		// apply settings
		onSettingsChanged(settings(), null);

		isFirstStart = true;
		justCreated = true;

		mEngine = Engine.getInstance(this);

		//requestWindowFeature(Window.FEATURE_NO_TITLE);

		//==========================================
		// Battery state listener
		intentReceiver = new BroadcastReceiver() {

			@Override
			public void onReceive(Context context, Intent intent) {
				int level = intent.getIntExtra("level", 0);
				if (mReaderView != null)
					mReaderView.setBatteryState(level);
				else
					initialBatteryState = level;
			}

		};
		registerReceiver(intentReceiver, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));

		setVolumeControlStream(AudioManager.STREAM_MUSIC);

		if (initialBatteryState >= 0 && mReaderView != null)
			mReaderView.setBatteryState(initialBatteryState);

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

		if (tts != null) {
			tts.shutdown();
			tts = null;
			ttsInitialized = false;
			ttsError = false;
		}


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
		if (intentReceiver != null) {
			unregisterReceiver(intentReceiver);
			intentReceiver = null;
		}

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
			int n = 0;
			try {
				n = Integer.parseInt(value);
			} catch (NumberFormatException e) {
				// ignore
			}
			if (n < 0)
				n = 0;
			else if (n > 2)
				n = 2;
			if (mBrowser != null)
				mBrowser.setCoverPageSizeOption(n);
		} else if (key.equals(PROP_APP_FILE_BROWSER_SIMPLE_MODE)) {
			if (mBrowser != null)
				mBrowser.setSimpleViewMode(flg);
		} else if (key.equals(PROP_APP_CLOUDSYNC_GOOGLEDRIVE_ENABLED)) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				mSyncGoogleDriveEnabled = flg;
				updateGoogleDriveSynchronizer();
			}
		} else if (key.equals(PROP_APP_CLOUDSYNC_GOOGLEDRIVE_SETTINGS)) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				mSyncGoogleDriveEnabledSettings = flg;
				updateGoogleDriveSynchronizer();
			}
		} else if (key.equals(PROP_APP_CLOUDSYNC_GOOGLEDRIVE_BOOKMARKS)) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				mSyncGoogleDriveEnabledBookmarks = flg;
				updateGoogleDriveSynchronizer();
			}
		} else if (key.equals(PROP_APP_CLOUDSYNC_GOOGLEDRIVE_CURRENTBOOK)) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				mSyncGoogleDriveEnabledCurrentBooks = flg;
				updateGoogleDriveSynchronizer();
			}
		}
		//
	}

	private void buildGoogleDriveSynchronizer() {
		if (null != mGoogleDriveSync)
			return;
		// build synchronizer instance
		// DeviceInfo.getSDKLevel() not applicable here -> compile error about Android API compatibility
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			GoogleDriveRemoteAccess googleDriveRemoteAccess = new GoogleDriveRemoteAccess(this);
			mGoogleDriveSync = new Synchronizer(this, googleDriveRemoteAccess, getString(R.string.app_name), REQUEST_CODE_GOOGLE_DRIVE_SIGN_IN);
			mGoogleDriveSync.setOnSyncStatusListener(new OnSyncStatusListener() {
				@Override
				public void onSyncStarted(Synchronizer.SyncDirection direction) {
					// Show sync indicator
					runInReader(() -> mReaderView.showCloudSyncProgress(100));
					if (Synchronizer.SyncDirection.SyncFrom == direction) {
						log.d("Starting synchronization from Google Drive");
					} else if (Synchronizer.SyncDirection.SyncTo == direction) {
						log.d("Starting synchronization to Google Drive");
					}
				}

				@Override
				public void OnSyncProgress(Synchronizer.SyncDirection direction, int current, int total) {
					runInReader(() -> {
						int total_ = total;
						log.v("sync progress: current=" + current + "; total=" + total);
						if (current > total_)
							total_ = current;
						mReaderView.showCloudSyncProgress(10000 * current / total_);
					});
				}

				@Override
				public void onSyncCompleted(Synchronizer.SyncDirection direction) {
					log.d("Google Drive SyncTo successfully completed");
					showToast(R.string.googledrive_sync_completed);
					// Hide sync indicator
					runInReader(() -> mReaderView.hideSyncProgress());
				}

				@Override
				public void onSyncError(Synchronizer.SyncDirection direction, String errorString) {
					// Hide sync indicator
					runInReader(() -> mReaderView.hideSyncProgress());
					if (null != errorString)
						showToast(R.string.googledrive_sync_failed_with, errorString);
					else
						showToast(R.string.googledrive_sync_failed);
				}

				@Override
				public void onAborted(Synchronizer.SyncDirection direction) {
					// Hide sync indicator
					runInReader(() -> mReaderView.hideSyncProgress());
					showToast(R.string.googledrive_sync_aborted);
				}

				@Override
				public void onSettingsLoaded(Properties settings) {
					// Apply downloaded (filtered) settings
					mSuppressSettingsCopyToCloud = true;
					mergeSettings(settings, true);
				}

				@Override
				public void onBookmarksLoaded(BookInfo bookInfo) {
					waitForCRDBService(() -> {
						Services.getHistory().updateBookInfo(bookInfo);
						getDB().saveBookInfo(bookInfo);
						if (null != mReaderView) {
							BookInfo currentBook = mReaderView.getBookInfo();
							if (null != currentBook) {
								FileInfo currentFileInfo = currentBook.getFileInfo();
								if (null != currentFileInfo) {
									if (currentFileInfo.equals(bookInfo.getFileInfo())) {
										// if the book indicated by the bookInfo is currently open.
										Bookmark lastPos = bookInfo.getLastPosition();
										if (null != lastPos) {
											mReaderView.goToBookmark(lastPos);
										}
									}
								}
							}
						}
					});
				}

				@Override
				public void onCurrentBookInfoLoaded(FileInfo fileInfo) {
					loadDocument(fileInfo, false);
				}

			});
		}
	}

	private void updateGoogleDriveSynchronizer() {
		// DeviceInfo.getSDKLevel() not applicable here -> compile error about Android API compatibility
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			if (mSyncGoogleDriveEnabled) {
				if (null == mGoogleDriveSync) {
					log.d("Google Drive sync is enabled.");
					buildGoogleDriveSynchronizer();
				}
				mGoogleDriveSync.setTarget(Synchronizer.SyncTarget.SETTINGS, mSyncGoogleDriveEnabledSettings);
				mGoogleDriveSync.setTarget(Synchronizer.SyncTarget.BOOKMARKS, mSyncGoogleDriveEnabledBookmarks);
				mGoogleDriveSync.setTarget(Synchronizer.SyncTarget.CURRENTBOOKINFO, mSyncGoogleDriveEnabledCurrentBooks);
			} else {
				if (null != mGoogleDriveSync) {
					log.d("Google Drive sync is disabled.");
					// ask user: cleanup & sign out
					askConfirmation(R.string.googledrive_disabled_cleanup_question,
							() -> mGoogleDriveSync.abort( () -> {
								mGoogleDriveSync.cleanupAndSignOut();
								mGoogleDriveSync = null;
							} ),
							() -> mGoogleDriveSync.abort( () -> {
								mGoogleDriveSync.signOut();
								mGoogleDriveSync = null;
							} ));
				}
			}
		}
	}

	public void forceSyncToGoogleDrive() {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			if (null == mGoogleDriveSync)
				buildGoogleDriveSynchronizer();
			mGoogleDriveSync.startSyncTo(true, false, true);
		}
	}

	public void forceSyncFromGoogleDrive() {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			if (null == mGoogleDriveSync)
				buildGoogleDriveSynchronizer();
			mGoogleDriveSync.startSyncFrom(true, false, true);
		}
	}

	public BookInfo getBookInfoToSync() {
		return mBookInfoToSync;
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
		Uri uri = null;
		if (Intent.ACTION_VIEW.equals(intent.getAction())) {
			uri = intent.getData();
			intent.setData(null);
			if (uri != null) {
				fileToOpen = filePathFromUri(uri);
			}
		}
		if (fileToOpen == null && intent.getExtras() != null) {
			log.d("extras=" + intent.getExtras());
			fileToOpen = intent.getExtras().getString(OPEN_FILE_PARAM);
		}
		if (fileToOpen != null) {
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
		super.onPause();
		if (mReaderView != null) {
			// save book info to "sync to" as in the actual sync operation the readerView is no longer available
			BookInfo bookInfo = mReaderView.getBookInfo();
			if (null != bookInfo && null != bookInfo.getFileInfo()) {
				// make copy
				mBookInfoToSync = new BookInfo(bookInfo);
			}
			mReaderView.onAppPause();
		}
		Services.getCoverpageManager().removeCoverpageReadyListener(mHomeFrame);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			if (mSyncGoogleDriveEnabled && mGoogleDriveSync != null && !mGoogleDriveSync.isBusy()) {
				mGoogleDriveSync.startSyncTo(false, true, false);
			}
		}
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
		log.i("CoolReader.onResume()");
		super.onResume();
		//Properties props = SettingsManager.instance(this).get();

		if (mReaderView != null)
			mReaderView.onAppResume();

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
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			if (mSyncGoogleDriveEnabled && mGoogleDriveSync != null && !mGoogleDriveSync.isBusy()) {
				// when the program starts, the local settings file is already updated, so the local file is always newer than the remote one
				// Therefore, the synchronization mode is quiet, i.e. without comparing modification times and without prompting the user for action.
				mGoogleDriveSync.startSyncFrom(true, true, false);
			}
		}
	}

	@Override
	protected void onSaveInstanceState(Bundle outState) {
		log.i("CoolReader.onSaveInstanceState()");
		super.onSaveInstanceState(outState);
	}

	static final boolean LOAD_LAST_DOCUMENT_ON_START = true;

	@Override
	protected void onStart() {
		log.i("CoolReader.onStart() version=" + getVersion() + ", fileToLoadOnStart=" + fileToLoadOnStart);
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
						mReaderView.stopTTS();
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
		if (changedProps.size() > 0) {
			// After all, sync to the cloud with delay
			BackgroundThread.instance().postGUI(() -> {
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
					if (mSyncGoogleDriveEnabled && mSyncGoogleDriveEnabledSettings && null != mGoogleDriveSync) {
						if (mSuppressSettingsCopyToCloud) {
							// Immediately after downloading settings from Google Drive
							// prevent uploading settings file
							mSuppressSettingsCopyToCloud = false;
						} else if (!mGoogleDriveSync.isBusy()) {
							// After setting changed in OptionsDialog
							log.d("Some settings is changed, uploading to cloud...");
							mGoogleDriveSync.startSyncToOnly(Synchronizer.SyncTarget.SETTINGS, false);
						}
					}
				}
			}, 1000);
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
		setCurrentFrame(mHomeFrame);
	}

	private void runInReader(final Runnable task) {
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
				if (initialBatteryState >= 0)
					mReaderView.setBatteryState(initialBatteryState);
			}
		});
	}

	public boolean isBrowserCreated() {
		return mBrowserFrame != null;
	}

	private void runInBrowser(final Runnable task) {
		waitForCRDBService(() -> {
			if (mBrowserFrame == null) {
				mBrowser = new FileBrowser(CoolReader.this, Services.getEngine(), Services.getScanner(), Services.getHistory());
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
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				// Save last opened document on cloud
				if (mSyncGoogleDriveEnabled && mSyncGoogleDriveEnabledCurrentBooks && null != mGoogleDriveSync && !mGoogleDriveSync.isBusy())
					mGoogleDriveSync.startSyncToOnly(Synchronizer.SyncTarget.CURRENTBOOKINFO, false);
			}
		} : doneCallback, errorCallback));
	}

	public void loadDocumentFromUri(Uri uri, Runnable doneCallback, Runnable errorCallback) {
		runInReader(() -> {
			ContentResolver contentResolver = getContentResolver();
			InputStream inputStream;
			try {
				inputStream = contentResolver.openInputStream(uri);
				// Don't save the last opened document from the stream in the cloud, since we still cannot open it later in this program.
				mReaderView.loadDocumentFromStream(inputStream, uri.getPath(), doneCallback, errorCallback);
			} catch (FileNotFoundException e) {
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
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			if (null != mGoogleDriveSync) {
				mGoogleDriveSync.onActivityResultHandler(requestCode, resultCode, intent);
			}
		}
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


	// ========================================================================================
	// TTS
	TTS tts;
	boolean ttsInitialized;
	boolean ttsError;

	public boolean initTTS(final OnTTSCreatedListener listener) {
		if (ttsError || !TTS.isFound()) {
			if (!ttsError) {
				ttsError = true;
				showToast("TTS is not available");
			}
			return false;
		}
		if (!phoneStateChangeHandlerInstalled) {
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
						mReaderView.stopTTS();
						mReaderView.save();
					}
				});
				phoneStateChangeHandlerInstalled = true;
			}
		}
		if (ttsInitialized && tts != null) {
			BackgroundThread.instance().executeGUI(() -> listener.onCreated(tts));
			return true;
		}
		if (ttsInitialized && tts != null) {
			showToast("TTS initialization is already called");
			return false;
		}
		showToast("Initializing TTS");
		tts = new TTS(this, status -> {
			//tts.shutdown();
			L.i("TTS init status: " + status);
			if (status == TTS.SUCCESS) {
				ttsInitialized = true;
				BackgroundThread.instance().executeGUI(() -> listener.onCreated(tts));
			} else {
				ttsError = true;
				BackgroundThread.instance().executeGUI(() -> showToast("Cannot initialize TTS"));
			}
		});
		return true;
	}


	// ============================================================
	private AudioManager am;
	private int maxVolume;

	public AudioManager getAudioManager() {
		if (am == null) {
			am = (AudioManager) getSystemService(AUDIO_SERVICE);
			maxVolume = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
		}
		return am;
	}

	public int getVolume() {
		AudioManager am = getAudioManager();
		if (am != null) {
			return am.getStreamVolume(AudioManager.STREAM_MUSIC) * 100 / maxVolume;
		}
		return 0;
	}

	public void setVolume(int volume) {
		AudioManager am = getAudioManager();
		if (am != null) {
			am.setStreamVolume(AudioManager.STREAM_MUSIC, volume * maxVolume / 100, 0);
		}
	}

	public void showOptionsDialog(final OptionsDialog.Mode mode) {
		BackgroundThread.instance().postBackground(() -> {
			final String[] mFontFaces = Engine.getFontFaceList();
			BackgroundThread.instance().executeGUI(() -> {
				OptionsDialog dlg = new OptionsDialog(CoolReader.this, mReaderView, mFontFaces, mode);
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
			if (file.deleteFile()) {
				final FileInfo finalFile = file;
				waitForCRDBService(() -> Services.getHistory().removeBookInfo(getDB(), finalFile, true, true));
			}
			if (file.parent != null)
				directoryUpdated(file.parent);
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

	int CURRENT_NOTIFICATOIN_VERSION = 1;

	public void setLastNotificationId(int notificationId) {
		try {
			SharedPreferences.Editor editor = getPrefs().edit();
			editor.putInt(PREF_LAST_NOTIFICATION, notificationId);
			editor.commit();
		} catch (Exception e) {
			// ignore
		}
	}

	public int getLastNotificationId() {
		int res = getPrefs().getInt(PREF_LAST_NOTIFICATION, 0);
		log.i("getLastNotification() = " + res);
		return res;
	}


	public void showNotifications() {
		int lastNoticeId = getLastNotificationId();
		if (lastNoticeId >= CURRENT_NOTIFICATOIN_VERSION)
			return;
		if (DeviceInfo.getSDKLevel() >= DeviceInfo.HONEYCOMB)
			if (lastNoticeId <= 1)
				notification1();
		setLastNotificationId(CURRENT_NOTIFICATOIN_VERSION);
	}

	public void notification1() {
		if (hasHardwareMenuKey())
			return; // don't show notice if hard key present
		showNotice(R.string.note1_reader_menu,
				() -> setSetting(PROP_TOOLBAR_LOCATION, String.valueOf(VIEWER_TOOLBAR_SHORT_SIDE), false),
				() -> setSetting(PROP_TOOLBAR_LOCATION, String.valueOf(VIEWER_TOOLBAR_NONE), false));
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

	public void showCurrentBook() {
		BookInfo bi = Services.getHistory().getLastBook();
		if (bi != null)
			loadDocument(bi.getFileInfo(), false);
	}

}
