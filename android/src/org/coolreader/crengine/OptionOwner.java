package org.coolreader.crengine;

import android.view.LayoutInflater;

public interface OptionOwner {
	public BaseActivity getActivity();
	public Properties getProperties();
	public LayoutInflater getInflater();
}
