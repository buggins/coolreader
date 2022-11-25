/*
 * CoolReader for Android
 * Copyright (C) 2014 Vadim Lopatin <coolreader.org@gmail.com>
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

import android.view.View;

public class EinkScreenTolino extends EinkScreenNook {

	@Override
	public void prepareController(View view, boolean isPartially) {
		//System.err.println("Sleep = " + isPartially);
		if (isPartially || mIsSleep != isPartially) {
			tolinoSleepController(isPartially, view);
//			if (isPartially)
			return;
		}
		if (mRefreshNumber == -1) {
			switch (mUpdateMode) {
				case Clear:
					tolinoSetMode(view, mUpdateMode);
					break;
				case Active:
					if (mUpdateInterval == 0) {
						tolinoSetMode(view, mUpdateMode);
					}
					break;
			}
			mRefreshNumber = 0;
			return;
		}
		if (mUpdateMode == EinkUpdateMode.Clear) {
			tolinoSetMode(view, mUpdateMode);
			return;
		}
		if (mUpdateInterval > 0 || mUpdateMode == EinkUpdateMode.Fast) {
			if (mRefreshNumber == 0 || (mUpdateMode == EinkUpdateMode.Fast && mRefreshNumber < mUpdateInterval)) {
				switch (mUpdateMode) {
					case Active:
						tolinoSetMode(view, mUpdateMode);
						break;
					case Fast:
						tolinoSetMode(view, mUpdateMode);
						break;
				}
			} else if (mUpdateInterval <= mRefreshNumber) {
				tolinoSetMode(view, EinkUpdateMode.Clear);
				mRefreshNumber = -1;
			}
			if (mUpdateInterval > 0) {
				mRefreshNumber++;
			}
		}
	}


	// private methods
	private void tolinoSleepController(boolean toSleep, View view) {
		if (toSleep != mIsSleep) {
			log.d("+++SleepController " + toSleep);
			mIsSleep = toSleep;
			if (mIsSleep) {
				switch (mUpdateMode) {
					case Clear:
						break;
					case Fast:
						break;
					case Active:
						tolinoSetMode(view, EinkUpdateMode.Clear);
						mRefreshNumber = -1;
				}
			} else {
				setupController(mUpdateMode, mUpdateInterval, view);
			}
		}
	}

	private void tolinoSetMode(View view, EinkUpdateMode mode) {
		TolinoEpdController.setMode(view, mode);
	}
}
