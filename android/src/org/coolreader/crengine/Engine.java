package org.coolreader.crengine;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.app.AlertDialog;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Environment;
import android.util.Log;

/**
 * CoolReader Engine class.
 * 
 * Only one instance is allowed.
 */
public class Engine {

	public static final Logger log = L.create("en");
	
	private final CoolReader mActivity;
	private final BackgroundThread mBackgroundThread;
	private File[] mountedRootsList;
	private Map<String, String> mountedRootsMap;
	
	static final private String LIBRARY_NAME = "cr3engine-45-15";

	// private final View mMainView;
	// private final ExecutorService mExecutor =
	// Executors.newFixedThreadPool(1);

	/**
	 * Get storage root directories.
	 * 
	 * @return array of r/w storage roots
	 */
	public File[] getStorageDirectories(boolean writableOnly) {
		Collection<File> res = new HashSet<File>(2);
		for (File dir : mountedRootsList) {
			if (dir.isDirectory() && (!writableOnly || dir.canWrite()))
				res.add(dir);
		}
		return res.toArray(new File[res.size()]);
	}
	
	public Map<String, String> getMountedRootsMap() {
		return mountedRootsMap;
	}

	/**
	 * Get or create writable subdirectory for specified base directory
	 * 
	 * @param dir
	 *            is base directory
	 * @param subdir
	 *            is subdirectory name, null to use base directory
	 * @param createIfNotExists
	 *            is true to force directory creation
	 * @return writable directory, null if not exist or not writable
	 */
	public static File getSubdir(File dir, String subdir,
			boolean createIfNotExists, boolean writableOnly) {
		if (dir == null)
			return null;
		File dataDir = dir;
		if (subdir != null) {
			dataDir = new File(dataDir, subdir);
			if (!dataDir.isDirectory() && createIfNotExists)
				dataDir.mkdir();
		}
		if (dataDir.isDirectory() && (!writableOnly || dataDir.canWrite()))
			return dataDir;
		return null;
	}

	/**
	 * Returns array of writable data directories on external storage
	 * 
	 * @param subdir
	 * @param createIfNotExists
	 * @return
	 */
	public File[] getDataDirectories(String subdir,
			boolean createIfNotExists, boolean writableOnly) {
		File[] roots = getStorageDirectories(writableOnly);
		ArrayList<File> res = new ArrayList<File>(roots.length);
		for (File dir : roots) {
			File dataDir = getSubdir(dir, ".cr3", createIfNotExists,
					writableOnly);
			if (subdir != null)
				dataDir = getSubdir(dataDir, subdir, createIfNotExists,
						writableOnly);
			if (dataDir != null)
				res.add(dataDir);
		}
		return res.toArray(new File[] {});
	}

	public interface EngineTask {
		public void work() throws Exception;

		public void done();

		public void fail(Exception e);
	}

	// public static class FatalError extends RuntimeException {
	// private Engine engine;
	// private String msg;
	// public FatalError( Engine engine, String msg )
	// {
	// this.engine = engine;
	// this.msg = msg;
	// }
	// public void handle()
	// {
	// engine.fatalError(msg);
	// }
	// }

	public final static boolean LOG_ENGINE_TASKS = false;

	private class TaskHandler implements Runnable {
		final EngineTask task;

		public TaskHandler(EngineTask task) {
			this.task = task;
		}

		public String toString() {
			return "[handler for " + this.task.toString() + "]";
		}

		public void run() {
			try {
				if (LOG_ENGINE_TASKS)
					log.i("running task.work() "
							+ task.getClass().getName());
				if (!initialized)
					throw new IllegalStateException("Engine not initialized");
				// run task
				task.work();
				if (LOG_ENGINE_TASKS)
					log.i("exited task.work() "
							+ task.getClass().getName());
				// post success callback
				mBackgroundThread.postGUI(new Runnable() {
					public void run() {
						if (LOG_ENGINE_TASKS)
							log.i("running task.done() "
									+ task.getClass().getName()
									+ " in gui thread");
						task.done();
					}
				});
				// } catch ( final FatalError e ) {
				// TODO:
				// Handler h = view.getHandler();
				//
				// if ( h==null ) {
				// View root = view.getRootView();
				// h = root.getHandler();
				// }
				// if ( h==null ) {
				// //
				// e.handle();
				// } else {
				// h.postAtFrontOfQueue(new Runnable() {
				// public void run() {
				// e.handle();
				// }
				// });
				// }
			} catch (final Exception e) {
				log.e("exception while running task "
						+ task.getClass().getName(), e);
				// post error callback
				mBackgroundThread.postGUI(new Runnable() {
					public void run() {
						log.e("running task.fail(" + e.getMessage()
								+ ") " + task.getClass().getSimpleName()
								+ " in gui thread ");
						task.fail(e);
					}
				});
			}
		}
	}

	/**
	 * Execute task in Engine thread
	 * 
	 * @param task
	 *            is task to execute
	 */
	public void execute(final EngineTask task) {
		if (LOG_ENGINE_TASKS)
			log.d("executing task " + task.getClass().getSimpleName());
		TaskHandler taskHandler = new TaskHandler(task);
		mBackgroundThread.executeBackground(taskHandler);
	}

