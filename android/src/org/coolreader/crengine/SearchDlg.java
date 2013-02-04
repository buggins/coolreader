package org.coolreader.crengine;

import org.coolreader.R;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.CheckBox;
import android.widget.EditText;

public class SearchDlg  extends BaseDialog {
	BaseActivity mCoolReader;
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

	
	public SearchDlg(BaseActivity coolReader, ReaderView readerView, String initialText)
	{
		super(coolReader, coolReader.getResources().getString(R.string.win_title_search), true, false);
        setCancelable(true);
		this.mCoolReader = coolReader;
		this.mReaderView = readerView;
		setPositiveButtonImage(R.drawable.cr3_button_find, R.string.action_search);
        mInflater = LayoutInflater.from(getContext());
        mDialogView = mInflater.inflate(R.layout.search_dialog, null);
    	mEditView = (EditText)mDialogView.findViewById(R.id.search_text);
    	if (initialText != null)
    		mEditView.setText(initialText);
    	mCaseSensitive = (CheckBox)mDialogView.findViewById(R.id.search_case_sensitive);
    	mReverse = (CheckBox)mDialogView.findViewById(R.id.search_reverse);
		// setup buttons
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setView(mDialogView);
	}
}
