package org.coolreader.sync;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;

import org.coolreader.crengine.BackgroundThread;
import org.coolreader.crengine.Bookmark;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.util.Log;

public class SyncServiceAccessor {
	private final static String TAG = "cr3sync";
	private Activity mActivity;
    private SyncService mSyncService;
    private boolean mSyncServiceBound;
    private File syncDirectory;

    public interface ChangeInfoReceiver {
    	void onChanges(Collection<ChangeInfo> list);
    };
    
	public SyncServiceAccessor(Activity activity) {
		mActivity = activity;
	}

	private Runnable connectCallback;
    public void bind(final Runnable boundCallback) {
    	Log.v(TAG, "binding SyncService");
    	if (mSyncService != null) {
    		boundCallback.run();
    		return;
    	}
    	connectCallback = boundCallback;
    	if (mActivity.bindService(new Intent(mActivity, 
                SyncService.class), mSyncServiceConnection, Context.BIND_AUTO_CREATE)) {
            mSyncServiceBound = true;
    	} else {
    		Log.e(TAG, "cannot bind SyncService");
    	}
    }

    public void unbind() {
    	Log.v(TAG, "unbinding SyncService");
        if (mSyncServiceBound) {
            // Detach our existing connection.
            mActivity.unbindService(mSyncServiceConnection);
            mSyncServiceBound = false;
        }
    }
    
    public void setSyncDirectory(final File dir) {
    	BackgroundThread.instance().postGUI(new Runnable() {
			@Override
			public void run() {
		    	if (!mSyncServiceBound || mSyncService == null) {
		    		syncDirectory = dir;
		    	} else {
		    		mSyncService.setSyncDirectory(dir);
		    	}
			}
    	});
    }

    public void checkChanges(final ChangeInfoReceiver callback) {
    	BackgroundThread.instance().postBackground(new Runnable() {
			@Override
			public void run() {
		    	if (!mSyncServiceBound || mSyncService == null) {
		    		Log.e(TAG, "checkChanges: service is not bound");
		    		return;
		    	}
		    	final List<ChangeInfo> res = new ArrayList<ChangeInfo>();
		    	mSyncService.sync(res, 5000);
		    	if (res.size() > 0) {
			    	BackgroundThread.instance().postGUI(new Runnable() {
						@Override
						public void run() {
							if (mSyncServiceBound)
								callback.onChanges(res);
						}
			    	});
		    	}
			}
    	});
    }

    /**
     * Retrieve remote changes synchronously.
     * @param maxRecords is limit of record number
     * @return list with read records, null if no updates received
     */
    public List<ChangeInfo> checkChangesSync(int maxRecords) {
    	if (!mSyncServiceBound || mSyncService == null) {
    		Log.e(TAG, "checkChanges: service is not bound");
    		return null;
    	}
    	final List<ChangeInfo> list = new ArrayList<ChangeInfo>();
    	mSyncService.sync(list, maxRecords);
    	if (list.size() == 0)
    		return null;
    	return list;
    }

    public void saveBookmark(String fileName, Bookmark bookmark, boolean sync) {
    	ChangeInfo change = new ChangeInfo(bookmark, fileName, false);
    	save(change, sync);
    }
    
    public void removeBookmark(String fileName, Bookmark bookmark) {
    	ChangeInfo change = new ChangeInfo(bookmark, fileName, true);
    	save(change, false);
    }
    
    public void removeFile(String fileName) {
    	ChangeInfo change = new ChangeInfo(null, fileName, true);
    	save(change, false);
    }
    
    public void removeFileLastPosition(String fileName) {
    	Bookmark bmk = new Bookmark();
    	bmk.setType(Bookmark.TYPE_LAST_POSITION);
    	bmk.setStartPos("no_position");
    	ChangeInfo change = new ChangeInfo(bmk, fileName, true);
    	save(change, false);
    }
    
    public void save(ChangeInfo change, boolean sync) {
    	if (sync)
    		saveSync(Collections.singleton(change));
    	else
    		save(Collections.singleton(change), null);
    }

    public void saveSync(final Collection<ChangeInfo> list) {
    	if (!mSyncServiceBound || mSyncService == null) {
    		Log.e(TAG, "setSyncDirectory: service is not bound");
    		return;
    	}
    	mSyncService.saveBookmarks(list);
    }

    public void save(final Collection<ChangeInfo> list, final Runnable doneHandler) {
    	BackgroundThread.instance().postBackground(new Runnable() {
			@Override
			public void run() {
		    	if (!mSyncServiceBound || mSyncService == null) {
		    		Log.e(TAG, "setSyncDirectory: service is not bound");
		    		return;
		    	}
		    	mSyncService.saveBookmarks(list);
				if (doneHandler != null) {
			    	BackgroundThread.instance().postGUI(new Runnable() {
						@Override
						public void run() {
							if (mSyncServiceBound)
								doneHandler.run();
						}
			    	});
				}
			}
    	});
    }

    private ServiceConnection mSyncServiceConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder service) {
        	mSyncService = ((SyncService.LocalBinder)service).getService();
        	Log.i(TAG, "connected to SyncService");
        	if (connectCallback != null)
        		connectCallback.run();
        	connectCallback = null;
        	if (syncDirectory != null) {
            	Log.i(TAG, "setting sync directory " + syncDirectory);
        		mSyncService.setSyncDirectory(syncDirectory);
        	}
            // DEBUG
//            if (mSyncService != null)
//            	mSyncService.test();
        }

        public void onServiceDisconnected(ComponentName className) {
            mSyncService = null;
        	Log.i(TAG, "disconnected from SyncService");
        }
    };

}
