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
}
