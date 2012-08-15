package org.coolreader.plugins;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

import org.coolreader.crengine.Utils;


public class OnlineStoreAuthors implements AsyncResponse {
	private ArrayList<OnlineStoreAuthor> list = new ArrayList<OnlineStoreAuthor>();
	public void add(OnlineStoreAuthor author) {
		list.add(author);
	}
	public int size() {
		return list.size();
	}
	public OnlineStoreAuthor get(int index) {
		return list.get(index);
	}
	public void sortByName() {
		Collections.sort(list, new Comparator<OnlineStoreAuthor>() {
			@Override
			public int compare(OnlineStoreAuthor lhs, OnlineStoreAuthor rhs) {
				return Utils.cmp(lhs.lastName, rhs.lastName);
			}
		});
		
	}
}