package org.coolreader.crengine;

import org.coolreader.crengine.Engine.HyphDict;

public class Services {

	public static final Logger log = L.create("sv");
	
	private static Engine mEngine;
	private static Scanner mScanner;
	private static History mHistory;
	private static CoverpageManager mCoverpageManager;

	public static Engine getEngine() { return mEngine; }
	public static Scanner getScanner() { return mScanner; }
	public static History getHistory() { return mHistory; }
	public static CoverpageManager getCoverpageManager() { return mCoverpageManager; }
	
	static void onFirstActivityCreated(BaseActivity activity) {
		log.i("First activity is created");
		// testing background thread
		mEngine = Engine.getInstance(activity);
		
        String code = SettingsManager.instance(activity).getSetting(ReaderView.PROP_HYPHENATION_DICT, Engine.HyphDict.RUSSIAN.toString());
        Engine.HyphDict dict = HyphDict.byCode(code);
		mEngine.setHyphenationDictionary(dict);
		
       	mScanner = new Scanner(activity, mEngine);
       	mScanner.initRoots(mEngine.getMountedRootsMap());

       	mHistory = new History(mScanner);
		mScanner.setDirScanEnabled(SettingsManager.instance(activity).getBool(ReaderView.PROP_APP_BOOK_PROPERTY_SCAN_ENABLED, true));
		mCoverpageManager = new CoverpageManager();
	}
	static void onLastActivityDestroyed() {
		log.i("Last activity is destroyed");
		if (mCoverpageManager == null) {
			log.i("Will not destroy services: finish only activity creation detected");
			return;
		}
		mCoverpageManager.clear();
		BackgroundThread.instance().postBackground(new Runnable() {
			@Override
			public void run() {
				log.i("Stopping background thread");
				mEngine.uninit();
				BackgroundThread.instance().quit();
				mEngine = null;
			}
		});
		mHistory = null;
		mScanner = null;
		mCoverpageManager = null;
	}
}
