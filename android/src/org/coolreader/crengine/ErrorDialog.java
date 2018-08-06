package org.coolreader.crengine;

import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.TextView;

import org.coolreader.R;

public class ErrorDialog extends BaseDialog {
	public ErrorDialog(BaseActivity activity, final String title, final String message) {
		super(activity, title, false, true);
		// TODO: improve this dialog
		LayoutInflater mInflater = LayoutInflater.from(getContext());
		ViewGroup layout = (ViewGroup)mInflater.inflate(R.layout.error_msg_dlg, null);
		TextView msgTextView = (TextView) layout.findViewById(R.id.msg_textview);
		if (msgTextView != null) {
			msgTextView.setText(message);
		}
		setView(layout);
	}
}
