package org.coolreader.db;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.util.Log;

public class CRDBServiceAccessor {
	private final static String TAG = "cr3db";
	private Activity mActivity;
    private CRDBService.LocalBinder mService;
    private boolean mServiceBound;

    public CRDBService.LocalBinder get() {
    	if (mService == null)
    		throw new RuntimeException("no service");
    	return mService;
    }
    
	public CRDBServiceAccessor(Activity activity) {
		mActivity = activity;
	}

    public void bind() {
    	Log.v(TAG, "binding CRDBService");
    	if (mActivity.bindService(new Intent(mActivity, 
                CRDBService.class), mServiceConnection, Context.BIND_AUTO_CREATE)) {
            mServiceBound = true;
    	} else {
    		Log.e(TAG, "cannot bind CRDBService");
    	}
    }

    public void unbind() {
    	Log.v(TAG, "unbinding CRDBService");
        if (mServiceBound) {
            // Detach our existing connection.
            mActivity.unbindService(mServiceConnection);
            mServiceBound = false;
        }
    }
    
    private ServiceConnection mServiceConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder service) {
        	mService = ((CRDBService.LocalBinder)service);
        	Log.i(TAG, "connected to CRDBService");
        }

        public void onServiceDisconnected(ComponentName className) {
            mService = null;
        	Log.i(TAG, "disconnected from CRDBService");
        }
    };

}
