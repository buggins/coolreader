package org.coolreader.crengine;

import java.util.ArrayList;

import android.database.DataSetObserver;
import android.widget.ListAdapter;

public abstract class BaseListAdapter implements ListAdapter {
	private ArrayList<DataSetObserver> observers = new ArrayList<DataSetObserver>();
	
	public void registerDataSetObserver(DataSetObserver observer) {
		observers.add(observer);
	}

	public void unregisterDataSetObserver(DataSetObserver observer) {
		observers.remove(observer);
	}
	
	public void notifyDataSetChanged() {
		for (DataSetObserver observer : observers) {
			observer.onChanged();
		}
	}

	public void notifyInvalidated() {
		for (DataSetObserver observer : observers) {
			observer.onInvalidated();
		}
	}
}
