package org.coolreader.db;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;

import org.coolreader.crengine.BookInfo;
import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;
import org.coolreader.crengine.Utils;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;

public class CRDBService extends Service {
	public static final Logger log = L.create("db");

    private MainDB mainDB = new MainDB();
    private CoverDB coverDB = new CoverDB();
	
    @Override
    public void onCreate() {
    	log.i("onCreate()");
    	mThread = new ServiceThread("crdb");
    	execTask(new OpenDatabaseTask());
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
    	execTask(new CloseDatabaseTask());
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
    
    private FlushDatabaseTask lastFlushTask;
    private class FlushDatabaseTask implements Runnable {
    	private boolean force;
    	public FlushDatabaseTask(boolean force) {
    		this.force = force;
    		lastFlushTask = this;
    	}
		@Override
		public void run() {
			long elapsed = Utils.timeInterval(lastFlushTime);
			if (force || (lastFlushTask == this && elapsed > MIN_FLUSH_INTERVAL)) {
		    	mainDB.flush();
		    	coverDB.flush();
		    	lastFlushTime = Utils.timeStamp();
			}
		}
    }
    
    private void clearCaches() {
		mainDB.clearCaches();
		coverDB.clearCaches();
    }

    private static final long MIN_FLUSH_INTERVAL = 15000;
    private long lastFlushTime;
    
    /**
     * Schedule flush.
     */
    private void flush() {
   		execTask(new FlushDatabaseTask(false), MIN_FLUSH_INTERVAL);
    }

    /**
     * Flush ASAP.
     */
    private void forceFlush() {
   		execTask(new FlushDatabaseTask(true));
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
		execTask(new Runnable() {
			@Override
			public void run() {
				mainDB.saveOPDSCatalog(id, url, name);
			}
		});
	}

	public void loadOPDSCatalogs(final OPDSCatalogsLoadingCallback callback, final Handler handler) {
		execTask(new Runnable() {
			@Override
			public void run() {
				final ArrayList<FileInfo> list = new ArrayList<FileInfo>(); 
				mainDB.loadOPDSCatalogs(list);
				sendTask(handler, new Runnable() {
					@Override
					public void run() {
						callback.onOPDSCatalogsLoaded(list);
					}
				});
			}
		});
	}

	//=======================================================================================
    // coverpage DB access code
    //=======================================================================================
    public interface CoverpageLoadingCallback {
    	void onCoverpageLoaded(long bookId, byte[] data);
    }
    
	public void saveBookCoverpage(final long bookId, final byte[] data) {
		if (data == null)
			return;
		execTask(new Runnable() {
			@Override
			public void run() {
				coverDB.saveBookCoverpage(bookId, data);
			}
		});
		flush();
	}
	
	public void loadBookCoverpage(final long bookId, final CoverpageLoadingCallback callback, final Handler handler) 
	{
		execTask(new Runnable() {
			@Override
			public void run() {
				final byte[] data = coverDB.loadBookCoverpage(bookId);
				sendTask(handler, new Runnable() {
					@Override
					public void run() {
						callback.onCoverpageLoaded(bookId, data);
					}
				});
			}
		});
	}
	
	public void deleteCoverpage(final long bookId) {
		execTask(new Runnable() {
			@Override
			public void run() {
				coverDB.deleteCoverpage(bookId);
			}
		});
		flush();
	}

	//=======================================================================================
    // Item groups access code
    //=======================================================================================
    public interface ItemGroupsLoadingCallback {
    	void onItemGroupsLoaded(FileInfo parent);
    }

    public interface FileInfoListLoadingCallback {
    	void onFileInfoListLoaded(long authorId, ArrayList<FileInfo> list);
    }
    
    public interface FileInfoLoadingCallback {
    	void onFileInfoListLoaded(ArrayList<FileInfo> list);
    }
    
    public interface RecentBooksLoadingCallback {
    	void onRecentBooksListLoaded(ArrayList<BookInfo> bookList);
    }
    
    public interface BookSearchCallback {
    	void onBooksFound(ArrayList<FileInfo> fileList);
    }
    
	public void loadAuthorsList(FileInfo parent, final ItemGroupsLoadingCallback callback, final Handler handler) {
		final FileInfo p = new FileInfo(parent); 
		execTask(new Runnable() {
			@Override
			public void run() {
				mainDB.loadAuthorsList(p);
				sendTask(handler, new Runnable() {
					@Override
					public void run() {
						callback.onItemGroupsLoaded(p);
					}
				});
			}
		});
	}

	public void loadSeriesList(FileInfo parent, final ItemGroupsLoadingCallback callback, final Handler handler) {
		final FileInfo p = new FileInfo(parent); 
		execTask(new Runnable() {
			@Override
			public void run() {
				mainDB.loadSeriesList(p);
				sendTask(handler, new Runnable() {
					@Override
					public void run() {
						callback.onItemGroupsLoaded(p);
					}
				});
			}
		});
	}
	
	public void loadTitleList(FileInfo parent, final ItemGroupsLoadingCallback callback, final Handler handler) {
		final FileInfo p = new FileInfo(parent); 
		execTask(new Runnable() {
			@Override
			public void run() {
				mainDB.loadTitleList(p);
				sendTask(handler, new Runnable() {
					@Override
					public void run() {
						callback.onItemGroupsLoaded(p);
					}
				});
			}
		});
	}

	public void findAuthorBooks(final long authorId, final FileInfoListLoadingCallback callback, final Handler handler) {
		execTask(new Runnable() {
			@Override
			public void run() {
				final ArrayList<FileInfo> list = new ArrayList<FileInfo>();
				mainDB.findAuthorBooks(list, authorId);
				sendTask(handler, new Runnable() {
					@Override
					public void run() {
						callback.onFileInfoListLoaded(authorId, list);
					}
				});
			}
		});
	}
	
	public void findSeriesBooks(final long seriesId, final FileInfoListLoadingCallback callback, final Handler handler) {
		execTask(new Runnable() {
			@Override
			public void run() {
				final ArrayList<FileInfo> list = new ArrayList<FileInfo>();
				mainDB.findSeriesBooks(list, seriesId);
				sendTask(handler, new Runnable() {
					@Override
					public void run() {
						callback.onFileInfoListLoaded(seriesId, list);
					}
				});
			}
		});
	}

	public void loadRecentBooks(final int maxCount, final RecentBooksLoadingCallback callback, final Handler handler) {
		execTask(new Runnable() {
			@Override
			public void run() {
				final ArrayList<BookInfo> list = mainDB.loadRecentBooks(maxCount);
				sendTask(handler, new Runnable() {
					@Override
					public void run() {
						callback.onRecentBooksListLoaded(list);
					}
				});
			}
		});
	}
	
	public void findByPatterns(final int maxCount, final String author, final String title, final String series, final String filename, final BookSearchCallback callback, final Handler handler) {
		execTask(new Runnable() {
			@Override
			public void run() {
				final ArrayList<FileInfo> list = mainDB.findByPatterns(maxCount, author, title, series, filename);
				sendTask(handler, new Runnable() {
					@Override
					public void run() {
						callback.onBooksFound(list);
					}
				});
			}
		});
	}

	private ArrayList<FileInfo> deepCopyFileInfos(final Collection<FileInfo> src) {
		final ArrayList<FileInfo> list = new ArrayList<FileInfo>(src.size());
		for (FileInfo fi : src)
			list.add(new FileInfo(fi));
		return list;
	}
	
	public void saveFileInfos(final Collection<FileInfo> list) {
		execTask(new Runnable() {
			@Override
			public void run() {
				mainDB.saveFileInfos(list);
			}
		});
		flush();
	}

	public void loadFileInfos(final ArrayList<String> pathNames, final FileInfoLoadingCallback callback, final Handler handler) {
		execTask(new Runnable() {
			@Override
			public void run() {
				final ArrayList<FileInfo> list = mainDB.loadFileInfos(pathNames);
				sendTask(handler, new Runnable() {
					@Override
					public void run() {
						callback.onFileInfoListLoaded(list);
					}
				});
			}
		});
	}
	
	public void deleteBook(final FileInfo fileInfo)	{
		execTask(new Runnable() {
			@Override
			public void run() {
				mainDB.deleteBook(fileInfo);
			}
		});
		flush();
	}

	public void deleteRecentPosition(final FileInfo fileInfo) {
		execTask(new Runnable() {
			@Override
			public void run() {
				mainDB.deleteRecentPosition(fileInfo);
			}
		});
		flush();
	}

	/**
	 * Execute runnable in CDRDBService background thread.
	 * Exceptions will be ignored, just dumped into log.
	 * @param task is Runnable to execute
	 */
	private void execTask(final Runnable task) {
		mThread.post(new Runnable() {
			@Override
			public void run() {
				try {
					task.run();
				} catch (Exception e) {
					log.e("Exception while running DB task in background", e);
				}
			}
		});
	}
	
	/**
	 * Execute runnable in CDRDBService background thread, delayed.
	 * Exceptions will be ignored, just dumped into log.
	 * @param task is Runnable to execute
	 */
	private void execTask(final Runnable task, long delay) {
		mThread.postDelayed(new Runnable() {
			@Override
			public void run() {
				try {
					task.run();
				} catch (Exception e) {
					log.e("Exception while running DB task in background", e);
				}
			}
		}, delay);
	}
	
	/**
	 * Send task to handler, if specified, otherwise run immediately.
	 * Exceptions will be ignored, just dumped into log.
	 * @param handler is handler to send task to, null to run immediately
	 * @param task is Runnable to execute
	 */
	private void sendTask(Handler handler, Runnable task) {
		try {
			if (handler != null) {
				handler.post(task);
			} else {
				task.run();
			}
		} catch (Exception e) {
			log.e("Exception in DB callback", e);
		}
	}

	/**
     * Class for clients to access.  Because we know this service always
     * runs in the same process as its clients, we don't need to deal with
     * IPC.
     * Provides interface for asynchronous operations with database.
     */
    public class LocalBinder extends Binder {
        private CRDBService getService() {
            return CRDBService.this;
        }
        
    	public void saveBookCoverpage(long bookId, byte[] data) {
    		getService().saveBookCoverpage(bookId, data);
    	}
    	
    	public void deleteBookCoverpage(long bookId) {
    		getService().deleteCoverpage(bookId);
    	}

    	public void loadBookCoverpage(final long bookId, final CoverpageLoadingCallback callback) {
    		getService().loadBookCoverpage(bookId, callback, new Handler());
    	}
    	
    	public void loadOPDSCatalogs(final OPDSCatalogsLoadingCallback callback) {
    		getService().loadOPDSCatalogs(callback, new Handler());
    	}

    	public void saveOPDSCatalog(final Long id, final String url, final String name) {
    		getService().saveOPDSCatalog(id, url, name);
    	}

    	public void loadAuthorsList(FileInfo parent, final ItemGroupsLoadingCallback callback) {
    		getService().loadAuthorsList(parent, callback, new Handler());
    	}

    	public void loadSeriesList(FileInfo parent, final ItemGroupsLoadingCallback callback) {
    		getService().loadSeriesList(parent, callback, new Handler());
    	}
    	
    	public void loadTitleList(FileInfo parent, final ItemGroupsLoadingCallback callback) {
    		getService().loadTitleList(parent, callback, new Handler());
    	}

    	public void loadAuthorBooks(long authorId, FileInfoListLoadingCallback callback) {
    		getService().findAuthorBooks(authorId, callback, new Handler());
    	}
    	
    	public void loadSeriesBooks(long seriesId, FileInfoListLoadingCallback callback) {
    		getService().findSeriesBooks(seriesId, callback, new Handler());
    	}

    	public void loadRecentBooks(final int maxCount, final RecentBooksLoadingCallback callback) {
    		getService().loadRecentBooks(maxCount, callback, new Handler());
    	}

    	public void saveFileInfos(final Collection<FileInfo> list) {
    		getService().saveFileInfos(deepCopyFileInfos(list));
    	}
    	
    	public void findByPatterns(final int maxCount, final String author, final String title, final String series, final String filename, final BookSearchCallback callback) {
    		getService().findByPatterns(maxCount, author, title, series, filename, callback, new Handler());
    	}
    	
    	public void loadFileInfos(final ArrayList<String> pathNames, final FileInfoLoadingCallback callback) {
    		getService().loadFileInfos(pathNames, callback, new Handler());
    	}

    	public void deleteBook(final FileInfo fileInfo)	{
    		getService().deleteBook(new FileInfo(fileInfo));
    	}

    	public void deleteRecentPosition(final FileInfo fileInfo)	{
    		getService().deleteRecentPosition(new FileInfo(fileInfo));
    	}

    	public void flush() {
    		getService().forceFlush();
    	}
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    private ServiceThread mThread;
    private final IBinder mBinder = new LocalBinder();
    
}
