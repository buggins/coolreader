// Main Class
package org.coolreader;

import java.io.File;
import java.lang.reflect.Field;
import java.util.Map;

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
import org.coolreader.donations.BillingService;
import org.coolreader.donations.BillingService.RequestPurchase;
import org.coolreader.donations.BillingService.RestoreTransactions;
import org.coolreader.donations.Consts;
import org.coolreader.donations.Consts.PurchaseState;
import org.coolreader.donations.Consts.ResponseCode;
import org.coolreader.donations.PurchaseObserver;
import org.coolreader.donations.ResponseHandler;
import org.koekak.android.ebookdownloader.SonyBookSelector;

import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Debug;
import android.os.Handler;
import android.util.Log;
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
	int initialBatteryState = -1;
	BroadcastReceiver intentReceiver;

	private boolean justCreated = false;
	
	
	/** Called when the activity is first created. */
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
    	startServices();
    	
//    	Intent intent = getIntent();
//    	if (intent != null && intent.getBooleanExtra("EXIT", false)) {
//    		log.i("CoolReader.onCreate() - EXIT extra parameter found: exiting app");
//   		    finish();
//   		    return;
//    	}
    	
    
		log.i("CoolReader.onCreate() entered");
		super.onCreate(savedInstanceState);

		
		
		
		
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
	        mHandler = new Handler();
	        mPurchaseObserver = new CRPurchaseObserver(mHandler);
	        mBillingService = new BillingService();
	        mBillingService.setContext(this);
	
	        //mPurchaseDatabase = new PurchaseDatabase(this);
	
	        // Check if billing is supported.
	        ResponseHandler.register(mPurchaseObserver);
	        billingSupported = mBillingService.checkBillingSupported();
		} catch (VerifyError e) {
			log.e("Exception while trying to initialize billing service for donations");
		}
        if (!billingSupported) {
        	log.i("Billing is not supported");
        } else {
        	log.i("Billing is supported");
        }

		N2EpdController.n2MainActivity = this;


        //Services.getEngine().showProgress( 0, R.string.progress_starting_cool_reader );

		//this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, 
        //       WindowManager.LayoutParams.FLAG_FULLSCREEN );
//		startupView = new View(this) {
//		};
//		startupView.setBackgroundColor(Color.BLACK);


//		if ( DeviceInfo.FORCE_LIGHT_THEME ) {
//			setTheme(android.R.style.Theme_Light);
//			getWindow().setBackgroundDrawableResource(drawable.editbox_background);
//		}
//		if ( DeviceInfo.FORCE_LIGHT_THEME ) {
//			mFrame.setBackgroundColor( Color.WHITE );
//			setTheme(R.style.Dialog_Fullscreen_Day);
//		}
		
