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
        if (DeviceInfo.SONY_NAVIGATION_KEYS) {
            if (keyCode == ReaderView.SONY_DPAD_RIGHT_SCANCODE || keyCode == ReaderView.SONY_DPAD_DOWN_SCANCODE || keyCode==KeyEvent.KEYCODE_DPAD_DOWN || keyCode == KeyEvent.KEYCODE_DPAD_RIGHT)
                dir = 1;
            else if (keyCode == ReaderView.SONY_DPAD_LEFT_SCANCODE || keyCode == ReaderView.SONY_DPAD_UP_SCANCODE || keyCode==KeyEvent.KEYCODE_DPAD_UP || keyCode == KeyEvent.KEYCODE_DPAD_LEFT )
                dir = -1;
        } else {
            if (keyCode == ReaderView.NOOK_KEY_NEXT_RIGHT || keyCode == KeyEvent.KEYCODE_DPAD_RIGHT || keyCode == ReaderView.NOOK_KEY_SHIFT_DOWN)
                dir = 1;
            if (keyCode == ReaderView.NOOK_KEY_PREV_RIGHT || keyCode == KeyEvent.KEYCODE_DPAD_LEFT || keyCode == ReaderView.NOOK_KEY_SHIFT_UP)
                dir = -1;
        }
        if (dir != 0) {
            int willFit = getChildCount();
            int currentPos = getFirstVisiblePosition();
            int nextPos = ( dir > 0 ) ? Math.min(currentPos + willFit, getCount() - 1) : Math.max(0, currentPos - willFit);
            setSelection(nextPos);  
            clearFocus();       
            return true;
        }
        return super.onKeyDown(keyCode, event);
	}
}
