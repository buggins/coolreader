package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.app.AlertDialog;
import android.content.Context;
import android.database.DataSetObserver;
import android.os.Bundle;
import android.util.Log;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class BookmarksDlg  extends AlertDialog {
	CoolReader mCoolReader;
	ReaderView mReaderView;
	private LayoutInflater mInflater;
	BookInfo mBookInfo;
	BookmarkList mList;
	
	final int SHORTCUT_COUNT = 10;
	class BookmarkList extends ListView {
		private ListAdapter mAdapter;
		public BookmarkList( Context context ) {
			super(context);
			mAdapter = new ListAdapter() {
				public boolean areAllItemsEnabled() {
					return true;
				}

				public boolean isEnabled(int arg0) {
					return true;
				}

				public int getCount() {
					return SHORTCUT_COUNT;
				}

				public Object getItem(int position) {
					return mBookInfo.findShortcutBookmark(position+1);
				}

				public long getItemId(int position) {
					return position;
				}

				public int getItemViewType(int position) {
					return 0;
				}

				
				public View getView(int position, View convertView, ViewGroup parent) {
					View view;
					if ( convertView==null ) {
						//view = new TextView(getContext());
						view = mInflater.inflate(R.layout.bookmark_shortcut_item, null);
					} else {
						view = (View)convertView;
					}
					TextView labelView = (TextView)view.findViewById(R.id.shortcut_bookmark_item_shortcut);
					TextView percentView = (TextView)view.findViewById(R.id.shortcut_bookmark_item_percent);
					TextView posTextView = (TextView)view.findViewById(R.id.shortcut_bookmark_item_pos_text);
					TextView titleTextView = (TextView)view.findViewById(R.id.shortcut_bookmark_item_title);
					Bookmark b = mBookInfo.findShortcutBookmark(position+1);
					labelView.setText(String.valueOf(position+1));
					if ( b!=null ) {
						percentView.setText(String.valueOf(b.getPercent()) + "%");
						posTextView.setText(b.getPosText());
						titleTextView.setText(b.getTitleText());
					} else {
						percentView.setText("");
						posTextView.setText("[no bookmark]");
						titleTextView.setText("");
					}
					return view;
				}

				public int getViewTypeCount() {
					return 1;
				}

				public boolean hasStableIds() {
					return true;
				}

				public boolean isEmpty() {
					return false;
				}

				private ArrayList<DataSetObserver> observers = new ArrayList<DataSetObserver>();
				
				public void registerDataSetObserver(DataSetObserver observer) {
					observers.add(observer);
				}

				public void unregisterDataSetObserver(DataSetObserver observer) {
					observers.remove(observer);
				}
			};
			setAdapter(mAdapter);
		}
		@Override
		public boolean performItemClick(View view, int position, long id) {
			Bookmark b = mBookInfo.findShortcutBookmark(position+1);
			if ( b==null ) {
				mReaderView.addBookmark(position+1);
				dismiss();
				return true;
			}
			showContextMenu();
			return true;
		}
		
		
	}
	
	public BookmarksDlg( CoolReader activity, ReaderView readerView )
	{
		super(activity);
		mCoolReader = activity;
		mReaderView = readerView;
		mBookInfo = mReaderView.getBookInfo();
		mList = new BookmarkList(activity); 
		setView(mList);
	}
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.v("cr3", "creating OptionsDialog");
		setTitle("Bookmarks");
        setCancelable(true);
        mInflater = LayoutInflater.from(getContext());
		
		super.onCreate(savedInstanceState);
	}
	@Override
	public boolean onContextItemSelected(MenuItem item) {
		
		int shortcut = mList.getSelectedItemPosition();
		if ( shortcut>=0 && shortcut<SHORTCUT_COUNT )
		switch (item.getItemId()) {
		case R.id.bookmark_shortcut_add:
			mReaderView.addBookmark(shortcut+1);
			dismiss();
			return true;
		case R.id.bookmark_shortcut_goto:
			mReaderView.goToBookmark(shortcut+1);
			dismiss();
			return true;
		}
		return super.onContextItemSelected(item);
	}
	@Override
	public void onCreateContextMenu(ContextMenu menu, View v,
			ContextMenuInfo menuInfo) {
	    MenuInflater inflater = mCoolReader.getMenuInflater();
	    inflater.inflate(R.menu.cr3_bookmark_shortcut_context_menu, menu);
		//super.onCreateContextMenu(menu, v, menuInfo);
	}
	

}
