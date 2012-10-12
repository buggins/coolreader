package org.coolreader.crengine;

import org.coolreader.R;

import android.app.Dialog;
import android.content.DialogInterface;
import android.graphics.PixelFormat;
import android.util.Log;
import android.view.GestureDetector;
import android.view.GestureDetector.SimpleOnGestureListener;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.TextView;

public class BaseDialog extends Dialog {

	View layoutView;
	ViewGroup buttonsLayout;
	ViewGroup contentsLayout;
	BaseActivity activity;
	String title;
	boolean needCancelButton;
	int positiveButtonImage;
	int positiveButtonContentDescriptionId = R.string.dlg_button_ok;
	int negativeButtonImage;
	int negativeButtonContentDescriptionId = R.string.action_go_back;
	int thirdButtonImage;
	int thirdButtonContentDescriptionId;
	public void setPositiveButtonImage(int imageId, int descriptionId) {
		positiveButtonImage = imageId;
		positiveButtonContentDescriptionId = descriptionId;
	}
	public void setNegativeButtonImage(int imageId, int descriptionId) {
		negativeButtonImage = imageId;
		negativeButtonContentDescriptionId = descriptionId;
	}
	public void setThirdButtonImage(int imageId, int descriptionId) {
		thirdButtonImage = imageId;
		thirdButtonContentDescriptionId = descriptionId;
	}
	
