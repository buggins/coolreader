/*
 * CoolReader for Android
 * Copyright (C) 2021 Aleksey Chernov <valexlin@gmail.com>
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
import android.view.View;

import java.util.List;

public interface EinkScreen {

	enum EinkUpdateMode {
		Unspecified(-1),
		Regal(3),				// also known as 'SNOW Field'
		Clear(0),				// old name CMODE_CLEAR
		Fast(1),				// old name CMODE_ONESHOT
		Active(2),			// old name CMODE_ACTIVE
		A2(4),				// Fast 'A2' mode
		;

		public static EinkUpdateMode byCode(int code) {
			for (EinkUpdateMode mode : EinkUpdateMode.values()) {
				if (mode.code == code)
					return mode;
			}
			return EinkUpdateMode.Unspecified;
		}

		EinkUpdateMode(int code) {
			this.code = code;
		}

		public final int code;
	}

	void setupController(EinkUpdateMode mode, int updateInterval, View view);

	void prepareController(View view, boolean isPartially);

	void updateController(View view, boolean isPartially);

	void refreshScreen(View view);

	EinkUpdateMode getUpdateMode();

	int getUpdateInterval();

	int getFrontLightValue(Context context);

	boolean setFrontLightValue(Context context, int value);

	int getWarmLightValue(Context context);

	boolean setWarmLightValue(Context context, int value);

	List<Integer> getFrontLightLevels(Context context);

	List<Integer> getWarmLightLevels(Context context);

	boolean isAppOptimizationEnabled();
}
