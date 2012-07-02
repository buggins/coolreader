package org.coolreader.crengine;

import org.coolreader.CoolReader;
import org.coolreader.crengine.ReaderActivity.ReaderViewLayout;

import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;

public class BrowserActivity extends BaseActivity {

	static class BrowserViewLayout extends ViewGroup {
		private View contentView;
		public BrowserViewLayout(Context context, View contentView) {
			super(context);
			this.contentView = contentView;
		}

		@Override
		protected void onLayout(boolean changed, int l, int t, int r, int b) {
			contentView.layout(l, t, r, b);
		}
	}

	public FileBrowser getBrowser() { return mBrowser; }
	
	private FileBrowser mBrowser;
	private Engine mEngine;
	private BrowserViewLayout mFrame;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Activities.setBrowser(this);
		mEngine = Engine.getInstance(this);
		
		mBrowser = new FileBrowser(this, Services.getEngine(), Services.getScanner(), Services.getHistory());
		mBrowser.setCoverPagesEnabled(SettingsManager.instance(this).getBool(ReaderView.PROP_APP_SHOW_COVERPAGES, true));
		mBrowser.setCoverPageFontFace(SettingsManager.instance(this).getSetting(ReaderView.PROP_FONT_FACE, DeviceInfo.DEF_FONT_FACE));
		mBrowser.setCoverPageSizeOption(SettingsManager.instance(this).getInt(ReaderView.PROP_APP_COVERPAGE_SIZE, 1));
        mBrowser.setSortOrder(SettingsManager.instance(this).getSetting(ReaderView.PROP_APP_BOOK_SORT_ORDER));
		mBrowser.setSimpleViewMode(SettingsManager.instance(this).getBool(ReaderView.PROP_APP_FILE_BROWSER_SIMPLE_MODE, false));
        mBrowser.init();
        mBrowser.showDirectory(Services.getScanner().getRoot(), null);
		mFrame = new BrowserViewLayout(this, mBrowser);
		super.onCreate(savedInstanceState);
	}

	@Override
	protected void onStart() {
		// TODO Auto-generated method stub
		super.onStart();
	}

	@Override
	protected void onStop() {
		// TODO Auto-generated method stub
		super.onStop();
	}

	@Override
	protected void onPause() {
		// TODO Auto-generated method stub
		super.onPause();
	}

	@Override
	protected void onResume() {
		// TODO Auto-generated method stub
		super.onResume();
	}

	@Override
	protected void onDestroy() {
		mBrowser.onClose();
		super.onDestroy();
		Activities.setBrowser(null);
	}

	public void setCurrentTheme(InterfaceTheme theme) {
		super.setCurrentTheme(theme);
		if (mBrowser != null)
			mBrowser.onThemeChanged();
	}

	public void applyAppSetting( String key, String value )
	{
		super.applyAppSetting(key, value);
		boolean flg = "1".equals(value);
        if (key.equals(PROP_APP_BOOK_SORT_ORDER)) {
        	if (getBrowser() != null)
        		getBrowser().setSortOrder(value);
        } else if (key.equals(PROP_APP_FILE_BROWSER_SIMPLE_MODE)) {
        	if (getBrowser() != null)
        		getBrowser().setSimpleViewMode(flg);
        } else if ( key.equals(PROP_APP_SHOW_COVERPAGES) ) {
        	if (getBrowser() != null)
        		getBrowser().setCoverPagesEnabled(flg);
        } else if ( key.equals(PROP_APP_BOOK_PROPERTY_SCAN_ENABLED) ) {
        	Services.getScanner().setDirScanEnabled(flg);
        } else if ( key.equals(PROP_FONT_FACE) ) {
        	if (getBrowser() != null)
        		getBrowser().setCoverPageFontFace(value);
        } else if ( key.equals(PROP_APP_COVERPAGE_SIZE) ) {
        	int n = 0;
        	try {
        		n = Integer.parseInt(value);
        	} catch (NumberFormatException e) {
        		// ignore
        	}
        	if (n < 0)
        		n = 0;
        	else if (n > 2)
        		n = 2;
        	if (getBrowser() != null)
        		getBrowser().setCoverPageSizeOption(n);
        } else if ( key.equals(PROP_APP_FILE_BROWSER_SIMPLE_MODE) ) {
        	if (getBrowser()!=null )
        		getBrowser().setSimpleViewMode(flg);
        } else if ( key.equals(PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS) ) {
        	Services.getScanner().setHideEmptyDirs(flg);
        }
        //
	}
	
}
