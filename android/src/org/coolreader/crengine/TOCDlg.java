package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.app.AlertDialog;
import android.database.DataSetObserver;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class TOCDlg extends AlertDialog {
	CoolReader mCoolReader;
	ReaderView mReaderView;
	TOCItem mTOC;
	ListView mListView;
	ArrayList<TOCItem> mItems = new ArrayList<TOCItem>(); 
	private LayoutInflater mInflater;
	
	private void initItems( TOCItem toc, boolean expanded )
	{
		for ( int i=0; i<toc.getChildCount(); i++ ) {
			TOCItem child = toc.getChild(i);
			if ( expanded ) {
				child.setGlobalIndex(mItems.size());
				mItems.add(child);
			} else {
				child.setGlobalIndex(-1); // invisible
			}
			initItems(child, expanded && child.getExpanded());
		}
	}
	private void initItems()
	{
		mItems.clear();
		initItems(mTOC, true);
	}
	
	private void expand( TOCItem item )
	{
		item.setExpanded(true);
		// expand all parents
		for ( TOCItem p = item.getParent(); p!=null; p = p.getParent() )
			p.setExpanded(true);
		initItems();
		refreshList();
	}
	
	private void collapse( TOCItem item )
	{
		item.setExpanded(false);
		initItems();
		refreshList();
	}
	
	private void refreshList()
	{
		mListView.setAdapter(new ListAdapter() {
			public boolean areAllItemsEnabled() {
				return true;
			}

			public boolean isEnabled(int arg0) {
				return true;
			}

			public int getCount() {
				return mItems.size();
			}

			public Object getItem(int position) {
				return mItems.get(position);
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
					view = mInflater.inflate(R.layout.toc_item, null);
				} else {
					view = (View)convertView;
				}
				TextView pageTextView = (TextView)view.findViewById(R.id.toc_page);
				TextView titleTextView = (TextView)view.findViewById(R.id.toc_title);
				TOCItem item = mItems.get(position);
				StringBuilder buf = new StringBuilder(item.getLevel()*2);
				for ( int i=0; i<item.getLevel(); i++ )
					buf.append("  ");
				if ( item.getChildCount()>0 ) {
					if ( item.getExpanded() )
						buf.append("- ");
					else
						buf.append("+ ");
				}
				buf.append(item.getName());
				titleTextView.setText(buf.toString());
				pageTextView.setText(String.valueOf(item.getPage()));
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
		});
	}

	public TOCDlg( CoolReader coolReader, ReaderView readerView, TOCItem toc )
	{
		super(coolReader);
        setCancelable(true);
		this.mCoolReader = coolReader;
		this.mReaderView = readerView;
		this.mTOC = toc;
		this.mListView = new ListView(mCoolReader);
		setTitle("Table of Contents");
		setView(mListView);
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
        mInflater = LayoutInflater.from(getContext());
		super.onCreate(savedInstanceState);
		expand( mTOC );
	}
	
	

}
