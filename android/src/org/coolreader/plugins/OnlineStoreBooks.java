package org.coolreader.plugins;

import org.coolreader.crengine.Utils;

import java.util.ArrayList;
import java.util.Collections;


public class OnlineStoreBooks implements AsyncResponse {
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
	
	public void sortBySeriesAndTitle() {
		Collections.sort(list, (lhs, rhs) -> {
			if (lhs.sequenceName != null) {
				if (rhs.sequenceName == null)
					return -1;
				int res = lhs.sequenceName.compareToIgnoreCase(rhs.sequenceName);
				if (res != 0)
					return res;
				if (lhs.sequenceNumber < rhs.sequenceNumber)
					return -1;
				else if (lhs.sequenceNumber > rhs.sequenceNumber)
					return 1;
				else
					return Utils.cmp(lhs.bookTitle, rhs.bookTitle);
			}
			if (rhs.sequenceName != null)
				return 1;
			return Utils.cmp(lhs.bookTitle, rhs.bookTitle);
		});
	}
}