	/**
	 * Schedule task for execution in Engine thread
	 * 
	 * @param task
	 *            is task to execute
	 */
	public void post(final EngineTask task) {
		if (LOG_ENGINE_TASKS)
			log.d("executing task " + task.getClass().getSimpleName());
		TaskHandler taskHandler = new TaskHandler(task);
		mBackgroundThread.postBackground(taskHandler);
	}

	/**
	 * Schedule Runnable for execution in GUI thread after all current Engine
	 * queue tasks done.
	 * 
	 * @param task
	 */
	public void runInGUI(final Runnable task) {
		execute(new EngineTask() {

			public void done() {
				mBackgroundThread.postGUI(task);
			}

			public void fail(Exception e) {
				// do nothing
			}

			public void work() throws Exception {
				// do nothing
			}
		});
	}

	public void fatalError(String msg) {
		AlertDialog dlg = new AlertDialog.Builder(mActivity).setMessage(msg)
				.setTitle("CoolReader fatal error").show();
		try {
			Thread.sleep(10);
		} catch (InterruptedException e) {
			// do nothing
		}
		dlg.dismiss();
		mActivity.finish();
	}

	private ProgressDialog mProgress;
	private boolean enable_progress = true;
	private boolean progressShown = false;
	private static int PROGRESS_STYLE = ProgressDialog.STYLE_HORIZONTAL;
	private Drawable progressIcon = null;

	// public void setProgressDrawable( final BitmapDrawable drawable )
	// {
	// if ( enable_progress ) {
	// mBackgroundThread.executeGUI( new Runnable() {
	// public void run() {
	// // show progress
	// log.v("showProgress() - in GUI thread");
	// if ( mProgress!=null && progressShown ) {
	// hideProgress();
	// progressIcon = drawable;
	// showProgress(mProgressPos, mProgressMessage);
	// //mProgress.setIcon(drawable);
	// }
	// }
	// });
	// }
	// }
	public void showProgress(final int mainProgress, final int resourceId) {
		showProgress(mainProgress,
				mActivity.getResources().getString(resourceId));
	}

	private String mProgressMessage = null;
	private int mProgressPos = 0;

	private volatile int nextProgressId = 0;

	public class DelayedProgress {
		private volatile boolean cancelled;
		private volatile boolean shown;

		/**
		 * Cancel scheduled progress.
		 */
		public void cancel() {
			cancelled = true;
		}
		/**
		 * Cancel and hide scheduled progress.
		 */
		public void hide() {
			this.cancelled = true;
			BackgroundThread.instance().executeGUI(new Runnable() {
				@Override
				public void run() {
					if ( shown )
						hideProgress();
					shown = false;
				}
				
			});
		}

		DelayedProgress( final int percent, final String msg, final int delayMillis ) {
			this.cancelled = false;
			BackgroundThread.instance().postGUI(new Runnable() {
				@Override
				public void run() {
					if ( !cancelled ) {
						showProgress( percent, msg );
						shown = true;
					}
				}
				
			}, delayMillis);
		}
	}
	
	/**
	 * Display progress dialog after delay.
	 * (thread-safe)
	 * @param mainProgress is percent*100
	 * @param msg is progress message text
	 * @param delayMillis is delay before display of progress
	 * @return DelayedProgress object which can be use to hide or cancel this schedule
	 */
	public DelayedProgress showProgressDelayed(final int mainProgress, final String msg, final int delayMillis ) {
		return new DelayedProgress(mainProgress, msg, delayMillis);
	}
	
	/**
	 * Show progress dialog.
	 * (thread-safe)
	 * @param mainProgress is percent*100
	 * @param msg is progress message
	 */
	public void showProgress(final int mainProgress, final String msg) {
		final int progressId = ++nextProgressId;
		mProgressMessage = msg;
		mProgressPos = mainProgress;
		if (mainProgress == 10000) {
			//log.v("mainProgress==10000 : calling hideProgress");
			hideProgress();
			return;
		}
		log.v("showProgress(" + mainProgress + ", \"" + msg
				+ "\") is called : " + Thread.currentThread().getName());
		if (enable_progress) {
			mBackgroundThread.executeGUI(new Runnable() {
				public void run() {
					// show progress
					//log.v("showProgress() - in GUI thread");
					if (progressId != nextProgressId) {
						//Log.v("cr3",
						//		"showProgress() - skipping duplicate progress event");
						return;
					}
					if (mProgress == null) {
						try {
							if (mActivity != null && mActivity.isStarted()) {
								mProgress = new ProgressDialog(mActivity);
								mProgress
										.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
								if (progressIcon != null)
									mProgress.setIcon(progressIcon);
								else
									mProgress.setIcon(R.drawable.cr3_logo);
								mProgress.setMax(10000);
								mProgress.setCancelable(false);
								mProgress.setProgress(mainProgress);
								mProgress
										.setTitle(mActivity
												.getResources()
												.getString(
														R.string.progress_please_wait));
								mProgress.setMessage(msg);
								mProgress.show();
								progressShown = true;
							}
						} catch (Exception e) {
							Log.e("cr3",
									"Exception while trying to show progress dialog",
									e);
							progressShown = false;
							mProgress = null;
						}
					} else {
						mProgress.setProgress(mainProgress);
						mProgress.setMessage(msg);
						if (!mProgress.isShowing()) {
							mProgress.show();
							progressShown = true;
						}
					}
				}
			});
		}
	}

