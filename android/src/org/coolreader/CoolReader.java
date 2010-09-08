// Main Class
package org.coolreader;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import org.coolreader.crengine.ReaderView;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;

public class CoolReader extends Activity
{
	
	void installLibrary()
	{
		File sopath = getDir("libs", Context.MODE_PRIVATE);
		File soname = new File(sopath, "libcr3engine.so");
		try {
			sopath.mkdirs();
	    	File zip = new File(getPackageCodePath());
	    	ZipFile zipfile = new ZipFile(zip);
	    	ZipEntry zipentry = zipfile.getEntry("lib/armeabi/libcr3engine.so");
	    	if ( !soname.exists() || zipentry.getSize()!=soname.length() ) {
		    	InputStream is = zipfile.getInputStream(zipentry);
				OutputStream os = new FileOutputStream(soname);
				final int BUF_SIZE = 0x10000;
				byte[] buf = new byte[BUF_SIZE];
				int n;
				while ((n = is.read(buf)) > 0)
				    os.write(buf, 0, n);
		        is.close();
		        os.close();
	    	}
		} catch ( IOException e ) {
	        Log.e("cr3", "cannot install cr3engine library", e);
		}
		System.load(soname.getAbsolutePath());
		String[] fileList = new File( Environment.getRootDirectory(), "fonts").list(
				new FilenameFilter()
		{ public boolean  accept(File  dir, String  filename)
			{
				return true; //filename.endsWith(".ttf");
			}
			});
		System.out.println(fileList);
	}
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
    	installLibrary();
        super.onCreate(savedInstanceState);
        setContentView(new ReaderView(this));
    }
}
