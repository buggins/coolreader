// Main Class
package org.coolreader;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Map;

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
import org.coolreader.crengine.CRToolBar.OnActionHandler;
import org.coolreader.crengine.DeviceInfo;
import org.coolreader.crengine.Engine;
import org.coolreader.crengine.ErrorDialog;
import org.coolreader.crengine.FileBrowser;
import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.History.BookInfoLoadedCallack;
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
import org.koekak.android.ebookdownloader.SonyBookSelector;

import android.Manifest;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
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

public class CoolReader extends BaseActivity
{
	public static final Logger log = L.create("cr");
	
	private ReaderView mReaderView;
	private ReaderViewLayout mReaderFrame;
	private FileBrowser mBrowser;
	private View mBrowserTitleBar;
	private CRToolBar mBrowserToolBar;
	private BrowserViewLayout mBrowserFrame;
	CRRootView mHomeFrame;
	private Engine mEngine;
	//View startupView;
	//CRDB mDB;
	private ViewGroup mCurrentFrame;
	private ViewGroup mPreviousFrame;
	
	
	String fileToLoadOnStart = null;
	
	private boolean isFirstStart = true;
	private boolean phoneStateChangeHandlerInstalled = false;
	int initialBatteryState = -1;
	BroadcastReceiver intentReceiver;

	private boolean justCreated = false;

	private static final int PERM_REQUEST_STORAGE_CODE = 1;
	private static final int PERM_REQUEST_READ_PHONE_STATE_CODE = 2;

	/** Called when the activity is first created. */
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
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
				if ( mReaderView!=null )
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
		
