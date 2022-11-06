/*
 * CoolReader for Android
 * Copyright (C) 2012 Vadim Lopatin <coolreader.org@gmail.com>
 * Copyright (C) 2013 Alexey Kabelitskiy <akabelytskyi@hmstn.com>
 * Copyright (C) 2018,2020,2021 Aleksey Chernov <valexlin@gmail.com>
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

import android.os.Handler;

import org.coolreader.genrescollection.GenresCollection;

public class Services {

	public static final Logger log = L.create("sv");

	private static Engine mEngine;
	private static Scanner mScanner;
	private static History mHistory;
	private static CoverpageManager mCoverpageManager;
	private static FileSystemFolders mFSFolders;
	private static GenresCollection mGenresCollection;
	private static DocumentFileCache mDocumentCache;

	public static Engine getEngine() {
		if (null != mEngine)
			return mEngine;
		throw new RuntimeException("Services.getEngine(): trying to get null object");
	}

	public static Scanner getScanner() {
		if (null != mScanner)
			return mScanner;
		throw new RuntimeException("Services.getScanner(): trying to get null object");
	}

	public static History getHistory() {
		if (null != mHistory)
			return mHistory;
		throw new RuntimeException("Services.getHistory(): trying to get null object");
	}

	public static CoverpageManager getCoverpageManager() {
		if (null != mCoverpageManager)
			return mCoverpageManager;
		throw new RuntimeException("Services.getCoverpageManager(): trying to get null object");
	}

	public static FileSystemFolders getFileSystemFolders() {
		if (null != mFSFolders)
			return mFSFolders;
		throw new RuntimeException("Services.getFileSystemFolders(): trying to get null object");
	}

	public static GenresCollection getGenresCollection() {
		if (null != mGenresCollection)
			return mGenresCollection;
		throw new RuntimeException("Services.getGenresCollection(): trying to get null object");
	}

	public static DocumentFileCache getDocumentCache() {
		if (null != mDocumentCache)
			return mDocumentCache;
		throw new RuntimeException("Services.getDocumentCache(): trying to get null object");
	}

	public static boolean isStopped() {
		return null == mEngine || null == mScanner || null == mHistory || null == mCoverpageManager || null == mFSFolders || null == mGenresCollection || null == mDocumentCache;
	}

	public static void startServices(BaseActivity activity) {
		log.i("First activity is created");
		// testing background thread
		//mSettings = activity.settings();
		BackgroundThread.instance().setGUIHandler(new Handler());
		mEngine = Engine.getInstance(activity);
		mScanner = new Scanner(activity, mEngine);
		mScanner.initRoots(Engine.getMountedRootsMap(), mEngine.getAppPrivateDirs());
		mHistory = new History(mScanner);
		mScanner.setDirScanEnabled(activity.settings().getBool(ReaderView.PROP_APP_BOOK_PROPERTY_SCAN_ENABLED, true));
		mCoverpageManager = new CoverpageManager();
		mFSFolders = new FileSystemFolders(mScanner);
		mGenresCollection = GenresCollection.getInstance(activity);
		mDocumentCache = new DocumentFileCache(activity);
	}

	// called after user grant permissions for external storage
	public static void refreshServices(BaseActivity activity) {
		mEngine.initAgain();
		mScanner.initRoots(Engine.getMountedRootsMap(), mEngine.getAppPrivateDirs());
	}

	public static void stopServices() {
		log.i("Last activity is destroyed");
		if (mCoverpageManager == null) {
			log.i("Will not destroy services: finish only activity creation detected");
			return;
		}
		mCoverpageManager.clear();
		BackgroundThread.instance().postBackground(() -> {
			log.i("Stopping background thread");
			if (mEngine == null)
				return;
			mEngine.uninit();
			BackgroundThread.instance().quit();
			mEngine = null;
		});
		mHistory = null;
		mScanner = null;
		mCoverpageManager = null;
		mFSFolders = null;
		mGenresCollection = null;
		mDocumentCache = null;
	}
}
