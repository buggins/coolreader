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

	private final int ITEM_POSITION=0;
	private final int ITEM_COMMENT=1;
	private final int ITEM_CORRECTION=2;
	private final int ITEM_SHORTCUT=3;
	
	class BookmarkListAdapter implements ListAdapter {
		public boolean areAllItemsEnabled() {
			return true;
		}

		public boolean isEnabled(int arg0) {
			return true;
		}

		public int getCount() {
			return mBookInfo.getBookmarkCount();
		}

		public Object getItem(int position) {
			if ( position<0 || position>=mBookInfo.getBookmarkCount() )
				return null;
			return mBookInfo.getBookmark(position);
		}

		public long getItemId(int position) {
			return position;
		}

		
		
		public int getItemViewType(int position) {
			Bookmark bm = (Bookmark)getItem(position);
			if ( bm==null )
				return ITEM_POSITION;
			switch ( bm.getType() ) {
			case Bookmark.TYPE_COMMENT:
				return ITEM_COMMENT;
			case Bookmark.TYPE_CORRECTION:
				return ITEM_CORRECTION;
			default:
				if ( bm.getShortcut()>0 )
					return ITEM_SHORTCUT;
				return ITEM_POSITION;
			}
		}

		public int getViewTypeCount() {
			return 4;
		}

		
		public View getView(int position, View convertView, ViewGroup parent) {
			View view;
			if ( convertView==null ) {
				//view = new TextView(getContext());
				int type = getItemViewType(position);
				int res = R.layout.bookmark_position_item;
				switch ( type ) {
				case ITEM_COMMENT:
					res = R.layout.bookmark_comment_item;
					break;
				case ITEM_CORRECTION:
					res = R.layout.bookmark_correction_item;
					break;
				case ITEM_SHORTCUT:
					res = R.layout.bookmark_shortcut_item;
					break;
				}
				view = mInflater.inflate(res, null);
			} else {
				view = (View)convertView;
			}
			TextView labelView = (TextView)view.findViewById(R.id.bookmark_item_shortcut);
			TextView posTextView = (TextView)view.findViewById(R.id.bookmark_item_pos_text);
			TextView titleTextView = (TextView)view.findViewById(R.id.bookmark_item_title);
			TextView commentTextView = (TextView)view.findViewById(R.id.bookmark_item_comment_text);
			Bookmark b = (Bookmark)getItem(position);
			if ( labelView!=null ) {
				if ( b!=null && b.getShortcut()>0 )
					labelView.setText(String.valueOf(b.getShortcut()));
				else
					labelView.setText(String.valueOf(position+1));
			}
			if ( b!=null ) {
				String percentString = FileBrowser.formatPercent(b.getPercent());
				String s1 = b.getTitleText();
				String s2 = b.getPosText();
				String s3 = b.getCommentText();
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
				if ( titleTextView!=null )
					titleTextView.setText(s1);
				if ( posTextView!=null )
					posTextView.setText(s2);
				if ( commentTextView!=null )
					commentTextView.setText(s3);
			} else {
				if ( commentTextView!=null )
					commentTextView.setText("");
				if ( titleTextView!=null )
					titleTextView.setText("");
				if ( posTextView!=null )
					posTextView.setText("");
			}
			return view;
		}

		public boolean hasStableIds() {
			return true;
		}

		public boolean isEmpty() {
			return mBookInfo.getBookmarkCount()==0;
		}

		private ArrayList<DataSetObserver> observers = new ArrayList<DataSetObserver>();
		
		public void registerDataSetObserver(DataSetObserver observer) {
			observers.add(observer);
		}

		public void unregisterDataSetObserver(DataSetObserver observer) {
			observers.remove(observer);
		}
	}
	
	class ShortcutBookmarkListAdapter extends BookmarkListAdapter {
		public int getCount() {
			return SHORTCUT_COUNT;
		}

		public Object getItem(int position) {
			return mBookInfo.findShortcutBookmark(position+1);
		}

		public int getItemViewType(int position) {
			return ITEM_SHORTCUT;
		}
	}
	
	class BookmarkList extends ListView {
		private ListAdapter mAdapter;
		private boolean mShortcutMode = false;
		
		public boolean isShortcutMode() {
			return mShortcutMode;
		}
		public void setShortcutMode( boolean shortcutMode ) {
			updateAdapter( shortcutMode ? new ShortcutBookmarkListAdapter() : new BookmarkListAdapter() );
		}
		public void updateAdapter( BookmarkListAdapter adapter ) {
			mAdapter = adapter;
			setAdapter(mAdapter);
		}
		public BookmarkList( Context context, boolean shortcutMode ) {
			super(context);
			setChoiceMode(ListView.CHOICE_MODE_SINGLE);
			setShortcutMode(shortcutMode);
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
	
	final int SHORTCUT_COUNT = 10;
	
	public BookmarksDlg( CoolReader activity, ReaderView readerView )
	{
		super(activity, 0, 0, false);
		mThis = this; // for inner classes
		mCoolReader = activity;
		mReaderView = readerView;
		mBookInfo = mReaderView.getBookInfo();
		mList = new BookmarkList(activity, false);
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
