package org.coolreader.crengine;

import java.io.File;

import org.coolreader.crengine.Engine.HyphDict;
import org.coolreader.db.CRDBService;
import org.coolreader.db.CRDBServiceAccessor;
import org.coolreader.sync.SyncServiceAccessor;

public class Services {

	public static final Logger log = L.create("sv");
	
	private static Engine mEngine;
	private static Scanner mScanner;
	private static SyncServiceAccessor mSyncService;
	private static CRDBServiceAccessor mCRDBService;
	private static History mHistory;

	public static Engine getEngine() { return mEngine; }
	public static Scanner getScanner() { return mScanner; }
	public static SyncServiceAccessor getSyncService() { return mSyncService; }
	public static CRDBServiceAccessor getDBService() { return mCRDBService; }
	public static CRDBService.LocalBinder getDB() { return mCRDBService.get(); }
	public static History getHistory() { return mHistory; }
	
	static void onFirstActivityCreated(BaseActivity activity) {
		// testing background thread
		mEngine = Engine.getInstance(activity);
		
        String code = SettingsManager.instance(activity).getSetting(ReaderView.PROP_HYPHENATION_DICT, Engine.HyphDict.RUSSIAN.toString());
        Engine.HyphDict dict = HyphDict.byCode(code);
		mEngine.setHyphenationDictionary(dict);
		
       	mScanner = new Scanner(activity, mEngine);
       	mScanner.initRoots(mEngine.getMountedRootsMap());

       	mSyncService = new SyncServiceAccessor(activity);
		mSyncService.bind(new Runnable() {
			@Override
			public void run() {
				log.i("Initialization after SyncService is bound");
				BackgroundThread.instance().postGUI(new Runnable() {
					@Override
					public void run() {
						FileInfo downloadDirectory = mScanner.getDownloadDirectory();
						if (downloadDirectory != null)
			        	mSyncService.setSyncDirectory(new File(downloadDirectory.getPathName()));
					}
				});
			}
		});
		mCRDBService = new CRDBServiceAccessor(activity, mEngine.getPathCorrector());
        mCRDBService.bind(new Runnable() {
			@Override
			public void run() {
				log.i("Initialization after SyncService is bound");
				mHistory.loadFromDB(200);
			}
        });
       	mHistory = new History(mScanner);
		mScanner.setDirScanEnabled(SettingsManager.instance(activity).getBool(ReaderView.PROP_APP_BOOK_PROPERTY_SCAN_ENABLED, true));
	}
	static void onLastActivityDestroyed() {
		
		mCRDBService.unbind();
		mSyncService.unbind();
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
		mEngine = null;
	}
}
