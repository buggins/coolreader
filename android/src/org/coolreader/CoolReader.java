// Main Class
package org.coolreader;

import java.io.File;

import org.coolreader.crengine.Engine;
import org.coolreader.crengine.ReaderView;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;

public class CoolReader extends Activity
{
	Engine engine;
	ReaderView readerView;
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
		Log.i("cr3", "CoolReader.onCreate()");
        super.onCreate(savedInstanceState);
		engine = new Engine(this);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, 
               WindowManager.LayoutParams.FLAG_FULLSCREEN );
		readerView = new ReaderView(this, engine);
		setContentView( readerView );
        engine.showProgress( 5, "Starting Cool Reader..." );
    }

	@Override
	protected void onDestroy() {
		Log.i("cr3", "CoolReader.onDestroy()");
		if ( readerView!=null ) {
			readerView.destroy();
			readerView = null;
		}
		if ( engine!=null ) {
			engine.uninit();
			engine = null;
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

	@Override
	protected void onStart() {
		Log.i("cr3", "CoolReader.onStart()");
		super.onStart();
		readerView.loadLastDocument(new Runnable() {
			public void run() {
				// cannot open recent book: load another one
				// TODO:
		        File sddir = Environment.getExternalStorageDirectory();
		        File booksdir = new File( sddir, "books");
		        File exampleFile = new File( booksdir, "volkov.fb2");
		        //File exampleFile = new File( booksdir, "naslednik.fb2.zip");
		        //File exampleFile = new File( booksdir, "krisis.fb2.zip");
		        //File exampleFile = new File( booksdir, "bibl.fb2.zip");
		        //File exampleFile = new File( booksdir, "drabkin.fb2.zip");
		        //File exampleFile = new File( booksdir, "BurglarsTrip.fb2.zip");
		        //File exampleFile = new File( booksdir, "kalma.fb2.zip");
		        //File exampleFile = new File( booksdir, "example.fb2");
		        //readerView.loadDocument(exampleFile.getAbsolutePath());
		        readerView.showFileSelector();
			}
		});
	}

	@Override
	protected void onStop() {
		readerView.close();
		super.onStop();
	}
	
	
}
