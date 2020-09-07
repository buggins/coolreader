package org.coolreader.tts;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.util.Log;

import java.util.ArrayList;

public class TTSControlServiceAccessor {
	private final static String TAG = "ttssrv";
	private Activity mActivity;
	private volatile TTSControlBinder mService;
	private volatile boolean mServiceBound;
	private ArrayList<TTSControlBinder.Callback> onConnectCallbacks = new ArrayList<>();
	private boolean bindIsCalled;
	private final Object mLocker = new Object();

	public TTSControlServiceAccessor(Activity activity) {
		mActivity = activity;
	}

	public void bind(final TTSControlBinder.Callback boundCallback) {
		synchronized (this) {
			if (mService != null) {
				Log.v(TAG, "TTSControlService is already bound");
				if (boundCallback != null)
					boundCallback.run(mService);
				return;
			}
		}
		//Log.v(TAG, "binding TTSControlService");
		if (boundCallback != null) {
			synchronized (mLocker) {
				onConnectCallbacks.add(boundCallback);
			}
		}
		if (!bindIsCalled) {
			bindIsCalled = true;
			if (mActivity.bindService(new Intent(mActivity, TTSControlService.class), mServiceConnection, Context.BIND_AUTO_CREATE)) {
				mServiceBound = true;
				Log.v(TAG, "binding TTSControlService in progress...");
			} else {
				Log.e(TAG, "cannot bind TTSControlService");
			}
		}
	}

	public void unbind() {
		Log.v(TAG, "unbinding TTSControlService");
		if (mServiceBound) {
			// Detach our existing connection.
			mActivity.unbindService(mServiceConnection);
			mServiceBound = false;
			bindIsCalled = false;
		}
	}

	private ServiceConnection mServiceConnection = new ServiceConnection() {
		public void onServiceConnected(ComponentName className, IBinder service) {
			synchronized (TTSControlServiceAccessor.this) {
				mService = ((TTSControlBinder) service);
				Log.i(TAG, "connected to TTSControlService");
			}
			synchronized (mLocker) {
				if (onConnectCallbacks.size() != 0) {
					// run once
					for (TTSControlBinder.Callback callback : onConnectCallbacks)
						callback.run(mService);
					onConnectCallbacks.clear();
				}
			}
		}

		public void onServiceDisconnected(ComponentName className) {
			synchronized (TTSControlServiceAccessor.this) {
				mService = null;
			}
			Log.i(TAG, "disconnected from TTSControlService");
		}
	};

}
