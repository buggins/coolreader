package org.coolreader.crengine;

import org.coolreader.CoolReader;

public class Activities {
	private static CoolReader mainActivity;
	private static ReaderActivity readerActivity;
	public static void setMain(CoolReader coolReader) {
		mainActivity = coolReader;
	}
	public static CoolReader getMain() {
		return mainActivity;
	}
	public static ReaderActivity getReader() {
		return readerActivity;
	}
	public static void setReader(ReaderActivity reader) {
		readerActivity = reader;
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

}
