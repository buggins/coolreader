package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.R;
import org.coolreader.crengine.CRToolBar.OnActionHandler;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

public class BrowserActivity extends BaseActivity {

	public static final Logger log = L.create("ba");
	
	static class BrowserViewLayout extends ViewGroup {
		private BaseActivity activity;
		private FileBrowser contentView;
		private View titleView;
		private CRToolBar toolbarView;
		public BrowserViewLayout(BaseActivity context, FileBrowser contentView, CRToolBar toolbar, View titleView) {
			super(context);
			this.activity = context;
			this.contentView = contentView;
			
			
			this.titleView = titleView;
			this.titleView.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
			this.toolbarView = toolbar;
			this.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
			this.addView(titleView);
			this.addView(toolbarView);
			this.addView(contentView);
			this.onThemeChanged(context.getCurrentTheme());
			titleView.setFocusable(false);
			titleView.setFocusableInTouchMode(false);
			toolbarView.setFocusable(false);
			toolbarView.setFocusableInTouchMode(false);
			contentView.setFocusable(false);
			contentView.setFocusableInTouchMode(false);
		}
		
		public void onThemeChanged(InterfaceTheme theme) {
			titleView.setBackgroundResource(theme.getBrowserStatusBackground());
			//toolbarView.setButtonAlpha(theme.getToolbarButtonAlpha());
			toolbarView.setBackgroundResource(theme.getBrowserToolbarBackground(toolbarView.isVertical()));
			toolbarView.onThemeChanged(theme);
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
				toolbarView.setBackgroundResource(activity.getCurrentTheme().getBrowserToolbarBackground(true));
			} else {
				int tbHeight = toolbarView.getMeasuredHeight();
				toolbarView.layout(l, t, r, t + tbHeight);
				titleView.layout(l, t + tbHeight, r, t + titleHeight + tbHeight);
				contentView.layout(l, t + titleHeight + tbHeight, r, b);
				toolbarView.setBackgroundResource(activity.getCurrentTheme().getBrowserToolbarBackground(false));
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
				toolbarView.measure(MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, w), 
						MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, h));
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
	
	private String title = "";
	public void setTitle(String title) {
		this.title = title;
		if (mTitleBar != null)
			((TextView)mTitleBar.findViewById(R.id.title)).setText(title);
	}
	
	private FileBrowser mBrowser;
	private BrowserViewLayout mFrame;
	private View mTitleBar;
	private CRToolBar mToolBar;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Activities.setBrowser(this);
		super.onCreate(savedInstanceState);
		
		waitForCRDBService(new Runnable() {
			@Override
			public void run() {
				mBrowser = new FileBrowser(BrowserActivity.this, Services.getEngine(), Services.getScanner(), Services.getHistory());
				mBrowser.setCoverPagesEnabled(SettingsManager.instance(BrowserActivity.this).getBool(ReaderView.PROP_APP_SHOW_COVERPAGES, true));
				mBrowser.setCoverPageFontFace(SettingsManager.instance(BrowserActivity.this).getSetting(ReaderView.PROP_FONT_FACE, DeviceInfo.DEF_FONT_FACE));
				mBrowser.setCoverPageSizeOption(SettingsManager.instance(BrowserActivity.this).getInt(ReaderView.PROP_APP_COVERPAGE_SIZE, 1));
		        mBrowser.setSortOrder(SettingsManager.instance(BrowserActivity.this).getSetting(ReaderView.PROP_APP_BOOK_SORT_ORDER));
				mBrowser.setSimpleViewMode(SettingsManager.instance(BrowserActivity.this).getBool(ReaderView.PROP_APP_FILE_BROWSER_SIMPLE_MODE, false));
		        mBrowser.init();

				LayoutInflater inflater = LayoutInflater.from(BrowserActivity.this);// activity.getLayoutInflater();
				
				mTitleBar = inflater.inflate(R.layout.browser_status_bar, null);
				setTitle("Cool Reader browser window");

		        mToolBar = new CRToolBar(BrowserActivity.this, ReaderAction.createList(
		        		ReaderAction.FILE_BROWSER_UP, 
		        		ReaderAction.CURRENT_BOOK,
		        		ReaderAction.CURRENT_BOOK_DIRECTORY,
		        		ReaderAction.FILE_BROWSER_ROOT, 
		        		ReaderAction.OPTIONS,
		        		ReaderAction.RECENT_BOOKS,
		        		ReaderAction.OPDS_CATALOGS,
		        		ReaderAction.SEARCH,
						ReaderAction.EXIT
		        		));
		        mToolBar.setBackgroundResource(R.drawable.ui_status_background_browser_dark);
		        mToolBar.setOnActionHandler(new OnActionHandler() {
					@Override
					public boolean onActionSelected(ReaderAction item) {
						switch (item.cmd) {
						case DCMD_EXIT:
							Activities.finish();
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
							showBrowserOptionsDialog();
							break;
						}
						return false;
					}
				});
				mFrame = new BrowserViewLayout(BrowserActivity.this, mBrowser, mToolBar, mTitleBar);
				setContentView(mFrame);

				if (getIntent() == null)
					mBrowser.showDirectory(Services.getScanner().getDownloadDirectory(), null);
			}
		});

		
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
		if (mFrame != null)
			mFrame.onThemeChanged(theme);
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
