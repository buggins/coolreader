/*
 * CoolReader for Android
 * Copyright (C) 2011,2012 Vadim Lopatin <coolreader.org@gmail.com>
 * Copyright (C) 2018 Yuri Plotnikov <plotnikovya@gmail.com>
 * Copyright (C) 2018,2021 Aleksey Chernov <valexlin@gmail.com>
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

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.text.method.LinkMovementMethod;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.TabHost;
import android.widget.TabHost.TabContentFactory;
import android.widget.TextView;

import org.coolreader.BuildConfig;
import org.coolreader.CoolReader;
import org.coolreader.R;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.Random;

public class AboutDialog extends BaseDialog implements TabContentFactory {
	final CoolReader mCoolReader;
	
	private View mAppTab;
	private View mDirsTab;
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
			btn.setOnClickListener(v -> installPackage(packageName));
		}
	}
	
	private void setupInAppDonationButton( final Button btn, final double amount ) {
		btn.setText("$" + amount);
		btn.setOnClickListener(v -> mCoolReader.makeDonation(amount));
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
		mAppTab = inflater.inflate(R.layout.about_dialog_app, null);
		((TextView)mAppTab.findViewById(R.id.version)).setText("Cool Reader " + mCoolReader.getVersion());

		mDirsTab = inflater.inflate(R.layout.about_dialog_dirs, null);
		TextView fonts_dir = mDirsTab.findViewById(R.id.fonts_dirs);

		ArrayList<String> fontsDirs = Engine.getFontsDirs();
		StringBuilder sbuf = new StringBuilder();
		Iterator<String> it = fontsDirs.iterator();
		while (it.hasNext()) {
			String s = it.next();
			sbuf.append(s);
			if (it.hasNext()) {
				sbuf.append("\n");
			}
		}
		fonts_dir.setText(sbuf.toString());

		ArrayList<String> testuresDirs = Engine.getDataDirs(Engine.DataDirType.TexturesDirs);
		sbuf = new StringBuilder();
		it = testuresDirs.iterator();
		while (it.hasNext()) {
			String s = it.next();
			sbuf.append(s);
			if (it.hasNext()) {
				sbuf.append("\n");
			}
		}
		TextView textures_dir = mDirsTab.findViewById(R.id.textures_dirs);
		textures_dir.setText(sbuf.toString());

		ArrayList<String> backgroundsDirs = Engine.getDataDirs(Engine.DataDirType.BackgroundsDirs);
		sbuf = new StringBuilder();
		it = backgroundsDirs.iterator();
		while (it.hasNext()) {
			String s = it.next();
			sbuf.append(s);
			if (it.hasNext()) {
				sbuf.append("\n");
			}
		}
		TextView backgrounds_dir = mDirsTab.findViewById(R.id.backgrounds_dirs);
		backgrounds_dir.setText(sbuf.toString());

		ArrayList<String> hyphDirs = Engine.getDataDirs(Engine.DataDirType.HyphsDirs);
		sbuf = new StringBuilder();
		it = hyphDirs.iterator();
		while (it.hasNext()) {
			String s = it.next();
			sbuf.append(s);
			if (it.hasNext()) {
				sbuf.append("\n");
			}
		}
		TextView hyph_dir = mDirsTab.findViewById(R.id.hyph_dirs);
		hyph_dir.setText(sbuf.toString());

		mLicenseTab = inflater.inflate(R.layout.about_dialog_license, null);
		String license = Engine.getInstance(mCoolReader).loadResourceUtf8(R.raw.license);
		((TextView)mLicenseTab.findViewById(R.id.license)).setText(license);
		boolean billingSupported = mCoolReader.isDonationSupported();

		if (billingSupported) {
			mDonationTab = inflater.inflate(R.layout.about_dialog_donation2, null);
			setupInAppDonationButton(mDonationTab.findViewById(R.id.btn_about_donation_install_vip), 100);
			setupInAppDonationButton(mDonationTab.findViewById(R.id.btn_about_donation_install_platinum), 30);
			setupInAppDonationButton(mDonationTab.findViewById(R.id.btn_about_donation_install_gold), 10);
			setupInAppDonationButton(mDonationTab.findViewById(R.id.btn_about_donation_install_silver), 3);
			setupInAppDonationButton(mDonationTab.findViewById(R.id.btn_about_donation_install_bronze), 1);
			setupInAppDonationButton(mDonationTab.findViewById(R.id.btn_about_donation_install_iron), 0.3);
			updateTotalDonations();
			mCoolReader.setDonationListener(total -> updateTotalDonations());
			setOnDismissListener(dialog -> mCoolReader.setDonationListener(null));
		} else {
			mDonationTab = inflater.inflate(R.layout.about_dialog_donation, null);
			if (BuildConfig.GSUITE_AVAILABLE) {
				setupDonationButton(mDonationTab.findViewById(R.id.btn_about_donation_install_gold), "org.coolreader.donation.gold");
				setupDonationButton(mDonationTab.findViewById(R.id.btn_about_donation_install_silver), "org.coolreader.donation.silver");
				setupDonationButton(mDonationTab.findViewById(R.id.btn_about_donation_install_bronze), "org.coolreader.donation.bronze");
			}
			TextView paypalLink = mDonationTab.findViewById(R.id.paypalLink);
			if (null != paypalLink) {
				paypalLink.setMovementMethod(LinkMovementMethod.getInstance());
			}
		}
		
		tabs.setup();
		TabHost.TabSpec tsApp = tabs.newTabSpec("App");
		tsApp.setIndicator("", 
				getContext().getResources().getDrawable(R.drawable.cr3_menu_link));
		tsApp.setContent(this);
		tabs.addTab(tsApp);

		TabHost.TabSpec tsDirectories = tabs.newTabSpec("Directories");
		tsDirectories.setIndicator("",
				getContext().getResources().getDrawable(R.drawable.ic_menu_archive));
		tsDirectories.setContent(this);
		tabs.addTab(tsDirectories);

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
			tabs.setCurrentTab(3);
		
	}
	private static Random rnd = new Random(android.os.SystemClock.uptimeMillis()); 

	
	@Override
	public View createTabContent(String tag) {

		if ( "App".equals(tag) )
			return mAppTab;
		else if ( "Directories".equals(tag) )
			return mDirsTab;
		else if ( "License".equals(tag) )
			return mLicenseTab;
		else if ( "Donation".equals(tag) )
			return mDonationTab;
		return null;
	}
	
}
