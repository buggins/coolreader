// Main Class
package org.coolreader;

import org.coolreader.crengine.Engine;
import org.coolreader.crengine.ReaderView;

import android.app.Activity;
import android.os.Bundle;
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
        super.onCreate(savedInstanceState);
		engine = new Engine(this);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, 
               WindowManager.LayoutParams.FLAG_FULLSCREEN );
		readerView = new ReaderView(this, engine);
		setContentView( readerView );
    }

	@Override
	protected void onDestroy() {
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
}
