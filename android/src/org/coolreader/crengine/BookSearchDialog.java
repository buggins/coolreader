package org.coolreader.crengine;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;

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
	
	public BookSearchDialog( CoolReader activity, SearchCallback callback )
	{
		super(activity, activity.getString( R.string.dlg_book_search), true, false);
		mCoolReader = activity;
		this.callback = callback;
		setTitle(mCoolReader.getString( R.string.dlg_book_search));
		mInflater = LayoutInflater.from(getContext());
		View view = mInflater.inflate(R.layout.book_search_dialog, null);
		authorEdit = (EditText)view.findViewById(R.id.search_text_author);
		titleEdit = (EditText)view.findViewById(R.id.search_text_title);
		seriesEdit = (EditText)view.findViewById(R.id.search_text_series);
		filenameEdit = (EditText)view.findViewById(R.id.search_text_filename);
		statusText = (TextView)view.findViewById(R.id.search_status);
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
		BackgroundThread.instance().postGUI(new Runnable() {
			@Override
			public void run() {
				if ( searchTaskId == mySearchTaskId ) {
					if ( searchActive )
						return;
					searchActive = true;
					find( new SearchCallback() {
						@Override
						public void done(FileInfo[] results) {
							searchActive = false;
							statusText.setText(mCoolReader.getString(R.string.dlg_book_search_found) + " " + results.length);
							if ( searchTaskId != mySearchTaskId ) {
								postSearchTask();
							}
						}
					});
				}
			}
		}, 3000);
	}
	
	public interface SearchCallback {
		public void done( FileInfo[] results );
	}

	private static String addWildcard( String s, boolean before, boolean after ) {
		if ( s==null || s.length()==0 )
			return s;
		if ( before )
			s = "%" + s;
		if ( after )
			s = s + "%";
		return s;
	}
	
	private final static int MAX_RESULTS = 50; 
	protected void find( final SearchCallback cb ) {
		final String author = addWildcard(authorEdit.getText().toString().trim(), false, true);
		final String series = addWildcard(seriesEdit.getText().toString().trim(), false, true);
		final String title = addWildcard(titleEdit.getText().toString().trim(), true, true);
		final String filename = addWildcard(filenameEdit.getText().toString().trim(), true, true);
		BackgroundThread.instance().executeBackground( new Runnable() {
			@Override
			public void run() {
				if (mCoolReader == null || mCoolReader.getDB() == null)
					return;
				final FileInfo[] results = mCoolReader.getDB().findByPatterns(MAX_RESULTS, author, title, series, filename);
				BackgroundThread.instance().executeGUI( new Runnable() {
					@Override
					public void run() {
						cb.done(results);
					}
				});
			}
		});
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
