package org.coolreader.crengine;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.view.LayoutInflater;
import android.view.View;
import android.widget.TabHost;
import android.widget.TabHost.TabContentFactory;
import android.widget.TextView;

public class AboutDialog extends BaseDialog implements TabContentFactory {
	final CoolReader mCoolReader;
	
	private View mAppTab;
	private View mLicenseTab;
	
	public AboutDialog( CoolReader activity)
	{
		super(activity, 0, 0, false);
		mCoolReader = activity;
		//setTitle(mCoolReader.getString(R.string.dlg_about));
		LayoutInflater inflater = LayoutInflater.from(getContext());
		TabHost tabs = (TabHost)inflater.inflate(R.layout.about_dialog, null);
		mAppTab = (View)inflater.inflate(R.layout.about_dialog_app, null);
		((TextView)mAppTab.findViewById(R.id.version)).setText("Cool Reader " + mCoolReader.getVersion());
		mLicenseTab = (View)inflater.inflate(R.layout.about_dialog_license, null);
		String license = mCoolReader.getEngine().loadResourceUtf8(R.raw.license);
		((TextView)mLicenseTab.findViewById(R.id.license)).setText(license);
		tabs.setup();
		TabHost.TabSpec tsApp = tabs.newTabSpec("App");
		tsApp.setIndicator("", 
				getContext().getResources().getDrawable(R.drawable.cr3_menu_link));
		tsApp.setContent(this);
		tabs.addTab(tsApp);

		TabHost.TabSpec tsLicense = tabs.newTabSpec("License");
		tsLicense.setIndicator("", 
				getContext().getResources().getDrawable(R.drawable.ic_menu_star));
		tsLicense.setContent(this);
		tabs.addTab(tsLicense);
		
		setView( tabs );
	}



	@Override
	public View createTabContent(String tag) {
		if ( "App".equals(tag) )
			return mAppTab;
		else if ( "License".equals(tag) )
			return mLicenseTab;
		return null;
	}
}
