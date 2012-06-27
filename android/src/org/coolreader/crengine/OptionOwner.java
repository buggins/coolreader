package org.coolreader.crengine;

import org.coolreader.CoolReader;

import android.view.LayoutInflater;

public interface OptionOwner {
	public ReaderActivity getActivity();
	public Properties getProperties();
	public LayoutInflater getInflater();
}
