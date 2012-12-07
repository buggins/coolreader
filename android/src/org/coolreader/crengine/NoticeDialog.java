package org.coolreader.crengine;

import org.coolreader.R;

import android.app.Dialog;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

public class NoticeDialog extends Dialog {

	public NoticeDialog(BaseActivity activity, final Runnable onOkButton, final Runnable onCancelButton) {
		super(activity, activity.getCurrentTheme().getThemeId());
		setOwnerActivity(activity);
        LayoutInflater mInflater = LayoutInflater.from(getContext());
        ViewGroup layout = (ViewGroup)mInflater.inflate(R.layout.notice_dialog, null);
        setTitle(R.string.app_name);
        Button button1 = (Button)layout.findViewById(R.id.base_dlg_btn_positive);
        button1.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
        		onOkButton.run();
        		dismiss();
			}
		});
        Button button2 = (Button)layout.findViewById(R.id.base_dlg_btn_negative);
        if (onCancelButton != null)
	        button2.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
	        		onCancelButton.run();
	        		dismiss();
				}
			});
        else
        	button2.setVisibility(View.GONE);
		setContentView(layout);
	}

}
