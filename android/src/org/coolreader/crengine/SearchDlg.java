package org.coolreader.crengine;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.CheckBox;
import android.widget.EditText;

public class SearchDlg  extends BaseDialog {
	CoolReader mCoolReader;
	ReaderView mReaderView;
	private LayoutInflater mInflater;
	View mDialogView;
	EditText mEditView;
	CheckBox mCaseSensitive;
	CheckBox mReverse;
	
	@Override
	protected void onPositiveButtonClick()
	{
		// override it
    	String pattern = mEditView.getText().toString();
    	if ( pattern==null || pattern.length()==0 ) 
    		mCoolReader.showToast("No pattern specified");
    	else
    		mReaderView.findText( mEditView.getText().toString(), mReverse.isChecked(), !mCaseSensitive.isChecked() );
        cancel();
	}
	
	@Override
	protected void onNegativeButtonClick()
	{
		// override it
        cancel();
	}

	
	public SearchDlg( CoolReader coolReader, ReaderView readerView )
	{
		super(coolReader, R.string.dlg_button_find, R.string.dlg_button_cancel, false);
        setCancelable(true);
		this.mCoolReader = coolReader;
		this.mReaderView = readerView;
        mInflater = LayoutInflater.from(getContext());
        mDialogView = mInflater.inflate(R.layout.search_dialog, null);
    	mEditView = (EditText)mDialogView.findViewById(R.id.search_text);
    	mCaseSensitive = (CheckBox)mDialogView.findViewById(R.id.search_case_sensitive);
    	mReverse = (CheckBox)mDialogView.findViewById(R.id.search_reverse);
		setTitle(mCoolReader.getResources().getString(R.string.win_title_search));
		// setup buttons
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setView(mDialogView);
	}
}