//		mFrame.addView(startupView);
//        log.i("initializing browser");
//        log.i("initializing reader");
//        
//        fileToLoadOnStart = null;
//		Intent intent = getIntent();
//		if ( intent!=null && Intent.ACTION_VIEW.equals(intent.getAction()) ) {
//			Uri uri = intent.getData();
//			if ( uri!=null ) {
//				fileToLoadOnStart = extractFileName(uri);
//			}
//			intent.setData(null);
//		}
        
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
		//if (billingSupported) {
			//mPurchaseDatabase.close();
		//}
		mBillingService.unbind();

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
        if ( key.equals(PROP_APP_KEY_BACKLIGHT_OFF) ) {
			setKeyBacklightDisabled(flg);
        } else if ( key.equals(PROP_APP_SCREEN_BACKLIGHT_LOCK) ) {
        	int n = 0;
        	try {
        		n = Integer.parseInt(value);
        	} catch (NumberFormatException e) {
        		// ignore
        	}
			setScreenBacklightDuration(n);
        } else if ( key.equals(PROP_APP_DICTIONARY) ) {
        	setDict(value);
        } else if (key.equals(PROP_APP_BOOK_SORT_ORDER)) {
        	if (mBrowser != null)
        		mBrowser.setSortOrder(value);
        } else if (key.equals(PROP_APP_FILE_BROWSER_SIMPLE_MODE)) {
        	if (mBrowser != null)
        		mBrowser.setSimpleViewMode(flg);
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
        } else if ( key.equals(PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS) ) {
        	Services.getScanner().setHideEmptyDirs(flg);
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
//		String fileToOpen = null;
//		if ( Intent.ACTION_VIEW.equals(intent.getAction()) ) {
//			Uri uri = intent.getData();
//			if ( uri!=null ) {
//				fileToOpen = extractFileName(uri);
//			}
//			intent.setData(null);
//		}
//		log.v("onNewIntent, fileToOpen=" + fileToOpen);
//		if ( fileToOpen!=null ) {
//			// load document
//			final String fn = fileToOpen;
//			BackgroundThread.instance().postGUI(new Runnable() {
//				@Override
//				public void run() {
//					loadDocument(fn, new Runnable() {
//						public void run() {
//							log.v("onNewIntent, loadDocument error handler called");
//							showToast("Error occured while loading " + fn);
//							Services.getEngine().hideProgress();
//						}
//					});
//				}
//			}, 100);
//		}
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
					showToast("Cannot open book");
					showRootWindow();
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
		
		// Donations support code
		if (billingSupported)
			ResponseHandler.register(mPurchaseObserver);
		
		PhoneStateReceiver.setPhoneActivityHandler(new Runnable() {
			@Override
			public void run() {
				if (mReaderView != null) {
					mReaderView.stopTTS();
					mReaderView.save();
				}
			}
		});
		
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
		
		
//		if ( fileToLoadOnStart==null ) {
//			if ( mReaderView!=null && currentView==mReaderView && mReaderView.isBookLoaded() ) {
//				log.v("Book is already opened, showing ReaderView");
//				showReader();
//				return;
//			}
//			
//			//!stopped && 
////			if ( restarted && mReaderView!=null && mReaderView.isBookLoaded() ) {
////				log.v("Book is already opened, showing ReaderView");
////		        restarted = false;
////		        return;
////			}
//		}
		if ( !stopped ) {
			//Services.getEngine().showProgress( 500, R.string.progress_starting_cool_reader );
		}
        //log.i("waiting for engine tasks completion");
        //engine.waitTasksCompletion();
//		restarted = false;
		stopped = false;

		final String fileName = fileToLoadOnStart;
//		BackgroundThread.instance().postGUI(new Runnable() {
//			public void run() {
//				log.i("onStart, scheduled runnable: load document");
//				Services.getEngine().execute(new LoadLastDocumentTask(fileName));
//			}
//		});
		log.i("CoolReader.onStart() exiting");
	}
	
//	class LoadLastDocumentTask implements Engine.EngineTask {
//
//		final String fileName;
//		public LoadLastDocumentTask( String fileName ) {
//			super();
//			this.fileName = fileName;
//		}
//		
//		public void done() {
//	        log.i("onStart, scheduled task: trying to load " + fileToLoadOnStart);
//			if ( fileName!=null || LOAD_LAST_DOCUMENT_ON_START ) {
//				//currentView=mReaderView;
//				if ( fileName!=null ) {
//					log.v("onStart() : loading " + fileName);
//					mReaderView.loadDocument(fileName, new Runnable() {
//						public void run() {
//							// cannot open recent book: load another one
//							log.e("Cannot open document " + fileToLoadOnStart + " starting file browser");
//							showBrowser(null);
//						}
//					});
//				} else {
//					log.v("onStart() : loading last document");
//					mReaderView.loadLastDocument(new Runnable() {
//						public void run() {
//							// cannot open recent book: load another one
//							log.e("Cannot open last document, starting file browser");
//							showBrowser(null);
//						}
//					});
//				}
//			} else {
//				showBrowser(null);
//			}
//			fileToLoadOnStart = null;
//		}
//
//		public void fail(Exception e) {
//	        log.e("onStart, scheduled task failed", e);
//		}
//
//		public void work() throws Exception {
//	        log.v("onStart, scheduled task work()");
//		}
//    }
 

	private boolean stopped = false;
	@Override
	protected void onStop() {
		log.i("CoolReader.onStop() entering");
		// Donations support code
		if (billingSupported)
			ResponseHandler.unregister(mPurchaseObserver);
		super.onStop();
		stopped = true;
		// will close book at onDestroy()
		if ( CLOSE_BOOK_ON_STOP )
			mReaderView.close();

		
		log.i("CoolReader.onStop() exiting");
	}

//	public void showView( View view, boolean hideProgress )
//	{
//		if (!isStarted())
//			return;
//		if ( hideProgress )
//		BackgroundThread.instance().postGUI(new Runnable() {
//			public void run() {
//				Services.getEngine().hideProgress();
//			}
//		});
//		if ( currentView==view ) {
//			log.v("showView : view " + view.getClass().getSimpleName() + " is already shown");
//			return;
//		}
//		log.v("showView : showing view " + view.getClass().getSimpleName());
//		mFrame.bringChildToFront(view);
//		for ( int i=0; i<mFrame.getChildCount(); i++ ) {
//			View v = mFrame.getChildAt(i);
//			v.setVisibility(view==v?View.VISIBLE:View.INVISIBLE);
//		}
//		currentView = view;
//	}
	
