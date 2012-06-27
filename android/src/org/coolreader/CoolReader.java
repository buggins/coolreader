// Main Class
package org.coolreader;

import java.io.File;
import java.lang.reflect.Field;

import org.coolreader.crengine.Activities;
import org.coolreader.crengine.BackgroundThread;
import org.coolreader.crengine.BaseActivity;
import org.coolreader.crengine.DeviceInfo;
import org.coolreader.crengine.Engine;
import org.coolreader.crengine.Engine.HyphDict;
import org.coolreader.crengine.FileBrowser;
import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.History;
import org.coolreader.crengine.InterfaceTheme;
import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;
import org.coolreader.crengine.Properties;
import org.coolreader.crengine.ReaderAction;
import org.coolreader.crengine.ReaderView;
import org.coolreader.crengine.Scanner;
import org.coolreader.crengine.SettingsManager;
import org.coolreader.db.CRDBService;
import org.coolreader.db.CRDBServiceAccessor;
import org.coolreader.sync.SyncServiceAccessor;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.PixelFormat;
import android.net.Uri;
import android.os.Bundle;
import android.os.Debug;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;

public class CoolReader extends BaseActivity
{
	public static final Logger log = L.create("cr");
	
	Engine mEngine;
	ReaderView mReaderView;
	Scanner mScanner;
	FileBrowser mBrowser;
	FrameLayout mFrame;
	//View startupView;
	History mHistory;
	//CRDB mDB;
	
	public Scanner getScanner()
	{
		return mScanner;
	}
	
	public History getHistory() 
	{
		return mHistory;
	}
	
	public Engine getEngine() {
		return mEngine;
	}
	
	public FileBrowser getBrowser() {
		return mBrowser;
	}
	
	public ReaderView getReaderView() 
	{
		return mReaderView;
	}
	
	public CRDBService.LocalBinder getDB()
	{
		return mCRDBService.get();
	}
	
	public void setCurrentTheme(InterfaceTheme theme) {
		super.setCurrentTheme(theme);
		if (mBrowser != null)
			mBrowser.onThemeChanged();
	}

	
	
	int initialBatteryState = -1;
	String fileToLoadOnStart = null;
	BroadcastReceiver intentReceiver;
	
	private String mVersion = "3.0";
	
	public String getVersion() {
		return mVersion;
	}
	
	private boolean isFirstStart = true;
	
	/** Called when the activity is first created. */
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
    	Activities.setMain(this);
		log.i("CoolReader.onCreate() entered");
		super.onCreate(savedInstanceState);

		try {
			PackageInfo pi = getPackageManager().getPackageInfo(getPackageName(), 0);
			mVersion = pi.versionName;
		} catch ( NameNotFoundException e ) {
			// ignore
		}
		log.i("CoolReader version : " + getVersion());
		
		// testing background thread
		mEngine = Engine.getInstance(this);
		
       	mScanner = new Scanner(this, mEngine);
       	mScanner.initRoots(mEngine.getMountedRootsMap());

		mSyncService = new SyncServiceAccessor(this);
		mSyncService.bind(new Runnable() {
			@Override
			public void run() {
				log.i("Initialization after SyncService is bound");
				BackgroundThread.instance().postGUI(new Runnable() {
					@Override
					public void run() {
						FileInfo downloadDirectory = mScanner.getDownloadDirectory();
						if (downloadDirectory != null)
			        	mSyncService.setSyncDirectory(new File(downloadDirectory.getPathName()));
					}
				});
			}
		});
		mCRDBService = new CRDBServiceAccessor(this, mEngine.getPathCorrector());
        mCRDBService.bind(new Runnable() {
			@Override
			public void run() {
				log.i("Initialization after SyncService is bound");
				mHistory.loadFromDB(200);
			}
        });

    	isFirstStart = true;
		
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


