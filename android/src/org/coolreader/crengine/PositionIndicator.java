package org.coolreader.crengine;

import org.coolreader.CoolReader;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.view.View;
import android.view.ViewGroup.LayoutParams;

class PositionIndicator extends View {

	private final int INDICATOR_HEIGHT = 8;
	
	private int color = 0;
	private int percent = 0;
	
	public PositionIndicator(CoolReader context) {
		super(context);
		setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
		this.color = context.settings().getColor(Settings.PROP_STATUS_FONT_COLOR, 0);
		//setBackgroundColor(0xC0404040);
	}
	
	public void setColor(int color) {
		this.color = color & 0xFFFFFF;
		if (isShown())
			invalidate();
	}
	
	@Override
	protected void onDraw(Canvas canvas) {
		Paint readPaint = Utils.createSolidPaint(0xC0000000 | color);
		Paint unreadPaint = Utils.createSolidPaint(0x40000000 | color);
		int w = getWidth();
		int h = getHeight();
		int pos = percent;
		int x = w * pos / 10000;
		canvas.drawRect(new Rect(getLeft(), h/2 - 3, getLeft() + x, h/2 + 0), readPaint);
		canvas.drawRect(new Rect(getLeft() + x, h/2 - 3, getLeft() + w, h/2 + 0), unreadPaint);
	}

	@Override
	protected void onLayout(boolean changed, int left, int top, int right,
			int bottom) {
		super.onLayout(changed, left, top, right, bottom);
	}

	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		int w = MeasureSpec.getSize(widthMeasureSpec);
		int h = INDICATOR_HEIGHT;
		setMeasuredDimension(w, h);
	}
	
	public void setPosition(int percent) {
		if (this.percent == percent)
			return;
		this.percent = percent;
		if (isShown())
			invalidate();
	}
}