	/**
	 * Hide progress dialog (if shown).
	 * (thread-safe)
	 */
	public void hideProgress() {
		final int progressId = ++nextProgressId;
		log.v("hideProgress() - is called : "
				+ Thread.currentThread().getName());
		// log.v("hideProgress() is called");
		mBackgroundThread.executeGUI(new Runnable() {
			public void run() {
				// hide progress
//				log.v("hideProgress() - in GUI thread");
				if (progressId != nextProgressId) {
//					Log.v("cr3",
//							"hideProgress() - skipping duplicate progress event");
					return;
				}
				if (mProgress != null) {
					// if ( mProgress.isShowing() )
					// mProgress.hide();
					progressShown = false;
					progressIcon = null;
					if (mProgress.isShowing())
						mProgress.dismiss();
					mProgress = null;
//					log.v("hideProgress() - in GUI thread, finished");
				}
			}
		});
	}

	public boolean isProgressShown() {
		return progressShown;
	}

	public String loadFileUtf8(File file) {
		try {
			InputStream is = new FileInputStream(file);
			return loadResourceUtf8(is);
		} catch (Exception e) {
			log.e("cannot load resource from file " + file);
			return null;
		}
	}

	public String loadResourceUtf8(int id) {
		try {
			InputStream is = this.mActivity.getResources().openRawResource(id);
			return loadResourceUtf8(is);
		} catch (Exception e) {
			log.e("cannot load resource " + id);
			return null;
		}
	}

	public String loadResourceUtf8(InputStream is) {
		try {
			int available = is.available();
			if (available <= 0)
				return null;
			byte buf[] = new byte[available];
			if (is.read(buf) != available)
				throw new IOException("Resource not read fully");
			is.close();
			String utf8 = new String(buf, 0, available, "UTF8");
			return utf8;
		} catch (Exception e) {
			log.e("cannot load resource");
			return null;
		}
	}

	public byte[] loadResourceBytes(int id) {
		try {
			InputStream is = this.mActivity.getResources().openRawResource(id);
			return loadResourceBytes(is);
		} catch (Exception e) {
			log.e("cannot load resource");
			return null;
		}
	}

	public static byte[] loadResourceBytes(File f) {
		if (f == null || !f.isFile() || !f.exists())
			return null;
		FileInputStream is = null;
		try {
			is = new FileInputStream(f);
			byte[] res = loadResourceBytes(is);
			return res;
		} catch (IOException e) {
			log.e("Cannot open file " + f);
		}
		return null;
	}

	public static byte[] loadResourceBytes(InputStream is) {
		try {
			int available = is.available();
			if (available <= 0)
				return null;
			byte buf[] = new byte[available];
			if (is.read(buf) != available)
				throw new IOException("Resource not read fully");
			is.close();
			return buf;
		} catch (Exception e) {
			log.e("cannot load resource");
			return null;
		}
	}

	/**
	 * Initialize CoolReader Engine
	 * 
	 * @param fontList
	 *            is array of .ttf font pathnames to load
	 */
	public Engine(CoolReader activity, BackgroundThread backgroundThread) {
		this.mActivity = activity;
		this.mBackgroundThread = backgroundThread;
		installLibrary();
		initMountRoots();
		// this.mMainView = mainView;
		//
//		log.i("Engine() : initializing Engine in UI thread");
//		if (!initialized) {
//			installLibrary();
//		}
		initializeStarted = true;
		log.i("Engine() : scheduling init task");
		BackgroundThread.backgroundExecutor.execute(new Runnable() {
			public void run() {
				try {
					log.i("Engine() : running init() in engine thread");
					init();
					// android.view.ViewRoot.getRunQueue().post(new Runnable() {
					// public void run() {
					//
					// }
					// });
				} catch (final Exception e) {
					log.e("Exception while initializing Engine", e);
					// handler.post(new Runnable() {
					// public void run() {
					// // TODO: fatal error
					// }
					// });
				}
			}
		});
	}

	private native boolean initInternal(String[] fontList);

	private native void uninitInternal();

	private native String[] getFontFaceListInternal();

	private native String[] getArchiveItemsInternal(String arcName); // pairs:
																		// pathname,
																		// size

	private native boolean setCacheDirectoryInternal(String dir, int size);

	private native boolean scanBookPropertiesInternal(FileInfo info);

    private static native void suspendLongOperationInternal(); // cancel current long operation in engine thread (swapping to cache file) -- call it from GUI thread
    
    public static void suspendLongOperation() {
    	if (isInitialized()) {
    		suspendLongOperationInternal();
    	}
    }
	
	/**
	 * Checks whether specified directlry or file is symbolic link.
	 * (thread-safe)
	 * @param pathName is path to check
	 * @return true if specified directory or file is link (symlink)
	 */
	public native boolean isLink(String pathName);
	
	private static final int HYPH_NONE = 0;
	private static final int HYPH_ALGO = 1;
	private static final int HYPH_DICT = 2;