		log.i("CoolReader.window=" + getWindow());
		WindowManager.LayoutParams lp = new WindowManager.LayoutParams();
		lp.alpha = 1.0f;
		lp.dimAmount = 0.0f;
		lp.format = PixelFormat.RGB_565;
		lp.gravity = Gravity.CENTER_HORIZONTAL | Gravity.CENTER_VERTICAL;
		lp.horizontalMargin = 0;
		lp.verticalMargin = 0;
		lp.windowAnimations = 0;
		lp.layoutAnimationParameters = null;
		lp.memoryType = WindowManager.LayoutParams.MEMORY_TYPE_NORMAL;
		getWindow().setAttributes(lp);
		
		// load settings
		Properties props = SettingsManager.instance(this).get();
		String theme = props.getProperty(ReaderView.PROP_APP_THEME, DeviceInfo.FORCE_LIGHT_THEME ? "WHITE" : "LIGHT");
		String lang = props.getProperty(ReaderView.PROP_APP_LOCALE, Lang.DEFAULT.code);
		setLanguage(lang);
    	
		mFrame = new FrameLayout(this);
		setCurrentTheme(theme);
		BackgroundThread.instance().setGUI(mFrame);

		requestWindowFeature(Window.FEATURE_NO_TITLE);

		setFullscreen( props.getBool(ReaderView.PROP_APP_FULLSCREEN, (DeviceInfo.EINK_SCREEN?true:false)));
		int orientation = props.getInt(ReaderView.PROP_APP_SCREEN_ORIENTATION, 4); //(DeviceInfo.EINK_SCREEN?0:4)
		if ( orientation < 0 || orientation > 4 )
			orientation = 0;
		setScreenOrientation(orientation);
		int backlight = props.getInt(ReaderView.PROP_APP_SCREEN_BACKLIGHT, -1);
		if ( backlight<-1 || backlight>100 )
			backlight = -1;
		setScreenBacklightLevel(backlight);

        mEngine.showProgress( 0, R.string.progress_starting_cool_reader );

        // wait until all background tasks are executed
        BackgroundThread.instance().syncWithBackground();

        String code = props.getProperty(ReaderView.PROP_HYPHENATION_DICT, Engine.HyphDict.RUSSIAN.toString());
        Engine.HyphDict dict = HyphDict.byCode(code);
		mEngine.setHyphenationDictionary(dict);
		
		//this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, 
        //       WindowManager.LayoutParams.FLAG_FULLSCREEN );
//		startupView = new View(this) {
//		};
//		startupView.setBackgroundColor(Color.BLACK);
		setScreenBacklightDuration(props.getInt(ReaderView.PROP_APP_SCREEN_BACKLIGHT_LOCK, 3));

		// open DB
		final String SQLITE_DB_NAME = "cr3db.sqlite";
		File dbdir = getDir("db", Context.MODE_PRIVATE);
		dbdir.mkdirs();
		File dbfile = new File(dbdir, SQLITE_DB_NAME);
		File externalDir = Engine.getExternalSettingsDir();
		if ( externalDir!=null ) {
			dbfile = Engine.checkOrMoveFile(externalDir, dbdir, SQLITE_DB_NAME);
		}
		//mDB = new CRDB(dbfile);

       	mHistory = new History(this, mScanner);

//		if ( DeviceInfo.FORCE_LIGHT_THEME ) {
//			setTheme(android.R.style.Theme_Light);
//			getWindow().setBackgroundDrawableResource(drawable.editbox_background);
//		}
//		if ( DeviceInfo.FORCE_LIGHT_THEME ) {
//			mFrame.setBackgroundColor( Color.WHITE );
//			setTheme(R.style.Dialog_Fullscreen_Day);
//		}
		
		mReaderView = null; //new ReaderView(this, mEngine, props);

		mScanner.setDirScanEnabled(props.getBool(ReaderView.PROP_APP_BOOK_PROPERTY_SCAN_ENABLED, true));
		
