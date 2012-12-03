package org.coolreader.crengine;

import java.util.ArrayList;

import android.database.DataSetObserver;
import android.widget.BaseAdapter;

public abstract class BaseListAdapter extends BaseAdapter {
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
			observer.onChanged();
			//observer.onInvalidated();
		}
	}

	
	// default behavior implementation: single item view type, all items enabled, ids == positions
	
	@Override
	public boolean isEmpty() {
		return getCount() > 0;
	}

	@Override
	public boolean areAllItemsEnabled() {
		return true;
	}

	@Override
	public boolean isEnabled(int position) {
		return true;
	}

	@Override
	public long getItemId(int position) {
		return position;
	}

	@Override
	public int getItemViewType(int position) {
		return 0;
	}

	@Override
	public int getViewTypeCount() {
		return 1;
	}

	@Override
	public boolean hasStableIds() {
		return true;
	}

}