	private native boolean setHyphenationMethod(int type, byte[] dictData);

	public ArrayList<ZipEntry> getArchiveItems(String zipFileName) {
		final int itemsPerEntry = 2;
		String[] in = getArchiveItemsInternal(zipFileName);
		ArrayList<ZipEntry> list = new ArrayList<ZipEntry>();
		for (int i = 0; i <= in.length - itemsPerEntry; i += itemsPerEntry) {
			ZipEntry e = new ZipEntry(in[i]);
			e.setSize(Integer.valueOf(in[i + 1]));
			e.setCompressedSize(Integer.valueOf(in[i + 1]));
			list.add(e);
		}
		return list;
	}

	public static class HyphDict {
		private static HyphDict[] values = new HyphDict[] {};
		public final static HyphDict NONE = new HyphDict("NONE", HYPH_NONE, 0, "[None]");
		public final static HyphDict ALGORITHM = new HyphDict("ALGORITHM", HYPH_ALGO, 0, "[Algorythmic]"); 
		public final static HyphDict RUSSIAN = new HyphDict("RUSSIAN", HYPH_DICT, R.raw.russian_enus_hyphen, "Russian"); 
		public final static HyphDict ENGLISH = new HyphDict("ENGLISH", HYPH_DICT, R.raw.english_us_hyphen, "English US"); 
		public final static HyphDict GERMAN = new HyphDict("GERMAN", HYPH_DICT, R.raw.german_hyphen, "German"); 
		public final static HyphDict UKRAINIAN = new HyphDict("UKRAINIAN", HYPH_DICT,R.raw.ukrain_hyphen, "Ukrainian"); 
		public final static HyphDict SPANISH = new HyphDict("SPANISH", HYPH_DICT,R.raw.spanish_hyphen, "Spanish"); 
		public final static HyphDict FRENCH = new HyphDict("FRENCH", HYPH_DICT,R.raw.french_hyphen, "French"); 
		public final static HyphDict BULGARIAN = new HyphDict("BULGARIAN", HYPH_DICT, R.raw.bulgarian_hyphen, "Bulgarian"); 
		public final static HyphDict SWEDISH = new HyphDict("SWEDISH", HYPH_DICT, R.raw.swedish_hyphen, "Swedish"); 
		public final static HyphDict POLISH = new HyphDict("POLISH", HYPH_DICT, R.raw.polish_hyphen, "Polish");
		public final static HyphDict HUNGARIAN = new HyphDict("HUNGARIAN", HYPH_DICT, R.raw.hungarian_hyphen, "Hungarian");
		
		public final String code;
		public final int type;
		public final int resource;
		public final String name;
		public final File file;

		
		public static HyphDict[] values() {
			return values;
		}

		private static void add(HyphDict dict) {
			// Arrays.copyOf(values, values.length+1); -- absent until API level 9
			HyphDict[] list = new HyphDict[values.length+1];
			for (int i=0; i<values.length; i++)
				list[i] = values[i];
			list[list.length-1] = dict;
			values = list;
		}
		
		private HyphDict(String code, int type, int resource, String name) {
			this.type = type;
			this.resource = resource;
			this.name = name;
			this.file = null;
			this.code = code;
			// register in list
			add(this);
		}

		private HyphDict(File file) {
			this.type = HYPH_DICT;
			this.resource = 0;
			this.name = file.getName();
			this.file = file;
			this.code = this.name;
			// register in list
			add(this);
		}

		public static HyphDict byCode(String code) {
			for (HyphDict dict : values)
				if (dict.toString().equals(code))
					return dict;
			return NONE;
		}

		public static HyphDict byFileName(String fileName) {
			for (HyphDict dict : values)
				if (dict.file!=null && dict.file.getName().equals(fileName))
					return dict;
			return NONE;
		}

		@Override
		public String toString() {
			return code;
		}
		
		public static boolean fromFile(File file) {
			if (file==null || !file.exists() || !file.isFile() || !file.canRead())
				return false;
			String fn = file.getName();
			if (!fn.toLowerCase().endsWith(".pdb") && !fn.toLowerCase().endsWith(".pattern"))
				return false; // wrong file name
			if (byFileName(file.getName())!=NONE)
				return false; // already registered
			new HyphDict(file);
			return true;
		}
	};

	private HyphDict currentHyphDict = null;

	public boolean setHyphenationDictionary(final HyphDict dict) {
		log.i("setHyphenationDictionary( " + dict + " ) is called");
		if (currentHyphDict == dict)
			return false;
		currentHyphDict = dict;
		// byte[] image = loadResourceBytes(R.drawable.tx_old_book);
		mBackgroundThread.postBackground(new Runnable() {
			public void run() {
				if (!initialized)
					throw new IllegalStateException("CREngine is not initialized");
				byte[] data = null;
				if (dict.type == HYPH_DICT) {
					if (dict.resource!=0) {
						data = loadResourceBytes(dict.resource);
					} else if (dict.file!=null) {
						data = loadResourceBytes(dict.file);
					}
				}
				log.i("Setting engine's hyphenation dictionary to "
						+ dict);
				setHyphenationMethod(dict.type, data);
			}
		});
		return true;
	}

