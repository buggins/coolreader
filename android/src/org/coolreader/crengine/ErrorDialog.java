/*
 * CoolReader for Android
 * Copyright (C) 2018 Aleksey Chernov <valexlin@gmail.com>
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
