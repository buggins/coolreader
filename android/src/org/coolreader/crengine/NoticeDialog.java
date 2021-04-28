package org.coolreader.crengine;

import android.app.Dialog;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import org.coolreader.R;

public class NoticeDialog extends Dialog {

	private final String mButton1Text;
	private final String mButton2Text;
	private final Runnable mButton1Runnable;
	private final Runnable mButton2Runnable;
	private String mMessageText;

	public NoticeDialog(BaseActivity activity, final Runnable onOkButton) {
		this(activity, R.string.dlg_button_ok, onOkButton, R.string.dlg_button_cancel, null, null);
	}

	public NoticeDialog(BaseActivity activity, final Runnable onOkButton, final Runnable onCancelButton) {
		this(activity, R.string.dlg_button_ok, onOkButton, R.string.dlg_button_cancel, onCancelButton, null);
	}

	public NoticeDialog(BaseActivity activity, int button1TextRes, final Runnable button1Runnable, int button2TextRes, final Runnable button2Runnable, final Runnable dismissRunnable) {
		super(activity, activity.getCurrentTheme().getThemeId());
		setOwnerActivity(activity);
		mButton1Text = activity.getString(button1TextRes);
		mButton1Runnable = button1Runnable;
		mButton2Text = activity.getString(button2TextRes);
		mButton2Runnable = button2Runnable;
		setCancelable(false);
		setCanceledOnTouchOutside(false);
		if (null != dismissRunnable)
			setOnDismissListener(dialog -> dismissRunnable.run());
	}

	public void setMessage(int resourceId) {
		mMessageText = getContext().getString(resourceId);
		TextView textView = findViewById(R.id.notice_text_view);
		if (null != textView)
			textView.setText(resourceId);
	}

	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.notice_dialog);
		Button button1 = findViewById(R.id.notice_dlg_btn_positive);
		button1.setText(mButton1Text);
		button1.setOnClickListener(v -> {
			if (mButton1Runnable != null)
				mButton1Runnable.run();
			dismiss();
		});
		Button button2 = findViewById(R.id.notice_dlg_btn_negative);
		button2.setText(mButton2Text);
		if (mButton2Runnable != null)
			button2.setOnClickListener(v -> {
				mButton2Runnable.run();
				dismiss();
			});
		else
			button2.setVisibility(View.GONE);
		if (mMessageText.length() > 0) {
			TextView textView = findViewById(R.id.notice_text_view);
			textView.setText(mMessageText);
		}
	}
}
