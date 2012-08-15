package org.coolreader.plugins;

public interface PurchaseBookCallback extends ErrorCallback {
	void onBookPurchased(String bookId, double newAccountBalance);
}
