// Main Class
package org.coolreader;

import java.io.File;
import java.lang.reflect.Field;

import org.coolreader.crengine.BackgroundThread;
import org.coolreader.crengine.BaseDialog;
import org.coolreader.crengine.BookmarksDlg;
import org.coolreader.crengine.CRDB;
import org.coolreader.crengine.Engine;
import org.coolreader.crengine.FileBrowser;
import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.History;
import org.coolreader.crengine.OptionsDialog;
import org.coolreader.crengine.ReaderView;
import org.coolreader.crengine.Scanner;
import org.coolreader.crengine.Engine.HyphDict;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.os.PowerManager;
import android.text.InputFilter;
import android.text.method.DigitsKeyListener;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.Toast;

public class CoolReader extends Activity
{
	Engine mEngine;
	ReaderView mReaderView;
	Scanner mScanner;
	FileBrowser mBrowser;
	FrameLayout mFrame;
	//View startupView;
	History mHistory;
	CRDB mDB;
	private BackgroundThread mBackgroundThread;
	
	public History getHistory() 
	{
		return mHistory;
	}
	
	public ReaderView getReaderView() 
	{
		return mReaderView;
	}
	
	public CRDB getDB()
	{
		return mDB;
	}
	
	private static String PREF_FILE = "CR3LastBook";
	private static String PREF_LAST_BOOK = "LastBook";
	public String getLastSuccessfullyOpenedBook()
	{
		SharedPreferences pref = getSharedPreferences(PREF_FILE, 0);
		String res = pref.getString(PREF_LAST_BOOK, null);
		pref.edit().putString(PREF_LAST_BOOK, null).commit();
		return res;
	}
	
	public void setLastSuccessfullyOpenedBook( String filename )
	{
		SharedPreferences pref = getSharedPreferences(PREF_FILE, 0);
		pref.edit().putString(PREF_LAST_BOOK, filename).commit();
	}
	
	private boolean mFullscreen = false;
	public boolean isFullscreen() {
		return mFullscreen;
	}

