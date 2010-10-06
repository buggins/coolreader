package org.coolreader.crengine;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.CheckBox;
import android.widget.EditText;

public class SearchDlg  extends AlertDialog {
	CoolReader mCoolReader;
	ReaderView mReaderView;
	private LayoutInflater mInflater;
	View mDialogView;
	EditText mEditView;
	CheckBox mCaseSensitive;
	CheckBox mReverse;
	
	public SearchDlg( CoolReader coolReader, ReaderView readerView )
	{
		super(coolReader);
        setCancelable(true);
		this.mCoolReader = coolReader;
		this.mReaderView = readerView;
        mInflater = LayoutInflater.from(getContext());
        mDialogView = mInflater.inflate(R.layout.search_dialog, null);
    	mEditView = (EditText)mDialogView.findViewById(R.id.search_text);
    	mCaseSensitive = (CheckBox)mDialogView.findViewById(R.id.search_case_sensitive);
    	mReverse = (CheckBox)mDialogView.findViewById(R.id.search_reverse);
		setTitle(mCoolReader.getResources().getString(R.string.win_title_search));
		setView(mDialogView);
		// setup buttons
        setButton(AlertDialog.BUTTON_POSITIVE, "Ok", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                //cancel();
            	String pattern = mEditView.getText().toString();
            	if ( pattern==null || pattern.length()==0 ) 
            		mCoolReader.showToast("No pattern specified");
            	else
            		mReaderView.findText( mEditView.getText().toString(), mReverse.isChecked(), !mCaseSensitive.isChecked() );
                dialog.cancel();
            }
        });
 
        setButton(AlertDialog.BUTTON_NEGATIVE, "Cancel",
            new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int whichButton) {
                    dialog.cancel();
                }
            });
		
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
	}
	
	

}
