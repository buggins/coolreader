/*
 * CoolReader for Android
 * Copyright (C) 2014 fuero <the_master_of_disaster@gmx.at>
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

import android.graphics.Rect;
import android.view.View;

import de.telekom.epub.utils.ScreenHelper;

public class TolinoEpdController {
	private static final String LOG_TAG = TolinoEpdController.class.getSimpleName();

	public static boolean partialRefresh(View view, int mode, int waveform) {
		Rect r = new Rect();
		view.getGlobalVisibleRect(r);
		return partialRefresh(r.left, r.top, r.right, r.bottom, mode, waveform);
	}

	public static boolean partialRefresh(int left, int top, int right, int bottom, int mode, int waveform) {
		if ((right - left > 1) && (bottom - top > 1)) {
			return ScreenHelper.RegionalRefresh(left, top, right, bottom, mode, waveform) == 1;
		}
		return false;
	}

	public static void setMode(View view, EinkScreen.EinkUpdateMode mode) {
		switch (mode) {
			case Clear:
				ScreenHelper.FullRefresh();
				break;
			case Fast:
				partialRefresh(view, ScreenHelper.NATIVE_UPDATE_MODE_FULL, ScreenHelper.NATIVE_WAVEFORM_MODE_GC16);
				break;
			case Active:
				partialRefresh(view, ScreenHelper.NATIVE_UPDATE_MODE_FULL, ScreenHelper.NATIVE_WAVEFORM_MODE_A2);
				break;
		}
	}
}