	public boolean scanBookProperties(FileInfo info) {
		if (!initialized)
			throw new IllegalStateException("CREngine is not initialized");
		return scanBookPropertiesInternal(info);
	}

	public String[] getFontFaceList() {
		if (!initialized)
			throw new IllegalStateException("CREngine is not initialized");
		return getFontFaceListInternal();
	}

	final static int CACHE_DIR_SIZE = 32000000;

	private String createCacheDir(File baseDir, String subDir) {
		String cacheDirName = null;
		if (baseDir.isDirectory()) {
			if (baseDir.canWrite()) {
				if (subDir != null) {
					baseDir = new File(baseDir, subDir);
					baseDir.mkdir();
				}
				if (baseDir.exists() && baseDir.canWrite()) {
					File cacheDir = new File(baseDir, "cache");
					if (cacheDir.exists() || cacheDir.mkdirs()) {
						if (cacheDir.canWrite()) {
							cacheDirName = cacheDir.getAbsolutePath();
							CR3_SETTINGS_DIR_NAME = baseDir.getAbsolutePath(); 
						}
					}
				}
			} else {
				log.i(baseDir.toString() + " is read only");
			}
		} else {
			log.i(baseDir.toString() + " is not found");
		}
		return cacheDirName;
	}
	
	public static String getExternalSettingsDirName() {
		return CR3_SETTINGS_DIR_NAME;
	}
	
	public static File getExternalSettingsDir() {
		return CR3_SETTINGS_DIR_NAME!=null ? new File(CR3_SETTINGS_DIR_NAME) : null;
	}
	
	public static boolean moveFile( File oldPlace, File newPlace ) {
		boolean removeNewFile = true;
		log.i("Moving file " + oldPlace.getAbsolutePath() + " to " + newPlace.getAbsolutePath());
		if ( !oldPlace.exists() ) {
			log.e("File " + oldPlace.getAbsolutePath() + " does not exist!");
			return false;
		}
		FileOutputStream os = null;
		FileInputStream is = null;
		try {
			if ( !newPlace.createNewFile() )
				return false; // cannot create file
			os = new FileOutputStream(newPlace);
			is = new FileInputStream(oldPlace);
			byte[] buf = new byte[0x10000];
			for (;;) {
				int bytesRead = is.read(buf);
				if ( bytesRead<=0 )
					break;
				os.write(buf, 0, bytesRead);
			}
			removeNewFile = false;
			oldPlace.delete();
			return true;
		} catch ( IOException e ) {
			return false;
		} finally {
			try {
				if (os != null)
					os.close();
			} catch (IOException ee) {
				// ignore
			}
			try {
				if (is != null)
					is.close();
			} catch (IOException ee) {
				// ignore
			}
			if ( removeNewFile )
				newPlace.delete();
		}
	}
	
	/**
	 * Checks whether file under old path exists, and moves it to better place when necessary.
	 * Can be slow if big file is being moved. 
	 * @param bestPlace is desired directory for file (e.g. new place after migration)
	 * @param oldPlace is old (obsolete) directory for file (e.g. location from older releases)
	 * @param filename is name of file
	 * @return file to use (from old or new place)
	 */
	public static File checkOrMoveFile( File bestPlace, File oldPlace, String filename ) {
		if ( !bestPlace.exists() ) {
			bestPlace.mkdirs();
		}
		File oldFile = new File(oldPlace, filename);
		if ( bestPlace.isDirectory() && bestPlace.canWrite() ) {
			File bestFile = new File(bestPlace, filename);
			if (bestFile.exists())
				return bestFile; // already exists
			if (oldFile.exists() && oldFile.isFile()) {
				// move file
				if (moveFile(oldFile, bestFile))
					return bestFile;
				return oldFile;
			}
			return bestFile;
		}
		return oldFile;
	}

	private static String CR3_SETTINGS_DIR_NAME;
	
	public final static String CACHE_BASE_DIR_NAME = ".cr3"; // "Books"
	private void initCacheDirectory() {
		String cacheDirName = null;
		// SD card
		cacheDirName = createCacheDir(
				Environment.getExternalStorageDirectory(), CACHE_BASE_DIR_NAME);
		// non-standard SD mount points
		if (cacheDirName == null) {
			for ( String dirname : Scanner.SD_MOUNT_POINTS ) {
				cacheDirName = createCacheDir(new File(dirname),
						CACHE_BASE_DIR_NAME);
				if ( cacheDirName!=null )
					break;
			}
		}
		// internal flash
		if (cacheDirName == null) {
			File cacheDir = mActivity.getCacheDir();
			if (!cacheDir.isDirectory())
				cacheDir.mkdir();
			cacheDirName = createCacheDir(cacheDir, null);
			// File cacheDir = mActivity.getDir("cache", Context.MODE_PRIVATE);
//			if (cacheDir.isDirectory() && cacheDir.canWrite())
//				cacheDirName = cacheDir.getAbsolutePath();
		}
		// set cache directory for engine
		if (cacheDirName != null) {
			log.i(cacheDirName
					+ " will be used for cache, maxCacheSize=" + CACHE_DIR_SIZE);
			setCacheDirectoryInternal(cacheDirName, CACHE_DIR_SIZE);
		}
	}

