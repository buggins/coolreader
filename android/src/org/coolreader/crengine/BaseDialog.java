/*
 * CoolReader for Android
 * Copyright (C) 2010-2012,2014,2020 Vadim Lopatin <coolreader.org@gmail.com>
 * Copyright (C) 2012 Michael Berganovsky <mike0berg@gmail.com>
 * Copyright (C) 2018,2021 Aleksey Chernov <valexlin@gmail.com>
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

import android.app.Dialog;
import android.util.Log;
import android.view.GestureDetector;
import android.view.GestureDetector.SimpleOnGestureListener;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.TextView;

import org.coolreader.R;

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
	
	public static final boolean DARK_THEME = !DeviceInfo.FORCE_HC_THEME;
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
        setOnDismissListener(dialog -> onClose());
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
		if (null != view)
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
		ImageButton positiveButton = layout.findViewById(R.id.base_dlg_btn_positive);
		ImageButton negativeButton = layout.findViewById(R.id.base_dlg_btn_negative);
		ImageButton backButton = layout.findViewById(R.id.base_dlg_btn_back);
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
				negativeButton.setOnClickListener(v -> onThirdButtonClick());
			}
			positiveButton.setOnClickListener(v -> onPositiveButtonClick());
			//negativeButton.setOnClickListener(new View.OnClickListener() {
			backButton.setOnClickListener(v -> onNegativeButtonClick());
		} else {
			layout.removeView(positiveButton);
			layout.removeView(negativeButton);
			if (title != null) {
				backButton.setOnClickListener(v -> onPositiveButtonClick());
			} else {
				parent.removeView(layout);
                buttonsLayout = null;
			}
		}
		if (title != null)
			setTitle(title);
		if (buttonsLayout != null) {
			buttonsLayout.setOnTouchListener((v, event) -> {
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
			});
		}
	}

	@Override
	public void setTitle(CharSequence title) {
		this.title = String.valueOf(title);
		if (buttonsLayout != null) {
	        TextView lbl = buttonsLayout.findViewById(R.id.base_dlg_title);
	        if (lbl != null)
	        	lbl.setText(title != null ? title : "");
		}
	}

	protected View createLayout( View view )
	{
        LayoutInflater mInflater = LayoutInflater.from(getContext());
        ViewGroup layout = (ViewGroup)mInflater.inflate(R.layout.base_dialog, null);
        buttonsLayout = layout.findViewById(R.id.base_dlg_button_panel);
        if (buttonsLayout != null) {
            if ( needCancelButton || title != null) {
            	createButtonsPane(layout, buttonsLayout);
            } else {
            	layout.removeView(buttonsLayout);
                buttonsLayout = null;
            }
        }
        contentsLayout =  layout.findViewById(R.id.base_dialog_content_view);
        if (null != view)
            contentsLayout.addView(view);
        setTitle(title);
		return layout;
	}
	
	protected void onCreate() {
		// when dialog is created
		Log.d("DLG","BaseDialog.onCreate()");
		activity.onDialogCreated(this);
	}
	
	protected void onClose() {
		// when dialog is closed
		Log.d("DLG","BaseDialog.onClose()");
		if (needCancelButton)
			onNegativeButtonClick();
		else if (buttonsLayout != null)
			onPositiveButtonClick();
		activity.onDialogClosed(this);
	}

	
	/**
	 * Set View's gesture handlers for LTR and RTL horizontal fling
	 * @param view
	 * @param ltrHandler, pass null to call onNegativeButtonClick
	 * @param rtlHandler, pass null to call onPositiveButtonClick
	 */
	public void setFlingHandlers(View view, Runnable ltrHandler, Runnable rtlHandler) {
		// cancel
		if (ltrHandler == null)
			ltrHandler = this::onNegativeButtonClick;
		// ok
		if (rtlHandler == null)
			rtlHandler = this::onPositiveButtonClick;
		final GestureDetector detector = new GestureDetector(new MyGestureListener(ltrHandler, rtlHandler));
		view.setOnTouchListener((v, event) -> detector.onTouchEvent(event));
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
			return true;
		}
        if( this.view != null ) {
            if (this.view.onKeyDown(keyCode, event))
            	return true;
        }
        return super.onKeyDown(keyCode, event);
    }

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			onNegativeButtonClick();
			return true;
		}
		return super.onKeyUp(keyCode, event);
	}

	protected View view;
}
