package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.CoolReader;

import android.app.AlertDialog;
import android.os.Bundle;

public class TOCDlg extends AlertDialog {
	CoolReader mCoolReader;
	ReaderView mReaderView;
	TOCItem mTOC;
	ArrayList<TOCItem> mItems = new ArrayList<TOCItem>(); 
	
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
	
	public TOCDlg( CoolReader coolReader, ReaderView readerView, TOCItem toc )
	{
		super(coolReader);
		this.mCoolReader = coolReader;
		this.mReaderView = readerView;
		this.mTOC = toc;
	}
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
	}
	
	

}
