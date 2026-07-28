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
import android.util.DisplayMetrics;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ListView;

public class BaseListView  extends ListView {

    //The Values were originally tested on a 440dpi screen, hence the conversion factor in the constructor
    private float SWIPE_THRESHOLD_PX = 100f;

    private float NO_MOVE_THRESHOLD_PX = 5f;

    private float touchStartY = 0f;


    public BaseListView(Context context, boolean fastScrollEnabled) {
        super(context);
        setFocusable(true);
        setFocusableInTouchMode(true);
        setFastScrollEnabled(fastScrollEnabled);
        DisplayMetrics displayMetrics = getResources().getDisplayMetrics();
        //The Values were originally tested on a 440dpi screen, hence the conversion factor
        final float dpiDependentFactor = displayMetrics.ydpi / 440;
        SWIPE_THRESHOLD_PX = Math.round(dpiDependentFactor * SWIPE_THRESHOLD_PX);
        NO_MOVE_THRESHOLD_PX = Math.round(dpiDependentFactor * NO_MOVE_THRESHOLD_PX);
    }

    /** This method is designed to allow the ListView to scroll page-wise.
     *  FileBrowser.MyGestureListener or any other listener should not be impaired by this.
     * */

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
            boolean returnValue = false;

        if (DeviceInfo.PAGEWISE_SCROLLING) {
            switch (ev.getAction()) {
                //Start registering the events.
                //Allow ACTION_DOWN to be passed normally to the ListView
                //to keep the click/longclick functionality
                case MotionEvent.ACTION_DOWN:
                    touchStartY = ev.getY();
                    returnValue = super.onTouchEvent(ev);
                    break;

                case MotionEvent.ACTION_MOVE:
                    /**
                     * Interrupt normal scrolling, when NO_MOVE_THRESHOLD_PX is reached.
                     * Once the movement starts, claim the following events.
                     * NO_MOVE_THRESHOLD_PX is there to allow for some little movement when
                     * performing a click.
                     */

                    //TODO: NO_MOVE_THRESHOLD_PX may need some testing and fine tuning
                    returnValue = true;
                    if (Math.abs(touchStartY - ev.getY()) >= NO_MOVE_THRESHOLD_PX) {
                        ev.setAction(MotionEvent.ACTION_CANCEL);
                    }

                    super.onTouchEvent(ev);
                    break;

                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    /**
                     * Scroll page-wise when SWIPE_THRESHOLD_PX is reached.
                     */
                    //TODO maybe tweak SWIPE_THRESHOLD_PX too, current values are rather experimental
                    super.onTouchEvent(ev);

                    if (Math.abs(touchStartY - ev.getY()) >= SWIPE_THRESHOLD_PX) {  //delta is positive = swiped up (next page)
                        if (touchStartY - ev.getY() > 0) {
                            scrollPage(1);
                        } else {
                            scrollPage(-1);
                        }
                        returnValue = true;
                    }
                    break;
            }
        } else {
            returnValue = super.onTouchEvent(ev);
        }
        return returnValue;
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
        else if (keyCode == KeyEvent.KEYCODE_8 || keyCode == ReaderView.NOOK_KEY_NEXT_RIGHT || keyCode == KeyEvent.KEYCODE_DPAD_RIGHT || keyCode == ReaderView.NOOK_KEY_SHIFT_DOWN || keyCode == KeyEvent.KEYCODE_PAGE_DOWN)
            dir = 1;
        else if (keyCode == KeyEvent.KEYCODE_2 || keyCode == ReaderView.NOOK_KEY_PREV_RIGHT || keyCode == KeyEvent.KEYCODE_DPAD_LEFT || keyCode == ReaderView.NOOK_KEY_SHIFT_UP || keyCode == KeyEvent.KEYCODE_PAGE_UP)
            dir = -1;
        //}
        if (dir != 0) {
            scrollPage(dir);
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    /** Scroll the ListView page-wise.
     * Logic is:
     * 1. Scrolling down - the last (bottom) not fully visible view should be the first
     *      fully displayed view on the next page.
     * 2. Scrolling up - the first (top) not fully visible view should be the last
     *      fully displayed view on the previous page
     * */
    private void scrollPage(int dir) {
        int firstPos = getFirstVisiblePosition();
        int lastPos  = getLastVisiblePosition();
        int count    = getCount();

        int delta = 0;
        int newTopItem = 0;
        if( dir < 0 ) {
            //Check if the currently displayed top childview is fully visible or incomplete
            View v = getChildAt(0);
            if( v != null ) {
                int fh = v.getHeight();
                Rect r = new Rect(0,0,v.getWidth(),fh);
                getChildVisibleRect(v, r, null);
                delta = (r.height() < fh) ? 1 : 0;
            }

            //Scrolling up, account for when the items can have different heights
            // (e.g. by using a larger font, breaking into more lines, etc.)
            newTopItem = Math.max(0, firstPos - 1 + delta);
            Rect visibleListViewRect = new Rect();
            this.getGlobalVisibleRect(visibleListViewRect);
            int visibleHeight = visibleListViewRect.height();
            int usableHeight = 0;
            for (int i = newTopItem; i >= 0;  i--) {
                //getchildAt() returns null if the item is not visible.
                //Ask adapter directly to get the children parameters
                View view = getAdapter().getView(i, null, this);
                /**
                 * Because normally ListViews are measured lazy, view.getMeasuredHeight() can return 0
                 * when the view hasn't gone through a layout pass yet.
                 * Inflate and measure each item view, forcing a layout pass.
                 * MeasureSpec.UNSPECIFIED for height lets the view report its natural height,
                 * and MeasureSpec.EXACTLY for width constrains it to the real ListView width
                 * so it wraps correctly.
                 */
                view.measure( View.MeasureSpec.makeMeasureSpec(this.getWidth(), MeasureSpec.EXACTLY),
                              View.MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));
                usableHeight += view.getMeasuredHeight() + getDividerHeight();
                if (usableHeight > visibleHeight) {
                    //use last view that fits fully into the next page as top view
                    break;
                }
                newTopItem = i;
            }
        }

        int nextPos = ( dir > 0 ) ? Math.min(lastPos, count - 1) : Math.max(0, newTopItem) ;

        setSelection(nextPos);
        clearFocus();
    }
}