		mBrowser = new FileBrowser(this, mEngine, mScanner, mHistory);
		mBrowser.setCoverPagesEnabled(props.getBool(ReaderView.PROP_APP_SHOW_COVERPAGES, true));
		mBrowser.setCoverPageFontFace(props.getProperty(ReaderView.PROP_FONT_FACE, DeviceInfo.DEF_FONT_FACE));
		mBrowser.setCoverPageSizeOption(props.getInt(ReaderView.PROP_APP_COVERPAGE_SIZE, 1));

		
		mFrame.addView(mReaderView);
		mFrame.addView(mBrowser);
//		mFrame.addView(startupView);
		setContentView( mFrame );
        log.i("initializing browser");
        mBrowser.init();
		showView(mBrowser, false);
        log.i("initializing reader");
        mBrowser.setSortOrder( props.getProperty(ReaderView.PROP_APP_BOOK_SORT_ORDER));
		mBrowser.setSimpleViewMode(props.getBool(ReaderView.PROP_APP_FILE_BROWSER_SIMPLE_MODE, false));
        mBrowser.showDirectory(mScanner.getRoot(), null);
        
        fileToLoadOnStart = null;
		Intent intent = getIntent();
		if ( intent!=null && Intent.ACTION_VIEW.equals(intent.getAction()) ) {
			Uri uri = intent.getData();
			if ( uri!=null ) {
				fileToLoadOnStart = extractFileName(uri);
			}
			intent.setData(null);
		}
		if ( initialBatteryState>=0 )
			mReaderView.setBatteryState(initialBatteryState);
        
		
        log.i("CoolReader.onCreate() exiting");
    }

    private SyncServiceAccessor mSyncService;
    public SyncServiceAccessor getSyncService() {
    	return mSyncService;
    }
    private CRDBServiceAccessor mCRDBService;
    
    
    boolean mDestroyed = false;
	@Override
	protected void onDestroy() {

		log.i("CoolReader.onDestroy() entered");
		mDestroyed = true;
		if ( !CLOSE_BOOK_ON_STOP )
			mReaderView.close();
		
		//if ( mReaderView!=null )
		//	mReaderView.close();
		
		//if ( mHistory!=null && mDB!=null ) {
			//history.saveToDB();
		//}
		if ( intentReceiver!=null ) {
			unregisterReceiver(intentReceiver);
			intentReceiver = null;
		}

		if ( mReaderView!=null ) {
			mReaderView.destroy();
		}
		
		if ( mEngine!=null ) {
			//mEngine.uninit();
		}

		mCRDBService.unbind();
//		if ( BackgroundThread.instance()!=null ) {
//			BackgroundThread.instance().quit();
//		}
			
		mReaderView = null;
		//mEngine = null;
		
		//===========================
		// Donations support code
		//if (billingSupported) {
			//mPurchaseDatabase.close();
		//}
		
		log.i("CoolReader.onDestroy() exiting");
		super.onDestroy();
		mSyncService.unbind();
		BackgroundThread.instance().postBackground(new Runnable() {
			@Override
			public void run() {
				log.i("Stopping background thread");
				mEngine.uninit();
				BackgroundThread.instance().quit();
				mEngine = null;
			}
		});
    	Activities.setMain(null);
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
		String fileToOpen = null;
		if ( Intent.ACTION_VIEW.equals(intent.getAction()) ) {
			Uri uri = intent.getData();
			if ( uri!=null ) {
				fileToOpen = extractFileName(uri);
			}
			intent.setData(null);
		}
		log.v("onNewIntent, fileToOpen=" + fileToOpen);
		if ( fileToOpen!=null ) {
			// load document
			final String fn = fileToOpen;
			BackgroundThread.instance().postGUI(new Runnable() {
				@Override
				public void run() {
					mReaderView.loadDocument(fn, new Runnable() {
						public void run() {
							log.v("onNewIntent, loadDocument error handler called");
							showToast("Error occured while loading " + fn);
							mEngine.hideProgress();
						}
					});
				}
			}, 100);
		}
	}

	@Override
	protected void onPause() {
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
		log.i("CoolReader.onResume()");
		super.onResume();
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
		
		PhoneStateReceiver.setPhoneActivityHandler(new Runnable() {
			@Override
			public void run() {
				if (mReaderView != null) {
					mReaderView.stopTTS();
					mReaderView.save();
				}
			}
		});

		BackgroundThread.instance().postGUI(new Runnable() {
			public void run() {
				// fixing font settings
				Properties settings = mReaderView.getSettings();
				if (SettingsManager.instance(CoolReader.this).fixFontSettings(settings)) {
					log.i("Missing font settings were fixed");
					mBrowser.setCoverPageFontFace(settings.getProperty(ReaderView.PROP_FONT_FACE, DeviceInfo.DEF_FONT_FACE));
					mReaderView.setSettings(settings, null);
				}
			}
		});
		
		if (!isFirstStart)
			return;
		isFirstStart = false;
		
		if ( fileToLoadOnStart==null ) {
			if ( mReaderView!=null && currentView==mReaderView && mReaderView.isBookLoaded() ) {
				log.v("Book is already opened, showing ReaderView");
				showReader();
				return;
			}
			
			//!stopped && 
//			if ( restarted && mReaderView!=null && mReaderView.isBookLoaded() ) {
//				log.v("Book is already opened, showing ReaderView");
//		        restarted = false;
//		        return;
//			}
		}
		if ( !stopped ) {
			mEngine.showProgress( 500, R.string.progress_starting_cool_reader );
		}
        //log.i("waiting for engine tasks completion");
        //engine.waitTasksCompletion();
//		restarted = false;
		stopped = false;

		final String fileName = fileToLoadOnStart;
		BackgroundThread.instance().postGUI(new Runnable() {
			public void run() {
				log.i("onStart, scheduled runnable: load document");
		        mEngine.execute(new LoadLastDocumentTask(fileName));
			}
		});
		log.i("CoolReader.onStart() exiting");
	}
	
	class LoadLastDocumentTask implements Engine.EngineTask {

		final String fileName;
		public LoadLastDocumentTask( String fileName ) {
			super();
			this.fileName = fileName;
		}
		
		public void done() {
	        log.i("onStart, scheduled task: trying to load " + fileToLoadOnStart);
			if ( fileName!=null || LOAD_LAST_DOCUMENT_ON_START ) {
				//currentView=mReaderView;
				if ( fileName!=null ) {
					log.v("onStart() : loading " + fileName);
					mReaderView.loadDocument(fileName, new Runnable() {
						public void run() {
							// cannot open recent book: load another one
							log.e("Cannot open document " + fileToLoadOnStart + " starting file browser");
							showBrowser(null);
						}
					});
				} else {
					log.v("onStart() : loading last document");
					mReaderView.loadLastDocument(new Runnable() {
						public void run() {
							// cannot open recent book: load another one
							log.e("Cannot open last document, starting file browser");
							showBrowser(null);
						}
					});
				}
			} else {
				showBrowser(null);
			}
			fileToLoadOnStart = null;
		}

		public void fail(Exception e) {
	        log.e("onStart, scheduled task failed", e);
		}

		public void work() throws Exception {
	        log.v("onStart, scheduled task work()");
		}
    }
 

	public final static boolean CLOSE_BOOK_ON_STOP = false;
	private boolean stopped = false;
	@Override
	protected void onStop() {
		log.i("CoolReader.onStop() entering");
		super.onStop();
		stopped = true;
		// will close book at onDestroy()
		if ( CLOSE_BOOK_ON_STOP )
			mReaderView.close();

		
		log.i("CoolReader.onStop() exiting");
	}

	private View currentView;
	public void showView( View view )
	{
		mReaderView.setOnFront(view == mReaderView);
		showView( view, true );
	}
	public void showView( View view, boolean hideProgress )
	{
		if (!isStarted())
			return;
		if ( hideProgress )
		BackgroundThread.instance().postGUI(new Runnable() {
			public void run() {
				mEngine.hideProgress();
			}
		});
		if ( currentView==view ) {
			log.v("showView : view " + view.getClass().getSimpleName() + " is already shown");
			return;
		}
		log.v("showView : showing view " + view.getClass().getSimpleName());
		mFrame.bringChildToFront(view);
		for ( int i=0; i<mFrame.getChildCount(); i++ ) {
			View v = mFrame.getChildAt(i);
			v.setVisibility(view==v?View.VISIBLE:View.INVISIBLE);
		}
		currentView = view;
	}
	
	public void showReader()
	{
		log.v("showReader() is called");
		showView(mReaderView);
	}
	
	public boolean isBookOpened()
	{
		return mReaderView.isBookLoaded();
	}
	
	public void loadDocument( FileInfo item )
	{
		//showView(readerView);
		//setContentView(readerView);
		mReaderView.loadDocument(item, null);
	}
	
	public void showBrowser( final FileInfo fileToShow )
	{
		log.v("showBrowser() is called");
		if (currentView != null && currentView == mReaderView) {
			mReaderView.save();
			releaseBacklightControl();
		}
		mEngine.runInGUI( new Runnable() {
			public void run() {
				if (mBrowser == null)
					return;
				showView(mBrowser);
		        if (fileToShow == null || mBrowser.isBookShownInRecentList(fileToShow))
		        	mBrowser.showLastDirectory();
		        else
		        	mBrowser.showDirectory(fileToShow, fileToShow);
			}
		});
	}

	public void showBrowserRecentBooks()
	{
		log.v("showBrowserRecentBooks() is called");
		if ( currentView == mReaderView )
			mReaderView.save();
		mEngine.runInGUI( new Runnable() {
			public void run() {
				showView(mBrowser);
	        	mBrowser.showRecentBooks();
			}
		});
	}

	public void showBrowserRoot()
	{
		log.v("showBrowserRoot() is called");
		if ( currentView == mReaderView )
			mReaderView.save();
		mEngine.runInGUI( new Runnable() {
			public void run() {
				showView(mBrowser);
	        	mBrowser.showRootDirectory();
			}
		});
	}

	private void fillMenu(Menu menu) {
		menu.clear();
	    MenuInflater inflater = getMenuInflater();
	    if ( currentView==mReaderView ) {
	    	inflater.inflate(R.menu.cr3_reader_menu, menu);
	    	MenuItem item = menu.findItem(R.id.cr3_mi_toggle_document_styles);
	    	if ( item!=null )
	    		item.setTitle(mReaderView.getDocumentStylesEnabled() ? R.string.mi_book_styles_disable : R.string.mi_book_styles_enable);
	    	item = menu.findItem(R.id.cr3_mi_toggle_day_night);
	    	if ( item!=null )
	    		item.setTitle(mReaderView.isNightMode() ? R.string.mi_night_mode_disable : R.string.mi_night_mode_enable);
	    	item = menu.findItem(R.id.cr3_mi_toggle_text_autoformat);
	    	if ( item!=null ) {
	    		if (mReaderView.isTextFormat())
	    			item.setTitle(mReaderView.isTextAutoformatEnabled() ? R.string.mi_text_autoformat_disable : R.string.mi_text_autoformat_enable);
	    		else
	    			menu.removeItem(item.getItemId());
	    	}
	    } else {
	    	FileInfo currDir = mBrowser.getCurrentDir();
	    	inflater.inflate(currDir!=null && currDir.isOPDSRoot() ? R.menu.cr3_browser_menu : R.menu.cr3_browser_menu, menu);
	    	if ( !isBookOpened() ) {
	    		MenuItem item = menu.findItem(R.id.book_back_to_reading);
	    		if ( item!=null )
	    			item.setEnabled(false);
	    	}
    		MenuItem item = menu.findItem(R.id.book_toggle_simple_mode);
    		if ( item!=null )
    			item.setTitle(mBrowser.isSimpleViewMode() ? R.string.mi_book_browser_normal_mode : R.string.mi_book_browser_simple_mode );
	    }
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		fillMenu(menu);
	    return true;
	}

	@Override
	public boolean onPrepareOptionsMenu(Menu menu) {
		fillMenu(menu);
	    return true;
	}

	public void saveSetting( String name, String value ) {
		mReaderView.saveSetting(name, value);
	}
	public String getSetting( String name ) {
		return mReaderView.getSetting(name);
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		int itemId = item.getItemId();
		if ( mReaderView.onMenuItem(itemId))
			return true; // processed by ReaderView
		// other commands
		switch ( itemId ) {
		case R.id.book_toggle_simple_mode:
			mBrowser.setSimpleViewMode(!mBrowser.isSimpleViewMode());
			mReaderView.saveSetting(ReaderView.PROP_APP_FILE_BROWSER_SIMPLE_MODE, mBrowser.isSimpleViewMode()?"1":"0");
			return true;
		case R.id.mi_browser_options:
			// TODO: fix it
			//showOptionsDialog(OptionsDialog.Mode.BROWSER);
			return true;
//		case R.id.book_sort_order:
//			mBrowser.showSortOrderMenu();
//			return true;
		case R.id.book_root:
			mBrowser.showRootDirectory();
			return true;
		case R.id.book_opds_root:
			mBrowser.showOPDSRootDirectory();
			return true;
		case R.id.catalog_add:
			mBrowser.editOPDSCatalog(null);
			return true;
		case R.id.book_recent_books:
			mBrowser.showRecentBooks();
			return true;
		case R.id.book_find:
			mBrowser.showFindBookDialog();
			return true;
		case R.id.cr3_mi_user_manual:
			showReader();
			mReaderView.showManual();
			return true;
		case R.id.book_scan_recursive:
			mBrowser.scanCurrentDirectoryRecursive();
			return true;
		case R.id.book_back_to_reading:
			if ( isBookOpened() )
				showReader();
			else
				showToast("No book opened");
			return true;
		default:
			return false;
			//return super.onOptionsItemSelected(item);
		}
	}
	

	private static class DefKeyAction {
		public int keyCode;
		public int type;
		public ReaderAction action;
		public DefKeyAction(int keyCode, int type, ReaderAction action) {
			this.keyCode = keyCode;
			this.type = type;
			this.action = action;
		}
		public String getProp() {
			return ReaderView.PROP_APP_KEY_ACTIONS_PRESS + ReaderAction.getTypeString(type) + keyCode;			
		}
	}
	private static class DefTapAction {
		public int zone;
		public boolean longPress;
		public ReaderAction action;
		public DefTapAction(int zone, boolean longPress, ReaderAction action) {
			this.zone = zone;
			this.longPress = longPress;
			this.action = action;
		}
	}
	private static DefKeyAction[] DEF_KEY_ACTIONS = {
		new DefKeyAction(KeyEvent.KEYCODE_BACK, ReaderAction.NORMAL, ReaderAction.GO_BACK),
		new DefKeyAction(KeyEvent.KEYCODE_BACK, ReaderAction.LONG, ReaderAction.EXIT),
		new DefKeyAction(KeyEvent.KEYCODE_BACK, ReaderAction.DOUBLE, ReaderAction.EXIT),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_CENTER, ReaderAction.NORMAL, ReaderAction.RECENT_BOOKS),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_CENTER, ReaderAction.LONG, ReaderAction.BOOKMARKS),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_UP, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_DOWN, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_UP, ReaderAction.LONG, (DeviceInfo.EINK_SONY? ReaderAction.PAGE_UP_10 : ReaderAction.REPEAT)),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_DOWN, ReaderAction.LONG, (DeviceInfo.EINK_SONY? ReaderAction.PAGE_DOWN_10 : ReaderAction.REPEAT)),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_LEFT, ReaderAction.NORMAL, (DeviceInfo.NAVIGATE_LEFTRIGHT ? ReaderAction.PAGE_UP : ReaderAction.PAGE_UP_10)),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_RIGHT, ReaderAction.NORMAL, (DeviceInfo.NAVIGATE_LEFTRIGHT ? ReaderAction.PAGE_DOWN : ReaderAction.PAGE_DOWN_10)),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_LEFT, ReaderAction.LONG, ReaderAction.REPEAT),
		new DefKeyAction(KeyEvent.KEYCODE_DPAD_RIGHT, ReaderAction.LONG, ReaderAction.REPEAT),
		new DefKeyAction(KeyEvent.KEYCODE_VOLUME_UP, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(KeyEvent.KEYCODE_VOLUME_DOWN, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(KeyEvent.KEYCODE_VOLUME_UP, ReaderAction.LONG, ReaderAction.REPEAT),
		new DefKeyAction(KeyEvent.KEYCODE_VOLUME_DOWN, ReaderAction.LONG, ReaderAction.REPEAT),
		new DefKeyAction(KeyEvent.KEYCODE_MENU, ReaderAction.NORMAL, ReaderAction.READER_MENU),
		new DefKeyAction(KeyEvent.KEYCODE_MENU, ReaderAction.LONG, ReaderAction.OPTIONS),
		new DefKeyAction(KeyEvent.KEYCODE_CAMERA, ReaderAction.NORMAL, ReaderAction.NONE),
		new DefKeyAction(KeyEvent.KEYCODE_CAMERA, ReaderAction.LONG, ReaderAction.NONE),
		new DefKeyAction(KeyEvent.KEYCODE_SEARCH, ReaderAction.NORMAL, ReaderAction.SEARCH),
		new DefKeyAction(KeyEvent.KEYCODE_SEARCH, ReaderAction.LONG, ReaderAction.TOGGLE_SELECTION_MODE),
		
		new DefKeyAction(ReaderView.NOOK_KEY_NEXT_RIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(ReaderView.NOOK_KEY_SHIFT_DOWN, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(ReaderView.NOOK_KEY_PREV_LEFT, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(ReaderView.NOOK_KEY_PREV_RIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(ReaderView.NOOK_KEY_SHIFT_UP, ReaderAction.NORMAL, ReaderAction.PAGE_UP),

		new DefKeyAction(ReaderView.NOOK_12_KEY_NEXT_LEFT, ReaderAction.NORMAL, (DeviceInfo.EINK_NOOK ? ReaderAction.PAGE_UP : ReaderAction.PAGE_DOWN)),
		new DefKeyAction(ReaderView.NOOK_12_KEY_NEXT_LEFT, ReaderAction.LONG, (DeviceInfo.EINK_NOOK ? ReaderAction.PAGE_UP_10 : ReaderAction.PAGE_DOWN_10)),
		
		new DefKeyAction(ReaderView.KEYCODE_PAGE_BOTTOMLEFT, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
//		new DefKeyAction(ReaderView.KEYCODE_PAGE_BOTTOMRIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_TOPLEFT, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_TOPRIGHT, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_BOTTOMLEFT, ReaderAction.LONG, ReaderAction.PAGE_UP_10),
//		new DefKeyAction(ReaderView.KEYCODE_PAGE_BOTTOMRIGHT, ReaderAction.LONG, ReaderAction.PAGE_UP_10),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_TOPLEFT, ReaderAction.LONG, ReaderAction.PAGE_DOWN_10),
		new DefKeyAction(ReaderView.KEYCODE_PAGE_TOPRIGHT, ReaderAction.LONG, ReaderAction.PAGE_DOWN_10),
		
		new DefKeyAction(ReaderView.SONY_DPAD_DOWN_SCANCODE, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(ReaderView.SONY_DPAD_UP_SCANCODE, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(ReaderView.SONY_DPAD_DOWN_SCANCODE, ReaderAction.LONG, ReaderAction.PAGE_DOWN_10),
		new DefKeyAction(ReaderView.SONY_DPAD_UP_SCANCODE, ReaderAction.LONG, ReaderAction.PAGE_UP_10),

		new DefKeyAction(KeyEvent.KEYCODE_8, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(KeyEvent.KEYCODE_2, ReaderAction.NORMAL, ReaderAction.PAGE_UP),
		new DefKeyAction(KeyEvent.KEYCODE_8, ReaderAction.LONG, ReaderAction.PAGE_DOWN_10),
		new DefKeyAction(KeyEvent.KEYCODE_2, ReaderAction.LONG, ReaderAction.PAGE_UP_10),
		
		new DefKeyAction(ReaderView.KEYCODE_ESCAPE, ReaderAction.NORMAL, ReaderAction.PAGE_DOWN),
		new DefKeyAction(ReaderView.KEYCODE_ESCAPE, ReaderAction.LONG, ReaderAction.REPEAT),
		
//	    public static final int KEYCODE_PAGE_BOTTOMLEFT = 0x5d; // fwd
//	    public static final int KEYCODE_PAGE_BOTTOMRIGHT = 0x5f; // fwd
//	    public static final int KEYCODE_PAGE_TOPLEFT = 0x5c; // back
//	    public static final int KEYCODE_PAGE_TOPRIGHT = 0x5e; // back
		
	};
	private static DefTapAction[] DEF_TAP_ACTIONS = {
		new DefTapAction(1, false, ReaderAction.PAGE_UP),
		new DefTapAction(2, false, ReaderAction.PAGE_UP),
		new DefTapAction(4, false, ReaderAction.PAGE_UP),
		new DefTapAction(1, true, ReaderAction.GO_BACK), // back by link
		new DefTapAction(2, true, ReaderAction.TOGGLE_DAY_NIGHT),
		new DefTapAction(4, true, ReaderAction.PAGE_UP_10),
		new DefTapAction(3, false, ReaderAction.PAGE_DOWN),
		new DefTapAction(6, false, ReaderAction.PAGE_DOWN),
		new DefTapAction(7, false, ReaderAction.PAGE_DOWN),
		new DefTapAction(8, false, ReaderAction.PAGE_DOWN),
		new DefTapAction(9, false, ReaderAction.PAGE_DOWN),
		new DefTapAction(3, true, ReaderAction.TOGGLE_AUTOSCROLL),
		new DefTapAction(6, true, ReaderAction.PAGE_DOWN_10),
		new DefTapAction(7, true, ReaderAction.PAGE_DOWN_10),
		new DefTapAction(8, true, ReaderAction.PAGE_DOWN_10),
		new DefTapAction(9, true, ReaderAction.PAGE_DOWN_10),
		new DefTapAction(5, false, ReaderAction.READER_MENU),
		new DefTapAction(5, true, ReaderAction.OPTIONS),
	};
	
	
	private boolean isValidFontFace(String face) {
		String[] fontFaces = mEngine.getFontFaceList();
		if (fontFaces == null)
			return true;
		for (String item : fontFaces) {
			if (item.equals(face))
				return true;
		}
		return false;
	}

	public File getSettingsFile(int profile) {
		if (profile == 0)
			return propsFile;
		return new File(propsFile.getAbsolutePath() + ".profile" + profile);
	}
	
	File propsFile;
	private static final String SETTINGS_FILE_NAME = "cr3.ini";
	private static boolean DEBUG_RESET_OPTIONS = false;

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
	

	
	public void applyAppSetting( String key, String value )
	{
		super.applyAppSetting(key, value);
		boolean flg = "1".equals(value);
        if (key.equals(PROP_APP_BOOK_SORT_ORDER)) {
        	if (getBrowser() != null)
        		getBrowser().setSortOrder(value);
        } else if (key.equals(PROP_APP_FILE_BROWSER_SIMPLE_MODE)) {
        	if (getBrowser() != null)
        		getBrowser().setSimpleViewMode(flg);
        } else if ( key.equals(PROP_APP_SHOW_COVERPAGES) ) {
        	if (getBrowser() != null)
        		getBrowser().setCoverPagesEnabled(flg);
        } else if ( key.equals(PROP_APP_BOOK_PROPERTY_SCAN_ENABLED) ) {
			getScanner().setDirScanEnabled(flg);
        } else if ( key.equals(PROP_FONT_FACE) ) {
        	if (getBrowser() != null)
        		getBrowser().setCoverPageFontFace(value);
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
        	if (getBrowser() != null)
        		getBrowser().setCoverPageSizeOption(n);
        } else if ( key.equals(PROP_APP_FILE_BROWSER_SIMPLE_MODE) ) {
        	if (getBrowser()!=null )
        		getBrowser().setSimpleViewMode(flg);
        } else if ( key.equals(PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS) ) {
        	getScanner().setHideEmptyDirs(flg);
        }
        //
	}
	
	
}