	private boolean addMountRoot(Map<String, String> list, String pathname, int resourceId)
	{
		return addMountRoot(list, pathname, mActivity.getResources().getString(resourceId));
	}
	
	private boolean addMountRoot(Map<String, String> list, String path, String name) {
		if (list.containsKey(path))
			return false;
		try {
			File dir = new File(path);
			if (dir.exists() && dir.isDirectory()) {
				String[] d = dir.list();
				if (d!=null && d.length>0) {
					log.i("Adding FS root: " + path + " " + name);
					list.put(path, name);
					return true;
				}
			}
		} catch (Exception e) {
			// ignore
		}
		return false;
	}
	
	private static final String[] SYSTEM_ROOT_PATHS = {"/system", "/data", "/mnt"};
	private void autoAddRoots(Map<String, String> list, String rootPath, String[] pathsToExclude)
	{
		try {
			File root = new File(rootPath);
			File[] files = root.listFiles();
			if ( files!=null ) {
				for ( File f : files ) {
					if ( !f.isDirectory() )
						continue;
					String fullPath = f.getAbsolutePath();
					if ( isLink(fullPath) ) {
						L.d("skipping symlink " + fullPath);
						continue;
					}
					boolean skip = false;
					for ( String path : pathsToExclude ) {
						if ( fullPath.startsWith(path) ) {
							skip = true;
							break;
						}
					}
					if ( skip )
						continue;
					if ( !f.canWrite() ) {
						L.i("Path is readonly: " + f.getAbsolutePath());
						continue;
					}
					L.i("Found possible mount point " + f.getAbsolutePath());
					addMountRoot(list, f.getAbsolutePath(), f.getAbsolutePath());
				}
			}
		} catch ( Exception e ) {
			L.w("Exception while trying to auto add roots");
		}
	}
	
	private void initMountRoots() {
		Map<String, String> map = new LinkedHashMap<String, String>();
		String sdpath = Environment.getExternalStorageDirectory().getAbsolutePath();
		if ( "/nand".equals(sdpath) && new File("/sdcard").isDirectory() )
			sdpath = "/sdcard";
		addMountRoot(map, sdpath, R.string.dir_sd_card);
		// internal SD card on Nook
		addMountRoot(map, "/system/media/sdcard", R.string.dir_internal_sd_card);
		// internal memory
		addMountRoot(map, "/media", R.string.dir_internal_memory);
		addMountRoot(map, "/nand", R.string.dir_internal_memory);
		// internal SD card on PocketBook 701 IQ
		addMountRoot(map, "/PocketBook701", R.string.dir_internal_sd_card);
		// external SD
		addMountRoot(map, "/mnt/extsd", "External SD /mnt/extsd");
		// external SD
		addMountRoot(map, "/mnt/external1", "External SD /mnt/external1");
		// external SD / Galaxy S
		addMountRoot(map, "/mnt/ext.sd", "External SD /mnt/ext.sd");
		addMountRoot(map, "/ext.sd", "External SD /ext.sd");
		// Asus EEE PAD Transformer
		addMountRoot(map, "/Removable/MicroSD", "MicroSD");
		// external SD card Huawei S7
		addMountRoot(map, "/sdcard2", R.string.dir_sd_card_2);
		
		// auto detection
		autoAddRoots(map, "/", SYSTEM_ROOT_PATHS);
		autoAddRoots(map, "/mnt", new String[] {});
		
		mountedRootsMap = map;
		Collection<File> list = new ArrayList<File>();
		for (String f : map.keySet()) {
			list.add(new File(f));
		}
		mountedRootsList = list.toArray(new File[] {});
	}
	
	private void init() throws IOException {
		if (initialized)
			throw new IllegalStateException("Already initialized");
		String[] fonts = findFonts();
		findExternalHyphDictionaries();
		if (!initInternal(fonts))
			throw new IOException("Cannot initialize CREngine JNI");
		// Initialization of cache directory
		initCacheDirectory();
		initialized = true;
	}

	// public void waitTasksCompletion()
	// {
	// log.i("waiting for engine tasks completion");
	// try {
	// mExecutor.awaitTermination(0, TimeUnit.SECONDS);
	// } catch (InterruptedException e) {
	// // ignore
	// }
	// }

	/**
	 * Uninitialize engine.
	 */
	public void uninit() {
		log.i("Engine.uninit() is called");
		BackgroundThread.backgroundExecutor.execute(new Runnable() {
			public void run() {
				log.i("Engine.uninit() : in background thread");
				if (initialized) {
					uninitInternal();
					initialized = false;
				}
			}
		});
	}

	protected void finalize() throws Throwable {
		log.i("Engine.finalize() is called");
		// if ( initialized ) {
		// //uninitInternal();
		// initialized = false;
		// }
	}

	public static boolean isInitialized() {
		return initialized;
	}
	
	static private boolean initialized = false;
	static private boolean initializeStarted = false;

