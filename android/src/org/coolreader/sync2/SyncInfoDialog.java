package org.coolreader.sync2;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import org.coolreader.R;
import org.coolreader.crengine.BaseActivity;
import org.coolreader.crengine.BaseDialog;

public class SyncInfoDialog extends BaseDialog {

	private Button m_positiveButton;
	private Button m_negativeButton;
	private View.OnClickListener m_onPositiveClickListener;
	private View.OnClickListener m_onNegativeClickListener;

	public SyncInfoDialog(BaseActivity activity, final String title, final String message) {
		super(activity, title, false, true);
		LayoutInflater inflater = LayoutInflater.from(getContext());
		ViewGroup layout = (ViewGroup)inflater.inflate(R.layout.sync_dir_dialog, null);
		TextView msgTextView = (TextView) layout.findViewById(R.id.msg_textview);
		if (msgTextView != null) {
			msgTextView.setText(message);
		}
		m_positiveButton = layout.findViewById(R.id.base_dlg_btn_positive);
		m_positiveButton.setOnClickListener(v -> {
			if (null != m_onPositiveClickListener)
				m_onPositiveClickListener.onClick(v);
			dismiss();
		});
		m_negativeButton = layout.findViewById(R.id.base_dlg_btn_negative);
		m_negativeButton.setOnClickListener(v -> {
			if (null != m_onNegativeClickListener)
				m_onNegativeClickListener.onClick(v);
			dismiss();
		});
		setView(layout);
	}

	void setPositiveButtonLabel(String label) {
		m_positiveButton.setText(label);
	}

	void setNegativeButtonLabel(String label) {
		m_negativeButton.setText(label);
	}

	void setOnPositiveClickListener(View.OnClickListener listener) {
		m_onPositiveClickListener = listener;
	}

	void setOnNegativeClickListener(View.OnClickListener listener) {
		m_onNegativeClickListener = listener;
	}

}
