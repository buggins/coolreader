package org.coolreader.crengine;

import android.app.Activity;
import android.app.Dialog;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.Button;

public class BaseDialog extends Dialog {

	public BaseDialog( Activity activity, int positiveButtonText, int negativeButtonText )
	{
		super(activity);
		setOwnerActivity(activity);
		this.mPositiveButtonText = positiveButtonText;
		this.mNegativeButtonText = negativeButtonText;
	}

	public void setView( View view )
	{
		this.view = view;
		setContentView(createLayout(view));
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

	protected ViewGroup createButtonsPane()
	{
		if ( mNegativeButtonText==0 && mPositiveButtonText==0 )
			return null;
		LinearLayout layout = new LinearLayout( getContext() ); 
		layout.setOrientation(LinearLayout.HORIZONTAL);
		layout.setGravity(Gravity.FILL_HORIZONTAL | Gravity.BOTTOM);
		layout.setBaselineAligned(false);
		if ( mPositiveButtonText!=0 ) {
			Button positiveButton = new Button(getContext());
			positiveButton.setText(mPositiveButtonText);
			positiveButton.setOnClickListener(new View.OnClickListener() {
				public void onClick(View v) {
					onPositiveButtonClick();
				}
			});
			layout.addView(positiveButton);
		}
		if ( mNegativeButtonText!=0 ) {
			Button negativeButton = new Button(getContext());
			negativeButton.setText(mNegativeButtonText);
			negativeButton.setOnClickListener(new View.OnClickListener() {
				public void onClick(View v) {
					onNegativeButtonClick();
				}
			});
			layout.addView(negativeButton);
		}
		return layout;
	}

	protected ViewGroup createLayout( View view )
	{
		LinearLayout layout = new LinearLayout(getContext());
		layout.setBaselineAligned(false);
		layout.setGravity(Gravity.FILL);
		layout.setOrientation(LinearLayout.VERTICAL);
		layout.addView(view);
		ViewGroup buttons = createButtonsPane();
		if ( buttons!=null )
			layout.addView(buttons);
		return layout;
	}

	protected int mPositiveButtonText = 0;
	protected int mNegativeButtonText = 0;
	protected View view;
}