	public void applyFullscreen( Window wnd )
	{
		if ( mFullscreen ) {
			//mActivity.getWindow().requestFeature(Window.)
			wnd.setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, 
			        WindowManager.LayoutParams.FLAG_FULLSCREEN );
		} else {
			wnd.setFlags(0, 
			        WindowManager.LayoutParams.FLAG_FULLSCREEN );
		}
	}
	public void setFullscreen( boolean fullscreen )
	{
		if ( mFullscreen!=fullscreen ) {
			mFullscreen = fullscreen;
			applyFullscreen( getWindow() );
		}
	}
	
	String fileToLoadOnStart = null;
	BroadcastReceiver intentReceiver;
	PowerManager.WakeLock wl = null;
	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
		Log.i("cr3", "CoolReader.onCreate() entered");
		super.onCreate(savedInstanceState);
		
		intentReceiver = new BroadcastReceiver() {

			@Override
			public void onReceive(Context context, Intent intent) {
				Log.i("cr3", "Battery state changed. Intent=" + intent);
				int level = intent.getIntExtra("level", 0);
				mReaderView.setBatteryState(level);
			}
			
		};
		registerReceiver(intentReceiver, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
		
		PowerManager pm = (PowerManager)getSystemService(
	            Context.POWER_SERVICE);
		wl = pm.newWakeLock(
	        PowerManager.SCREEN_BRIGHT_WAKE_LOCK
	        | PowerManager.ON_AFTER_RELEASE,
	        "cr3");
		
        // testing background thread
    	mBackgroundThread = BackgroundThread.instance();
		mFrame = new FrameLayout(this);
		mEngine = new Engine(this, mBackgroundThread);
		mBackgroundThread.setGUI(mFrame);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		//this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, 
        //       WindowManager.LayoutParams.FLAG_FULLSCREEN );
//		startupView = new View(this) {
//		};
//		startupView.setBackgroundColor(Color.BLACK);
		mReaderView = new ReaderView(this, mEngine, mBackgroundThread);
		File dbdir = getDir("db", Context.MODE_PRIVATE);
		dbdir.mkdirs();
		File dbfile = new File(dbdir, "cr3db.sqlite");
		mDB = new CRDB(dbfile);
       	mHistory = new History(mDB);
		mScanner = new Scanner(this, mDB, mEngine); //, Environment.getExternalStorageDirectory(), "SD"
		mBrowser = new FileBrowser(this, mEngine, mScanner, mHistory);
		mFrame.addView(mReaderView);
		mFrame.addView(mBrowser);
//		mFrame.addView(startupView);
		setContentView( mFrame );
		showView(mBrowser);
        Log.i("cr3", "initializing scanner");
        mScanner.initRoots();
        Log.i("cr3", "initializing browser");
        mBrowser.init();
        Log.i("cr3", "initializing reader");
        mReaderView.init();
        mBrowser.showDirectory(mScanner.getRoot(), null);
        Window wnd = getWindow();
        if ( wnd!=null ) {
        	LayoutParams attrs =  wnd.getAttributes();
        	//attrs.screenBrightness = 0.7f;
        	// hack to set buttonBrightness field
        	try {
	        	Field bb = attrs.getClass().getField("buttonBrightness");
	        	if ( bb!=null )
	        		bb.set(attrs, Float.valueOf(0.0f));
        	} catch ( Exception e ) {
        		Log.e("cr3", "WindowManager.LayoutParams.buttonBrightness field is not found, cannot turn buttons backlight off");
        	}
        	//attrs.buttonBrightness = 0;
        	wnd.setAttributes(attrs);
        	//attrs.screenOrientation = LayoutParams.SCREEN_;
        }

        
        fileToLoadOnStart = null;
		Intent intent = getIntent();
		if ( intent!=null && Intent.ACTION_VIEW.equals(intent.getAction()) ) {
			Uri uri = intent.getData();
			if ( uri!=null ) {
				fileToLoadOnStart = extractFileName(uri);
			}
			intent.setData(null);
		}
        
        Log.i("cr3", "CoolReader.onCreate() exiting");
    }

	@Override
	protected void onDestroy() {

		Log.i("cr3", "CoolReader.onDestroy() entered");
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
			mEngine.uninit();
		}
		if ( mDB!=null ) {
			final CRDB db = mDB;
			mBackgroundThread.executeBackground(new Runnable() {
				public void run() {
					db.close();
				}
			});
		}
