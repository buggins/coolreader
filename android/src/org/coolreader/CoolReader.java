// Main Class
package org.coolreader;

import java.io.File;

import org.coolreader.crengine.CRDB;
import org.coolreader.crengine.Engine;
import org.coolreader.crengine.FileBrowser;
import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.History;
import org.coolreader.crengine.ReaderView;
import org.coolreader.crengine.Scanner;
import org.coolreader.crengine.Engine.HyphDict;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Environment;
import android.text.InputFilter;
import android.text.method.DigitsKeyListener;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.Toast;

public class CoolReader extends Activity
{
	Engine engine;
	ReaderView readerView;
	Scanner scanner;
	FileBrowser browser;
	FrameLayout frame;
	View startupView;
	History history;
	CRDB db;
	
	public History getHistory() 
	{
		return history;
	}
	
	public CRDB getDB()
	{
		return db;
	}
	
	private static final String BUNDLE_KEY_HISTORY = "cr3.history";
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
		Log.i("cr3", "CoolReader.onCreate()");
        super.onCreate(savedInstanceState);
		frame = new FrameLayout(this);
		engine = new Engine(this, frame);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, 
               WindowManager.LayoutParams.FLAG_FULLSCREEN );
		startupView = new View(this) {
		};
		startupView.setBackgroundColor(Color.BLACK);
		readerView = new ReaderView(this, engine);
		File dbdir = getDir("db", Context.MODE_PRIVATE);
		dbdir.mkdirs();
		File dbfile = new File(dbdir, "cr3db.sqlite");
		db = new CRDB(dbfile);
       	history = new History(db);
		scanner = new Scanner(db, engine, Environment.getExternalStorageDirectory(), "SD");
		browser = new FileBrowser(this, engine, scanner, history);
		frame.addView(readerView);
		frame.addView(browser);
		frame.addView(startupView);
		setContentView( frame );
		showView(startupView);
        Log.i("cr3", "initializing browser");
        browser.init();
        Log.i("cr3", "initializing reader");
        readerView.init();
    }

	@Override
	protected void onDestroy() {
		Log.i("cr3", "CoolReader.onDestroy()");
		if ( history!=null && db!=null ) {
			//history.saveToDB();
		}
		if ( readerView!=null ) {
			readerView.destroy();
			readerView = null;
		}
		if ( engine!=null ) {
			engine.uninit();
			engine = null;
		}
		if ( db!=null ) {
			db.close();
			db = null;
		}
			
		// TODO Auto-generated method stub
		super.onDestroy();
	}

	@Override
	protected void onPause() {
		Log.i("cr3", "CoolReader.onPause()");
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

	@Override
	protected void onRestart() {
		Log.i("cr3", "CoolReader.onRestart()");
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
		engine.setHyphenationDictionary( HyphDict.RUSSIAN );
        engine.showProgress( 5, "Starting Cool Reader..." );
        Log.i("cr3", "waiting for engine tasks completion");
        //engine.waitTasksCompletion();
        engine.execute(new Engine.EngineTask() {

			@Override
			public void done() {
		        Log.i("cr3", "trying to load last document");
				if ( LOAD_LAST_DOCUMENT_ON_START ) {
					readerView.loadLastDocument(new Runnable() {
						public void run() {
							// cannot open recent book: load another one
							Log.e("cr3", "Cannot open last document, starting file browser");
							showBrowser();
						}
					});
				} else {
					showBrowser();
				}
			}

			@Override
			public void fail(Exception e) {
			}

			@Override
			public void work() throws Exception {
				// do nothing
			}
        	
        });
	}

	@Override
	protected void onStop() {
		readerView.close();
		super.onStop();
	}

	private View currentView;
	public void showView( View view )
	{
		if ( currentView==view )
			return;
		frame.bringChildToFront(view);
		for ( int i=0; i<frame.getChildCount(); i++ ) {
			View v = frame.getChildAt(i);
			v.setVisibility(view==v?View.VISIBLE:View.INVISIBLE);
		}
	}
	
	public void showReader()
	{
		showView(readerView);
	}
	
	public boolean isBookOpened()
	{
		return readerView.isBookLoaded();
	}
	
	public void loadDocument( FileInfo item )
	{
		//showView(readerView);
		//setContentView(readerView);
		readerView.loadDocument(item);
	}
	
	public void showBrowser()
	{
		readerView.save();
		showView(browser);
		//setContentView(browser);
		//browser.start();
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
	
	public void showInputDialog( final String title, boolean isNumberEdit, final InputHandler handler )
	{
        final AlertDialog.Builder alert = new AlertDialog.Builder(this);
        final EditText input = new EditText(this);
        if ( isNumberEdit )
	        input.getText().setFilters(new InputFilter[] {
	        	new DigitsKeyListener()        
	        });
        alert.setTitle(title);
        alert.setView(input);
        alert.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                String value = input.getText().toString().trim();
                try {
                	if ( handler.validate(value) )
                		handler.onOk(value);
                	else
                		handler.onCancel();
                } catch ( Exception e ) {
                	handler.onCancel();
                }
                dialog.cancel();
            }
        });
 
        alert.setNegativeButton("Cancel",
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        dialog.cancel();
                        handler.onCancel();
                    }
                });
        alert.show();
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch ( item.getItemId() ) {
		case R.id.cr3_mi_open_file:
			Log.i("cr3", "Open File menu selected");
			showBrowser();
			//showToast("TOC feature is not implemented");
			break;
		case R.id.cr3_mi_gotopage:
			Log.i("cr3", "gotopage menu item selected");
			showToast("Goto Page feature is not implemented");
			break;
		case R.id.cr3_mi_options:
			Log.i("cr3", "options menu item selected");
			showToast("Options feature is not implemented");
			break;
		case R.id.cr3_mi_bookmarks:
			Log.i("cr3", "Bookmarks menu item selected");
			showToast("Bookmarks feature is not implemented");
			break;
		case R.id.cr3_mi_search:
			Log.i("cr3", "Search menu item selected");
			showToast("Search feature is not implemented");
			break;
		case R.id.cr3_mi_exit:
			Log.i("cr3", "exit menu item selected");
			finish();
			break;
		case R.id.cr3_mi_go_page:
			showInputDialog("Enter page number", true, new InputHandler() {
				int pageNumber = 0;
				@Override
				public boolean validate(String s) {
					pageNumber = Integer.valueOf(s); 
					return pageNumber>0;
				}
				@Override
				public void onOk(String s) {
					readerView.doCommand(ReaderView.ReaderCommand.DCMD_GO_PAGE, pageNumber-1);
				}
				@Override
				public void onCancel() {
				}
			});
			break;
		case R.id.cr3_mi_go_percent:
			showInputDialog("Enter position %", true, new InputHandler() {
				int percent = 0;
				@Override
				public boolean validate(String s) {
					percent = Integer.valueOf(s); 
					return percent>=0 && percent<=100;
				}
				@Override
				public void onOk(String s) {
					readerView.goToPercent(percent);
				}
				@Override
				public void onCancel() {
				}
			});
			break;
		default:
			return super.onOptionsItemSelected(item);
		}
		return true;
	}
}