//	public void showReader()
//	{
//		log.v("showReader() is called");
//		showView(mReaderView);
//	}
//	
//	public boolean isBookOpened()
//	{
//		return mReaderView.isBookLoaded();
//	}
//	
//	public void loadDocument( FileInfo item )
//	{
//		//showView(readerView);
//		//setContentView(readerView);
//		mReaderView.loadDocument(item, null);
//	}
	
//	public void showBrowser( final FileInfo fileToShow )
//	{
//		log.v("showBrowser() is called");
//		if (currentView != null && currentView == mReaderView) {
//			mReaderView.save();
//			releaseBacklightControl();
//		}
//		Services.getEngine().runInGUI( new Runnable() {
//			public void run() {
//				if (mBrowser == null)
//					return;
//				showView(mBrowser);
//		        if (fileToShow == null || mBrowser.isBookShownInRecentList(fileToShow))
//		        	mBrowser.showLastDirectory();
//		        else
//		        	mBrowser.showDirectory(fileToShow, fileToShow);
//			}
//		});
//	}
//
//	public void showBrowserRecentBooks()
//	{
//		log.v("showBrowserRecentBooks() is called");
//		if ( currentView == mReaderView )
//			mReaderView.save();
//		Services.getEngine().runInGUI( new Runnable() {
//			public void run() {
//				showView(mBrowser);
//	        	mBrowser.showRecentBooks();
//			}
//		});
//	}
//
//	public void showBrowserRoot()
//	{
//		log.v("showBrowserRoot() is called");
//		if ( currentView == mReaderView )
//			mReaderView.save();
//		Services.getEngine().runInGUI( new Runnable() {
//			public void run() {
//				showView(mBrowser);
//	        	mBrowser.showRootDirectory();
//			}
//		});
//	}

//	private void fillMenu(Menu menu) {
//		menu.clear();
//	    MenuInflater inflater = getMenuInflater();
//	    if ( currentView==mReaderView ) {
//	    	inflater.inflate(R.menu.cr3_reader_menu, menu);
//	    	MenuItem item = menu.findItem(R.id.cr3_mi_toggle_document_styles);
//	    	if ( item!=null )
//	    		item.setTitle(mReaderView.getDocumentStylesEnabled() ? R.string.mi_book_styles_disable : R.string.mi_book_styles_enable);
//	    	item = menu.findItem(R.id.cr3_mi_toggle_day_night);
//	    	if ( item!=null )
//	    		item.setTitle(mReaderView.isNightMode() ? R.string.mi_night_mode_disable : R.string.mi_night_mode_enable);
//	    	item = menu.findItem(R.id.cr3_mi_toggle_text_autoformat);
//	    	if ( item!=null ) {
//	    		if (mReaderView.isTextFormat())
//	    			item.setTitle(mReaderView.isTextAutoformatEnabled() ? R.string.mi_text_autoformat_disable : R.string.mi_text_autoformat_enable);
//	    		else
//	    			menu.removeItem(item.getItemId());
//	    	}
//	    } else {
//	    	FileInfo currDir = mBrowser.getCurrentDir();
//	    	inflater.inflate(currDir!=null && currDir.isOPDSRoot() ? R.menu.cr3_browser_menu : R.menu.cr3_browser_menu, menu);
//	    	if ( !isBookOpened() ) {
//	    		MenuItem item = menu.findItem(R.id.book_back_to_reading);
//	    		if ( item!=null )
//	    			item.setEnabled(false);
//	    	}
//    		MenuItem item = menu.findItem(R.id.book_toggle_simple_mode);
//    		if ( item!=null )
//    			item.setTitle(mBrowser.isSimpleViewMode() ? R.string.mi_book_browser_normal_mode : R.string.mi_book_browser_simple_mode );
//	    }
//	}
	
//	@Override
//	public boolean onCreateOptionsMenu(Menu menu) {
//		fillMenu(menu);
//	    return true;
//	}

//	@Override
//	public boolean onPrepareOptionsMenu(Menu menu) {
//		fillMenu(menu);
//	    return true;
//	}

//	public void saveSetting( String name, String value ) {
//		mReaderView.saveSetting(name, value);
//	}
//	public String getSetting( String name ) {
//		return mReaderView.getSetting(name);
//	}
	
