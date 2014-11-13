package org.coolreader.plugins;

public interface PurchaseBookCallback extends ErrorCallback {
	void onBookPurchased(String bookId, double newAccountBalance);
	void onLowBalance(String bookId, double accountBalance, double bookPrice);
}
