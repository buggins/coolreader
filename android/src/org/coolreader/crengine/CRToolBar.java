package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.R;

import android.graphics.Matrix;
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
	private int buttonHeight;
	private int buttonWidth;
	private int visibleButtonCount;
	private int visibleNonButtonCount;
	private boolean isVertical;
	final private int preferredItemHeight;
	private int BUTTON_SPACING = 4;
	private final int BAR_SPACING = 8;
	public void setVertical(boolean vertical) {
		this.isVertical = vertical;
		if (isVertical) {
			//setPadding(BUTTON_SPACING, BUTTON_SPACING, BAR_SPACING, BUTTON_SPACING);
			setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.FILL_PARENT));
		} else {
			//setPadding(BUTTON_SPACING, BAR_SPACING, BUTTON_SPACING, BUTTON_SPACING);
			setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
		}
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
		//BUTTON_SPACING = preferredItemHeight / 5;
		int dpi = context.getDensityDpi();
		for (int i=0; i<actions.size(); i++) {
			ReaderAction item = actions.get(i);
			int iconId = item.iconId;
			if (iconId == 0) {
				visibleNonButtonCount++;
				continue;
			}
			Drawable d = context.getResources().getDrawable(iconId);
			visibleButtonCount++;
			int w = d.getIntrinsicWidth() * dpi / 160 + 8;
			int h = d.getIntrinsicHeight() * dpi / 160 + 8;
			if (buttonWidth < w) {
				buttonWidth = w;
			}
			if (buttonHeight < h) {
				buttonHeight = h;
			}
		}
	}

	private OnActionHandler onActionHandler;
	
	public void setOnActionHandler(OnActionHandler handler) {
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
				rect.top += buttonHeight + BUTTON_SPACING;
			} else {
				rc.top = rc.bottom - buttonHeight;
				rect.bottom -= buttonHeight + BUTTON_SPACING;
			}
		} else {
			if (left) {
				rc.right = rc.left + buttonWidth;
				rect.left += buttonWidth + BUTTON_SPACING;
			} else {
				rc.left = rc.right - buttonWidth;
				rect.right -= buttonWidth + BUTTON_SPACING;
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
		removeAllViews();

		View divider = new View(getContext());
		addView(divider);
		if (isVertical()) {
			divider.setBackgroundResource(R.drawable.divider_light_vertical_tiled);
			divider.layout(right - 8, top, right, bottom);
		} else {
			divider.setBackgroundResource(R.drawable.divider_light_tiled);
			divider.layout(left, bottom - 8, right, bottom);
		}
		
		Rect rect = new Rect(left + getPaddingLeft() + BUTTON_SPACING, top + getPaddingTop() + BUTTON_SPACING, right - getPaddingRight() - BUTTON_SPACING, bottom - getPaddingBottom() - BUTTON_SPACING);
		if (rect.isEmpty())
			return;
		ArrayList<ReaderAction> itemsToShow = new ArrayList<ReaderAction>();
		int maxButtonCount = 1;
		if (isVertical) {
			rect.right -= BAR_SPACING;
			int maxHeight = bottom - top - getPaddingTop() - getPaddingBottom();
			maxButtonCount = maxHeight / (buttonHeight + BUTTON_SPACING);
		} else {
			rect.bottom -= BAR_SPACING;
			int maxWidth = right - left - getPaddingLeft() - getPaddingRight();
			maxButtonCount = maxWidth / (buttonWidth + BUTTON_SPACING);
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
	        int maxWidth = buttonWidth + BUTTON_SPACING + BUTTON_SPACING + BAR_SPACING + getPaddingLeft() + getPaddingRight();
	        setMeasuredDimension(maxWidth, contentHeight);
        } else {
	        int contentWidth = MeasureSpec.getSize(widthMeasureSpec);
	        int maxHeight = buttonHeight + BUTTON_SPACING + BUTTON_SPACING + BAR_SPACING + getPaddingTop() + getPaddingBottom();
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
