package org.coolreader.crengine;

import java.util.ArrayList;
import java.util.concurrent.Callable;

import org.coolreader.crengine.ReaderView.Sync;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

/**
 * Allows running tasks either in background thread or in GUI thread.
 */
public class BackgroundThread extends Thread {
	
	private final static Object LOCK = new Object(); 

	private static BackgroundThread instance;
	
	// singleton
	public static BackgroundThread instance()
	{
		if ( instance==null ) {
			synchronized( LOCK ) {
				if ( instance==null ) {
					instance = new BackgroundThread();
					instance.start();
				}
			}
		}
		return instance;
	}
	
	public static Handler getBackgroundHandler() {
		if (instance == null)
			return null;
		return instance.handler;
	}

	public static Handler getGUIHandler() {
		if (instance().guiHandler == null)
			return null;
		return instance().guiHandler;
	}

	public final static boolean CHECK_THREAD_CONTEXT = true; 

	/**
	 * Throws exception if not in background thread.
	 */
	public final static void ensureBackground()
	{
		if ( CHECK_THREAD_CONTEXT && !isBackgroundThread() ) {
			L.e("not in background thread", new Exception("ensureInBackgroundThread() is failed"));
			throw new RuntimeException("ensureInBackgroundThread() is failed");
		}
	}
	
	/**
	 * Throws exception if not in GUI thread.
	 */
	public final static void ensureGUI()
	{
		if ( CHECK_THREAD_CONTEXT && isBackgroundThread() ) {
			L.e("not in GUI thread", new Exception("ensureGUI() is failed"));
			throw new RuntimeException("ensureGUI() is failed");
		}
	}
	
	// 
	private Handler handler;
	private ArrayList<Runnable> posted = new ArrayList<Runnable>();
	private Handler guiHandler;
	private ArrayList<Runnable> postedGUI = new ArrayList<Runnable>();

	/**
	 * Set view to post GUI tasks to.
	 * @param guiTarget is view to post GUI tasks to.
	 */
	public void setGUIHandler(Handler guiHandler) {
		this.guiHandler = guiHandler;
		if (guiHandler != null) {
			// forward already posted events
			synchronized(postedGUI) {
				L.d("Engine.setGUI: " + postedGUI.size() + " posted tasks to copy");
				for ( Runnable task : postedGUI )
					guiHandler.post( task );
			}
		}
	}

	/**
	 * Create background thread executor.
	 */
	private BackgroundThread() {
		super();
		setName("BackgroundThread" + Integer.toHexString(hashCode()));
		Log.i("cr3", "Created new background thread instance");
	}

	@Override
	public void run() {
		Log.i("cr3", "Entering background thread");
		Looper.prepare();
		handler = new Handler() {
			public void handleMessage( Message message )
			{
				Log.d("cr3", "message: " + message);
			}
		};
		Log.i("cr3", "Background thread handler is created");
		synchronized(posted) {
			for ( Runnable task : posted ) {
				Log.i("cr3", "Copying posted bg task to handler : " + task);
				handler.post(task);
			}
			posted.clear();
		}
		Looper.loop();
		handler = null;
		instance = null;
		Log.i("cr3", "Exiting background thread");
	}

	//private final static boolean USE_LOCK = false;
	private Runnable guard( final Runnable r )
	{
		return r;
//		if ( !USE_LOCK )
//			return r;
//		return new Runnable() {
//			public void run() {
//				synchronized (LOCK) {
//					r.run();
//				}
//			}
//		};
	}

	/**
	 * Post runnable to be executed in background thread.
	 * @param task is runnable to execute in background thread.
	 */
	public void postBackground( Runnable task )
	{
		Engine.suspendLongOperation();
		if ( mStopped ) {
			L.i("Posting task " + task + " to GUI queue since background thread is stopped");
			postGUI( task );
			return;
		}
		task = guard(task);
		if ( handler==null ) {
			synchronized(posted) {
				L.i("Adding task " + task + " to posted list since handler is not yet created");
				posted.add(task);
			}
		} else {
			handler.post(task);
		}
	}

	/**
	 * Post runnable to be executed in GUI thread
	 * @param task is runnable to execute in GUI thread
	 */
	public void postGUI( Runnable task )
	{
		postGUI(task, 0);
	}

