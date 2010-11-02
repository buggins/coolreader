package org.coolreader.crengine;

import org.coolreader.R;

import android.app.Activity;
import android.app.Dialog;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.Button;

public class BaseDialog extends Dialog {

	View layoutView;
	ViewGroup buttonsLayout;
	ViewGroup contentsLayout;
	public BaseDialog( Activity activity, int positiveButtonText, int negativeButtonText )
	{
		super(activity);
		setOwnerActivity(activity);
		this.mPositiveButtonText = positiveButtonText;
		this.mNegativeButtonText = negativeButtonText;
        setCancelable(true);
	}

	public void setView( View view )
	{
		this.view = view;
		if ( layoutView==null ) {
			layoutView = createLayout(view);
			setContentView(layoutView);
		}
		contentsLayout.removeAllViews();
		contentsLayout.addView(view);
	}
	
	protected void onPositiveButtonClick()
	{
		// override it
		dismiss();
	}
	
	protected void onNegativeButtonClick()
	{
		// override it
		dismiss();
	}

	protected void createButtonsPane( ViewGroup layout )
	{
		if ( mNegativeButtonText==0 && mPositiveButtonText==0 ) {
			layout.setVisibility(View.INVISIBLE);
			return;
		}
		//getWindow().getDecorView().getWidth()
		if ( mPositiveButtonText!=0 ) {
			Button positiveButton = (Button)layout.findViewById(R.id.base_dlg_btn_positive);
			if ( positiveButton==null ) {
				positiveButton = new Button(getContext());
				layout.addView(positiveButton);
			}
			positiveButton.setText(mPositiveButtonText);
			positiveButton.setOnClickListener(new View.OnClickListener() {
				public void onClick(View v) {
					onPositiveButtonClick();
				}
			});
		}
		if ( mNegativeButtonText!=0 ) {
			Button negativeButton = (Button)layout.findViewById(R.id.base_dlg_btn_negative);
			if ( negativeButton==null ) {
				negativeButton = new Button(getContext());
				layout.addView(negativeButton);
			}
			negativeButton.setText(mNegativeButtonText);
			negativeButton.setOnClickListener(new View.OnClickListener() {
				public void onClick(View v) {
					onNegativeButtonClick();
				}
			});
		}
	}

	@Override
	public void setTitle(CharSequence title) {
		if ( title!=null )
			super.setTitle(title);
		else
			getWindow().requestFeature(Window.FEATURE_NO_TITLE);
	}

	protected View createLayout( View view )
	{
        LayoutInflater mInflater = LayoutInflater.from(getContext());
        View layout = mInflater.inflate(R.layout.base_dialog, null);
        buttonsLayout = (ViewGroup)layout.findViewById(R.id.base_dialog_buttons_view);
        if ( buttonsLayout!=null )
        	createButtonsPane(buttonsLayout);
        contentsLayout =  (ViewGroup)layout.findViewById(R.id.base_dialog_content_view);
        contentsLayout.addView(view);
		return layout;
	}

	protected int mPositiveButtonText = 0;
	protected int mNegativeButtonText = 0;
	protected View view;
}
