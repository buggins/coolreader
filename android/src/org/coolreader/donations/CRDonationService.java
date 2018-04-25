package org.coolreader.donations;

import java.util.ArrayList;

import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;
import org.json.JSONException;
import org.json.JSONObject;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentSender.SendIntentException;
import android.content.ServiceConnection;
import android.os.Bundle;
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
	
	public static class PurchaseInfo {
		public String productId;
		public String title;
		public String description;
		public String price;
		public long priceAmountMicros;
		public String currency;
		public boolean purchased;
		@Override
		public String toString() {
			return "[productId=" + productId + ", title=" + title
					+ ", description=" + description + ", price=" + price
					+ ", priceAmountMicros=" + priceAmountMicros
					+ ", currency=" + currency + ", purchased=" + purchased
					+ "]";
		}
	}
	
	public interface PurchaseListener {
		void onPurchaseCompleted(boolean success, String productId, float totalDonations);
	}
	
	private ArrayList<PurchaseInfo> mPurchases = new ArrayList<PurchaseInfo>();
	private ArrayList<PurchaseInfo> mProducts = new ArrayList<PurchaseInfo>();
	
	private float mTotalDonations = 0;
	
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
					mProducts = getProducts(SKUS);
					mPurchases = getPurchases();
					log.d("Product list: " + mProducts);
					log.d("Purchases list: " + mPurchases);
				} catch (RemoteException e) {
					log.e("RemoteException while trying to check if billing is supported");
					mBillingSupported = false;
				}
			    log.d("CRDonationService.onServiceConnected()  billingSupported=" + mBillingSupported);
			}
		};
	}
	
	private PurchaseInfo productById(String id) {
		if (id == null)
			return null;
		for (PurchaseInfo p: mProducts) {
			if (p.productId.equals(id))
				return p;
		}
		return null;
	}
	
	private final static String[] SKUS = {"donation0.3", "donation1", "donation3", "donation10", "donation30", "donation100"};
	
	private ArrayList<PurchaseInfo> getProducts(String[] skus) {
		ArrayList<PurchaseInfo> mPurchases = new ArrayList<PurchaseInfo>();
		Bundle querySkus = null;
		if (skus != null && skus.length > 0) {
			ArrayList<String> skuList = new ArrayList<String>();
			for(String id: skus) {
				skuList.add(id);
			}
			querySkus = new Bundle();
			querySkus.putStringArrayList("ITEM_ID_LIST", skuList);
		}
		try {
			Bundle skuDetails = mService.getSkuDetails(API_VERSION,
					   PACKAGE_NAME, "inapp", querySkus);
			int response = skuDetails.getInt("RESPONSE_CODE");
			if (response == 0) {
				ArrayList<String> responseList = skuDetails.getStringArrayList("DETAILS_LIST");

				if (responseList != null) {
					for (String thisResponse : responseList) {
						JSONObject object;
						try {
							object = new JSONObject(thisResponse);
						    PurchaseInfo p = new PurchaseInfo();
						    p.productId = object.getString("productId");
						    p.price = object.getString("price");
						    p.title = object.getString("title");
						    p.description = object.getString("description");
						    p.currency = object.getString("price_currency_code");
						    p.priceAmountMicros = Long.valueOf(object.getString("price_amount_micros")).longValue();
						    mPurchases.add(p);
						} catch (JSONException e) {
							log.e("Exception while reading product info");
						}
					}
				}
			}		
		} catch (RemoteException e) {
			log.e("RemoteException while trying to get purchases");
		}
		return mPurchases;
	}

	private ArrayList<PurchaseInfo> getPurchases() {
		ArrayList<PurchaseInfo> mPurchases = new ArrayList<PurchaseInfo>();
		try {
			Bundle ownedItems = mService.getPurchases(API_VERSION,
					   PACKAGE_NAME, "inapp", null);
			int response = ownedItems.getInt("RESPONSE_CODE");
			if (response == 0) {
				ArrayList<String> ownedSkus =
					      ownedItems.getStringArrayList("INAPP_PURCHASE_ITEM_LIST");
					   
			    for (int i = 0; i < ownedSkus.size(); ++i) {
				    String sku = ownedSkus.get(i);
				    PurchaseInfo p = productById(sku);
				    if (p != null) {
				    	p.purchased = true;
					    mPurchases.add(p);
				    }
				}
			}		
		} catch (RemoteException e) {
			log.e("RemoteException while trying to get purchases");
		}
		return mPurchases;
	}
	
	private PurchaseListener mCurrentListener;
	
	@SuppressLint("NewApi")
	public void purchase(String productId, PurchaseListener listener) {
		if (!isBillingSupported()) {
			if (mCurrentListener != null)
				mCurrentListener.onPurchaseCompleted(false, productId, 0);
		} 
		try {
			mCurrentListener = listener;
			Bundle buyIntentBundle = mService.getBuyIntent(API_VERSION, PACKAGE_NAME,
					productId, "inapp", "NO_PAYLOAD");
			PendingIntent pendingIntent = buyIntentBundle.getParcelable("BUY_INTENT");
			mActivity.startIntentSenderForResult(pendingIntent.getIntentSender(),
					   1001, new Intent(), Integer.valueOf(0), Integer.valueOf(0),
					   Integer.valueOf(0));
		} catch (RemoteException e) {
			if (mCurrentListener != null)
				mCurrentListener.onPurchaseCompleted(false, productId, 0);
			mCurrentListener = null;
		} catch (SendIntentException e) {
			if (mCurrentListener != null)
				mCurrentListener.onPurchaseCompleted(false, productId, 0);
			mCurrentListener = null;
		}
	}
	
	public void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (requestCode == 1001) {
			//int responseCode = data.getIntExtra("RESPONSE_CODE", 0);
			String purchaseData = data.getStringExtra("INAPP_PURCHASE_DATA");
			//String dataSignature = data.getStringExtra("INAPP_DATA_SIGNATURE");

			if (resultCode == RESULT_OK) {
				try {
					JSONObject jo = new JSONObject(purchaseData);
					if (jo != null) {
						String sku = jo.optString("productId");
						if (sku != null) {
							log.i("Purchase is completed for " + sku);
							if (mCurrentListener != null) {
								mCurrentListener.onPurchaseCompleted(true, sku, mTotalDonations);
							}
							return;
						}
					}
				}
				catch (Exception e) {
					log.e("Exception while checking activity result");
				}
				log.i("Purchase is failed");
				if (mCurrentListener != null) {
					mCurrentListener.onPurchaseCompleted(false, null, mTotalDonations);
				}
			}
			mCurrentListener = null;
		}
	}	

	@SuppressLint("NewApi")
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
	
	private final static int API_VERSION = 3;
	private String PACKAGE_NAME = "org.coolreader";
	private final static int RESULT_OK = 0;
	
	public boolean isBillingSupported() {
		return mBound && mBillingSupported;
	}
}
