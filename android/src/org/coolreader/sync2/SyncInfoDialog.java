/*
 * CoolReader for Android
 * Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>
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
	private OnCancelListener m_onCancelListener;

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

	public void setOnCancelListener(OnCancelListener listener) {
		super.setOnCancelListener(listener);
		m_onCancelListener = listener;
	}

	protected void onPositiveButtonClick()
	{
		if (null != m_onCancelListener)
			m_onCancelListener.onCancel(this);
		dismiss();
	}

	protected void onNegativeButtonClick()
	{
		if (null != m_onCancelListener)
			m_onCancelListener.onCancel(this);
		dismiss();
	}

}