	public static final boolean DARK_THEME = !DeviceInfo.FORCE_LIGHT_THEME;
	public BaseDialog( BaseActivity activity )
	{
		this( activity, "", false, false );
	}
	public BaseDialog( BaseActivity activity, String title, boolean showNegativeButton, boolean windowed )
	{
		this( activity, title, showNegativeButton, activity.isFullscreen(), activity.isNightMode(), windowed );
	}
	public BaseDialog( BaseActivity activity, String title, boolean showNegativeButton, boolean fullscreen, boolean dark, boolean windowed )
	{
		//super(activity, fullscreen ? R.style.Dialog_Fullscreen : R.style.Dialog_Normal);
		//super(activity, fullscreen ? R.style.Dialog_Fullscreen : android.R.style.Theme_Dialog); //android.R.style.Theme_Light_NoTitleBar_Fullscreen : android.R.style.Theme_Light
		super(activity,
				windowed ? activity.getCurrentTheme().getDialogThemeId() :
				(fullscreen
				? activity.getCurrentTheme().getFullscreenDialogThemeId()
				: activity.getCurrentTheme().getDialogThemeId()
				));
		setOwnerActivity(activity);
		this.activity = activity;
		this.title = title;
		this.needCancelButton = showNegativeButton;
		getWindow().requestFeature(Window.FEATURE_NO_TITLE);
//		requestWindowFeature(Window.FEATURE_OPTIONS_PANEL);
		if (!DeviceInfo.EINK_SCREEN) {
			WindowManager.LayoutParams lp = new WindowManager.LayoutParams();
			lp.alpha = 1.0f;
			lp.dimAmount = 0.0f;
			if (!DeviceInfo.EINK_SCREEN)
				lp.format = DeviceInfo.PIXEL_FORMAT;
			lp.gravity = Gravity.CENTER_HORIZONTAL | Gravity.CENTER_VERTICAL;
			lp.horizontalMargin = 0;
			lp.verticalMargin = 0;
			lp.windowAnimations = 0;
			lp.layoutAnimationParameters = null;
			//lp.memoryType = WindowManager.LayoutParams.MEMORY_TYPE_PUSH_BUFFERS;
			getWindow().setAttributes(lp);
		}
		Log.i("cr3", "BaseDialog.window=" + getWindow());
        setCancelable(true);
        setOnDismissListener(new OnDismissListener() {
			@Override
			public void onDismiss(DialogInterface dialog) {
				onClose();
			}
        });
        onCreate();
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

	protected void onThirdButtonClick()
	{
		// override it
		dismiss();
	}

	protected void createButtonsPane( ViewGroup parent, ViewGroup layout )
	{
		//getWindow().getDecorView().getWidth()
		ImageButton positiveButton = (ImageButton)layout.findViewById(R.id.base_dlg_btn_positive);
		ImageButton negativeButton = (ImageButton)layout.findViewById(R.id.base_dlg_btn_negative);
		ImageButton backButton = (ImageButton)layout.findViewById(R.id.base_dlg_btn_back);
		if (positiveButtonImage != 0) {
			positiveButton.setImageResource(positiveButtonImage);
			if (positiveButtonContentDescriptionId != 0)
				Utils.setContentDescription(positiveButton, getContext().getString(positiveButtonContentDescriptionId));
			//backButton.setImageResource(positiveButtonImage);
		}
		if (thirdButtonImage != 0) {
			negativeButton.setImageResource(thirdButtonImage);
			if (thirdButtonContentDescriptionId != 0)
				Utils.setContentDescription(negativeButton, getContext().getString(thirdButtonContentDescriptionId));
		}
		if (negativeButtonImage != 0) {
			if (thirdButtonImage == 0) {
				negativeButton.setImageResource(negativeButtonImage);
				if (negativeButtonContentDescriptionId != 0)
					Utils.setContentDescription(negativeButton, getContext().getString(negativeButtonContentDescriptionId));
			}
			backButton.setImageResource(negativeButtonImage);
			if (negativeButtonContentDescriptionId != 0)
				Utils.setContentDescription(backButton, getContext().getString(negativeButtonContentDescriptionId));
		}
		if (needCancelButton) {
			//layout.removeView(backButton);
			if (thirdButtonImage == 0) {
				layout.removeView(negativeButton);
			} else {
				negativeButton.setOnClickListener(new View.OnClickListener() {
					public void onClick(View v) {
						onThirdButtonClick();
					}
				});
			}
			positiveButton.setOnClickListener(new View.OnClickListener() {
				public void onClick(View v) {
					onPositiveButtonClick();
				}
			});
			//negativeButton.setOnClickListener(new View.OnClickListener() {
			backButton.setOnClickListener(new View.OnClickListener() {
				public void onClick(View v) {
					onNegativeButtonClick();
				}
			});
		} else {
			layout.removeView(positiveButton);
			layout.removeView(negativeButton);
			if (title != null) {
				backButton.setOnClickListener(new View.OnClickListener() {
					public void onClick(View v) {
						onPositiveButtonClick();
					}
				});
			} else {
				parent.removeView(layout);
                buttonsLayout = null;
			}
		}
		if (title != null)
			setTitle(title);
		if (buttonsLayout != null) {
			buttonsLayout.setOnTouchListener(new OnTouchListener() {
				@Override
				public boolean onTouch(View v, MotionEvent event) {
					if (event.getAction() == MotionEvent.ACTION_DOWN) {
						int x = (int)event.getX();
						int dx = v.getWidth();
						if (x < dx / 3) {
							if (needCancelButton)
								onNegativeButtonClick();
							else
								onPositiveButtonClick();
						} else if (x > dx * 2 / 3) {
							onPositiveButtonClick();
						}
						return true;
					}
					return false;
				}
			});
		}
	}

	@Override
	public void setTitle(CharSequence title) {
		this.title = String.valueOf(title);
		if (buttonsLayout != null) {
	        TextView lbl = (TextView)buttonsLayout.findViewById(R.id.base_dlg_title);
	        if (lbl != null)
	        	lbl.setText(title != null ? title : "");
		}
	}

	protected View createLayout( View view )
	{
        LayoutInflater mInflater = LayoutInflater.from(getContext());
        ViewGroup layout = (ViewGroup)mInflater.inflate(R.layout.base_dialog, null);
        buttonsLayout = (ViewGroup)layout.findViewById(R.id.base_dlg_button_panel);
        if (buttonsLayout != null) {
            if ( needCancelButton || title != null) {
            	createButtonsPane(layout, buttonsLayout);
            } else {
            	layout.removeView(buttonsLayout);
                buttonsLayout = null;
            }
        }
        contentsLayout =  (ViewGroup)layout.findViewById(R.id.base_dialog_content_view);
        contentsLayout.addView(view);
        setTitle(title);
		return layout;
	}
	
	protected void onCreate() {
		// when dialog is created
	}
	
	protected void onClose() {
		// when dialog is closed
	}
	
	
	
	/**
	 * Set View's gesture handlers for LTR and RTL horizontal fling
	 * @param view
	 * @param ltrHandler, pass null to call onNegativeButtonClick
	 * @param rtlHandler, pass null to call onPositiveButtonClick
	 */
	public void setFlingHandlers(View view, Runnable ltrHandler, Runnable rtlHandler) {
		if (ltrHandler == null)
			ltrHandler = new Runnable() {
				@Override
				public void run() {
					// cancel
					onNegativeButtonClick();
				}
			};
		if (rtlHandler == null)
			rtlHandler = new Runnable() {
				@Override
				public void run() {
					// ok
					onPositiveButtonClick();
				}
			};
		final GestureDetector detector = new GestureDetector(new MyGestureListener(ltrHandler, rtlHandler));
		view.setOnTouchListener(new OnTouchListener() {
			@Override
			public boolean onTouch(View v, MotionEvent event) {
				return detector.onTouchEvent(event);
			}
		});
	}

	private class MyGestureListener extends SimpleOnGestureListener {
		Runnable ltrHandler;
		Runnable rtlHandler;
		
		public MyGestureListener(Runnable ltrHandler, Runnable rtlHandler) {
			this.ltrHandler = ltrHandler;
			this.rtlHandler = rtlHandler;
		}

		@Override
		public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX,
				float velocityY) {
			if (e1 == null || e2 == null)
				return false;
			int thresholdDistance = activity.getPalmTipPixels() * 2;
			int thresholdVelocity = activity.getPalmTipPixels();
			int x1 = (int)e1.getX();
			int x2 = (int)e2.getX();
			int y1 = (int)e1.getY();
			int y2 = (int)e2.getY();
			int dist = x2 - x1;
			int adist = dist > 0 ? dist : -dist;
			int ydist = y2 - y1;
			int aydist = ydist > 0 ? ydist : -ydist;
			int vel = (int)velocityX;
			if (vel<0)
				vel = -vel;
			if (vel > thresholdVelocity && adist > thresholdDistance && adist > aydist * 2) {
				if (dist > 0) {
					Log.d("cr3", "LTR fling detected");
					if (ltrHandler != null) {
						ltrHandler.run();
						return true;
					}
				} else {
					Log.d("cr3", "RTL fling detected");
					if (rtlHandler != null) {
						rtlHandler.run();
						return true;
					}
				}
			}
			return false;
		}
		
	}
	
	@Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
		activity.onUserActivity();
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			onNegativeButtonClick();
			return true;
		}
        if( this.view != null ) {
            if (this.view.onKeyDown(keyCode, event))
            	return true;
        }
        return super.onKeyDown(keyCode, event);
    }
    
	protected View view;
}
