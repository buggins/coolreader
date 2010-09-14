// Main Class
package org.coolreader;

import org.coolreader.crengine.Engine;
import org.coolreader.crengine.ReaderView;

import android.app.Activity;
import android.os.Bundle;
import android.view.Window;

public class CoolReader extends Activity
{
	Engine engine;
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
		engine = new Engine(this);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(new ReaderView(this, engine));
    }

	@Override
	protected void onDestroy() {
		if ( engine!=null ) {
			engine.uninit();
			engine = null;
		}
			
		// TODO Auto-generated method stub
		super.onDestroy();
	}
}
