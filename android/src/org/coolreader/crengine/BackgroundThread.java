package org.coolreader.crengine;

import java.util.ArrayList;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.View;

public class BackgroundThread extends Thread {
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
	public void postBackground( Runnable task )
	{
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
		if ( Thread.currentThread()==this )
			task.run(); // run in this thread
		else 
			postBackground(task); // post
	}
	public void executeGUI( Runnable task )
	{
		Handler guiHandler = guiTarget.getHandler();
		if ( guiHandler!=null && guiHandler.getLooper().getThread()==Thread.currentThread() )
			task.run(); // run in this thread
		else
			postGUI(task);
	}
	public void quit()
	{
		executeBackground( new Runnable() {
			public void run() {
				Looper.myLooper().quit();
			}
		});
	}
}
