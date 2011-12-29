package org.coolreader.crengine;

import android.content.Context;
import android.view.KeyEvent;
import android.widget.ListView;

public class BaseListView  extends ListView {
	public BaseListView(Context context) {
		super(context);
        setFocusable(true);
        setFocusableInTouchMode(true);
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		int dir = 0;
		if (keyCode == 0)
			keyCode = event.getScanCode();
		if (keyCode == ReaderView.SONY_DPAD_RIGHT_SCANCODE || keyCode == ReaderView.SONY_DPAD_DOWN_SCANCODE || keyCode == ReaderView.NOOK_KEY_NEXT_RIGHT || keyCode == KeyEvent.KEYCODE_DPAD_RIGHT || keyCode == ReaderView.NOOK_KEY_SHIFT_DOWN)
			dir = 1;
		if (keyCode == ReaderView.SONY_DPAD_LEFT_SCANCODE || keyCode == ReaderView.SONY_DPAD_UP_SCANCODE || keyCode == ReaderView.NOOK_KEY_PREV_RIGHT || keyCode == KeyEvent.KEYCODE_DPAD_LEFT || keyCode == ReaderView.NOOK_KEY_SHIFT_UP)
			dir = -1;
//		boolean tm = isInTouchMode();
		if (dir != 0) {
			long ts = android.os.SystemClock.uptimeMillis();
			KeyEvent ev = new KeyEvent(ts, ts, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_SPACE, 0, dir > 0 ? 0 : KeyEvent.META_SHIFT_ON);
			super.onKeyDown(KeyEvent.KEYCODE_SPACE, ev);
//			int top = 0; //getTop();
//			if (dir > 0) {
//				// scroll down
//				int bottompos = pointToPosition(this.getWidth() / 2, getHeight() * 4/5);
//				if (bottompos != INVALID_POSITION) {
//					this.setSelectionFromTop(bottompos, top + 0);
//				}
//			} else {
//				// scroll up
//				int toppos = getFirstVisiblePosition();//  pointToPosition(this.getWidth() / 2, 20);
//				if (toppos != INVALID_POSITION) {
//					this.setSelectionFromTop(toppos, top + getHeight() * 4/5);
//					toppos = pointToPosition(this.getWidth() / 2, 20);
//					toppos = getFirstVisiblePosition(); //pointToPosition(this.getWidth() / 2, 20);
//					if (toppos != INVALID_POSITION) {
//						this.setSelectionFromTop(toppos, top + 0);
//					}
//				}
//			}
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}
}
