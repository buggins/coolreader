package org.coolreader.crengine;

import org.coolreader.R;

import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.widget.LinearLayout;
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
	        popup.setTouchInterceptor(new OnTouchListener() {
				@Override
				public boolean onTouch(View v, MotionEvent event) {
					// process & ignore all touch events
					return true;
				}
			});
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
			L.d("hiding progress indicator");
			popup.dismiss();
			popup = null;
		}
	}
	public boolean isShown() {
		return popup != null && popup.isShowing();
	}
}
