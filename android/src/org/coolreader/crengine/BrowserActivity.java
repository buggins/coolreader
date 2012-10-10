package org.coolreader.crengine;

import org.coolreader.R;
import org.coolreader.crengine.CRToolBar.OnActionHandler;

import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

public class BrowserActivity extends BaseActivity {

	public static final Logger log = L.create("ba");
	
	public FileBrowser getBrowser() { return mBrowser; }
	
	private String title = "";
	public void setTitle(String title) {
		this.title = title;
		if (mBrowserTitleBar != null)
			((TextView)mBrowserTitleBar.findViewById(R.id.title)).setText(title);
	}
	
	private FileBrowser mBrowser;
	private BrowserViewLayout mBrowserFrame;
	private View mBrowserTitleBar;
	private CRToolBar mBrowserToolBar;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Activities.setBrowser(this);
		super.onCreate(savedInstanceState);
		

		
	}

	@Override
	protected void onNewIntent(Intent intent) {
		log.i("onNewIntent : " + intent);
		processIntent(intent);
	}

	protected void processIntent(Intent intent) {
		final String dir = intent.getExtras() != null ? intent.getExtras().getString(Activities.OPEN_DIR_PARAM) : null;
		if (dir != null) {
			// postpone until DB service created
			waitForCRDBService(new Runnable() {
				@Override
				public void run() {
					mBrowser.showDirectory(Services.getScanner().pathToFileInfo(dir), null);
				}
			});
		}
	}
	
	boolean firstStart = true;
	@Override
	protected void onStart() {
		super.onStart();
		
		Intent intent = getIntent();
		if (intent != null && firstStart) {
			processIntent(intent);
		}
		firstStart = false;
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
		if (mBrowserFrame != null)
			mBrowserFrame.onThemeChanged(theme);
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

	public void directoryUpdated(FileInfo dir) {
		mBrowser.refreshDirectory(dir);
	}
}
