/*
 * CoolReader for Android
 * Copyright (C) 2011,2012,2014 Vadim Lopatin <coolreader.org@gmail.com>
 * Copyright (C) 2012 Michael Berganovsky <mike0berg@gmail.com>
 * Copyright (C) 2012 Cipi Bad
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

import android.content.Context;
import android.graphics.Rect;
import android.view.KeyEvent;
import android.view.View;
import android.widget.ListView;

public class BaseListView  extends ListView {
	public BaseListView(Context context, boolean fastScrollEnabled) {
		super(context);
        setFocusable(true);
        setFocusableInTouchMode(true);
        setFastScrollEnabled(fastScrollEnabled);
	}

	@Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        int dir = 0;
        if (keyCode == 0)
            keyCode = event.getScanCode();
        //if (DeviceInfo.SONY_NAVIGATION_KEYS) {
        if (keyCode == ReaderView.SONY_DPAD_RIGHT_SCANCODE || keyCode == ReaderView.SONY_DPAD_DOWN_SCANCODE || keyCode==KeyEvent.KEYCODE_DPAD_DOWN || keyCode == KeyEvent.KEYCODE_DPAD_RIGHT)
            dir = 1;
        else if (keyCode == ReaderView.SONY_DPAD_LEFT_SCANCODE || keyCode == ReaderView.SONY_DPAD_UP_SCANCODE || keyCode==KeyEvent.KEYCODE_DPAD_UP || keyCode == KeyEvent.KEYCODE_DPAD_LEFT )
            dir = -1;
        //} else {
        else if (keyCode == KeyEvent.KEYCODE_8 || keyCode == ReaderView.NOOK_KEY_NEXT_RIGHT || keyCode == KeyEvent.KEYCODE_DPAD_RIGHT || keyCode == ReaderView.NOOK_KEY_SHIFT_DOWN)
            dir = 1;
        else if (keyCode == KeyEvent.KEYCODE_2 || keyCode == ReaderView.NOOK_KEY_PREV_RIGHT || keyCode == KeyEvent.KEYCODE_DPAD_LEFT || keyCode == ReaderView.NOOK_KEY_SHIFT_UP)
            dir = -1;
        //}
        if (dir != 0) {            
            int firstPos = getFirstVisiblePosition();
            int lastPos  = getLastVisiblePosition();
            int count    = getCount();
            
            int delta = 1;
            if( dir < 0 ) {
                View v = getChildAt(0);
                if( v != null ) {
                    int fh = v.getHeight();
                    Rect r = new Rect(0,0,v.getWidth(),fh);
                    getChildVisibleRect(v, r, null);
                    delta = (r.height() < fh) ? 1 : 0;
                }
            }
                      
            int nextPos = ( dir > 0 ) ? Math.min(lastPos + 1, count - 1) : Math.max(0, firstPos - (lastPos - firstPos) + delta);
            
            // Log.w("CoolReader", "first =" + firstPos + " last = " + lastPos + " next = " + nextPos + " count = " + count);
            
            setSelection(nextPos);  
            clearFocus();
            
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }
}
