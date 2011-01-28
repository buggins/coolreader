package org.coolreader.crengine;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.view.LayoutInflater;
import android.view.View;
import android.widget.TabHost.TabContentFactory;
import android.widget.TableLayout;
import android.widget.TableLayout.LayoutParams;
import android.widget.TableRow;
import android.widget.TextView;

public class BookInfoDialog extends BaseDialog {
	private final CoolReader mCoolReader;
	private final LayoutInflater mIflater; 
	private Collection<String> mItems;
	private Map<String, Integer> mLabelMap;
	private void fillMap() {
		mLabelMap = new HashMap<String, Integer>();
		mLabelMap.put("section.file", R.string.book_info_section_file_properties);
		mLabelMap.put("section.book", R.string.book_info_section_book_properties);
		mLabelMap.put("section.position", R.string.book_info_section_current_position);
	}
	
	private void addItem(TableLayout table, String item) {
		int p = item.indexOf("=");
		if ( p<0 )
			return;
		String name = item.substring(0, p).trim();
		String value = item.substring(p+1).trim();
		if ( name.length()==0 || value.length()==0 )
			return;
		if ( "section".equals(name) ) {
			name = "";
			Integer id = mLabelMap.get(value);
			if ( id==null )
				return;
			String section = getContext().getString(id);
			if ( section!=null )
				value = section;
		} else {
			Integer id = mLabelMap.get(name);
			if ( id==null )
				return;
			String title = getContext().getString(id);
			if ( title!=null )
				name = title;
		}
		TableRow tableRow = new TableRow(mCoolReader);
		LayoutParams params = new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		tableRow.setLayoutParams(params);
		TextView nameView = new TextView(mCoolReader);
	}
	
	public BookInfoDialog( CoolReader activity, Collection<String> items)
	{
		super(activity, 0, 0, false);
		mCoolReader = activity;
		setTitle(mCoolReader.getString(R.string.dlg_book_info));
		fillMap();
		mIflater = LayoutInflater.from(getContext());
		View view = mIflater.inflate(R.layout.book_info_dialog, null);
		TableLayout table = (TableLayout)view.findViewById(R.id.table);
		setView( view );
	}

}
