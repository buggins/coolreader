/*
 * CoolReader for Android
 * Copyright (C) 2011,2012 Vadim Lopatin <coolreader.org@gmail.com>
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

import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;

import org.coolreader.CoolReader;
import org.coolreader.R;

public class BookSearchDialog extends BaseDialog {
	
	private final CoolReader mCoolReader;
	private final LayoutInflater mInflater;
	final EditText authorEdit;
	final EditText titleEdit;
	final EditText seriesEdit;
	final EditText filenameEdit;
	final TextView statusText;
	final SearchCallback callback;
	
	private int searchTaskId = 0;
	private boolean searchActive = false;
	private boolean closing = false;
	
	public BookSearchDialog(CoolReader activity, SearchCallback callback)
	{
		super(activity, activity.getString( R.string.dlg_book_search), true, false);
		mCoolReader = activity;
		this.callback = callback;
		setTitle(mCoolReader.getString( R.string.dlg_book_search));
		mInflater = LayoutInflater.from(getContext());
		View view = mInflater.inflate(R.layout.book_search_dialog, null);
		authorEdit = view.findViewById(R.id.search_text_author);
		titleEdit = view.findViewById(R.id.search_text_title);
		seriesEdit = view.findViewById(R.id.search_text_series);
		filenameEdit = view.findViewById(R.id.search_text_filename);
		statusText = view.findViewById(R.id.search_status);
		TextWatcher watcher = new TextWatcher() {

			@Override
			public void afterTextChanged(Editable s) {
			}

			@Override
			public void beforeTextChanged(CharSequence s, int start, int count,
					int after) {
			}

			@Override
			public void onTextChanged(CharSequence s, int start, int before,
					int count) {
				postSearchTask();
			}
			
		}; 
		authorEdit.addTextChangedListener(watcher);
		seriesEdit.addTextChangedListener(watcher);
		titleEdit.addTextChangedListener(watcher);
		filenameEdit.addTextChangedListener(watcher);
		setView( view );
	}

	private void postSearchTask() {
		if ( closing )
			return;
		final int mySearchTaskId = ++searchTaskId;
		BackgroundThread.instance().postGUI(() -> {
			if ( searchTaskId == mySearchTaskId ) {
				if ( searchActive )
					return;
				searchActive = true;
				find(results -> {
					searchActive = false;
					statusText.setText(mCoolReader.getString(R.string.dlg_book_search_found) + " " + results.length);
					if ( searchTaskId != mySearchTaskId ) {
						postSearchTask();
					}
				});
			}
		}, 3000);
	}
	
	public interface SearchCallback {
		public void done( FileInfo[] results );
	}

//	private static String addWildcard( String s, boolean before, boolean after ) {
//		if ( s==null || s.length()==0 )
//			return s;
//		if ( before )
//			s = "%" + s;
//		if ( after )
//			s = s + "%";
//		return s;
//	}
	
	private final static int MAX_RESULTS = 50; 
	protected void find( final SearchCallback cb ) {
		final String author = authorEdit.getText().toString().trim();
		final String series = seriesEdit.getText().toString().trim();
		final String title = titleEdit.getText().toString().trim();
		final String filename = filenameEdit.getText().toString().trim();
		if (mCoolReader == null || mCoolReader.getDB() == null)
			return;
		mCoolReader.getDB().findByPatterns(MAX_RESULTS, author, title, series, filename, fileList -> cb.done(fileList.toArray(new FileInfo[fileList.size()])));
	}
	
	@Override
	protected void onPositiveButtonClick() {
		searchTaskId++;
		closing = true;
		super.onPositiveButtonClick();
		find( callback );
	}

	@Override
	protected void onNegativeButtonClick() {
		searchTaskId++;
		closing = true;
		super.onNegativeButtonClick();
		callback.done(null);
	}
}