//		if ( mBackgroundThread!=null ) {
//			mBackgroundThread.quit();
//		}
			
		mDB = null;
		mReaderView = null;
		mEngine = null;
		mBackgroundThread = null;
		Log.i("cr3", "CoolReader.onDestroy() exiting");
		super.onDestroy();
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
		String fileToOpen = null;
		if ( Intent.ACTION_VIEW.equals(intent.getAction()) ) {
			Uri uri = intent.getData();
			if ( uri!=null ) {
				fileToOpen = extractFileName(uri);
			}
			intent.setData(null);
		}
		if ( fileToOpen!=null ) {
			// load document
			final String fn = fileToOpen;
			mReaderView.loadDocument(fileToOpen, new Runnable() {
				public void run() {
					showToast("Error occured while loading " + fn);
				}
			});
		}
	}

	@Override
	protected void onPause() {
		Log.i("cr3", "CoolReader.onPause() : saving reader state");
		if ( wl.isHeld() )
			wl.release();
		mReaderView.save();
		super.onPause();
	}

	@Override
	protected void onPostCreate(Bundle savedInstanceState) {
		Log.i("cr3", "CoolReader.onPostCreate()");
		super.onPostCreate(savedInstanceState);
	}

	@Override
	protected void onPostResume() {
		Log.i("cr3", "CoolReader.onPostResume()");
		super.onPostResume();
	}

	private boolean restarted = false;
	@Override
	protected void onRestart() {
		Log.i("cr3", "CoolReader.onRestart()");
		restarted = true;
		super.onRestart();
	}

	@Override
	protected void onRestoreInstanceState(Bundle savedInstanceState) {
		Log.i("cr3", "CoolReader.onRestoreInstanceState()");
		super.onRestoreInstanceState(savedInstanceState);
	}

	@Override
	protected void onResume() {
		Log.i("cr3", "CoolReader.onResume()");
		super.onResume();
	}

	@Override
	protected void onSaveInstanceState(Bundle outState) {
		Log.i("cr3", "CoolReader.onSaveInstanceState()");
		super.onSaveInstanceState(outState);
	}

	static final boolean LOAD_LAST_DOCUMENT_ON_START = true; 
	
	@Override
	protected void onStart() {
		Log.i("cr3", "CoolReader.onStart()");
		super.onStart();
		
		wl.acquire();
		
		//!stopped && 
		if ( restarted && mReaderView!=null && mReaderView.isBookLoaded() ) {
	        restarted = false;
	        return;
		}
		if ( !stopped ) {
	        mEngine.showProgress( 5, R.string.progress_starting_cool_reader );
			mEngine.setHyphenationDictionary( HyphDict.RUSSIAN );
		}
        //Log.i("cr3", "waiting for engine tasks completion");
        //engine.waitTasksCompletion();
		restarted = false;
		stopped = false;
        mEngine.execute(new Engine.EngineTask() {

			public void done() {
		        Log.i("cr3", "trying to load last document");
				if ( fileToLoadOnStart!=null || LOAD_LAST_DOCUMENT_ON_START ) {
					if ( fileToLoadOnStart!=null ) {
						mReaderView.loadDocument(fileToLoadOnStart, new Runnable() {
							public void run() {
								// cannot open recent book: load another one
								Log.e("cr3", "Cannot open document " + fileToLoadOnStart + " starting file browser");
								showBrowser(null);
							}
						});
					} else {
						mReaderView.loadLastDocument(new Runnable() {
							public void run() {
								// cannot open recent book: load another one
								Log.e("cr3", "Cannot open last document, starting file browser");
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
			}

			public void work() throws Exception {
				// do nothing
			}
        	
        });
	}

	private boolean stopped = false;
	@Override
	protected void onStop() {
		Log.i("cr3", "CoolReader.onStop() entering");
		stopped = true;
		// will close book at onDestroy()
		mReaderView.close();
		super.onStop();
		Log.i("cr3", "CoolReader.onStop() exiting");
	}

	private View currentView;
	public void showView( View view )
	{
		if ( currentView==view )
			return;
		mFrame.bringChildToFront(view);
		for ( int i=0; i<mFrame.getChildCount(); i++ ) {
			View v = mFrame.getChildAt(i);
			v.setVisibility(view==v?View.VISIBLE:View.INVISIBLE);
		}
	}
	
	public void showReader()
	{
		Log.v("cr3", "showReader() is called");
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
		mReaderView.loadDocument(item);
	}
	
	public void showBrowser( final FileInfo fileToShow )
	{
		Log.v("cr3", "showBrowser() is called");
		mReaderView.save();
		mEngine.runInGUI( new Runnable() {
			public void run() {
				showView(mBrowser);
		        mEngine.hideProgress();
		        if ( fileToShow==null )
		        	mBrowser.showLastDirectory();
		        else
		        	mBrowser.showDirectory(fileToShow, fileToShow);
			}
		});
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		//if ( currentView!=readerView )
		//	return false;
	    MenuInflater inflater = getMenuInflater();
	    inflater.inflate(R.menu.cr3_reader_menu, menu);
	    return true;
	}

	public void showToast( String msg )
	{
		Toast toast = Toast.makeText(this, msg, Toast.LENGTH_LONG);
		toast.show();
	}

	public interface InputHandler {
		boolean validate( String s ) throws Exception;
		void onOk( String s ) throws Exception;
		void onCancel();
	};
	
	public static class InputDialog extends BaseDialog {
		private InputHandler handler;
		private EditText input;
		public InputDialog( Activity activity, final String title, boolean isNumberEdit, final InputHandler handler )
		{
			super(activity, R.string.dlg_button_ok, R.string.dlg_button_cancel );
			this.handler = handler;
			setTitle(title);
	        input = new EditText(getContext());
	        if ( isNumberEdit )
		        input.getText().setFilters(new InputFilter[] {
		        	new DigitsKeyListener()        
		        });
	        setView(input);
		}
		@Override
		protected void onNegativeButtonClick() {
            cancel();
            handler.onCancel();
		}
		@Override
		protected void onPositiveButtonClick() {
            String value = input.getText().toString().trim();
            try {
            	if ( handler.validate(value) )
            		handler.onOk(value);
            	else
            		handler.onCancel();
            } catch ( Exception e ) {
            	handler.onCancel();
            }
            cancel();
		}
	}
	
	public void showInputDialog( final String title, boolean isNumberEdit, final InputHandler handler )
	{
        final InputDialog dlg = new InputDialog(this, title, isNumberEdit, handler);
        dlg.show();
	}

	
	
	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		// pass
		super.onConfigurationChanged(newConfig);
	}

	String[] mFontFaces;

	public void showOptionsDialog()
	{
		final CoolReader _this = this;
		mBackgroundThread.executeBackground(new Runnable() {
			public void run() {
				mFontFaces = mEngine.getFontFaceList();
				mBackgroundThread.executeGUI(new Runnable() {
					public void run() {
						OptionsDialog dlg = new OptionsDialog(_this, mReaderView, mFontFaces);
						dlg.show();
					}
				});
			}
		});
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch ( item.getItemId() ) {
		case R.id.cr3_mi_open_file:
			Log.i("cr3", "Open File menu selected");
			showBrowser(mReaderView.getOpenedFileInfo());
			//showToast("TOC feature is not implemented");
			break;
		case R.id.cr3_go_toc:
			mReaderView.showTOC();
			break;
		case R.id.cr3_mi_gotopage:
			Log.i("cr3", "gotopage menu item selected");
			break;
		case R.id.cr3_mi_options:
			Log.i("cr3", "options menu item selected");
			showOptionsDialog();
			//showToast("Options feature is not implemented");
			break;
		case R.id.cr3_mi_bookmarks:
			Log.i("cr3", "Bookmarks menu item selected");
			BookmarksDlg dlg = new BookmarksDlg(this, mReaderView);
			dlg.show();
			break;
		case R.id.cr3_mi_search:
			Log.i("cr3", "Search menu item selected");
			mReaderView.showSearchDialog();
			break;
		case R.id.cr3_mi_exit:
			Log.i("cr3", "exit menu item selected");
			finish();
			break;
		case R.id.cr3_mi_go_page:
			showInputDialog("Enter page number", true, new InputHandler() {
				int pageNumber = 0;
				public boolean validate(String s) {
					pageNumber = Integer.valueOf(s); 
					return pageNumber>0;
				}
				public void onOk(String s) {
					mReaderView.goToPage(pageNumber);
				}
				public void onCancel() {
				}
			});
			break;
		case R.id.cr3_mi_go_percent:
			showInputDialog("Enter position %", true, new InputHandler() {
				int percent = 0;
				public boolean validate(String s) {
					percent = Integer.valueOf(s); 
					return percent>=0 && percent<=100;
				}
				public void onOk(String s) {
					mReaderView.goToPercent(percent);
				}
				public void onCancel() {
				}
			});
			break;
		default:
			return false;
			//return super.onOptionsItemSelected(item);
		}
		return true;
	}


}
