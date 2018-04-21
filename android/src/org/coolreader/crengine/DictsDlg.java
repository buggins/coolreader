package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.CoolReader;
import org.coolreader.Dictionaries;
import org.coolreader.R;

import android.content.Context;
import android.database.DataSetObserver;
import android.graphics.Paint;
import android.os.Bundle;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.BaseAdapter;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class DictsDlg extends BaseDialog {
	CoolReader mCoolReader;
	ReaderView mReaderView;
	private LayoutInflater mInflater;
    DictList mList;
    String sSearchText;

	DictsDlg mThis;

	public final static int ITEM_POSITION=0;

	class DictListAdapter extends BaseAdapter {
		public boolean areAllItemsEnabled() {
			return true;
		}

		public boolean isEnabled(int arg0) {
			return true;
		}

		public int getCount() {
			return Dictionaries.getDictListExt(mCoolReader,true).size();
		}

		public Object getItem(int position) {
			if ( position<0 || position>=Dictionaries.getDictListExt(mCoolReader,true).size() )
				return null;
			return Dictionaries.getDictListExt(mCoolReader,true).get(position);
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
			int type = getItemViewType(position);
			int res = R.layout.dict_item;
			view = mInflater.inflate(res, null);
			TextView labelView = (TextView)view.findViewById(R.id.dict_item_shortcut);
			TextView titleTextView = (TextView)view.findViewById(R.id.dict_item_title);
		 	Dictionaries.DictInfo b = (Dictionaries.DictInfo)getItem(position);
			if ( labelView!=null ) {
				labelView.setText(String.valueOf(position+1));
			}
			if ( b!=null ) {
				if ( titleTextView!=null )
					titleTextView.setText(b.name);
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
			return Dictionaries.getDictList().length==0;
		}

		private ArrayList<DataSetObserver> observers = new ArrayList<DataSetObserver>();
		
		public void registerDataSetObserver(DataSetObserver observer) {
			observers.add(observer);
		}

		public void unregisterDataSetObserver(DataSetObserver observer) {
			observers.remove(observer);
		}
	}
	
	class DictList extends BaseListView {

		public DictList( Context context, boolean shortcutMode ) {
			super(context, true);
			setChoiceMode(ListView.CHOICE_MODE_SINGLE);
			setLongClickable(true);
			setAdapter(new DictListAdapter());
			setOnItemLongClickListener(new OnItemLongClickListener() {
				@Override
				public boolean onItemLongClick(AdapterView<?> arg0, View arg1,
						int position, long arg3) {
					openContextMenu(DictList.this);
					return true;
				}
			});
		}

		@Override
		public boolean performItemClick(View view, int position, long id) {
			//Dict b = mBookInfo.findShortcutDict(position+1);
			//if ( b==null ) {
			//	mThis.dismiss();
			//	return true;
			//selectedItem = position;
			mCoolReader.mDictionaries.setAdHocDict(Dictionaries.getDictListExt(mCoolReader,true).get(position));
			mCoolReader.findInDictionary( sSearchText);
			if (!mReaderView.getSettings().getBool(mReaderView.PROP_APP_SELECTION_PERSIST, false))
				mReaderView.clearSelection();
			dismiss();
			return true;
		}
		
		
	}
	
	final static int SHORTCUT_COUNT = 10;
	
	public DictsDlg( CoolReader activity, ReaderView readerView, String search_text )
	{
		super(activity, activity.getResources().getString(R.string.win_title_dicts), false, true);
		mThis = this; // for inner classes
        mInflater = LayoutInflater.from(getContext());
		sSearchText = search_text;
		mCoolReader = activity;
		mReaderView = readerView;
		//setPositiveButtonImage(R.drawable.cr3_button_add, R.string.mi_Dict_add);
		View frame = mInflater.inflate(R.layout.dict_list_dialog, null);
		ViewGroup body = (ViewGroup)frame.findViewById(R.id.dict_list);
		mList = new DictList(activity, false);
		body.addView(mList);
		setView(frame);
		setFlingHandlers(mList, null, null);
	}

	@Override
	protected void onPositiveButtonClick() {
		// add Dict
		DictsDlg.this.dismiss();
	}

	@Override
	protected void onNegativeButtonClick() {
		DictsDlg.this.dismiss();
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.v("cr3", "creating DictsDlg");
		//setTitle(mCoolReader.getResources().getString(R.string.win_title_Dicts));
        setCancelable(true);
		super.onCreate(savedInstanceState);
		registerForContextMenu(mList);
	}

}
