package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.R;

import android.content.Context;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.PopupWindow;

public class CRToolBar extends ViewGroup {

	public CRToolBar(BaseActivity context) {
		super(context);
		this.preferredItemHeight = context.getPreferredItemHeight();
	}

	private ArrayList<ReaderAction> actions = new ArrayList<ReaderAction>();
	private boolean showLabels;
	private int contentHeight;
	private int contentWidth;
	private int buttonHeight;
	private int buttonWidth;
	private int visibleButtonCount;
	private int visibleNonButtonCount;
	private boolean isVertical;
	final private int preferredItemHeight;
	public void setVertical(boolean vertical) {
		this.isVertical = vertical;
		if (isVertical)
			setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.FILL_PARENT));
		else
			setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
	}
	public boolean isVertical() {
		return this.isVertical;
	}
	public CRToolBar(BaseActivity context, ArrayList<ReaderAction> actions) {
		super(context);
		this.actions = actions;
		this.showLabels = true;
		this.preferredItemHeight = context.getPreferredItemHeight();
		buttonWidth = preferredItemHeight;
		buttonHeight = preferredItemHeight;
		for (int i=0; i<actions.size(); i++) {
			ReaderAction item = actions.get(i);
			int iconId = item.iconId;
			if (iconId == 0) {
				visibleNonButtonCount++;
				continue;
			}
			Drawable d = context.getResources().getDrawable(iconId);
			visibleButtonCount++;
			int w = d.getIntrinsicWidth() + 4;
			int h = d.getIntrinsicHeight();
			if (buttonWidth < w || buttonHeight < h) {
				buttonWidth = w;
				buttonHeight = h;
				contentHeight = buttonHeight + getPaddingTop() + getPaddingBottom();
				contentWidth = buttonWidth + getPaddingLeft() + getPaddingRight();
			}
		}
	}

	private OnActionHandler onActionHandler;
	
	public void setOnItemSelectedHandler(OnActionHandler handler) {
		this.onActionHandler = handler;
	}
	
	public interface OnActionHandler {
		boolean onActionSelected(ReaderAction item);
	}
	
	private void onMoreButtonClick() {
		// TODO: show additional items
	}
	
	private void onButtonClick(ReaderAction item) {
		if (onActionHandler != null)
			onActionHandler.onActionSelected(item);
	}
	
	private ImageButton addButton(Rect rect, final ReaderAction item, boolean left) {
		Rect rc = new Rect(rect);
		if (isVertical) {
			if (left) {
				rc.bottom = rc.top + buttonHeight;
				rect.top += buttonHeight;
			} else {
				rc.top = rc.bottom - buttonHeight;
				rect.bottom -= buttonHeight;
			}
		} else {
			if (left) {
				rc.right = rc.left + buttonWidth;
				rect.left += buttonWidth;
			} else {
				rc.left = rc.right - buttonWidth;
				rect.right -= buttonWidth;
			}
		}
		if (rc.isEmpty())
			return null;
		ImageButton ib = new ImageButton(getContext());
		if (item != null) {
			ib.setImageResource(item.iconId);
			ib.setTag(item);
		} else {
			ib.setImageDrawable(getResources().getDrawable(R.drawable.ic_menu_moreoverflow));
		}
		ib.setBackgroundDrawable(null);
		ib.layout(rc.left, rc.top, rc.right, rc.bottom);
		ib.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				if (item != null)
					onButtonClick(item);
				else
					onMoreButtonClick();
			}
		});
		addView(ib);
		return ib;
	}
	
	@Override
	protected void onLayout(boolean changed, int left, int top, int right,
			int bottom) {
		right -= left;
		bottom -= top;
		left = top = 0;
		Rect rect = new Rect(left + getPaddingLeft(), top + getPaddingTop(), right +  + getPaddingRight(), bottom + getPaddingBottom());
		if (rect.isEmpty())
			return;
		removeAllViews();
		ArrayList<ReaderAction> itemsToShow = new ArrayList<ReaderAction>();
		int maxButtonCount = 1;
		if (isVertical) {
			int maxHeight = bottom - top - getPaddingTop() - getPaddingBottom();
			maxButtonCount = maxHeight / buttonHeight;
		} else {
			int maxWidth = right - left - getPaddingLeft() - getPaddingRight();
			maxButtonCount = maxWidth / buttonWidth;
		}
		int count = 0;
		boolean addEllipsis = visibleButtonCount > maxButtonCount || visibleNonButtonCount > 0;
		if (addEllipsis) {
			addButton(rect, null, false);
		}
		for (int i = 0; i < actions.size(); i++) {
			if (count >= maxButtonCount - 1)
				break;
			ReaderAction item = actions.get(i);
			itemsToShow.add(item);
			count++;
			addButton(rect, item, true);
		}
	}

	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int widthMode = MeasureSpec.getMode(widthMeasureSpec);
//        if (widthMode != MeasureSpec.EXACTLY) {
//            throw new IllegalStateException(getClass().getSimpleName() + " can only be used " +
//                    "with android:layout_width=\"match_parent\" (or fill_parent)");
//        }
        
        int heightMode = MeasureSpec.getMode(heightMeasureSpec);
//        if (heightMode != MeasureSpec.AT_MOST) {
//            throw new IllegalStateException(getClass().getSimpleName() + " can only be used " +
//                    "with android:layout_height=\"wrap_content\"");
//        }

        if (isVertical) {
	        int contentHeight = MeasureSpec.getSize(heightMeasureSpec);
	
	        int maxWidth = contentWidth > 0 ?
	                contentWidth : MeasureSpec.getSize(widthMeasureSpec);
	        setMeasuredDimension(maxWidth, contentHeight);
        } else {
	        int contentWidth = MeasureSpec.getSize(widthMeasureSpec);
	    	
	        int maxHeight = contentHeight > 0 ?
	                contentHeight : MeasureSpec.getSize(heightMeasureSpec);
	        setMeasuredDimension(contentWidth, maxHeight);
        }
	}

	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		super.onSizeChanged(w, h, oldw, oldh);
	}

	
	public static PopupWindow showPopup(BaseActivity context, View anchor, ArrayList<ReaderAction> actions) {
		final CRToolBar tb = new CRToolBar(context, actions);
		tb.measure(anchor.getWidth(), ViewGroup.LayoutParams.WRAP_CONTENT);
		int w = tb.getMeasuredWidth();
		int р = tb.getMeasuredHeight();
		final PopupWindow popup = new PopupWindow(context);
		popup.setTouchInterceptor(new OnTouchListener() {
			
			@Override
			public boolean onTouch(View v, MotionEvent event) {
				if ( event.getAction()==MotionEvent.ACTION_OUTSIDE ) {
					popup.dismiss();
					return true;
				}
				return false;
			}
		});
		//popup.setBackgroundDrawable(new BitmapDrawable());
		popup.setWidth(WindowManager.LayoutParams.WRAP_CONTENT);
		popup.setHeight(WindowManager.LayoutParams.WRAP_CONTENT);
		popup.setFocusable(true);
		popup.setTouchable(true);
		popup.setOutsideTouchable(true);
		popup.setContentView(tb);
		int [] location = new int[2];
		anchor.getLocationOnScreen(location);
		int popupY = location[1];
		popup.showAtLocation(anchor, Gravity.TOP | Gravity.LEFT, location[0], popupY);
		return popup;
	}
}
