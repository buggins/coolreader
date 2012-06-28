package org.coolreader.crengine;

import org.coolreader.CoolReader;

public class Activities {
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
		// TODO: implement
	}

	public static void loadDocument( FileInfo item )
	{
		// TODO: load document
		//showView(readerView);
		//setContentView(readerView);
		//mReaderView.loadDocument(item, null);
	}
	
	public static void loadDocument( FileInfo item, Runnable callback )
	{
		// TODO: load document
		//showView(readerView);
		//setContentView(readerView);
		//mReaderView.loadDocument(item, null);
	}
	
	public static void loadDocument( String item, Runnable callback )
	{
		// TODO: load document
		//showView(readerView);
		//setContentView(readerView);
		//mReaderView.loadDocument(item, null);
	}
	
	public static void showBrowser(FileInfo dir) {
		// TODO: implement
	}
	
	public static void showBrowserRecentBooks() {
		// TODO: implement
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
