/*
 * CoolReader for Android
 * Copyright (C) 2012 Vadim Lopatin <coolreader.org@gmail.com>
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

import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.PopupWindow;

import org.coolreader.R;

public class ProgressPopup {
	private BaseActivity context;
	private View parent;
	private PopupWindow popup;
	public ProgressPopup(BaseActivity context, View parent) {
		this.context = context;
		this.parent = parent;
	}
	public void show() {
		if (popup == null) {
			L.d("showing progress indicator");
	        LayoutInflater inflater = LayoutInflater.from(context);
	        View content = inflater.inflate(R.layout.network_access_progress, null);
	        content.measure(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
			popup = new PopupWindow(content, content.getMeasuredWidth(), content.getMeasuredHeight());
	        //popup.setContentView(content);
	        popup.setBackgroundDrawable(null);
	        popup.setOutsideTouchable(true);
	        popup.showAtLocation(parent, Gravity.CENTER, 0, 0);
	        popup.setTouchable(true);
	        popup.setTouchInterceptor((v, event) -> {
				// process & ignore all touch events
				return true;
			});
	        popup.setOnDismissListener(() -> popup = null);
		}
	}
	public void hide() {
		if (popup != null) {
			L.d("hiding progress indicator");
			popup.dismiss();
			popup = null;
		}
	}
	public boolean isShown() {
		return popup != null && popup.isShowing();
	}
}
