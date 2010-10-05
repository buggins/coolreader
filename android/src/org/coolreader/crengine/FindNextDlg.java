package org.coolreader.crengine;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnKeyListener;
import android.widget.PopupWindow;

public class FindNextDlg extends PopupWindow {
	CoolReader mCoolReader;
	ReaderView mReaderView;
	View mPanel;
	final String pattern;
	final boolean caseInsensitive;
	public FindNextDlg( CoolReader coolReader, ReaderView readerView, final String pattern, final boolean caseInsensitive )
	{
		super((LayoutInflater.from(coolReader.getApplicationContext()).inflate(R.layout.search_popup, null)), 96, 32, true);
		mPanel = getContentView();
		this.pattern = pattern;
		this.caseInsensitive = caseInsensitive;
		mCoolReader = coolReader;
		mReaderView = readerView;
		setFocusable(true);
		setTouchable(true);
		setOutsideTouchable(false);
		mPanel.findViewById(R.id.search_btn_prev).setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				mReaderView.findNext(pattern, true, caseInsensitive);
			}
		});
		mPanel.findViewById(R.id.search_btn_next).setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				mReaderView.findNext(pattern, true, caseInsensitive);
			}
		});
		mPanel.findViewById(R.id.search_btn_close).setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				mReaderView.clearSelection();
				dismiss();
			}
		});
		mPanel.setOnKeyListener( new OnKeyListener() {

			public boolean onKey(View v, int keyCode, KeyEvent event) {
				if ( keyCode == KeyEvent.KEYCODE_BACK) {
					dismiss();
					return true;
				}
				return false;
			}
			
		});
		// TODO: do properly popup measurement
//		showAtLocation(mReaderView, Gravity.LEFT|Gravity.TOP, 4, 4);
//		setWindowLayoutMode(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
//		mPanel.measure(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
//		int dx = mPanel.getMeasuredWidth();
//		int dy = mPanel.getMeasuredHeight();
//		update(mReaderView, dx, dy);
		
	}
	
}