//	@Override
//	public boolean onOptionsItemSelected(MenuItem item) {
//		int itemId = item.getItemId();
//		if ( mReaderView.onMenuItem(itemId))
//			return true; // processed by ReaderView
//		// other commands
//		switch ( itemId ) {
//		case R.id.book_toggle_simple_mode:
//			mBrowser.setSimpleViewMode(!mBrowser.isSimpleViewMode());
//			mReaderView.saveSetting(ReaderView.PROP_APP_FILE_BROWSER_SIMPLE_MODE, mBrowser.isSimpleViewMode()?"1":"0");
//			return true;
//		case R.id.mi_browser_options:
//			// TODO: fix it
//			//showOptionsDialog(OptionsDialog.Mode.BROWSER);
//			return true;
////		case R.id.book_sort_order:
////			mBrowser.showSortOrderMenu();
////			return true;
//		case R.id.book_root:
//			mBrowser.showRootDirectory();
//			return true;
//		case R.id.book_opds_root:
//			mBrowser.showOPDSRootDirectory();
//			return true;
//		case R.id.catalog_add:
//			mBrowser.editOPDSCatalog(null);
//			return true;
//		case R.id.book_recent_books:
//			mBrowser.showRecentBooks();
//			return true;
//		case R.id.book_find:
//			mBrowser.showFindBookDialog();
//			return true;
//		case R.id.cr3_mi_user_manual:
//			showReader();
//			mReaderView.showManual();
//			return true;
//		case R.id.book_scan_recursive:
//			mBrowser.scanCurrentDirectoryRecursive();
//			return true;
//		case R.id.book_back_to_reading:
//			if ( isBookOpened() )
//				showReader();
//			else
//				showToast("No book opened");
//			return true;
//		default:
//			return false;
//			//return super.onOptionsItemSelected(item);
//		}
//	}
	

	
	
//	private boolean isValidFontFace(String face) {
//		String[] fontFaces = Services.getEngine().getFontFaceList();
//		if (fontFaces == null)
//			return true;
//		for (String item : fontFaces) {
//			if (item.equals(face))
//				return true;
//		}
//		return false;
//	}