	static int delayedTaskId = 0;
	/**
	 * Post runnable to be executed in GUI thread
	 * @param task is runnable to execute in GUI thread
	 * @param delay is delay before running task, in millis
	 */
	public void postGUI(final Runnable task, final long delay)
	{
		if ( guiHandler==null ) {
			synchronized( postedGUI ) {
				postedGUI.add(task);
			}
		} else {
			if ( delay>0 ) {
				final int id = ++delayedTaskId;
				//L.v("posting delayed (" + delay + ") task " + id + " " + task);
				guiHandler.postDelayed(new Runnable() {
					@Override
					public void run() {
						task.run();
						//L.v("finished delayed (" + delay + ") task " + id + " " + task);
					}
				}, delay);
			} else
				guiHandler.post(task);
		}
	}

	/**
	 * Run task instantly if called from the same thread, or post it through message queue otherwise.
	 * @param task is task to execute
	 */
	public void executeBackground( Runnable task )
	{
		Engine.suspendLongOperation();
		task = guard(task);
		if (isBackgroundThread() || mStopped)
			task.run(); // run in this thread
		else 
			postBackground(task); // post
	}

	// assume there are only two threads: main GUI and background
	public static boolean isGUIThread()
	{
		return !isBackgroundThread();
	}

	public static boolean isBackgroundThread()
	{
		return (Thread.currentThread() == instance);
	}

	public void executeGUI( Runnable task )
	{
		//Handler guiHandler = guiTarget.getHandler();
		//if ( guiHandler!=null && guiHandler.getLooper().getThread()==Thread.currentThread() )
		if (isGUIThread())
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
    
    
    /**
     * Waits until all pending background tasks are executed.
     */
    public void syncWithBackground() {
    	callBackground( new Callable<Integer>() {
			@Override
			public Integer call() throws Exception {
				return null;
			}
    	});
    }
	
    public <T> T callBackground( final Callable<T> srcTask )
    {
    	final Callable<T> task = srcTask; //guard(srcTask);
    	if ( isBackgroundThread() ) {
    		try {
    			return task.call();
    		} catch ( Exception e ) {
    			return null;
    		}
    	}
    	//L.d("executeSync called");
    	if(DBG) L.d("callBackground : posting Background task " + Thread.currentThread().getName());
    	final Sync<T> sync = new Sync<T>();
    	postBackground( new Runnable() {
    		public void run() {
    			if(DBG) L.d("callBackground : inside background thread " + Thread.currentThread().getName());
    			try {
    				sync.set( task.call() );
    			} catch ( Exception e ) {
    				sync.set( null );
    			}
    		}
    	});
    	if(DBG) L.d("callBackground : calling get " + Thread.currentThread().getName());
    	T res = sync.get();
    	if(DBG) L.d("callBackground : returned from get " + Thread.currentThread().getName());
    	//L.d("executeSync done");
    	return res;
    }
	
    private final static boolean DBG = false; 
    
    public <T> T callGUI( final Callable<T> task )
    {
    	if ( isGUIThread() ) {
    		try {
    			return task.call();
    		} catch ( Exception e ) {
    			return null;
    		}
    	}
    	if(DBG) L.d("callGUI : posting GUI task " + Thread.currentThread().getName());
    	final Sync<T> sync = new Sync<T>();
    	postGUI( new Runnable() {
    		public void run() {
    			if(DBG) L.d("callGUI : inside GUI thread " + Thread.currentThread().getName());
    	    	T result = null;
    			try {
        	    	L.d("callGUI : calling source callable " + Thread.currentThread().getName());
    				result = task.call();
    			} catch ( Exception e ) {
    				if(DBG) L.e("exception in postGUI", e);
    				throw new RuntimeException(e);
    			}
    			try {
    				if(DBG) L.d("callGUI : calling sync.set " + Thread.currentThread().getName());
    				sync.set( result );
    				if(DBG) L.d("callGUI : returned from sync.set " + Thread.currentThread().getName());
    			} catch ( Exception e ) {
    				if(DBG) L.e("exception in postGUI", e);
    				throw new RuntimeException(e);
    			}
    		}
    	});
    	if(DBG) L.d("callGUI : calling get " + Thread.currentThread().getName());
    	T res = sync.get();
    	if(DBG) L.d("callGUI : returned from get " + Thread.currentThread().getName());
    	return res;
    }
	
	private boolean mStopped = false;
	
	public void waitForBackgroundCompletion() {
		Engine.suspendLongOperation();
		callBackground(new Callable<Object>() {
			public Object call() {
				return null;
			}
		});
	}
	
	public void quit() {
		postBackground(new Runnable() {
			@Override
			public void run() {
				if (handler != null) {
					L.i("Calling quit() on background thread looper.");
					handler.getLooper().quit();
				}
			}
		});
	}
}
