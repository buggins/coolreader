package org.coolreader.crengine;

import java.io.File;

import android.app.Activity;

public class BaseActivity extends Activity {

	
	
	public Properties loadSettings(int profile) {
		return SettingsManager.instance(this).loadSettings(profile);
	}
	
	public void saveSettings(int profile, Properties settings) {
		SettingsManager.instance(this).saveSettings(profile, settings);
	}
	
	public void saveSettings(File f, Properties settings)
	{
		SettingsManager.instance(this).saveSettings(f, settings);
	}

	public void saveSettings(Properties settings)
	{
		SettingsManager.instance(this).saveSettings(settings);
	}
}
