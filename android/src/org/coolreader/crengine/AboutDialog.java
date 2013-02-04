package org.coolreader.crengine;

import java.util.Random;

import org.coolreader.CoolReader;
import org.coolreader.CoolReader.DonationListener;
import org.coolreader.R;

import android.content.ActivityNotFoundException;
import android.content.DialogInterface;
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
	
	private void setupInAppDonationButton( final Button btn, final double amount ) {
		btn.setText("$" + amount);
		btn.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				mCoolReader.makeDonation(amount);
			}
		});
	}
	
	private void updateTotalDonations() {
		double amount = mCoolReader.getTotalDonations();
		if (isPackageInstalled("org.coolreader.donation.gold"))
			amount += 10.0;
		if (isPackageInstalled("org.coolreader.donation.silver"))
			amount += 3.0;
		if (isPackageInstalled("org.coolreader.donation.bronze"))
			amount += 1.0;
		TextView text = ((TextView)mDonationTab.findViewById(R.id.btn_about_donation_total));
		if (text != null)
			text.setText(mCoolReader.getString(R.string.dlg_about_donation_total) + " $" + amount);
	}

	public AboutDialog( CoolReader activity)
	{
		super(activity);
		mCoolReader = activity;
		setTitle(R.string.dlg_about);
		LayoutInflater inflater = LayoutInflater.from(getContext());
		TabHost tabs = (TabHost)inflater.inflate(R.layout.about_dialog, null);
		mAppTab = (View)inflater.inflate(R.layout.about_dialog_app, null);
		((TextView)mAppTab.findViewById(R.id.version)).setText("Cool Reader " + mCoolReader.getVersion());
		mLicenseTab = (View)inflater.inflate(R.layout.about_dialog_license, null);
		String license = Engine.getInstance(mCoolReader).loadResourceUtf8(R.raw.license);
		((TextView)mLicenseTab.findViewById(R.id.license)).setText(license);
		boolean billingSupported = mCoolReader.isDonationSupported();
		mDonationTab = (View)inflater.inflate(billingSupported ? R.layout.about_dialog_donation2 : R.layout.about_dialog_donation, null);

		if (billingSupported) {
			setupInAppDonationButton( (Button)mDonationTab.findViewById(R.id.btn_about_donation_install_vip), 100);
			setupInAppDonationButton( (Button)mDonationTab.findViewById(R.id.btn_about_donation_install_platinum), 30);
			setupInAppDonationButton( (Button)mDonationTab.findViewById(R.id.btn_about_donation_install_gold), 10);
			setupInAppDonationButton( (Button)mDonationTab.findViewById(R.id.btn_about_donation_install_silver), 3);
			setupInAppDonationButton( (Button)mDonationTab.findViewById(R.id.btn_about_donation_install_bronze), 1);
			setupInAppDonationButton( (Button)mDonationTab.findViewById(R.id.btn_about_donation_install_iron), 0.3);
			updateTotalDonations();
			mCoolReader.setDonationListener(new DonationListener() {
				@Override
		    	public void onDonationTotalChanged(double total) {
		    		updateTotalDonations();
		    	}
		    });
			setOnDismissListener(new OnDismissListener() {
				@Override
				public void onDismiss(DialogInterface dialog) {
					mCoolReader.setDonationListener(null);
				}
			});
		} else {
			setupDonationButton( (Button)mDonationTab.findViewById(R.id.btn_about_donation_install_gold), "org.coolreader.donation.gold");
			setupDonationButton( (Button)mDonationTab.findViewById(R.id.btn_about_donation_install_silver), "org.coolreader.donation.silver");
			setupDonationButton( (Button)mDonationTab.findViewById(R.id.btn_about_donation_install_bronze), "org.coolreader.donation.bronze");
		}
		
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

		// 25% chance to show Donations tab
		if ((rnd.nextInt() & 3) == 3)
			tabs.setCurrentTab(2);
		
	}
	private static Random rnd = new Random(android.os.SystemClock.uptimeMillis()); 

	
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
