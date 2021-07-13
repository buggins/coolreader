package org.coolreader.donations;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;

import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;

public class CRDonationService {
	private Activity mActivity;
	public static final Logger log = L.create("crdonations");

	public CRDonationService(Activity activity) {
		mActivity = activity;
		connect();
	}

	public interface PurchaseListener {
		void onPurchaseCompleted(boolean success, String productId, float totalDonations);
	}

	public void connect() {
	}

	@SuppressLint("NewApi")
	public void purchase(String productId, PurchaseListener listener) {
		if (listener != null)
			listener.onPurchaseCompleted(false, productId, 0);
	}

	public void onActivityResult(int requestCode, int resultCode, Intent data) {
	}

	public void bind() {
	}

	public void unbind() {
	}

	public boolean isBillingSupported() {
		return false;
	}
}
