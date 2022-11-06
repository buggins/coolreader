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

public class EinkScreenDummy implements EinkScreen {
	@Override
	public void setupController(EinkUpdateMode mode, int updateInterval, View view) {
	}

	@Override
	public void prepareController(View view, boolean isPartially) {
	}

	@Override
	public void updateController(View view, boolean isPartially) {
	}

	@Override
	public void refreshScreen(View view) {
	}

	@Override
	public EinkUpdateMode getUpdateMode() {
		return EinkUpdateMode.Unspecified;
	}

	@Override
	public int getUpdateInterval() {
		return 0;
	}

	@Override
	public int getFrontLightValue(Context context) {
		return 0;
	}

	@Override
	public boolean setFrontLightValue(Context context, int value) {
		return false;
	}

	@Override
	public int getWarmLightValue(Context context) {
		return 0;
	}

	@Override
	public boolean setWarmLightValue(Context context, int value) {
		return false;
	}

	@Override
	public List<Integer> getFrontLightLevels(Context context) {
		return null;
	}

	@Override
	public List<Integer> getWarmLightLevels(Context context) {
		return null;
	}

	@Override
	public boolean isAppOptimizationEnabled() {
		return false;
	}
}
