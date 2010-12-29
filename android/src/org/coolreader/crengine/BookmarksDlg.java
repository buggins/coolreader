package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.CoolReader;
import org.coolreader.R;

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
import android.view.MenuItem.OnMenuItemClickListener;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.AdapterView.AdapterContextMenuInfo;

public class BookmarksDlg  extends BaseDialog {
	CoolReader mCoolReader;
	ReaderView mReaderView;
	private LayoutInflater mInflater;
	BookInfo mBookInfo;
	BookmarkList mList;
	BookmarksDlg mThis;
	
	final int SHORTCUT_COUNT = 10;
	class BookmarkList extends ListView {
		private ListAdapter mAdapter;
		public BookmarkList( Context context ) {
			super(context);
			//final BookmarkList mThisList = this;
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
					//TextView percentView = (TextView)view.findViewById(R.id.shortcut_bookmark_item_percent);
					TextView posTextView = (TextView)view.findViewById(R.id.shortcut_bookmark_item_pos_text);
					TextView titleTextView = (TextView)view.findViewById(R.id.shortcut_bookmark_item_title);
					Bookmark b = mBookInfo!=null ? mBookInfo.findShortcutBookmark(position+1) : null;
					labelView.setText(String.valueOf(position+1));
					if ( b!=null ) {
						String percentString = FileBrowser.formatPercent(b.getPercent());
						String s1 = b.getTitleText();
						String s2 = b.getPosText();
						if ( s1!=null && s2!=null ) {
							s1 = percentString + "   " + s1;
						} else if ( s1!=null ) {
							s2 = s1;
							s1 = percentString;  
						} else if ( s2!=null ) {
							s1 = percentString;
						} else {
							s1 = s2 = "";
						}
						//percentView.setText(FileBrowser.formatPercent(b.getPercent()));
						titleTextView.setText(s1);
						posTextView.setText(s2);
					} else {
						//percentView.setText("");
						titleTextView.setText("");
						posTextView.setText("");
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
			setChoiceMode(ListView.CHOICE_MODE_SINGLE);
			setAdapter(mAdapter);
		}
		@Override
		public boolean performItemClick(View view, int position, long id) {
			Bookmark b = mBookInfo.findShortcutBookmark(position+1);
			if ( b==null ) {
				mReaderView.addBookmark(position+1);
				mThis.dismiss();
				return true;
			}
			selectedItem = position;
			openContextMenu(this);
//			showContextMenu();
			return true;
		}
		
	}
	
	public BookmarksDlg( CoolReader activity, ReaderView readerView )
	{
		super(activity, 0, 0, false);
		mThis = this; // for inner classes
		mCoolReader = activity;
		mReaderView = readerView;
		mBookInfo = mReaderView.getBookInfo();
		mList = new BookmarkList(activity);
		setView(mList);
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.v("cr3", "creating BookmarksDlg");
		setTitle(mCoolReader.getResources().getString(R.string.win_title_bookmarks));
        setCancelable(true);
        mInflater = LayoutInflater.from(getContext());
		super.onCreate(savedInstanceState);
		registerForContextMenu(mList);
		//mList.
	}
	@Override
	public boolean onContextItemSelected(MenuItem item) {
		
		int shortcut = selectedItem; //mList.getSelectedItemPosition();
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
	
	private int selectedItem;
	@Override
	public void onCreateContextMenu(ContextMenu menu, View v,
			ContextMenuInfo menuInfo) {
	    MenuInflater inflater = mCoolReader.getMenuInflater();
	    inflater.inflate(R.menu.cr3_bookmark_shortcut_context_menu, menu);
	    AdapterContextMenuInfo mi = (AdapterContextMenuInfo)menuInfo;
	    if ( mi!=null )
	    	selectedItem = mi.position;
	    menu.setHeaderTitle(getContext().getString(R.string.context_menu_title_bookmark));
	    for ( int i=0; i<menu.size(); i++ ) {
	    	menu.getItem(i).setOnMenuItemClickListener(new OnMenuItemClickListener() {
				public boolean onMenuItemClick(MenuItem item) {
					onContextItemSelected(item);
					return true;
				}
			});
	    }
	}
	

}
