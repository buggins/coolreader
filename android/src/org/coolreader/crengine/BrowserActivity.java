package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.R;
import org.coolreader.crengine.CRToolBar.OnActionHandler;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

public class BrowserActivity extends BaseActivity {

	static class BrowserViewLayout extends ViewGroup {
		private FileBrowser contentView;
		private View titleView;
		private CRToolBar toolbarView;
		public BrowserViewLayout(Context context, FileBrowser contentView, CRToolBar toolbar, View titleView) {
			super(context);
			this.contentView = contentView;
			
			
			this.titleView = titleView;
			this.titleView.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
			this.toolbarView = toolbar;
			this.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
			this.addView(titleView);
			this.addView(toolbarView);
			this.addView(contentView);
		}

		@Override
		protected void onLayout(boolean changed, int l, int t, int r, int b) {
			r -= l;
			b -= t;
			t = 0;
			l = 0;
			int titleHeight = titleView.getMeasuredHeight();
			if (toolbarView.isVertical()) {
				int tbWidth = toolbarView.getMeasuredWidth();
				titleView.layout(l + tbWidth, t, r, t + titleHeight);
				toolbarView.layout(l, t, l + tbWidth, b);
				contentView.layout(l + tbWidth, t + titleHeight, r, b);
			} else {
				int tbHeight = toolbarView.getMeasuredHeight();
				toolbarView.layout(l, t, r, t + tbHeight);
				titleView.layout(l, t + tbHeight, r, t + titleHeight + tbHeight);
				contentView.layout(l, t + titleHeight + tbHeight, r, b);
			}
		}
		
		@Override
		protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
			int w = MeasureSpec.getSize(widthMeasureSpec);
			int h = MeasureSpec.getSize(heightMeasureSpec);

			
			toolbarView.setVertical(w > h);
			if (w > h) {
				// landscape
				toolbarView.setVertical(true);
				toolbarView.measure(widthMeasureSpec, heightMeasureSpec);
				int tbWidth = toolbarView.getMeasuredWidth();
				titleView.measure(MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, w - tbWidth), 
						MeasureSpec.makeMeasureSpec(MeasureSpec.UNSPECIFIED, 0));
				int titleHeight = titleView.getMeasuredHeight();
				contentView.measure(MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, w - tbWidth), 
						MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, h - titleHeight));
			} else {
				// portrait
				toolbarView.setVertical(false);
				titleView.measure(widthMeasureSpec, 
						MeasureSpec.makeMeasureSpec(MeasureSpec.UNSPECIFIED, 0));
				toolbarView.measure(widthMeasureSpec, 
						MeasureSpec.makeMeasureSpec(MeasureSpec.UNSPECIFIED, 0));
				int tbHeight = toolbarView.getMeasuredHeight();
				int titleHeight = titleView.getMeasuredHeight();
				contentView.measure(widthMeasureSpec, 
						MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, h - titleHeight - tbHeight));
			}
	        setMeasuredDimension(w, h);
		}
		
		@Override
		protected void onSizeChanged(int w, int h, int oldw, int oldh) {
			super.onSizeChanged(w, h, oldw, oldh);
		}
	}

	public FileBrowser getBrowser() { return mBrowser; }
	
	private FileBrowser mBrowser;
	private Engine mEngine;
	private BrowserViewLayout mFrame;
	private View mTitleBar;
	private CRToolBar mToolBar;
	
	private ArrayList<ReaderAction> createActionList(ReaderAction ... actions) {
		ArrayList<ReaderAction> list = new ArrayList<ReaderAction>(actions.length);
		for (ReaderAction item : actions)
			list.add(item);
		return list;
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Activities.setBrowser(this);
		super.onCreate(savedInstanceState);
		mEngine = Engine.getInstance(this);
		
		mBrowser = new FileBrowser(this, Services.getEngine(), Services.getScanner(), Services.getHistory());
		mBrowser.setCoverPagesEnabled(SettingsManager.instance(this).getBool(ReaderView.PROP_APP_SHOW_COVERPAGES, true));
		mBrowser.setCoverPageFontFace(SettingsManager.instance(this).getSetting(ReaderView.PROP_FONT_FACE, DeviceInfo.DEF_FONT_FACE));
		mBrowser.setCoverPageSizeOption(SettingsManager.instance(this).getInt(ReaderView.PROP_APP_COVERPAGE_SIZE, 1));
        mBrowser.setSortOrder(SettingsManager.instance(this).getSetting(ReaderView.PROP_APP_BOOK_SORT_ORDER));
		mBrowser.setSimpleViewMode(SettingsManager.instance(this).getBool(ReaderView.PROP_APP_FILE_BROWSER_SIMPLE_MODE, false));
        mBrowser.init();
        mBrowser.showDirectory(Services.getScanner().getRoot(), null);

		LayoutInflater inflater = LayoutInflater.from(this);// activity.getLayoutInflater();
		
		mTitleBar = inflater.inflate(R.layout.browser_status_bar, null);
        ((TextView)mTitleBar.findViewById(R.id.title)).setText("Cool Reader browser window");

        mToolBar = new CRToolBar(this, createActionList(
        		ReaderAction.FILE_BROWSER_UP, 
        		ReaderAction.FILE_BROWSER_ROOT, 
        		ReaderAction.OPDS_CATALOGS, 
        		ReaderAction.RECENT_BOOKS,
        		ReaderAction.CURRENT_BOOK,
        		ReaderAction.CURRENT_BOOK_DIRECTORY,
        		ReaderAction.SEARCH,
        		ReaderAction.OPTIONS
        		));
        mToolBar.setBackgroundColor(0x20404040);
        mToolBar.setOnItemSelectedHandler(new OnActionHandler() {
			@Override
			public boolean onActionSelected(ReaderAction item) {
				switch (item.cmd) {
				case DCMD_FILE_BROWSER_ROOT:
					Activities.showRootWindow();
				case DCMD_FILE_BROWSER_UP:
					mBrowser.showParentDirectory();
					break;
				case DCMD_OPDS_CATALOGS:
					mBrowser.showOPDSRootDirectory();
					break;
				case DCMD_RECENT_BOOKS_LIST:
					mBrowser.showRecentBooks();
					break;
				case DCMD_SEARCH:
					mBrowser.showFindBookDialog();
					break;
				case DCMD_CURRENT_BOOK:
					BookInfo bi = Services.getHistory().getLastBook();
					if (bi != null)
						Activities.loadDocument(bi.getFileInfo());
					break;
				case DCMD_OPTIONS_DIALOG:
					//mBrowser.
					// TODO: open browser options dialog
					break;
				}
				return false;
			}
		});
		mFrame = new BrowserViewLayout(this, mBrowser, mToolBar, mTitleBar);
		setContentView(mFrame);
	}

	
	@Override
	protected void onStart() {
		super.onStart();
		
		Intent intent = getIntent();
		if (intent != null) {
			String dir = intent.getExtras() != null ? intent.getExtras().getString(Activities.OPEN_DIR_PARAM) : null;
			if (dir != null) {
				mBrowser.showDirectory(Services.getScanner().pathToFileInfo(dir), null);
			}
		}
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

	public void directoryUpdated(FileInfo dir) {
		mBrowser.refreshDirectory(dir);
	}
}