//	public File getSettingsFile(int profile) {
//		if (profile == 0)
//			return propsFile;
//		return new File(propsFile.getAbsolutePath() + ".profile" + profile);
//	}
//	
//	File propsFile;

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
					mReaderView.getSurface().setFocusable(true);
					mReaderView.getSurface().setFocusableInTouchMode(true);
					mReaderView.getSurface().requestFocus();
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
					mReaderView.getSurface().setFocusable(true);
					mReaderView.getSurface().setFocusableInTouchMode(true);
					mReaderView.getSurface().requestFocus();
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
		loadDocument(item.getPathName(), null);
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
	
	private final static int DICTAN_ARTICLE_REQUEST_CODE = 100;
	
	private final static String DICTAN_ARTICLE_WORD = "article.word";
	
	private final static String DICTAN_ERROR_MESSAGE = "error.message";

	private final static int FLAG_ACTIVITY_CLEAR_TASK = 0x00008000;
	
	private void findInDictionaryInternal(String s) {
		switch (currentDict.internal) {
		case 0:
			Intent intent0 = new Intent(currentDict.action).setComponent(new ComponentName(
				currentDict.packageName, currentDict.className
				)).addFlags(DeviceInfo.getSDKLevel() >= 7 ? FLAG_ACTIVITY_CLEAR_TASK : Intent.FLAG_ACTIVITY_NEW_TASK);
			if (s!=null)
				intent0.putExtra(currentDict.dataKey, s);
			try {
				startActivity( intent0 );
			} catch ( ActivityNotFoundException e ) {
				showToast("Dictionary \"" + currentDict.name + "\" is not installed");
			}
			break;
		case 1:
			final String SEARCH_ACTION  = "colordict.intent.action.SEARCH";
			final String EXTRA_QUERY   = "EXTRA_QUERY";
			final String EXTRA_FULLSCREEN = "EXTRA_FULLSCREEN";
			final String EXTRA_HEIGHT  = "EXTRA_HEIGHT";
			final String EXTRA_WIDTH   = "EXTRA_WIDTH";
			final String EXTRA_GRAVITY  = "EXTRA_GRAVITY";
			final String EXTRA_MARGIN_LEFT = "EXTRA_MARGIN_LEFT";
			final String EXTRA_MARGIN_TOP  = "EXTRA_MARGIN_TOP";
			final String EXTRA_MARGIN_BOTTOM = "EXTRA_MARGIN_BOTTOM";
			final String EXTRA_MARGIN_RIGHT = "EXTRA_MARGIN_RIGHT";

			Intent intent1 = new Intent(SEARCH_ACTION);
			if (s!=null)
				intent1.putExtra(EXTRA_QUERY, s); //Search Query
			intent1.putExtra(EXTRA_FULLSCREEN, true); //
			try
			{
				startActivity(intent1);
			} catch ( ActivityNotFoundException e ) {
				showToast("Dictionary \"" + currentDict.name + "\" is not installed");
			}
			break;
		case 2:
			// Dictan support
			Intent intent2 = new Intent("android.intent.action.VIEW");
			// Add custom category to run the Dictan external dispatcher
            intent2.addCategory("info.softex.dictan.EXTERNAL_DISPATCHER");
            
   	        // Don't include the dispatcher in activity  
            // because it doesn't have any content view.	      
            intent2.setFlags(Intent.FLAG_ACTIVITY_NO_HISTORY);
		  
	        intent2.putExtra(DICTAN_ARTICLE_WORD, s);
			  
	        try {
	        	startActivityForResult(intent2, DICTAN_ARTICLE_REQUEST_CODE);
	        } catch (ActivityNotFoundException e) {
				showToast("Dictionary \"" + currentDict.name + "\" is not installed");
	        }
			break;
		}
	}

	public void showDictionary() {
		findInDictionaryInternal(null);
	}
	
	@Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent) {
        if (requestCode == DICTAN_ARTICLE_REQUEST_CODE) {
	       	switch (resultCode) {
	        	
	        	// The article has been shown, the intent is never expected null
			case RESULT_OK:
				break;
					
			// Error occured
			case RESULT_CANCELED: 
				String errMessage = "Unknown Error.";
				if (intent != null) {
					errMessage = "The Requested Word: " + 
					intent.getStringExtra(DICTAN_ARTICLE_WORD) + 
					". Error: " + intent.getStringExtra(DICTAN_ERROR_MESSAGE);
				}
				showToast(errMessage);
				break;
					
			// Must never occur
			default: 
				showToast("Unknown Result Code: " + resultCode);
				break;
			}
        }
    }
	
	private DictInfo currentDict = getDictList()[0];
	
	public void setDict( String id ) {
		for ( DictInfo d : getDictList() ) {
			if ( d.id.equals(id) ) {
				currentDict = d;
				return;
			}
		}
	}

	public void showAboutDialog() {
		AboutDialog dlg = new AboutDialog(this);
		dlg.show();
	}
	
	
	
	
	
	//==============================================================
	// 
	// Donations related code
	// (from Dungeons sample) 
	// 
	//==============================================================
    //private static final int DIALOG_CANNOT_CONNECT_ID = 1;
    //private static final int DIALOG_BILLING_NOT_SUPPORTED_ID = 2;
    /**
     * Used for storing the log text.
     */
    private static final String LOG_TEXT_KEY = "DUNGEONS_LOG_TEXT";

    /**
     * The SharedPreferences key for recording whether we initialized the
     * database.  If false, then we perform a RestoreTransactions request
     * to get all the purchases for this user.
     */
    private static final String DB_INITIALIZED = "db_initialized";

	
    /**
     * Each product in the catalog is either MANAGED or UNMANAGED.  MANAGED
     * means that the product can be purchased only once per user (such as a new
     * level in a game). The purchase is remembered by Android Market and
     * can be restored if this application is uninstalled and then
     * re-installed. UNMANAGED is used for products that can be used up and
     * purchased multiple times (such as poker chips). It is up to the
     * application to keep track of UNMANAGED products for the user.
     */
    //private enum Managed { MANAGED, UNMANAGED }

    private CRPurchaseObserver mPurchaseObserver;
    private BillingService mBillingService;
    private Handler mHandler;
    private DonationListener mDonationListener = null;
    private boolean billingSupported = false;
    private double mTotalDonations = 0;
    
    public boolean isDonationSupported() {
    	return billingSupported;
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
    public boolean makeDonation(double amount) {
		final String itemName = "donation" + (amount >= 1 ? String.valueOf((int)amount) : String.valueOf(amount));
    	log.i("makeDonation is called, itemName=" + itemName);
    	if (!billingSupported)
    		return false;
    	String mPayloadContents = null;
    	String mSku = itemName;
        if (!mBillingService.requestPurchase(mSku, mPayloadContents)) {
        	showToast("Purchase is failed");
        }
    	return true;
    }
    

	private static String DONATIONS_PREF_FILE = "cr3donations";
	private static String DONATIONS_PREF_TOTAL_AMOUNT = "total";
    /**
     * A {@link PurchaseObserver} is used to get callbacks when Android Market sends
     * messages to this application so that we can update the UI.
     */
    private class CRPurchaseObserver extends PurchaseObserver {
    	
    	private String TAG = "cr3Billing";
        public CRPurchaseObserver(Handler handler) {
            super(CoolReader.this, handler);
        }

        @Override
        public void onBillingSupported(boolean supported) {
            if (Consts.DEBUG) {
                Log.i(TAG, "supported: " + supported);
            }
            if (supported) {
            	billingSupported = true;
        		SharedPreferences pref = getSharedPreferences(DONATIONS_PREF_FILE, 0);
        		try {
        			mTotalDonations = pref.getFloat(DONATIONS_PREF_TOTAL_AMOUNT, 0.0f);
        		} catch (Exception e) {
        			log.e("exception while reading total donations from preferences", e);
        		}
            	// TODO:
//                restoreDatabase();
            }
        }

        @Override
        public void onPurchaseStateChange(PurchaseState purchaseState, String itemId,
                int quantity, long purchaseTime, String developerPayload) {
            if (Consts.DEBUG) {
                Log.i(TAG, "onPurchaseStateChange() itemId: " + itemId + " " + purchaseState);
            }

            if (developerPayload == null) {
                logProductActivity(itemId, purchaseState.toString());
            } else {
                logProductActivity(itemId, purchaseState + "\n\t" + developerPayload);
            }

            if (purchaseState == PurchaseState.PURCHASED) {
            	double amount = 0;
            	try {
	            	if (itemId.startsWith("donation"))
	            		amount = Double.parseDouble(itemId.substring(8));
            	} catch (NumberFormatException e) {
            		//
            	}

            	mTotalDonations += amount;
        		SharedPreferences pref = getSharedPreferences(DONATIONS_PREF_FILE, 0);
        		pref.edit().putString(DONATIONS_PREF_TOTAL_AMOUNT, String.valueOf(mTotalDonations)).commit();

            	if (mDonationListener != null)
            		mDonationListener.onDonationTotalChanged(mTotalDonations);
                //mOwnedItems.add(itemId);
            }
//            mCatalogAdapter.setOwnedItems(mOwnedItems);
//            mOwnedItemsCursor.requery();
        }

        @Override
        public void onRequestPurchaseResponse(RequestPurchase request,
                ResponseCode responseCode) {
            if (Consts.DEBUG) {
                Log.d(TAG, request.mProductId + ": " + responseCode);
            }
            if (responseCode == ResponseCode.RESULT_OK) {
                if (Consts.DEBUG) {
                    Log.i(TAG, "purchase was successfully sent to server");
                }
                logProductActivity(request.mProductId, "sending purchase request");
            } else if (responseCode == ResponseCode.RESULT_USER_CANCELED) {
                if (Consts.DEBUG) {
                    Log.i(TAG, "user canceled purchase");
                }
                logProductActivity(request.mProductId, "dismissed purchase dialog");
            } else {
                if (Consts.DEBUG) {
                    Log.i(TAG, "purchase failed");
                }
                logProductActivity(request.mProductId, "request purchase returned " + responseCode);
            }
        }

        @Override
        public void onRestoreTransactionsResponse(RestoreTransactions request,
                ResponseCode responseCode) {
            if (responseCode == ResponseCode.RESULT_OK) {
                if (Consts.DEBUG) {
                    Log.d(TAG, "completed RestoreTransactions request");
                }
                // Update the shared preferences so that we don't perform
                // a RestoreTransactions again.
                SharedPreferences prefs = getPreferences(Context.MODE_PRIVATE);
                SharedPreferences.Editor edit = prefs.edit();
                edit.putBoolean(DB_INITIALIZED, true);
                edit.commit();
            } else {
                if (Consts.DEBUG) {
                    Log.d(TAG, "RestoreTransactions error: " + responseCode);
                }
            }
        }
    }
    private void logProductActivity(String product, String activity) {
    	// TODO: some logging
    	Log.i(LOG_TEXT_KEY, activity);
    }


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
					getSyncService().removeFile(file.getPathName());
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
				getSyncService().removeFileLastPosition(item.getPathName());
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
			loadDocument(location, null);
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

