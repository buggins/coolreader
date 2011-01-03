package org.coolreader.crengine;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.app.Dialog;
import android.graphics.PixelFormat;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;

public class BaseDialog extends Dialog {

	View layoutView;
	ViewGroup buttonsLayout;
	ViewGroup contentsLayout;
	public static final boolean DARK_THEME = true;
	public BaseDialog( CoolReader activity, int positiveButtonText, int negativeButtonText, boolean windowed )
	{
		this( activity, positiveButtonText, negativeButtonText, activity.isFullscreen(), activity.isNightMode(), windowed );
	}
	public BaseDialog( CoolReader activity, int positiveButtonText, int negativeButtonText, boolean fullscreen, boolean dark, boolean windowed )
	{
		//super(activity, fullscreen ? R.style.Dialog_Fullscreen : R.style.Dialog_Normal);
		//super(activity, fullscreen ? R.style.Dialog_Fullscreen : android.R.style.Theme_Dialog); //android.R.style.Theme_Light_NoTitleBar_Fullscreen : android.R.style.Theme_Light
		super(activity,
				windowed ? (dark||DARK_THEME ? android.R.style.Theme_Dialog : android.R.style.Theme_Dialog) :
				(fullscreen
				? (	dark||DARK_THEME ? R.style.Dialog_Fullscreen_Night : R.style.Dialog_Fullscreen_Day )
				: (	dark||DARK_THEME ? R.style.Dialog_Normal_Night : R.style.Dialog_Normal_Day )
				));
		setOwnerActivity(activity);
		this.mPositiveButtonText = positiveButtonText;
		this.mNegativeButtonText = negativeButtonText;
//		requestWindowFeature(Window.FEATURE_OPTIONS_PANEL);
		WindowManager.LayoutParams lp = new WindowManager.LayoutParams();
		lp.alpha = 1.0f;
		lp.dimAmount = 0.0f;
		lp.format = PixelFormat.RGB_565;
		lp.gravity = Gravity.CENTER_HORIZONTAL | Gravity.CENTER_VERTICAL;
		lp.horizontalMargin = 0;
		lp.verticalMargin = 0;
		lp.windowAnimations = 0;
		lp.layoutAnimationParameters = null;
		//lp.memoryType = WindowManager.LayoutParams.MEMORY_TYPE_PUSH_BUFFERS;
		getWindow().setAttributes(lp);
		Log.i("cr3", "BaseDialog.window=" + getWindow());
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
		Button negativeButton = (Button)layout.findViewById(R.id.base_dlg_btn_negative);
		if ( negativeButton==null && mNegativeButtonText!=0 ) {
			negativeButton = new Button(getContext());
			layout.addView(negativeButton);
		}
		if ( negativeButton!=null ) {
			if ( mNegativeButtonText!=0 ) {
				negativeButton.setText(mNegativeButtonText);
				negativeButton.setOnClickListener(new View.OnClickListener() {
					public void onClick(View v) {
						onNegativeButtonClick();
					}
				});
			} else {
				negativeButton.setVisibility(View.INVISIBLE);
			}
		}
	}

	@Override
	public void setTitle(CharSequence title) {
		if ( title!=null )
			super.setTitle(title);
		else {
			getWindow().requestFeature(Window.FEATURE_NO_TITLE);
			//((CoolReader)getOwnerActivity()).applyFullscreen( getWindow() );
		}
	}

	protected View createLayout( View view )
	{
        LayoutInflater mInflater = LayoutInflater.from(getContext());
        ViewGroup layout = (ViewGroup)mInflater.inflate(R.layout.base_dialog, null);
        buttonsLayout = (ViewGroup)layout.findViewById(R.id.base_dialog_buttons_view);
        if ( buttonsLayout!=null ) {
            if ( mPositiveButtonText!=0 || mNegativeButtonText!=0 ) {
            	createButtonsPane(buttonsLayout);
            } else {
            	layout.removeView(buttonsLayout);
            }
        }
        contentsLayout =  (ViewGroup)layout.findViewById(R.id.base_dialog_content_view);
        contentsLayout.addView(view);
		return layout;
	}

	protected int mPositiveButtonText = 0;
	protected int mNegativeButtonText = 0;
	protected View view;
}
