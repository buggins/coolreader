package org.coolreader.crengine;

import android.app.Dialog;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import org.coolreader.R;

public class NoticeDialog extends Dialog {

	public NoticeDialog(BaseActivity activity, final Runnable onOkButton, final Runnable onCancelButton) {
		super(activity, activity.getCurrentTheme().getThemeId());
		setOwnerActivity(activity);
		LayoutInflater mInflater = LayoutInflater.from(getContext());
		ViewGroup layout = (ViewGroup) mInflater.inflate(R.layout.notice_dialog, null);
		setTitle(R.string.app_name);
		Button button1 = (Button) layout.findViewById(R.id.notice_dlg_btn_positive);
		button1.setOnClickListener(v -> {
			if (onOkButton != null)
				onOkButton.run();
			dismiss();
		});
		Button button2 = (Button) layout.findViewById(R.id.notice_dlg_btn_negative);
		if (onCancelButton != null)
			button2.setOnClickListener(v -> {
				onCancelButton.run();
				dismiss();
			});
		else
			button2.setVisibility(View.GONE);
		setContentView(layout);
	}

	public void setMessage(int resourceId) {
		TextView textView = findViewById(R.id.notice_text_view);
		textView.setText(resourceId);
	}

}
