package org.coolreader.donations;

import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteException;

import com.android.vending.billing.IInAppBillingService;

public class CRDonationService {
	private Activity mActivity;
	private IInAppBillingService mService;
	private ServiceConnection mServiceConn;
	private boolean mBound;
	public static final Logger log = L.create("crdonations");
	public boolean mBillingSupported;
	
	public CRDonationService(Activity activity) {
		mActivity = activity;
		PACKAGE_NAME = mActivity.getPackageName();
		connect();
	}
	
	public void connect() {
		mServiceConn = new ServiceConnection() {
			@Override
			public void onServiceDisconnected(ComponentName name) {
				log.d("CRDonationService.onServiceDisconnected()");
			    mService = null;
			    mBillingSupported = false;
			}
			@Override
			public void onServiceConnected(ComponentName name,
					IBinder service) {
				log.d("CRDonationService.onServiceConnected()");
			    mService = IInAppBillingService.Stub.asInterface(service);
			    try {
					mBillingSupported = mService.isBillingSupported(API_VERSION, PACKAGE_NAME, "inapp") == RESULT_OK;
				} catch (RemoteException e) {
					mBillingSupported = false;
				}
			    log.d("CRDonationService.onServiceConnected()  billingSupported=" + mBillingSupported);
			}
		};
	}

	public void bind() {
		log.d("CRDonationService.bind()");
		Intent serviceIntent = new Intent("com.android.vending.billing.InAppBillingService.BIND");
		serviceIntent.setPackage("com.android.vending");
		mActivity.bindService(serviceIntent, mServiceConn, Context.BIND_AUTO_CREATE);
		mBound = true;
	}

	
	public void unbind() {
		log.d("CRDonationService.unbind()");
		if (mService != null) {
			mActivity.unbindService(mServiceConn);
	    }	
		mBound = false;
	}
	
	private final int API_VERSION = 3;
	private String PACKAGE_NAME = "org.coolreader";
	private final int RESULT_OK = 0;
	
	public boolean isBillingSupported() {
		return mBillingSupported;
	}
}
