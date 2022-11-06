/*
 * CoolReader for Android
 * Copyright (C) 2010-2012 Vadim Lopatin <coolreader.org@gmail.com>
 * Copyright (C) 2018 Yuri Plotnikov <plotnikovya@gmail.com>
 * Copyright (C) 2019 Aleksey Chernov <valexlin@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.coolreader.crengine;

import org.coolreader.R;

import android.content.Context;
import android.database.DataSetObserver;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;

import java.util.ArrayList;

public class SearchDlg extends BaseDialog {
	BaseActivity mCoolReader;
	ReaderView mReaderView;
	private LayoutInflater mInflater;
	View mDialogView;
	EditText mEditView;
	CheckBox mCaseSensitive;
	CheckBox mReverse;
	BookInfo mBookInfo;
	ArrayList<String> mSearches;
	private SearchList mList;


	@Override
	protected void onPositiveButtonClick()
	{
		// override it
    	String pattern = mEditView.getText().toString();
    	if ( pattern==null || pattern.length()==0 ) 
    		mCoolReader.showToast("No pattern specified");
    	else if ( mBookInfo == null )
    		Log.e("search", "No opened book!");
    	else {
		    activity.getDB().saveSearchHistory(mBookInfo,
				    mEditView.getText().toString());
		    mReaderView.findText(mEditView.getText().toString(), mReverse.isChecked(), !mCaseSensitive.isChecked());
	    }
        cancel();
	}
	
	@Override
	protected void onNegativeButtonClick()
	{
		// override it
        cancel();
	}

	public final static int ITEM_POSITION=0;

	class SearchListAdapter extends BaseAdapter {
		public boolean areAllItemsEnabled() {
			return true;
		}

		public boolean isEnabled(int arg0) {
			return true;
		}

		public int getCount() {
			return mSearches.size();
		}

		public Object getItem(int position) {
			if ( position<0 || position>=mSearches.size() )
				return null;
			return mSearches.get(position);
		}

		public long getItemId(int position) {
			return position;
		}

		public int getItemViewType(int position) {
			return ITEM_POSITION;
		}

		public int getViewTypeCount() {
			return 4;
		}

		public View getView(int position, View convertView, ViewGroup parent) {
			View view;
			int res = R.layout.dict_item;
			view = mInflater.inflate(res, null);
			TextView labelView = view.findViewById(R.id.dict_item_shortcut);
			TextView titleTextView = view.findViewById(R.id.dict_item_title);
			String s = (String)getItem(position);
			if ( labelView!=null ) {
				labelView.setText(String.valueOf(position+1));
			}
			if ( s!=null ) {
				if ( titleTextView!=null )
					titleTextView.setText(s);
			} else {
				if ( titleTextView!=null )
					titleTextView.setText("");
			}
			return view;
		}

		public boolean hasStableIds() {
			return true;
		}

		public boolean isEmpty() {
			return mSearches.size()==0;
		}

		private ArrayList<DataSetObserver> observers = new ArrayList<>();

		public void registerDataSetObserver(DataSetObserver observer) {
			observers.add(observer);
		}

		public void unregisterDataSetObserver(DataSetObserver observer) {
			observers.remove(observer);
		}
	}

	class SearchList extends BaseListView {
		public SearchList(Context context) {
			super(context, true);
			setChoiceMode(ListView.CHOICE_MODE_SINGLE);
			setLongClickable(true);
			setAdapter(new SearchDlg.SearchListAdapter());
			setOnItemLongClickListener((arg0, arg1, position, arg3) -> {
				openContextMenu(SearchList.this);
				return true;
			});
		}

		@Override
		public boolean performItemClick(View view, int position, long id) {
			mEditView.setText(mSearches.get(position));
			return true;
		}
	}
	
	public SearchDlg(BaseActivity coolReader, ReaderView readerView, String initialText)
	{
		super(coolReader, coolReader.getResources().getString(R.string.win_title_search), true, false);
        setCancelable(true);
		this.mCoolReader = coolReader;
		this.mReaderView = readerView;
		this.mBookInfo = mReaderView.getBookInfo();
		setPositiveButtonImage(R.drawable.cr3_button_find, R.string.action_search);
        mInflater = LayoutInflater.from(getContext());
        mDialogView = mInflater.inflate(R.layout.search_dialog, null);
    	mEditView = mDialogView.findViewById(R.id.search_text);
    	if (initialText != null)
    		mEditView.setText(initialText);
    	mCaseSensitive = mDialogView.findViewById(R.id.search_case_sensitive);
    	mReverse = mDialogView.findViewById(R.id.search_reverse);
		activity.getDB().loadSearchHistory(this.mBookInfo, searches -> {
			mSearches = searches;
			ViewGroup body = mDialogView.findViewById(R.id.history_list);
			mList = new SearchList(activity);
			body.addView(mList);
		});
		//setView(mDialogView);
		//setFlingHandlers(mList, null, null);
		// setup buttons
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setView(mDialogView);
	}
}
