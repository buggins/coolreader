// Main Class
package org.coolreader;

import android.app.Activity;
import android.os.Bundle;

import org.coolreader.crengine.ReaderView;

public class CoolReader extends Activity
{
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(new ReaderView(this));
    }
}
