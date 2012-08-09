package org.coolreader.plugins;

import java.util.ArrayList;


public class OnlineStoreBooks extends AsyncResponse {
	public double account;
	public int pages;
	public int records;
	private ArrayList<OnlineStoreBook> list = new ArrayList<OnlineStoreBook>();
	public void add(OnlineStoreBook item) {
		list.add(item);
	}
	public int size() {
		return list.size();
	}
	public OnlineStoreBook get(int index) {
		return list.get(index);
	}
}