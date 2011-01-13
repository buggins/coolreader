package org.coolreader.crengine;

import org.coolreader.CoolReader;

import android.view.LayoutInflater;

public interface OptionOwner {
	public CoolReader getActivity();
	public Properties getProperties();
	public LayoutInflater getInflater();
}
