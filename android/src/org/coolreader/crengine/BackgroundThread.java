package org.coolreader.crengine;

import java.util.ArrayList;
import java.util.concurrent.Callable;

import org.coolreader.crengine.ReaderView.Sync;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.View;

public class BackgroundThread extends Thread {
	
	public final static Object LOCK = new Object(); 
	
	private Handler handler;
	private View guiTarget;
	private ArrayList<Runnable> posted = new ArrayList<Runnable>();
	private ArrayList<Runnable> postedGUI = new ArrayList<Runnable>();
	public void setGUI(  View guiTarget ) {
		this.guiTarget = guiTarget;
		synchronized(postedGUI) {
			for ( Runnable task : postedGUI )
				guiTarget.post( task );
		}
	}
	public BackgroundThread() {
		super();
		setName("BackgroundThread" + Integer.toHexString(hashCode()));
		start();
	}
	public void run() {
		Looper.prepare();
		handler = new Handler() {
			public void handleMessage( Message message )
			{
				Log.d("cr3", "message: " + message);
			}
		};
		synchronized(posted) {
			for ( Runnable task : posted ) {
				handler.post(task);
			}
			posted.clear();
		}
		Looper.loop();
	}
	private Runnable guard( final Runnable r )
	{
		return new Runnable() {
			public void run() {
				synchronized (LOCK) {
					r.run();
				}
			}
		};
	}
	public void postBackground( Runnable task )
	{
		if ( mStopped ) {
			Log.i("cr3", "Posting task " + task + " to GUI queue since background thread is stopped");
			postGUI( task );
			return;
		}
		task = guard(task);
		if ( handler==null ) {
			synchronized(posted) {
				posted.add(task);
			}
		} else {
			handler.post(task);
		}
	}
	public void postGUI( Runnable task )
	{
		if ( guiTarget==null ) {
			synchronized( postedGUI ) {
				postedGUI.add(task);
			}
		} else {
			guiTarget.post(task);
		}
	}
	/**
	 * Run task instantly if called from the same thread, or post it through message queue otherwise.
	 * @param task is task to execute
	 */
	public void executeBackground( Runnable task )
	{
		task = guard(task);
		if ( isBackgroundThread() || mStopped )
			task.run(); // run in this thread
		else 
			postBackground(task); // post
	}
	// assume there are only two threads: main GUI and background
	public boolean isGUIThread()
	{
		return !isBackgroundThread();
	}
	public boolean isBackgroundThread()
	{
		return ( Thread.currentThread()==this );
	}
	public void executeGUI( Runnable task )
	{
		//Handler guiHandler = guiTarget.getHandler();
		//if ( guiHandler!=null && guiHandler.getLooper().getThread()==Thread.currentThread() )
		if ( isGUIThread() )
			task.run(); // run in this thread
		else
			postGUI(task);
	}

    public <T> Callable<T> guard( final Callable<T> task )
    {
    	return new Callable<T>() {
    		public T call() throws Exception {
    			return task.call();
    		}
    	};
    }
	
    public <T> T callBackground( final Callable<T> srcTask )
    {
    	final Callable<T> task = guard(srcTask);
    	if ( isBackgroundThread() ) {
    		try {
    			return task.call();
    		} catch ( Exception e ) {
    			return null;
    		}
    	}
    	//Log.d("cr3", "executeSync called");
    	final Sync<T> sync = new Sync<T>();
    	postBackground( new Runnable() {
    		public void run() {
    			try {
    				sync.set( task.call() );
    			} catch ( Exception e ) {
    				sync.set( null );
    			}
    		}
    	});
    	T res = sync.get();
    	//Log.d("cr3", "executeSync done");
    	return res;
    }
	
    public <T> T callGUI( final Callable<T> task )
    {
    	if ( isGUIThread() ) {
    		try {
    			return task.call();
    		} catch ( Exception e ) {
    			return null;
    		}
    	}
    	//Log.d("cr3", "executeSync called");
    	final Sync<T> sync = new Sync<T>();
    	postBackground( new Runnable() {
    		public void run() {
    			try {
    				sync.set( task.call() );
    			} catch ( Exception e ) {
    			}
    		}
    	});
    	T res = sync.get();
    	return res;
    }
	
	private boolean mStopped = false;
	
	public void waitForBackgroundCompletion()
	{
		callBackground(new Callable<Object>() {
			public Object call() {
				return null;
			}
		});
	}
	
	public void quit()
	{
		callBackground(new Callable<Object>() {
			public Object call() {
				mStopped = true;
				Looper.myLooper().quit();
				return null;
			}
		});
	}
}