        log.i("CoolReader.onCreate() exiting");
    }

	public final static boolean CLOSE_BOOK_ON_STOP = false;
	
    boolean mDestroyed = false;
	@Override
	protected void onDestroy() {

		log.i("CoolReader.onDestroy() entered");
		if (!CLOSE_BOOK_ON_STOP && mReaderView != null)
			mReaderView.close();

		if ( tts!=null ) {
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
		if ( intentReceiver!=null ) {
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
	public void applyAppSetting( String key, String value )
	{
		super.applyAppSetting(key, value);
		boolean flg = "1".equals(value);
        if ( key.equals(PROP_APP_DICTIONARY) ) {
        	setDict(value);
        } else if (key.equals(PROP_APP_BOOK_SORT_ORDER)) {
        	if (mBrowser != null)
        		mBrowser.setSortOrder(value);
        } else if ( key.equals(PROP_APP_SHOW_COVERPAGES) ) {
        	if (mBrowser != null)
        		mBrowser.setCoverPagesEnabled(flg);
        } else if ( key.equals(PROP_APP_BOOK_PROPERTY_SCAN_ENABLED) ) {
        	Services.getScanner().setDirScanEnabled(flg);
        } else if ( key.equals(PROP_FONT_FACE) ) {
        	if (mBrowser != null)
        		mBrowser.setCoverPageFontFace(value);
        } else if ( key.equals(PROP_APP_COVERPAGE_SIZE) ) {
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
        } else if ( key.equals(PROP_APP_FILE_BROWSER_SIMPLE_MODE) ) {
        	if (mBrowser != null)
        		mBrowser.setSimpleViewMode(flg);
        }
        //
	}
	
	@Override
	public void setFullscreen( boolean fullscreen )
	{
		super.setFullscreen(fullscreen);
		if (mReaderFrame != null)
			mReaderFrame.updateFullscreen(fullscreen);
	}
	
	private String extractFileName( Uri uri )
	{
		if ( uri!=null ) {
			if ( uri.equals(Uri.parse("file:///")) )
				return null;
			else
				return uri.getPath();
		}
		return null;
	}

	@Override
	protected void onNewIntent(Intent intent) {
		log.i("onNewIntent : " + intent);
		if ( mDestroyed ) {
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
		if (Intent.ACTION_VIEW.equals(intent.getAction())) {
			Uri uri = intent.getData();
			intent.setData(null);
			if (uri != null) {
				fileToOpen = uri.getPath();
//				if (fileToOpen.startsWith("file://"))
//					fileToOpen = fileToOpen.substring("file://".length());
			}
		}
		if (fileToOpen == null && intent.getExtras() != null) {
			log.d("extras=" + intent.getExtras());
			fileToOpen = intent.getExtras().getString(OPEN_FILE_PARAM);
		}
		if (fileToOpen != null) {
			// patch for opening of books from ReLaunch (under Nook Simple Touch) 
			while (fileToOpen.indexOf("%2F") >= 0) {
				fileToOpen = fileToOpen.replace("%2F", "/");
			}
			log.d("FILE_TO_OPEN = " + fileToOpen);
			loadDocument(fileToOpen, new Runnable() {
				@Override
				public void run() {
					BackgroundThread.instance().postGUI(new Runnable() {
						@Override
						public void run() {
							// if document not loaded show error & then root window
							ErrorDialog errDialog = new ErrorDialog(CoolReader.this, CoolReader.this.getString(R.string.error), CoolReader.this.getString(R.string.cant_open_file));
							errDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
								@Override
								public void onDismiss(DialogInterface dialog) {
									showRootWindow();
								}
							});
							errDialog.show();
						}
					}, 500);
				}
			});
			return true;
		} else {
			log.d("No file to open");
			return false;
		}
	}

	@Override
	protected void onPause() {
		super.onPause();
		if (mReaderView != null)
			mReaderView.onAppPause();
		Services.getCoverpageManager().removeCoverpageReadyListener(mHomeFrame);
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
                if( res != null && res.length() > 0 ) {
                    SonyBookSelector selector = new SonyBookSelector(this);
                    long l = selector.getContentId(res);
                    if(l != 0) {
                       selector.setReadingTime(l);
                       selector.requestBookSelection(l);
                    }
                }
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
			waitForCRDBService(new Runnable() {
				@Override
				public void run() {
					Services.getHistory().loadFromDB(getDB(), 200);
					
					mHomeFrame = new CRRootView(CoolReader.this);
					Services.getCoverpageManager().addCoverpageReadyListener(mHomeFrame);
					mHomeFrame.requestFocus();
					
					showRootWindow();
					setSystemUiVisibility();
					
					notifySettingsChanged();
					
					showNotifications();
				}
			});
		}
		
		
		if ( isBookOpened() ) {
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
		if ( CLOSE_BOOK_ON_STOP )
			mReaderView.close();

		
		log.i("CoolReader.onStop() exiting");
	}

	private void requestStoragePermissions() {
		// check or request permission for storage
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
			int readExtStoragePermissionCheck = checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE);
			int writeExtStoragePermissionCheck = checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE);
			ArrayList<String> needPerms = new ArrayList<String>();
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
				requestPermissions(needPerms.toArray(templ), PERM_REQUEST_STORAGE_CODE);
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
				requestPermissions(new String[] { Manifest.permission.READ_PHONE_STATE } , PERM_REQUEST_READ_PHONE_STATE_CODE);
			} else {
				log.i("READ_PHONE_STATE permission already granted.");
			}
		}
	}

	public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
		log.i("CoolReader.onRequestPermissionsResult()");
		if (PERM_REQUEST_STORAGE_CODE == requestCode) {		// external storage read & write permissions
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
				getDBService().setPathCorrector(Engine.getInstance(this).getPathCorrector());
				getDBService().get().reopenDatabase();
				waitForCRDBService(new Runnable() {
					@Override
					public void run() {
						Services.getHistory().loadFromDB(getDB(), 200);
					}
				});
				mHomeFrame.refreshView();
			}
		} else if (PERM_REQUEST_READ_PHONE_STATE_CODE == requestCode) {
			if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
				log.i("read phone state permission is GRANTED, registering phone activity handler...");
				PhoneStateReceiver.setPhoneActivityHandler(new Runnable() {
					@Override
					public void run() {
						if (mReaderView != null) {
							mReaderView.stopTTS();
							mReaderView.save();
						}
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
	private static String dumpFields( Field[] fields, Object obj) {
		StringBuilder buf = new StringBuilder();
		try {
			for ( Field f : fields ) {
				if ( buf.length()>0 )
					buf.append(", ");
				buf.append(f.getName());
				buf.append("=");
				buf.append(f.get(obj));
			}
		} catch ( Exception e ) {
			
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
		Properties changedProps = oldProps!=null ? props.diff(oldProps) : props;
		if (mHomeFrame != null) {
			mHomeFrame.refreshOnlineCatalogs();
		}
		if (mReaderFrame != null) {
			mReaderFrame.updateSettings(props);
			if (mReaderView != null)
				mReaderView.updateSettings(props);
		}
        for ( Map.Entry<Object, Object> entry : changedProps.entrySet() ) {
    		String key = (String)entry.getKey();
    		String value = (String)entry.getValue();
    		applyAppSetting( key, value );
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
		runInReader(new Runnable() {
			@Override
			public void run() {
				// do nothing
			}
		});
	}
	
	public void showRootWindow() {
		setCurrentFrame(mHomeFrame);
	}
	
	private void runInReader(final Runnable task) {
		waitForCRDBService(new Runnable() {
			@Override
			public void run() {
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
			        mReaderFrame.getToolBar().setOnActionHandler(new OnActionHandler() {
						@Override
						public boolean onActionSelected(ReaderAction item) {
							if (mReaderView != null)
								mReaderView.onAction(item);
							return true;
						}
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
			}
		});
		
	}
	
	public boolean isBrowserCreated() {
		return mBrowserFrame != null;
	}
	
	private void runInBrowser(final Runnable task) {
		waitForCRDBService(new Runnable() {
			@Override
			public void run() {
				if (mBrowserFrame != null) {
					task.run();
					setCurrentFrame(mBrowserFrame);
				} else {
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
			        mBrowserToolBar.setOnActionHandler(new OnActionHandler() {
						@Override
						public boolean onActionSelected(ReaderAction item) {
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
						}
					});
					mBrowserFrame = new BrowserViewLayout(CoolReader.this, mBrowser, mBrowserToolBar, mBrowserTitleBar);
					
					task.run();
					setCurrentFrame(mBrowserFrame);

//					if (getIntent() == null)
//						mBrowser.showDirectory(Services.getScanner().getDownloadDirectory(), null);
				}
			}
		});
		
	}
	
	public void showBrowser() {
		runInBrowser(new Runnable() {
			@Override
			public void run() {
				// do nothing, browser is shown
			}
		});
	}
	
	public void showManual() {
		loadDocument("@manual", null);
	}
	
	public static final String OPEN_FILE_PARAM = "FILE_TO_OPEN";
	public void loadDocument(final String item, final Runnable callback)
	{
		runInReader(new Runnable() {
			@Override
			public void run() {
				mReaderView.loadDocument(item, callback);
			}
		});
	}
	
	public void loadDocument( FileInfo item )
	{
		loadDocument(item, null);
	}
	
	public void loadDocument( FileInfo item, Runnable callback )
	{
		log.d("Activities.loadDocument(" + item.pathname + ")");
		loadDocument(item.getPathName(), callback);
	}
	
	public void showOpenedBook()
	{
		showReader();
	}
	
	public static final String OPEN_DIR_PARAM = "DIR_TO_OPEN";
	public void showBrowser(final FileInfo dir) {
		runInBrowser(new Runnable() {
			@Override
			public void run() {
				mBrowser.showDirectory(dir, null);
			}
		});
	}
	
	public void showBrowser(final String dir) {
		runInBrowser(new Runnable() {
			@Override
			public void run() {
				mBrowser.showDirectory(Services.getScanner().pathToFileInfo(dir), null);
			}
		});
	}
	
	public void showRecentBooks() {
		log.d("Activities.showRecentBooks() is called");
		runInBrowser(new Runnable() {
			@Override
			public void run() {
				mBrowser.showRecentBooks();
			}
		});
	}

	public void showOnlineCatalogs() {
		log.d("Activities.showOnlineCatalogs() is called");
		runInBrowser(new Runnable() {
			@Override
			public void run() {
				mBrowser.showOPDSRootDirectory();
			}
		});
	}

	public void showDirectory(FileInfo path) {
		log.d("Activities.showDirectory(" + path + ") is called");
		showBrowser(path);
	}

	public void showCatalog(final FileInfo path) {
		log.d("Activities.showCatalog(" + path + ") is called");
		runInBrowser(new Runnable() {
			@Override
			public void run() {
				mBrowser.showDirectory(path, null);
			}
		});
	}

	
	
	public void setBrowserTitle(String title) {
		if (mBrowserFrame != null)
			mBrowserFrame.setBrowserTitle(title);
	}
	

	
	// Dictionary support
	
	
	public void findInDictionary( String s ) {
		if ( s!=null && s.length()!=0 ) {
			int start,end;
			
			// Skip over non-letter characters at the beginning and end of the search string
			for (start = 0 ;start<s.length(); start++)
				if (Character.isLetterOrDigit(s.charAt(start)))
 					break;
			for (end=s.length()-1; end>=start; end--)
				if (Character.isLetterOrDigit(s.charAt(end)))
 					break;

			if ( end > start ) {
    			final String pattern = s.substring(start,end+1);

				BackgroundThread.instance().postBackground(new Runnable() {
					@Override
					public void run() {
						BackgroundThread.instance().postGUI(new Runnable() {
							@Override
							public void run() {
								findInDictionaryInternal(pattern);
							}
						}, 100);
					}
				});
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
    }
	
	public void setDict( String id ) {
		mDictionaries.setDict(id);
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
		final String itemName = "donation" + (amount >= 1 ? String.valueOf((int)amount) : String.valueOf(amount));
    	log.i("makeDonation is called, itemName=" + itemName);
    	if (!mDonationService.isBillingSupported())
    		return false;
    	BackgroundThread.instance().postBackground(new Runnable() {
			@Override
			public void run() {
		        mDonationService.purchase(itemName, 
	        		new CRDonationService.PurchaseListener() {
						@Override
						public void onPurchaseCompleted(final boolean success, final String productId,
								final float totalDonations) {
							BackgroundThread.instance().postGUI(new Runnable() {
								@Override
								public void run() {
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
								}
							});
						}
				});
			}
    	});
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
		if ( ttsError || !TTS.isFound() ) {
			if ( !ttsError ) {
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
				PhoneStateReceiver.setPhoneActivityHandler(new Runnable() {
					@Override
					public void run() {
						if (mReaderView != null) {
							mReaderView.stopTTS();
							mReaderView.save();
						}
					}
				});
				phoneStateChangeHandlerInstalled = true;
			}
		}
		if ( ttsInitialized && tts!=null ) {
			BackgroundThread.instance().executeGUI(new Runnable() {
				@Override
				public void run() {
					listener.onCreated(tts);
				}
			});
			return true;
		}
		if ( ttsInitialized && tts!=null ) {
			showToast("TTS initialization is already called");
			return false;
		}
		showToast("Initializing TTS");
    	tts = new TTS(this, new TTS.OnInitListener() {
			@Override
			public void onInit(int status) {
				//tts.shutdown();
				L.i("TTS init status: " + status);
				if ( status==TTS.SUCCESS ) {
					ttsInitialized = true;
					BackgroundThread.instance().executeGUI(new Runnable() {
						@Override
						public void run() {
							listener.onCreated(tts);
						}
					});
				} else {
					ttsError = true;
					BackgroundThread.instance().executeGUI(new Runnable() {
						@Override
						public void run() {
							showToast("Cannot initialize TTS");
						}
					});
				}
			}
		});
		return true;
	}
	

    // ============================================================
	private AudioManager am;
	private int maxVolume;
	public AudioManager getAudioManager() {
		if ( am==null ) {
			am = (AudioManager)getSystemService(AUDIO_SERVICE);
			maxVolume = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
		}
		return am;
	}
	
	public int getVolume() {
		AudioManager am = getAudioManager();
		if (am!=null) {
			return am.getStreamVolume(AudioManager.STREAM_MUSIC) * 100 / maxVolume;
		}
		return 0;
	}
	
	public void setVolume( int volume ) {
		AudioManager am = getAudioManager();
		if (am!=null) {
			am.setStreamVolume(AudioManager.STREAM_MUSIC, volume * maxVolume / 100, 0);
		}
	}
	
	public void showOptionsDialog(final OptionsDialog.Mode mode)
	{
		BackgroundThread.instance().postBackground(new Runnable() {
			public void run() {
				final String[] mFontFaces = Engine.getFontFaceList();
				BackgroundThread.instance().executeGUI(new Runnable() {
					public void run() {
						OptionsDialog dlg = new OptionsDialog(CoolReader.this, mReaderView, mFontFaces, mode);
						dlg.show();
					}
				});
			}
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

	public void showBookmarksDialog()
	{
		BackgroundThread.instance().executeGUI(new Runnable() {
			@Override
			public void run() {
				BookmarksDlg dlg = new BookmarksDlg(CoolReader.this, mReaderView);
				dlg.show();
			}
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
	
	public void askDeleteBook(final FileInfo item)
	{
		askConfirmation(R.string.win_title_confirm_book_delete, new Runnable() {
			@Override
			public void run() {
				closeBookIfOpened(item);
				FileInfo file = Services.getScanner().findFileInTree(item);
				if (file == null)
					file = item;
				if (file.deleteFile()) {
					Services.getHistory().removeBookInfo(getDB(), file, true, true);
				}
				if (file.parent != null)
					directoryUpdated(file.parent);
			}
		});
	}
	
	public void askDeleteRecent(final FileInfo item)
	{
		askConfirmation(R.string.win_title_confirm_history_record_delete, new Runnable() {
			@Override
			public void run() {
				Services.getHistory().removeBookInfo(getDB(), item, true, false);
				directoryUpdated(Services.getScanner().createRecentRoot());
			}
		});
	}
	
	public void askDeleteCatalog(final FileInfo item)
	{
		askConfirmation(R.string.win_title_confirm_catalog_delete, new Runnable() {
			@Override
			public void run() {
				if (item != null && item.isOPDSDir()) {
					getDB().removeOPDSCatalog(item.id);
					directoryUpdated(Services.getScanner().createRecentRoot());
				}
			}
		});
	}
	
	public void saveSetting(String name, String value) {
		if (mReaderView != null)
			mReaderView.saveSetting(name, value);
	}
	
	public void editBookInfo(final FileInfo currDirectory, final FileInfo item) {
		Services.getHistory().getOrCreateBookInfo(getDB(), item, new BookInfoLoadedCallack() {
			@Override
			public void onBookInfoLoaded(BookInfo bookInfo) {
				if (bookInfo == null)
					bookInfo = new BookInfo(item);
				BookInfoEditDialog dlg = new BookInfoEditDialog(CoolReader.this, currDirectory, bookInfo, 
						currDirectory.isRecentDir());
				dlg.show();
			}
		});
	}
	
	public void editOPDSCatalog(FileInfo opds) {
		if (opds==null) {
			opds = new FileInfo();
			opds.isDirectory = true;
			opds.pathname = FileInfo.OPDS_DIR_PREFIX + "http://";
			opds.filename = "New Catalog";
			opds.isListed = true;
			opds.isScanned = true;
			opds.parent = Services.getScanner().getOPDSRoot();
		}
		OPDSCatalogEditDialog dlg = new OPDSCatalogEditDialog(CoolReader.this, opds, new Runnable() {
			@Override
			public void run() {
				refreshOPDSRootDirectory(true);
			}
		});
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
	
	public void notification1()
	{
		if (hasHardwareMenuKey())
			return; // don't show notice if hard key present
		showNotice(R.string.note1_reader_menu, new Runnable() {
			@Override
			public void run() {
				setSetting(PROP_TOOLBAR_LOCATION, String.valueOf(VIEWER_TOOLBAR_SHORT_SIDE), false);
			}
		}, new Runnable() {
			@Override
			public void run() {
				setSetting(PROP_TOOLBAR_LOCATION, String.valueOf(VIEWER_TOOLBAR_NONE), false);
			}
		});
	}
	
	/**
	 * Get last stored location.
	 * @param location
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
			loadDocument(location, new Runnable() {
				@Override
				public void run() {
					BackgroundThread.instance().postGUI(new Runnable() {
						@Override
						public void run() {
							// if document not loaded show error & then root window
							ErrorDialog errDialog = new ErrorDialog(CoolReader.this, "Error", "Can't open file!");
							errDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
								@Override
								public void onDismiss(DialogInterface dialog) {
									showRootWindow();
								}
							});
							errDialog.show();
						}
					}, 1000);
				}
			});
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

	public void showCurrentBook() {
		BookInfo bi = Services.getHistory().getLastBook();
		if (bi != null)
			loadDocument(bi.getFileInfo());
	}
	
}

