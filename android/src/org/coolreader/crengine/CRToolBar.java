package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.R;

import android.content.Context;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;

public class CRToolBar extends ViewGroup {
	private final Menu menu;
	private boolean showLabels;
	private int contentHeight;
	private int buttonHeight;
	private int buttonWidth;
	private int visibleButtonCount;
	private int visibleNonButtonCount;
	public CRToolBar(Context context, Menu menu) {
		super(context);
		this.menu = menu;
		this.showLabels = true;
		for (int i=0; i<menu.size(); i++) {
			MenuItem item = menu.getItem(i);
			if (!item.isVisible())
				continue;
			Drawable d = item.getIcon();
			if (d == null) {
				visibleNonButtonCount++;
				continue;
			}
			visibleButtonCount++;
			int w = d.getIntrinsicWidth() * 2;
			int h = d.getIntrinsicHeight();
			if (buttonWidth < w || buttonHeight < h) {
				buttonWidth = w;
				buttonHeight = h;
				contentHeight = buttonHeight + getPaddingTop() + getPaddingBottom();
			}
			
		}
	}

	private OnItemSelectedHandler onItemSelectedHandler;
	
	public void setOnItemSelectedHandler(OnItemSelectedHandler handler) {
		this.onItemSelectedHandler = handler;
	}
	
	public interface OnItemSelectedHandler {
		boolean onOptionsItemSelected(MenuItem item);
	}
	
	private void onMoreButtonClick() {
		// TODO: show additional items
	}
	
	private void onButtonClick(MenuItem item) {
		if (onItemSelectedHandler != null)
			onItemSelectedHandler.onOptionsItemSelected(item);
	}
	
	private ImageButton addButton(Rect rect, final MenuItem item, boolean left) {
		Rect rc = new Rect(rect);
		if (left) {
			rc.right = rc.left + buttonWidth;
			rect.left += buttonWidth;
		} else {
			rc.left = rc.right - buttonWidth;
			rect.right -= buttonWidth;
		}
		if (rc.isEmpty())
			return null;
		ImageButton ib = new ImageButton(getContext());
		if (item != null) {
			ib.setImageDrawable(item.getIcon());
			ib.setTag(item);
		} else {
			ib.setImageDrawable(getResources().getDrawable(R.drawable.cr3_menu_options)); // TODO: menu icon
		}
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
		Rect rect = new Rect(left + getPaddingLeft(), top + getPaddingTop(), right +  + getPaddingRight(), bottom + getPaddingBottom());
		if (rect.isEmpty())
			return;
		removeAllViews();
		ArrayList<MenuItem> itemsToShow = new ArrayList<MenuItem>();
		int maxWidth = right - left - getPaddingLeft() - getPaddingRight();
		int maxButtonCount = maxWidth / buttonWidth;
		int count = 0;
		boolean addEllipsis = visibleButtonCount > maxButtonCount || visibleNonButtonCount > 0;
		if (addEllipsis) {
			addButton(rect, null, false);
		}
		for (int i = 0; i < menu.size(); i++) {
			if (count >= maxButtonCount - 1)
				break;
			MenuItem item = menu.getItem(i);
			if (!item.isVisible())
				continue;
			itemsToShow.add(item);
			count++;
			addButton(rect, item, true);
		}
	}

	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int widthMode = MeasureSpec.getMode(widthMeasureSpec);
        if (widthMode != MeasureSpec.EXACTLY) {
            throw new IllegalStateException(getClass().getSimpleName() + " can only be used " +
                    "with android:layout_width=\"match_parent\" (or fill_parent)");
        }
        
        int heightMode = MeasureSpec.getMode(heightMeasureSpec);
        if (heightMode != MeasureSpec.AT_MOST) {
            throw new IllegalStateException(getClass().getSimpleName() + " can only be used " +
                    "with android:layout_height=\"wrap_content\"");
        }

        int contentWidth = MeasureSpec.getSize(widthMeasureSpec);

        int maxHeight = contentHeight > 0 ?
                contentHeight : MeasureSpec.getSize(heightMeasureSpec);
        setMeasuredDimension(contentWidth, maxHeight);
	}

	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		super.onSizeChanged(w, h, oldw, oldh);
	}
}
