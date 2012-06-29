package org.coolreader.crengine;

import org.coolreader.CoolReader;

import android.view.ViewGroup;

public class CRRootView extends ViewGroup {

	private final CoolReader mActivity;
	public CRRootView(CoolReader activity) {
		super(activity);
		this.mActivity = activity;
	}

	@Override
	protected void onLayout(boolean changed, int l, int t, int r, int b) {
		//
	}

}
