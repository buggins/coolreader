package org.coolreader.crengine;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;
import android.view.View;

/**
 * CoolReader Engine class.
 *
 * Only one instance is allowed.
 */
public class Engine {
	
	private final Activity activity;
	
	public interface EngineTask {
		public void work() throws Exception;
		public void done();
		public void fail( Exception e );
	}
	
	public static class FatalError extends RuntimeException {
		private Engine engine;
		private String msg;
		public FatalError( Engine engine, String msg )
		{
			this.engine = engine;
			this.msg = msg;
		}
		public void handle()
		{
			engine.fatalError(msg);
		}
	}
	
	private ExecutorService executor = Executors.newFixedThreadPool(1);

	private static class TaskHandler implements Runnable {
		final EngineTask task;
		final View view;
		public TaskHandler( EngineTask task, View view )
		{
			this.task = task;
			this.view = view;
		}
		public void run() {
			try {
				// run task
				task.work();
				// post success callback
				view.post(new Runnable() {
					public void run() {
						task.done();
					}
				});
			} catch ( final FatalError e ) {
				Handler h = view.getHandler();
				
				if ( h==null ) {
					View root = view.getRootView();
					h = root.getHandler();
				}
				if ( h==null ) {
					//
					e.handle();
				} else {
					h.postAtFrontOfQueue(new Runnable() {
						public void run() {
							e.handle();
						}
					});
				}
			} catch ( final Exception e ) {
				// post error callback
				view.post(new Runnable() {
					public void run() {
						task.fail(e);
					}
				});
			}
		}
	}
	
	public void execute( final EngineTask task, final View view )
	{
		Log.d("cr3", "executing task " + task.getClass().getSimpleName());
		TaskHandler taskHandler = new TaskHandler( task, view );
		executor.execute( taskHandler );
	}

	public void fatalError( String msg)
	{
		AlertDialog dlg = new AlertDialog.Builder(activity).setMessage(msg).setTitle("CoolReader fatal error").show();
		try {
			Thread.sleep(10);
		} catch ( InterruptedException e ) {
			// do nothing
		}
		dlg.dismiss();
		activity.finish();
	}
	
	/**
	 * Initialize CoolReader Engine
	 * @param fontList is array of .ttf font pathnames to load
	 */
	public Engine( Activity activity )
	{
		this.activity = activity;
	}

	private native boolean initInternal( String[] fontList );
	private native void uninitInternal();
	private native String[] getFontFaceListInternal();
	
	public String[] getFontFaceList()
	{
		if ( !initialized )
			throw new IllegalStateException("CREngine is not initialized");
		return getFontFaceListInternal();
	}
	
	public void init() throws IOException
	{
		if ( initialized )
			throw new IllegalStateException("Already initialized");
    	installLibrary();
    	String[] fonts = findFonts();
		if ( !initInternal( fonts ) )
			throw new IOException("Cannot initialize CREngine JNI");
		initialized = true;
	}
	
	/**
	 * Uninitialize engine.
	 */
	public void uninit()
	{
		if ( !initialized )
			throw new IllegalStateException("Not initialized");
		uninitInternal();
		initialized = false;
	}
	
	protected void finalize() throws Throwable
	{
		if ( initialized )
			uninit();
	}
	
	static private boolean initialized = false;

	private String[] findFonts()
	{
		File fontDir = new File( Environment.getRootDirectory(), "fonts");
		// get font names
		String[] fileList = fontDir.list(
				new FilenameFilter()
		{ public boolean  accept(File  dir, String  filename)
			{
				return filename.endsWith(".ttf") && !filename.endsWith("Fallback.ttf");
			}
			});
		// append path
		for ( int i=0; i<fileList.length; i++ ) {
			fileList[i] = new File(fontDir, fileList[i]).getAbsolutePath();
			Log.v("cr3", "found font: " + fileList[i]);
		}
		return fileList;
	}
	
	private void installLibrary()
	{
		try {
			// try loading library w/o manual installation
			Log.i("cr3", "trying to load library cr3engine w/o installation");
			System.loadLibrary("cr3engine");
			Log.i("cr3", "cr3engine loaded successfully");
		} catch ( Exception ee ) {
			Log.i("cr3", "cr3engine not found using standard paths, will install manually");
			File sopath = activity.getDir("libs", Context.MODE_PRIVATE);
			File soname = new File(sopath, "libcr3engine.so");
			try {
				sopath.mkdirs();
		    	File zip = new File(activity.getPackageCodePath());
		    	ZipFile zipfile = new ZipFile(zip);
		    	ZipEntry zipentry = zipfile.getEntry("lib/armeabi/libcr3engine.so");
		    	if ( !soname.exists() || zipentry.getSize()!=soname.length() ) {
			    	InputStream is = zipfile.getInputStream(zipentry);
					OutputStream os = new FileOutputStream(soname);
			        Log.i("cr3", "Installing JNI library " + soname.getAbsolutePath());
					final int BUF_SIZE = 0x10000;
					byte[] buf = new byte[BUF_SIZE];
					int n;
					while ((n = is.read(buf)) > 0)
					    os.write(buf, 0, n);
			        is.close();
			        os.close();
		    	} else {
			        Log.i("cr3", "JNI library " + soname.getAbsolutePath() + " is up to date");
		    	}
				System.load(soname.getAbsolutePath());
			} catch ( Exception e ) {
		        Log.e("cr3", "cannot install cr3engine library", e);
			}
		}
	}
	
}
