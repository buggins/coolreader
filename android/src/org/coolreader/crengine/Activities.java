package org.coolreader.crengine;

import org.coolreader.CoolReader;

import android.content.Intent;

public class Activities {
	
	public static final Logger log = L.create("aa");
	
	private static CoolReader mainActivity;
	private static ReaderActivity readerActivity;
	private static BrowserActivity browserActivity;
	public static int activityCount() {
		int count = (mainActivity != null ? 1 : 0)
				+ (readerActivity != null ? 1 : 0)
				+ (browserActivity != null ? 1 : 0);
		return count;
	}
	private static void onChange(BaseActivity activity) {
		if (activity != null && activityCount() == 1) {
			Services.onFirstActivityCreated(activity);
		} else if (activity == null && activityCount() == 0) {
			Services.onLastActivityDestroyed();
		}
	}
	public static void setMain(CoolReader coolReader) {
		mainActivity = coolReader;
		onChange(coolReader);
	}
	public static CoolReader getMain() {
		return mainActivity;
	}
	public static ReaderActivity getReader() {
		return readerActivity;
	}
	public static BrowserActivity getBrowser() {
		return browserActivity;
	}
	public static void setReader(ReaderActivity reader) {
		readerActivity = reader;
		onChange(reader);
	}
	public static void setBrowser(BrowserActivity browser) {
		browserActivity = browser;
		onChange(browser);
	}

	public static void showReader() {
		log.d("Activities.showReader()");
	}

	public static void loadDocument( FileInfo item )
	{
		loadDocument(item, null);
	}
	
	public static void loadDocument( FileInfo item, Runnable callback )
	{
		log.d("Activities.loadDocument(" + item.pathname + ")");
		loadDocument(item.getPathName(), null);
		// TODO: load document
		//showView(readerView);
		//setContentView(readerView);
		//mReaderView.loadDocument(item, null);
	}
	
	public static String getString(int resourceId) {
		BaseActivity activity = getCurrentActivity();
		if (activity != null) {
			return activity.getString(resourceId);
		} else {
			return "unknown string";
		}
	}
	
	public static BaseActivity getCurrentActivity() {
		if (mainActivity != null)
			return mainActivity;
		if (browserActivity != null)
			return browserActivity;
		if (readerActivity != null)
			return readerActivity;
		return null;
	}
	
	public static void startActivity(Class<?> activityClass, String paramName, String paramValue) {
		BaseActivity activity = getCurrentActivity();
		if (activity != null) {
			Intent intent = new Intent(activity.getApplicationContext(), activityClass);
			if (paramName != null)
				intent.putExtra(paramName, paramValue);
			activity.startActivity(intent);
		}
	}

	public static final String OPEN_FILE_PARAM = "FILE_TO_OPEN";
	public static void loadDocument( String item, Runnable callback )
	{
		startActivity(ReaderActivity.class, OPEN_FILE_PARAM, item);
	}
	
	public static void showRootWindow() {
		startActivity(CoolReader.class, null, null);
	}
	
	public static final String OPEN_DIR_PARAM = "DIR_TO_OPEN";
	public static void showBrowser(FileInfo dir) {
		startActivity(BrowserActivity.class, OPEN_DIR_PARAM, dir.getPathName());
	}
	
	public static void showRecentBooks() {
		log.d("Activities.showRecentBooks() is called");
		startActivity(BrowserActivity.class, OPEN_DIR_PARAM, FileInfo.RECENT_DIR_TAG);
	}

	public static void showOnlineCatalogs() {
		log.d("Activities.showOnlineCatalogs() is called");
		startActivity(BrowserActivity.class, OPEN_DIR_PARAM, FileInfo.OPDS_LIST_TAG);
	}

	public static void showDirectory(FileInfo path) {
		log.d("Activities.showDirectory(" + path + ") is called");
		startActivity(BrowserActivity.class, OPEN_DIR_PARAM, path.getPathName());
	}

	public static void showCatalog(FileInfo path) {
		log.d("Activities.showCatalog(" + path + ") is called");
		startActivity(BrowserActivity.class, OPEN_DIR_PARAM, path.getPathName());
	}

	
	public static void editOPDSCatalog(FileInfo opds) {
		if (opds==null) {
			opds = new FileInfo();
			opds.isDirectory = true;
			opds.pathname = FileInfo.OPDS_DIR_PREFIX + "http://";
			opds.filename = "New Catalog";
			opds.isListed = true;
			opds.isScanned = true;
			opds.parent = Services.getScanner().getOPDSRoot();
		}
		OPDSCatalogEditDialog dlg = new OPDSCatalogEditDialog(getCurrentActivity(), opds, new Runnable() {
			@Override
			public void run() {
				refreshOPDSRootDirectory();
			}
		});
		dlg.show();
	}
	
	public static void refreshOPDSRootDirectory() {
		if (browserActivity != null)
			browserActivity.getBrowser().refreshOPDSRootDirectory();
		if (mainActivity != null)
			mainActivity.refreshOnlineCatalogs();
	}
	
	public static void applyAppSetting( String key, String value )
	{
		if (mainActivity != null)
			mainActivity.applyAppSetting(key, value);
		if (readerActivity != null)
			readerActivity.applyAppSetting(key, value);
	}

	public static boolean isBookOpened() {
		if (readerActivity == null)
			return false;
		return readerActivity.getReaderView().isBookLoaded();
	}

	public static void closeBookIfOpened(FileInfo book) {
		if (readerActivity == null)
			return;
		readerActivity.getReaderView().closeIfOpened(book);
	}
	
	public static void saveSetting(String name, String value) {
		if (readerActivity != null)
			readerActivity.getReaderView().saveSetting(name, value);
	}
	
}
