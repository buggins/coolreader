package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.R;

import android.database.DataSetObserver;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.ListAdapter;
import android.widget.TextView;

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
