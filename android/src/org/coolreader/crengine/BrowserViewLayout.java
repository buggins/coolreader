package org.coolreader.crengine;

import android.view.View;
import android.view.ViewGroup;
import android.view.View.MeasureSpec;
import android.view.ViewGroup.LayoutParams;

public class BrowserViewLayout extends ViewGroup {
	private BaseActivity activity;
	private FileBrowser contentView;
	private View titleView;
	private CRToolBar toolbarView;
	public BrowserViewLayout(BaseActivity context, FileBrowser contentView, CRToolBar toolbar, View titleView) {
		super(context);
		this.activity = context;
		this.contentView = contentView;
		
		
		this.titleView = titleView;
		this.titleView.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
		this.toolbarView = toolbar;
		this.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
		this.addView(titleView);
		this.addView(toolbarView);
		this.addView(contentView);
		this.onThemeChanged(context.getCurrentTheme());
		titleView.setFocusable(false);
		titleView.setFocusableInTouchMode(false);
		toolbarView.setFocusable(false);
		toolbarView.setFocusableInTouchMode(false);
		contentView.setFocusable(false);
		contentView.setFocusableInTouchMode(false);
	}
	
	public void onThemeChanged(InterfaceTheme theme) {
		titleView.setBackgroundResource(theme.getBrowserStatusBackground());
		//toolbarView.setButtonAlpha(theme.getToolbarButtonAlpha());
		toolbarView.setBackgroundResource(theme.getBrowserToolbarBackground(toolbarView.isVertical()));
		toolbarView.onThemeChanged(theme);
	}

	@Override
	protected void onLayout(boolean changed, int l, int t, int r, int b) {
		r -= l;
		b -= t;
		t = 0;
		l = 0;
		int titleHeight = titleView.getMeasuredHeight();
		if (toolbarView.isVertical()) {
			int tbWidth = toolbarView.getMeasuredWidth();
			titleView.layout(l + tbWidth, t, r, t + titleHeight);
			toolbarView.layout(l, t, l + tbWidth, b);
			contentView.layout(l + tbWidth, t + titleHeight, r, b);
			toolbarView.setBackgroundResource(activity.getCurrentTheme().getBrowserToolbarBackground(true));
		} else {
			int tbHeight = toolbarView.getMeasuredHeight();
			toolbarView.layout(l, t, r, t + tbHeight);
			titleView.layout(l, t + tbHeight, r, t + titleHeight + tbHeight);
			contentView.layout(l, t + titleHeight + tbHeight, r, b);
			toolbarView.setBackgroundResource(activity.getCurrentTheme().getBrowserToolbarBackground(false));
		}
	}
	
	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		int w = MeasureSpec.getSize(widthMeasureSpec);
		int h = MeasureSpec.getSize(heightMeasureSpec);

		
		toolbarView.setVertical(w > h);
		if (w > h) {
			// landscape
			toolbarView.setVertical(true);
			toolbarView.measure(MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, w), 
					MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, h));
			int tbWidth = toolbarView.getMeasuredWidth();
			titleView.measure(MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, w - tbWidth), 
					MeasureSpec.makeMeasureSpec(MeasureSpec.UNSPECIFIED, 0));
			int titleHeight = titleView.getMeasuredHeight();
			contentView.measure(MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, w - tbWidth), 
					MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, h - titleHeight));
		} else {
			// portrait
			toolbarView.setVertical(false);
			toolbarView.measure(MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, w), 
					MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, h));
			int tbHeight = toolbarView.getMeasuredHeight();
			titleView.measure(widthMeasureSpec, 
					MeasureSpec.makeMeasureSpec(MeasureSpec.UNSPECIFIED, 0));
			int titleHeight = titleView.getMeasuredHeight();
			contentView.measure(widthMeasureSpec, 
					MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, h - titleHeight - tbHeight));
		}
        setMeasuredDimension(w, h);
	}
	
	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		super.onSizeChanged(w, h, oldw, oldh);
	}
}