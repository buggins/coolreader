package org.coolreader.db;

import java.io.File;
import java.util.ArrayList;

import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.Environment;
import android.os.IBinder;

public class CRDBService extends Service {
	public static final Logger log = L.create("db");

    private MainDB mainDB = new MainDB();
    private CoverDB coverDB = new CoverDB();
	
    @Override
    public void onCreate() {
    	log.i("onCreate()");
    	mThread = new ServiceThread("crdb");
    	mThread.post(new OpenDatabaseTask());
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        log.i("Received start id " + startId + ": " + intent);
        // We want this service to continue running until it is explicitly
        // stopped, so return sticky.
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
    	log.i("onDestroy()");
    	mThread.post(new CloseDatabaseTask());
    	mThread.stop(5000);
    }

    private static File getDatabaseDir() {
    	File storage = Environment.getExternalStorageDirectory();
    	File cr3dir = new File(storage, ".cr3");
    	if (cr3dir.isDirectory())
    		cr3dir.mkdirs();
    	if (!cr3dir.isDirectory() || !cr3dir.canWrite()) {
	    	log.w("Cannot use " + cr3dir + " for writing database, will use data directory instead");
    		cr3dir = Environment.getDataDirectory();
    	}
    	log.i("DB directory: " + cr3dir);
    	return cr3dir;
    }
    
	final String SQLITE_DB_NAME = "cr3db.sqlite";
	final String SQLITE_COVER_DB_NAME = "cr3db_cover.sqlite";
    private class OpenDatabaseTask implements Runnable {
    	
		@Override
		public void run() {
	    	log.i("OpenDatabaseTask started");
	    	open();
	    	log.i("OpenDatabaseTask finished");
		}

		private boolean open() {
	    	File dir = getDatabaseDir();
	    	boolean res = mainDB.open(dir);
	    	res = coverDB.open(dir) && res;
	    	if (!res) {
	    		mainDB.close();
	    		coverDB.close();
	    	}
	    	return res;
	    }
	    
    }

    private class CloseDatabaseTask implements Runnable {
		@Override
		public void run() {
	    	log.i("OpenDatabaseTask started");
	    	close();
	    	log.i("OpenDatabaseTask finished");
		}

		private void close() {
			clearCaches();
    		mainDB.close();
    		coverDB.close();
	    }
    }
    
    private void clearCaches() {
		synchronized (coverpageCache) {
			coverpageCache.clear();
		}
    }

    public static class FileInfoCache {
    	private ArrayList<FileInfo> list = new ArrayList<FileInfo>();
    	public void add(FileInfo item) {
    		list.add(item);
    	}
    	public void clear() {
    		list.clear();
    	}
    }
    
	//=======================================================================================
    // OPDS catalogs access code
    //=======================================================================================
    public interface OPDSCatalogsLoadingCallback {
    	void onOPDSCatalogsLoaded(ArrayList<FileInfo> catalogs);
    }
    
	public void saveOPDSCatalog(final Long id, final String url, final String name) {
		mThread.post(new Runnable() {
			@Override
			public void run() {
				mainDB.saveOPDSCatalog(id, url, name);
			}
		});
	}

	public void loadOPDSCatalogs(final OPDSCatalogsLoadingCallback callback) {
		mThread.post(new Runnable() {
			@Override
			public void run() {
				ArrayList<FileInfo> list = new ArrayList<FileInfo>(); 
				mainDB.loadOPDSCatalogs(list);
				callback.onOPDSCatalogsLoaded(list);
			}
		});
	}

	//=======================================================================================
    // coverpage DB access code
    //=======================================================================================
    public interface CoverpageLoadingCallback {
    	void onCoverpageLoaded(long bookId, byte[] data);
    }
    
