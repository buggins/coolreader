package org.coolreader.crengine;

import org.coolreader.R;

import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.PopupWindow;
import android.widget.PopupWindow.OnDismissListener;

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
			popup = new PopupWindow(context);
	        LayoutInflater inflater = LayoutInflater.from(context);
	        View content = inflater.inflate(R.layout.network_access_progress, null);
	        popup.setContentView(content);
	        popup.showAtLocation(parent, Gravity.CENTER, 0, 0);
	        popup.setOnDismissListener(new OnDismissListener() {
				@Override
				public void onDismiss() {
					popup = null;
				}
			});
		}
	}
	public void hide() {
		if (popup != null) {
			popup.dismiss();
			popup = null;
		}
	}
	public boolean isShown() {
		return popup != null && popup.isShowing();
	}
}
