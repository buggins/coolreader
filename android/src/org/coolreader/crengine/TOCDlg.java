package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.R;

import android.database.DataSetObserver;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

public class TOCDlg extends BaseDialog {
	//CoolReader mCoolReader;
	ReaderView mReaderView;
	TOCItem mTOC;
	ListView mListView;
	int mCurrentPage;
	TOCItem mCurrentPageItem;
	ArrayList<TOCItem> mItems = new ArrayList<TOCItem>(); 
	private LayoutInflater mInflater;
	
	private void initItems( TOCItem toc, boolean expanded )
	{
		for ( int i=0; i<toc.getChildCount(); i++ ) {
			TOCItem child = toc.getChild(i);
			if ( child.getPage()<=mCurrentPage )
				mCurrentPageItem = child;
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
		mCurrentPageItem = null;
		mItems.clear();
		initItems(mTOC, true);
	}
	
	private void expand( TOCItem item )
	{
		if ( item==null )
			return;
		item.setExpanded(true);
		// expand all parents
		for ( TOCItem p = item.getParent(); p!=null; p = p.getParent() )
			p.setExpanded(true);
		initItems();
		refreshList();
		if ( mItems.size()>0 ) {
			if ( item.getGlobalIndex()>=0 ) {
				mListView.setSelection(item.getGlobalIndex());
				mListView.setSelectionFromTop(item.getGlobalIndex(), mListView.getHeight()/2);
			} else
				mListView.setSelection(0);
		}
	}
	
	private void collapse( TOCItem item )
	{
		item.setExpanded(false);
		initItems();
		refreshList();
	}
	
	private void refreshList()
	{
		mListView.setAdapter(new BaseAdapter() {
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
				TOCItem item = mItems.get(position);
				boolean isCurrentItem = item==mCurrentPageItem;
				return isCurrentItem ? 0 : 1;
			}

			
			public View getView(int position, View convertView, ViewGroup parent) {
				View view;
				TOCItem item = mItems.get(position);
				boolean isCurrentItem = item==mCurrentPageItem;
				if ( convertView==null ) {
					//view = new TextView(getContext());
					view = mInflater.inflate(isCurrentItem ? R.layout.toc_item_current : R.layout.toc_item, null);
				} else {
					view = (View)convertView;
				}
				TextView pageTextView = (TextView)view.findViewById(R.id.toc_page);
				TextView titleTextView = (TextView)view.findViewById(R.id.toc_title);
				TextView marginTextView = (TextView)view.findViewById(R.id.toc_level_margin);
				ImageView expandImageView = (ImageView)view.findViewById(R.id.toc_expand_icon);
				StringBuilder buf = new StringBuilder(item.getLevel()*2);
				for ( int i=1; i<item.getLevel(); i++ )
					buf.append("  ");
				if ( item.getChildCount()>0 ) {
					if ( item.getExpanded() ) {
						expandImageView.setImageResource(R.drawable.cr3_toc_item_expanded);
					} else {
						expandImageView.setImageResource(R.drawable.cr3_toc_item_collapsed);
					}
				} else {
					expandImageView.setImageResource(R.drawable.cr3_toc_item_normal);
				}
				marginTextView.setText(buf.toString());
				titleTextView.setText(item.getName());
				pageTextView.setText(String.valueOf(item.getPage()+1));
				return view;
			}

			public int getViewTypeCount() {
				return 2;
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

	public TOCDlg(BaseActivity coolReader, ReaderView readerView, TOCItem toc, int currentPage )
	{
		super(coolReader, coolReader.getResources().getString(R.string.win_title_toc), false, false);
        setCancelable(true);
//		this.mCoolReader = coolReader;
		this.mReaderView = readerView;
		this.mTOC = toc;
		this.mCurrentPage = currentPage;
		this.mListView = new BaseListView(getContext(), true);
		mListView.setOnItemClickListener(new OnItemClickListener() {

			@Override
			public void onItemClick(AdapterView<?> listview, View view,
					int position, long id) {
				TOCItem item = mItems.get(position);
				if ( item.getChildCount()==0 || item.getExpanded() ) {
					mReaderView.goToPage(item.getPage()+1);
					dismiss();
				} else {
					expand(item);
				}
			}
		});
		mListView.setOnItemLongClickListener(new OnItemLongClickListener() {

			@Override
			public boolean onItemLongClick(AdapterView<?> listview, View view,
					int position, long id) {
				TOCItem item = mItems.get(position);
				if ( item.getChildCount()==0 ) {
					mReaderView.goToPage(item.getPage()+1);
					dismiss();
				} else {
					if ( item.getExpanded() )
						collapse(item);
					else
						expand(item);
				}
				return true;
			}
		});
		mListView.setLongClickable(true);
		mListView.setClickable(true);
		mListView.setFocusable(true);
		mListView.setFocusableInTouchMode(true);
		mListView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
		setView(mListView);
		setFlingHandlers(mListView, new Runnable() {
			@Override
			public void run() {
				// cancel
				TOCDlg.this.dismiss();
			}
		}, new Runnable() {
			@Override
			public void run() {
				// 
				TOCDlg.this.dismiss();
			}
		});
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
        mInflater = LayoutInflater.from(getContext());
		super.onCreate(savedInstanceState);
		expand( mTOC );
		expand( mCurrentPageItem );
	}
	
	

}
