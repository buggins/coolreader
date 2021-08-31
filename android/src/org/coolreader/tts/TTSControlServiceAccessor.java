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
	private final Activity mActivity;
	private volatile TTSControlBinder mServiceBinder;
	private volatile boolean mServiceBound;
	private volatile boolean bindIsCalled;
	private final ArrayList<TTSControlBinder.Callback> onConnectCallbacks = new ArrayList<>();
	private final Object mLocker = new Object();

	public interface Callback {
		void run(TTSControlServiceAccessor ttsacc);
	}

	public TTSControlServiceAccessor(Activity activity) {
		mActivity = activity;
	}

	public void bind(final TTSControlBinder.Callback boundCallback) {
		synchronized (this) {
			if (mServiceBinder != null && mServiceBound) {
				Log.v(TAG, "TTSControlService is already bound");
				if (boundCallback != null)
					boundCallback.run(mServiceBinder);
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
			mServiceBinder = null;
		}
	}

	private final ServiceConnection mServiceConnection = new ServiceConnection() {
		public void onServiceConnected(ComponentName className, IBinder service) {
			synchronized (TTSControlServiceAccessor.this) {
				mServiceBinder = ((TTSControlBinder) service);
				Log.i(TAG, "connected to TTSControlService");
			}
			synchronized (mLocker) {
				if (onConnectCallbacks.size() != 0) {
					// run once
					for (TTSControlBinder.Callback callback : onConnectCallbacks)
						callback.run(mServiceBinder);
					onConnectCallbacks.clear();
				}
			}
		}

		public void onServiceDisconnected(ComponentName className) {
			synchronized (TTSControlServiceAccessor.this) {
				mServiceBound = false;
				bindIsCalled = false;
				mServiceBinder = null;
			}
			Log.i(TAG, "Connection to the TTSControlService has been lost");
		}
	};

}