	private String[] findFonts() {
		ArrayList<File> dirs = new ArrayList<File>();
		File[] dataDirs = getDataDirectories("fonts", false, false);
		for (File dir : dataDirs)
			dirs.add(dir);
		File[] rootDirs = getStorageDirectories(false);
		for (File dir : rootDirs)
			dirs.add(new File(dir, "fonts"));
		dirs.add(new File(Environment.getRootDirectory(), "fonts"));
		ArrayList<String> fontPaths = new ArrayList<String>();
		for (File fontDir : dirs) {
			if (fontDir.isDirectory()) {
				log.v("Scanning directory " + fontDir.getAbsolutePath()
						+ " for font files");
				// get font names
				String[] fileList = fontDir.list(new FilenameFilter() {
					public boolean accept(File dir, String filename) {
						String lc = filename.toLowerCase();
						return (lc.endsWith(".ttf") || lc.endsWith(".otf")
								|| lc.endsWith(".pfb") || lc.endsWith(".pfa"))
//								&& !filename.endsWith("Fallback.ttf")
								;
					}
				});
				// append path
				for (int i = 0; i < fileList.length; i++) {
					String pathName = new File(fontDir, fileList[i])
							.getAbsolutePath();
					fontPaths.add(pathName);
					log.v("found font: " + pathName);
				}
			}
		}
		return fontPaths.toArray(new String[] {});
	}

	private String SO_NAME = "lib" + LIBRARY_NAME + ".so";
	private boolean force_install_library = false;

	private void installLibrary() {
		try {
			if (force_install_library)
				throw new Exception("forcing install");
			// try loading library w/o manual installation
			log.i("trying to load library " + LIBRARY_NAME
					+ " w/o installation");
			System.loadLibrary(LIBRARY_NAME);
			// try invoke native method
			//log.i("trying execute native method ");
			//setHyphenationMethod(HYPH_NONE, new byte[] {});
			log.i(LIBRARY_NAME + " loaded successfully");
		} catch (Exception ee) {
			log.i(SO_NAME + " not found using standard paths, will install manually");
			File sopath = mActivity.getDir("libs", Context.MODE_PRIVATE);
			File soname = new File(sopath, SO_NAME);
			try {
				sopath.mkdirs();
				File zip = new File(mActivity.getPackageCodePath());
				ZipFile zipfile = new ZipFile(zip);
				ZipEntry zipentry = zipfile.getEntry("lib/armeabi/" + SO_NAME);
				if (!soname.exists() || zipentry.getSize() != soname.length()) {
					InputStream is = zipfile.getInputStream(zipentry);
					OutputStream os = new FileOutputStream(soname);
					Log.i("cr3",
							"Installing JNI library "
									+ soname.getAbsolutePath());
					final int BUF_SIZE = 0x10000;
					byte[] buf = new byte[BUF_SIZE];
					int n;
					while ((n = is.read(buf)) > 0)
						os.write(buf, 0, n);
					is.close();
					os.close();
				} else {
					log.i("JNI library " + soname.getAbsolutePath()
							+ " is up to date");
				}
				System.load(soname.getAbsolutePath());
				//setHyphenationMethod(HYPH_NONE, new byte[] {});
			} catch (Exception e) {
				log.e("cannot install " + LIBRARY_NAME + " library", e);
			}
		}
	}

	public static final BackgroundTextureInfo NO_TEXTURE = new BackgroundTextureInfo(
			BackgroundTextureInfo.NO_TEXTURE_ID, "(SOLID COLOR)", 0);
	private static final BackgroundTextureInfo[] internalTextures = {
			NO_TEXTURE,
			new BackgroundTextureInfo("bg_paper1", "Paper 1",
					R.drawable.bg_paper1),
			new BackgroundTextureInfo("bg_paper1_dark", "Paper 1 (dark)",
					R.drawable.bg_paper1_dark),
			new BackgroundTextureInfo("tx_wood_dark", "Wood (dark)",
					R.drawable.tx_wood_dark),
			new BackgroundTextureInfo("tx_wood", "Wood", R.drawable.tx_wood),
			new BackgroundTextureInfo("tx_wood_dark", "Wood (dark)",
					R.drawable.tx_wood_dark),
			new BackgroundTextureInfo("tx_fabric", "Fabric",
					R.drawable.tx_fabric),
			new BackgroundTextureInfo("tx_fabric_dark", "Fabric (dark)",
					R.drawable.tx_fabric_dark),
			new BackgroundTextureInfo("tx_fabric_indigo_fibre", "Fabric fibre",
					R.drawable.tx_fabric_indigo_fibre),
			new BackgroundTextureInfo("tx_fabric_indigo_fibre_dark",
					"Fabric fibre (dark)",
					R.drawable.tx_fabric_indigo_fibre_dark),
			new BackgroundTextureInfo("tx_gray_sand", "Gray sand",
					R.drawable.tx_gray_sand),
			new BackgroundTextureInfo("tx_gray_sand_dark", "Gray sand (dark)",
					R.drawable.tx_gray_sand_dark),
			new BackgroundTextureInfo("tx_green_wall", "Green wall",
					R.drawable.tx_green_wall),
			new BackgroundTextureInfo("tx_green_wall_dark",
					"Green wall (dark)", R.drawable.tx_green_wall_dark),
			new BackgroundTextureInfo("tx_metal_red_light", "Metall red",
					R.drawable.tx_metal_red_light),
			new BackgroundTextureInfo("tx_metal_red_dark", "Metall red (dark)",
					R.drawable.tx_metal_red_dark),
			new BackgroundTextureInfo("tx_metall_copper", "Metall copper",
					R.drawable.tx_metall_copper),
			new BackgroundTextureInfo("tx_metall_copper_dark",
					"Metall copper (dark)", R.drawable.tx_metall_copper_dark),
			new BackgroundTextureInfo("tx_metall_old_blue", "Metall blue",
					R.drawable.tx_metall_old_blue),
			new BackgroundTextureInfo("tx_metall_old_blue_dark",
					"Metall blue (dark)", R.drawable.tx_metall_old_blue_dark),
			new BackgroundTextureInfo("tx_old_book", "Old book",
					R.drawable.tx_old_book),
			new BackgroundTextureInfo("tx_old_book_dark", "Old book (dark)",
					R.drawable.tx_old_book_dark),
			new BackgroundTextureInfo("tx_old_paper", "Old paper",
					R.drawable.tx_old_paper),
			new BackgroundTextureInfo("tx_old_paper_dark", "Old paper (dark)",
					R.drawable.tx_old_paper_dark),
			new BackgroundTextureInfo("tx_paper", "Paper", R.drawable.tx_paper),
			new BackgroundTextureInfo("tx_paper_dark", "Paper (dark)",
					R.drawable.tx_paper_dark),
			new BackgroundTextureInfo("tx_rust", "Rust", R.drawable.tx_rust),
			new BackgroundTextureInfo("tx_rust_dark", "Rust (dark)",
					R.drawable.tx_rust_dark),
			new BackgroundTextureInfo("tx_sand", "Sand", R.drawable.tx_sand),
			new BackgroundTextureInfo("tx_sand_dark", "Sand (dark)",
					R.drawable.tx_sand_dark),
			new BackgroundTextureInfo("tx_stones", "Stones",
					R.drawable.tx_stones),
			new BackgroundTextureInfo("tx_stones_dark", "Stones (dark)",
					R.drawable.tx_stones_dark), };
	public static final String DEF_DAY_BACKGROUND_TEXTURE = "bg_paper1";
	public static final String DEF_NIGHT_BACKGROUND_TEXTURE = "bg_paper1_dark";