    private static final int COVERPAGE_CACHE_SIZE = 512 * 1024;
    private ByteArrayCache coverpageCache = new ByteArrayCache(COVERPAGE_CACHE_SIZE);
	public void saveBookCoverpage(final long bookId, final byte[] data) {
		if (data == null)
			return;
		synchronized (coverpageCache) {
			byte[] oldData = coverpageCache.get(bookId);
			if (oldData != null)
				return; // already in cache
			// update cache and DB
			coverpageCache.put(bookId, data);
		}
		mThread.post(new Runnable() {
			@Override
			public void run() {
				coverDB.saveBookCoverpage(bookId, data);
			}
		});
	}
	
	public void loadBookCoverpage(final long bookId, final CoverpageLoadingCallback callback) 
	{
		byte[] data = null;
		synchronized (coverpageCache) {
			data = coverpageCache.get(bookId);
		}
		if (data != null) {
			callback.onCoverpageLoaded(bookId, data);
			return;
		}
		mThread.post(new Runnable() {
			@Override
			public void run() {
				byte[] data = coverDB.loadBookCoverpage(bookId);
				if (data != null) {
					synchronized (coverpageCache) {
						coverpageCache.put(bookId, data);
					}
				}
				callback.onCoverpageLoaded(bookId, data);
			}
		});
	}
	
	public void deleteCoverpage(final long bookId) {
		mThread.post(new Runnable() {
			@Override
			public void run() {
				coverDB.deleteCoverpage(bookId);
				synchronized (coverpageCache) {
					coverpageCache.remove(bookId);
				}
			}
		});
	}

	//=======================================================================================
    // Item groups access code
    //=======================================================================================
    public interface ItemGroupsLoadingCallback {
    	void onItemGroupsLoaded(FileInfo parent);
    }
    
	public void loadAuthorsList(FileInfo parent, final ItemGroupsLoadingCallback callback) {
		final FileInfo p = new FileInfo(parent); 
		mThread.post(new Runnable() {
			@Override
			public void run() {
				mainDB.loadAuthorsList(p);
				callback.onItemGroupsLoaded(p);
			}
		});
	}

	public void loadSeriesList(FileInfo parent, final ItemGroupsLoadingCallback callback) {
		final FileInfo p = new FileInfo(parent); 
		mThread.post(new Runnable() {
			@Override
			public void run() {
				mainDB.loadSeriesList(p);
				callback.onItemGroupsLoaded(p);
			}
		});
	}
	
	public void loadTitleList(FileInfo parent, final ItemGroupsLoadingCallback callback) {
		final FileInfo p = new FileInfo(parent); 
		mThread.post(new Runnable() {
			@Override
			public void run() {
				mainDB.loadTitleList(p);
				callback.onItemGroupsLoaded(p);
			}
		});
	}
	
	/**
     * Class for clients to access.  Because we know this service always
     * runs in the same process as its clients, we don't need to deal with
     * IPC.
     */
    public class LocalBinder extends Binder {
        public CRDBService getService() {
            return CRDBService.this;
        }
        
    	public void saveBookCoverpage(long bookId, byte[] data) {
    		getService().saveBookCoverpage(bookId, data);
    	}
    	
    	public void deleteBookCoverpage(long bookId) {
    		getService().deleteCoverpage(bookId);
    	}

    	public void loadBookCoverpage(final long bookId, final CoverpageLoadingCallback callback) {
    		getService().loadBookCoverpage(bookId, callback);
    	}
    	
    	public void loadOPDSCatalogs(final OPDSCatalogsLoadingCallback callback) {
    		getService().loadOPDSCatalogs(callback);
    	}

    	public void loadAuthorsList(FileInfo parent, final ItemGroupsLoadingCallback callback) {
    		getService().loadAuthorsList(parent, callback);
    	}

    	public void loadSeriesList(FileInfo parent, final ItemGroupsLoadingCallback callback) {
    		getService().loadSeriesList(parent, callback);
    	}
    	
    	public void loadTitleList(FileInfo parent, final ItemGroupsLoadingCallback callback) {
    		getService().loadTitleList(parent, callback);
    	}
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    private ServiceThread mThread;
    private final IBinder mBinder = new LocalBinder();
    
}
