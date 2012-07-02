// Main Class
package org.coolreader;

import java.io.File;
import java.lang.reflect.Field;

import org.coolreader.crengine.Activities;
import org.coolreader.crengine.BackgroundThread;
import org.coolreader.crengine.BaseActivity;
import org.coolreader.crengine.CRRootView;
import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;
import org.coolreader.crengine.Services;
import org.coolreader.db.CRDBService;
import org.coolreader.db.CRDBServiceAccessor;
import org.coolreader.sync.SyncServiceAccessor;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Debug;

public class CoolReader extends BaseActivity
{
	public static final Logger log = L.create("cr");
	
	CRRootView mFrame;
	//View startupView;
	//CRDB mDB;
	
	public CRDBService.LocalBinder getDB()
	{
		return mCRDBService.get();
	}
	

	
	
	String fileToLoadOnStart = null;
	
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

		// testing background thread
		mSyncService = Services.getSyncService();
		mCRDBService = Services.getDBService();

    	isFirstStart = true;
		
    	


    	
		mFrame = new CRRootView(this);
		setContentView( mFrame );
		BackgroundThread.instance().setGUI(mFrame);



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
        log.i("initializing browser");
        log.i("initializing reader");
        
        fileToLoadOnStart = null;
		Intent intent = getIntent();
		if ( intent!=null && Intent.ACTION_VIEW.equals(intent.getAction()) ) {
			Uri uri = intent.getData();
			if ( uri!=null ) {
				fileToLoadOnStart = extractFileName(uri);
			}
			intent.setData(null);
		}
        
		
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
		mFrame.onClose();
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
		//if (billingSupported) {
			//mPurchaseDatabase.close();
		//}
		
		log.i("CoolReader.onDestroy() exiting");
		super.onDestroy();
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
					Activities.loadDocument(fn, new Runnable() {
						public void run() {
							log.v("onNewIntent, loadDocument error handler called");
							showToast("Error occured while loading " + fn);
							Services.getEngine().hideProgress();
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
		
		if (!isFirstStart)
			return;
		isFirstStart = false;
		
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
		super.onStop();
		stopped = true;
		// will close book at onDestroy()

		
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

	public File getSettingsFile(int profile) {
		if (profile == 0)
			return propsFile;
		return new File(propsFile.getAbsolutePath() + ".profile" + profile);
	}
	
	File propsFile;

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
	

	
	
	
}
