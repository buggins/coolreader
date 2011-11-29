package org.coolreader.crengine;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.TabHost;
import android.widget.TabHost.TabContentFactory;
import android.widget.TextView;

public class AboutDialog extends BaseDialog implements TabContentFactory {
	final CoolReader mCoolReader;
	
	private View mAppTab;
	private View mLicenseTab;
	private View mDonationTab;
	
	private boolean isPackageInstalled( String packageName ) {
		try {
			mCoolReader.getPackageManager().getApplicationInfo(packageName, 0);
			return true;
		} catch ( Exception e ) {
			return false;
		}
	}

	private void installPackage( String packageName ) {
		Log.i("cr3", "installPackageL " + packageName);
		try {
			mCoolReader.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=" + packageName)));
		} catch ( ActivityNotFoundException e ) {
			mCoolReader.showToast("Cannot run Android Market application");
		}
	}
	
	private void setupDonationButton( final Button btn, final String packageName ) {
		if ( isPackageInstalled(packageName)) {
			btn.setEnabled(false);
			btn.setText(R.string.dlg_about_donation_installed);
		} else {
			btn.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					installPackage(packageName);
				}
			});
		}
	}
	
	public AboutDialog( CoolReader activity)
	{
		super(activity);
		mCoolReader = activity;
		LayoutInflater inflater = LayoutInflater.from(getContext());
		TabHost tabs = (TabHost)inflater.inflate(R.layout.about_dialog, null);
		mAppTab = (View)inflater.inflate(R.layout.about_dialog_app, null);
		((TextView)mAppTab.findViewById(R.id.version)).setText("Cool Reader " + mCoolReader.getVersion());
		mLicenseTab = (View)inflater.inflate(R.layout.about_dialog_license, null);
		String license = mCoolReader.getEngine().loadResourceUtf8(R.raw.license);
		((TextView)mLicenseTab.findViewById(R.id.license)).setText(license);
		mDonationTab = (View)inflater.inflate(R.layout.about_dialog_donation, null);

		setupDonationButton( (Button)mDonationTab.findViewById(R.id.btn_about_donation_install_gold), "org.coolreader.donation.gold");
		setupDonationButton( (Button)mDonationTab.findViewById(R.id.btn_about_donation_install_silver), "org.coolreader.donation.silver");
		setupDonationButton( (Button)mDonationTab.findViewById(R.id.btn_about_donation_install_bronze), "org.coolreader.donation.bronze");
		
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
		
		TabHost.TabSpec tsDonation = tabs.newTabSpec("Donation");
		tsDonation.setIndicator("", 
				getContext().getResources().getDrawable(R.drawable.ic_menu_emoticons));
		tsDonation.setContent(this);
		tabs.addTab(tsDonation);
		
		setView( tabs );
	}

	@Override
	public View createTabContent(String tag) {
		if ( "App".equals(tag) )
			return mAppTab;
		else if ( "License".equals(tag) )
			return mLicenseTab;
		else if ( "Donation".equals(tag) )
			return mDonationTab;
		return null;
	}
	
}