	public BackgroundTextureInfo[] getAvailableTextures() {
		ArrayList<BackgroundTextureInfo> list = new ArrayList<BackgroundTextureInfo>(
				internalTextures.length);
		list.add(NO_TEXTURE);
		findExternalTextures(list);
		for (int i = 1; i < internalTextures.length; i++)
			list.add(internalTextures[i]);
		return list.toArray(new BackgroundTextureInfo[] {});
	}

	public void findHyphDictionariesFromDirectory(File dir) {
		for (File f : dir.listFiles()) {
			if (!f.isDirectory()) {
				if (HyphDict.fromFile(f))
					log.i("Registered external hyphenation dict " + f.getAbsolutePath());
			}
		}
	}

	public void findExternalHyphDictionaries() {
		for (File d : getStorageDirectories(false)) {
			File base = new File(d, ".cr3");
			if (!base.isDirectory())
				base = new File(d, "cr3");
			if (!base.isDirectory())
				continue;
			File subdir = new File(base, "hyph");
			if (subdir.isDirectory())
				findHyphDictionariesFromDirectory(subdir);
		}
	}

	public void findTexturesFromDirectory(File dir,
			Collection<BackgroundTextureInfo> listToAppend) {
		for (File f : dir.listFiles()) {
			if (!f.isDirectory()) {
				BackgroundTextureInfo item = BackgroundTextureInfo.fromFile(f
						.getAbsolutePath());
				if (item != null)
					listToAppend.add(item);
			}
		}
	}

	public void findExternalTextures(
			Collection<BackgroundTextureInfo> listToAppend) {
		for (File d : getStorageDirectories(false)) {
			File base = new File(d, ".cr3");
			if (!base.isDirectory())
				base = new File(d, "cr3");
			if (!base.isDirectory())
				continue;
			File subdirTextures = new File(base, "textures");
			File subdirBackgrounds = new File(base, "backgrounds");
			if (subdirTextures.isDirectory())
				findTexturesFromDirectory(subdirTextures, listToAppend);
			if (subdirBackgrounds.isDirectory())
				findTexturesFromDirectory(subdirBackgrounds, listToAppend);
		}
	}

	public byte[] getImageData(BackgroundTextureInfo texture) {
		if (texture.isNone())
			return null;
		if (texture.resourceId != 0) {
			byte[] data = loadResourceBytes(texture.resourceId);
			return data;
		} else if (texture.id != null && texture.id.startsWith("/")) {
			File f = new File(texture.id);
			byte[] data = loadResourceBytes(f);
			return data;
		}
		return null;
	}

	public BackgroundTextureInfo getTextureInfoById(String id) {
		if (id == null)
			return NO_TEXTURE;
		if (id.startsWith("/")) {
			BackgroundTextureInfo item = BackgroundTextureInfo.fromFile(id);
			if (item != null)
				return item;
		} else {
			for (BackgroundTextureInfo item : internalTextures)
				if (item.id.equals(id))
					return item;
		}
		return NO_TEXTURE;
	}

}
