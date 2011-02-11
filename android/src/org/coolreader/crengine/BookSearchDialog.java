package org.coolreader.crengine;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.view.LayoutInflater;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.TextView;

public class BookSearchDialog extends BaseDialog {
	
	private final CoolReader mCoolReader;
	private final LayoutInflater mInflater;
	final EditText authorEdit;
	final EditText titleEdit;
	final EditText seriesEdit;
	final EditText filenameEdit;
	
	public BookSearchDialog( CoolReader activity )
	{
		super(activity, R.string.dlg_button_find, R.string.dlg_button_cancel, false);
		mCoolReader = activity;
		setTitle(mCoolReader.getString( R.string.dlg_book_search));
		mInflater = LayoutInflater.from(getContext());
		View view = mInflater.inflate(R.layout.bookmark_edit_dialog, null);
		authorEdit = (EditText)view.findViewById(R.id.search_text_author);
		titleEdit = (EditText)view.findViewById(R.id.search_text_title);
		seriesEdit = (EditText)view.findViewById(R.id.search_text_series);
		filenameEdit = (EditText)view.findViewById(R.id.search_text_filename);
		setView( view );
	}

	@Override
	protected void onPositiveButtonClick() {
		super.onPositiveButtonClick();
	}

	@Override
	protected void onNegativeButtonClick() {
		super.onNegativeButtonClick();
	}
}
