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
		if (keyCode == ReaderView.NOOK_KEY_NEXT_RIGHT || keyCode == KeyEvent.KEYCODE_DPAD_RIGHT || keyCode == ReaderView.NOOK_KEY_SHIFT_DOWN)
			dir = 1;
		if (keyCode == ReaderView.NOOK_KEY_PREV_RIGHT || keyCode == KeyEvent.KEYCODE_DPAD_LEFT || keyCode == ReaderView.NOOK_KEY_SHIFT_UP)
			dir = -1;
		if (dir != 0) {
			if (dir > 0) {
				// scroll down
				int bottompos = pointToPosition(this.getWidth() / 2, getHeight() / 2);
				if (bottompos != INVALID_POSITION) {
					this.setSelectionFromTop(bottompos, 0);
				}
			} else {
				// scroll up
				int toppos = pointToPosition(this.getWidth() / 2, 20);
				if (toppos != INVALID_POSITION) {
					this.setSelectionFromTop(toppos, getHeight() / 2);
					toppos = pointToPosition(this.getWidth() / 2, 20);
					if (toppos != INVALID_POSITION) {
						this.setSelectionFromTop(toppos, 0);
					}
				}
			}
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}
}
