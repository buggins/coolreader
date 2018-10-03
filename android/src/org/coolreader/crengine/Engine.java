package org.coolreader.crengine;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Locale;
import java.util.Map;
import java.util.zip.ZipEntry;

import org.coolreader.R;

import android.app.AlertDialog;
import android.graphics.Bitmap;
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
	public static final Object lock = new Object();

	
	static final private String LIBRARY_NAME = "cr3engine-3-1-2";

	private BaseActivity mActivity;
	
	
	// private final View mMainView;
	// private final ExecutorService mExecutor =
	// Executors.newFixedThreadPool(1);

	/**
	 * Get storage root directories.
	 * 
	 * @return array of r/w storage roots
	 */
	public static File[] getStorageDirectories(boolean writableOnly) {
		Collection<File> res = new HashSet<File>(2);
		for (File dir : mountedRootsList) {
			if (dir.isDirectory() && dir.canRead() && dir.list().length > 0 && (!writableOnly || dir.canWrite()))
				res.add(dir);
		}
		return res.toArray(new File[res.size()]);
	}
	
	public static Map<String, String> getMountedRootsMap() {
		return mountedRootsMap;
	}

	public boolean isRootsMountPoint(String path) {
		if (mountedRootsMap == null)
			return false;
		return mountedRootsMap.containsKey(path);
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
	public static File[] getDataDirectories(String subdir,
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
				// run task
				task.work();
				if (LOG_ENGINE_TASKS)
					log.i("exited task.work() "
							+ task.getClass().getName());
				// post success callback
				BackgroundThread.instance().postGUI(new Runnable() {
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
				BackgroundThread.instance().postGUI(new Runnable() {
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
		BackgroundThread.instance().executeBackground(taskHandler);
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
		BackgroundThread.instance().postBackground(taskHandler);
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
				BackgroundThread.instance().postGUI(task);
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
			BackgroundThread.instance().executeGUI(new Runnable() {
				public void run() {
					// show progress
					//log.v("showProgress() - in GUI thread");
					if (progressId != nextProgressId) {
						//log.v("showProgress() - skipping duplicate progress event");
						return;
					}
					if (mProgress == null) {
						//log.v("showProgress() - creating progress window");
						try {
							if (mActivity != null && mActivity.isStarted()) {
								mProgress = new ProgressDialog(mActivity);
								mProgress
										.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
								if (progressIcon != null)
									mProgress.setIcon(progressIcon);
								else
									mProgress.setIcon(R.mipmap.cr3_logo);
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
		BackgroundThread.instance().executeGUI(new Runnable() {
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

	public static String loadFileUtf8(File file) {
		try {
			InputStream is = new FileInputStream(file);
			return loadResourceUtf8(is);
		} catch (Exception e) {
			log.w("cannot load resource from file " + file);
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

	public static String loadResourceUtf8(InputStream is) {
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

	private static Engine instance;

	public static Engine getInstance(BaseActivity activity) {
		if (instance == null) {
			instance = new Engine(activity);
		} else {
			instance.setParams(activity);
		}
		return instance;
	}
	
	private void setParams(BaseActivity activity) {
		this.mActivity = activity;
	}

	/**
	 * Initialize CoolReader Engine
	 * 
	 * @param fontList
	 *            is array of .ttf font pathnames to load
	 */
	private Engine(BaseActivity activity) {
		setParams(activity);
	}

	public void initAgain() {
		initMountRoots();
		mFonts = findFonts();
		findExternalHyphDictionaries();
		if (!initInternal(mFonts)) {
			log.i("Engine.initInternal failed!");
			throw new RuntimeException("Cannot initialize CREngine JNI");
		}
		initCacheDirectory();
		log.i("Engine() : initialization done");
	}

	// Native functions
	private native static boolean initInternal(String[] fontList);

	private native static void uninitInternal();

	private native static String[] getFontFaceListInternal();

	private native static String[] getFontFileNameListInternal();

	private native static String[] getArchiveItemsInternal(String arcName); // pairs: pathname, size

	private native static boolean setKeyBacklightInternal(int value);

	private native static boolean setCacheDirectoryInternal(String dir, int size);

	private native static boolean scanBookPropertiesInternal(FileInfo info);

	private native static byte[] scanBookCoverInternal(String path);

	private native static void drawBookCoverInternal(Bitmap bmp, byte[] data, String fontFace, String title, String authors, String seriesName, int seriesNumber, int bpp);
	
    private native static void suspendLongOperationInternal(); // cancel current long operation in engine thread (swapping to cache file) -- call it from GUI thread

    
    public static void suspendLongOperation() {
   		suspendLongOperationInternal();
    }
	
	/**
	 * Checks whether specified directlry or file is symbolic link.
	 * (thread-safe)
	 * @param pathName is path to check
	 * @return path link points to if specified directory is link (symlink), null for regular file/dir
	 */
	public native static String isLink(String pathName);
	
	public static String folowLink(String pathName) {
		String lnk = isLink(pathName);
		if (lnk == null)
			return pathName;
		String lnk2 = isLink(lnk);
		if (lnk2 == null)
			return lnk;
		return lnk2;
	}
	
	private static final int HYPH_NONE = 0;
	private static final int HYPH_ALGO = 1;
	private static final int HYPH_DICT = 2;
	private static final int HYPH_BOOK = 0;

	private native boolean setHyphenationMethod(int type, byte[] dictData);

	public ArrayList<ZipEntry> getArchiveItems(String zipFileName) {
		final int itemsPerEntry = 2;
		String[] in;
		synchronized(lock) {
		    in = getArchiveItemsInternal(zipFileName);
		}
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
		public final static HyphDict NONE = new HyphDict("NONE", HYPH_NONE, 0, "[None]", "");
		public final static HyphDict ALGORITHM = new HyphDict("ALGORITHM", HYPH_ALGO, 0, "[Algorythmic]", ""); 
		public final static HyphDict BOOK_LANGUAGE = new HyphDict("BOOK LANGUAGE", HYPH_BOOK, 0, "[From Book Language]", ""); 
		public final static HyphDict RUSSIAN = new HyphDict("RUSSIAN", HYPH_DICT, R.raw.russian_enus_hyphen, "Russian", "ru"); 
		public final static HyphDict ENGLISH = new HyphDict("ENGLISH", HYPH_DICT, R.raw.english_us_hyphen, "English US", "en"); 
		public final static HyphDict GERMAN = new HyphDict("GERMAN", HYPH_DICT, R.raw.german_hyphen, "German", "de"); 
		public final static HyphDict UKRAINIAN = new HyphDict("UKRAINIAN", HYPH_DICT,R.raw.ukrain_hyphen, "Ukrainian", "uk"); 
		public final static HyphDict SPANISH = new HyphDict("SPANISH", HYPH_DICT,R.raw.spanish_hyphen, "Spanish", "es"); 
		public final static HyphDict FRENCH = new HyphDict("FRENCH", HYPH_DICT,R.raw.french_hyphen, "French", "fr"); 
		public final static HyphDict BULGARIAN = new HyphDict("BULGARIAN", HYPH_DICT, R.raw.bulgarian_hyphen, "Bulgarian", "bg"); 
		public final static HyphDict SWEDISH = new HyphDict("SWEDISH", HYPH_DICT, R.raw.swedish_hyphen, "Swedish", "sv"); 
		public final static HyphDict POLISH = new HyphDict("POLISH", HYPH_DICT, R.raw.polish_hyphen, "Polish", "pl");
		public final static HyphDict HUNGARIAN = new HyphDict("HUNGARIAN", HYPH_DICT, R.raw.hungarian_hyphen, "Hungarian", "hu");
		public final static HyphDict GREEK = new HyphDict("GREEK", HYPH_DICT, R.raw.greek_hyphen, "Greek", "el");
		public final static HyphDict FINNISH = new HyphDict("FINNISH", HYPH_DICT, R.raw.finnish_hyphen, "Finnish", "fi");
		public final static HyphDict TURKISH = new HyphDict("TURKISH", HYPH_DICT, R.raw.turkish_hyphen, "Turkish", "tr");
		public final static HyphDict DUTCH = new HyphDict("DUTCH", HYPH_DICT, R.raw.dutch_hyphen, "Dutch", "nl");
		public final static HyphDict CATALAN = new HyphDict("CATALAN", HYPH_DICT, R.raw.catalan_hyphen, "Catalan", "ca");

		public final String code;
		public final int type;
		public final int resource;
		public final String name;
		public final File file;
		public String language;

		
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
		
		private HyphDict(String code, int type, int resource, String name, String language) {
			this.type = type;
			this.resource = resource;
			this.name = name;
			this.file = null;
			this.code = code;
			this.language = language;
			// register in list
			add(this);
		}

		private HyphDict(File file) {
			this.type = HYPH_DICT;
			this.resource = 0;
			this.name = file.getName();
			this.file = file;
			this.code = this.name;
			this.language = "";
			// register in list
			add(this);
		}

		private static HyphDict byLanguage(String language) {
			if (language != null && !language.trim().equals("")) {
				for (HyphDict dict : values) {
					if (dict != BOOK_LANGUAGE) {
						if (dict.language.equals(language))
							return dict;
					}
				}
			}
			return NONE;
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
		public String getName() {
			if (this == BOOK_LANGUAGE) {
				if (language != null && !language.trim().equals("")) { 
					return this.name + " (currently: " + this.language + ")";
				} else {
					return this.name + " (currently: none)";
				}
			} else {
				return name;
			}
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
	private String currentHyphLanguage = null;

	public boolean setHyphenationLanguage(final String wanted_language) {
		String language = getLanguage(wanted_language);
		log.i("setHyphenationLanguage( " + language + " ) is called");
		if (language == currentHyphLanguage || currentHyphDict != HyphDict.BOOK_LANGUAGE)
			return false;
		currentHyphLanguage = language;
		HyphDict dict = HyphDict.byLanguage(language);
		setHyphenationDictionaryInternal(dict);
		if (dict != null) {
			HyphDict.BOOK_LANGUAGE.language = language;
		} else {
			HyphDict.BOOK_LANGUAGE.language = "";
		}
		log.i("setHyphenationLanguage( " + language + " ) set to " + dict);
		return true;
	}

	private String getLanguage(final String language) {
		if (language == null || "".equals(language.trim())) {
			return "";
		} else if (language.contains("-")) {
			return language.substring(0, language.indexOf("-")).toLowerCase();
		} else {
			return language.toLowerCase();
		}
	}
		
	public boolean setHyphenationDictionary(final HyphDict dict) {
		log.i("setHyphenationDictionary( " + dict + " ) is called");
		if (currentHyphDict == dict)
			return false;
		currentHyphDict = dict;
		setHyphenationDictionaryInternal(dict);
		return true;
	}
	
	private void setHyphenationDictionaryInternal(final HyphDict dict) {
		// byte[] image = loadResourceBytes(R.drawable.tx_old_book);
		BackgroundThread.instance().postBackground(new Runnable() {
			public void run() {
				byte[] data = null;
				if (dict.type == HYPH_DICT) {
					if (dict.resource!=0) {
						data = loadResourceBytes(dict.resource);
					} else if (dict.file!=null) {
						data = loadResourceBytes(dict.file);
					}
				}
				log.i("Setting engine's hyphenation dictionary to " + dict);
				setHyphenationMethod(dict.type, data);
			}
		});
	}

	public boolean scanBookProperties(FileInfo info) {
		synchronized(lock) {
			long start = android.os.SystemClock.uptimeMillis();
			boolean res = scanBookPropertiesInternal(info);
			long duration = android.os.SystemClock.uptimeMillis() - start;
			L.v("scanBookProperties took " + duration + " ms for " + info.getPathName());
			return res;
		}
	}

	public byte[] scanBookCover(String path) {
		synchronized(lock) {
			long start = Utils.timeStamp();
			byte[] res = scanBookCoverInternal(path);
			long duration = Utils.timeInterval(start);
			L.v("scanBookCover took " + duration + " ms for " + path);
			return res;
		}
	}

	/**
	 * Draw book coverpage into bitmap buffer.
	 * If cover image specified, this image will be drawn (resized to buffer size).
	 * If no cover image, default coverpage will be drawn, with author, title, series.
	 * @param bmp is buffer to draw in.
	 * @param data is coverpage image data bytes, or empty array if no cover image
	 * @param fontFace is font face to use.
	 * @param title is book title.
	 * @param authors is book authors list
	 * @param seriesName is series name
	 * @param seriesNumber is series number
	 * @param bpp is bits per pixel (specify <=8 for eink grayscale dithering)
	 */
	public void drawBookCover(Bitmap bmp, byte[] data, String fontFace, String title, String authors, String seriesName, int seriesNumber, int bpp) {
		synchronized(lock) {
			long start = Utils.timeStamp();
			drawBookCoverInternal(bmp, data, fontFace, title, authors, seriesName, seriesNumber, bpp);
			long duration = Utils.timeInterval(start);
			L.v("drawBookCover took " + duration + " ms");
		}
	}

	public static String[] getFontFaceList() {
		synchronized(lock) {
			return getFontFaceListInternal();
		}
	}

	public static String[] getFontFileNameList() {
		synchronized(lock) {
			return getFontFileNameListInternal();
		}
	}

	private int currentKeyBacklightLevel = 1;
	public int getKeyBacklight() {
		return currentKeyBacklightLevel;
	}
	public boolean setKeyBacklight(int value) {
		currentKeyBacklightLevel = value;
		// thread safe
		return setKeyBacklightInternal(value);
	}
	
	final static int CACHE_DIR_SIZE = 32000000;

	private static String createCacheDir(File baseDir, String subDir) {
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
	private static void initCacheDirectory() {
		String cacheDirName = null;
		// SD card
		cacheDirName = createCacheDir(
				DeviceInfo.EINK_NOOK ? new File("/media/") : Environment.getExternalStorageDirectory(), CACHE_BASE_DIR_NAME);
		// non-standard SD mount points
		log.i(cacheDirName
				+ " will be used for cache, maxCacheSize=" + CACHE_DIR_SIZE);
		if (cacheDirName == null) {
			for (String dirname : mountedRootsMap.keySet()) {
				cacheDirName = createCacheDir(new File(dirname),
						CACHE_BASE_DIR_NAME);
				if ( cacheDirName!=null )
					break;
			}
		}
		// internal flash
//		if (cacheDirName == null) {
//			File cacheDir = mActivity.getCacheDir();
//			if (!cacheDir.isDirectory())
//				cacheDir.mkdir();
//			cacheDirName = createCacheDir(cacheDir, null);
//			// File cacheDir = mActivity.getDir("cache", Context.MODE_PRIVATE);
////			if (cacheDir.isDirectory() && cacheDir.canWrite())
////				cacheDirName = cacheDir.getAbsolutePath();
//		}
		// set cache directory for engine
		if (cacheDirName != null) {
			log.i(cacheDirName
					+ " will be used for cache, maxCacheSize=" + CACHE_DIR_SIZE);
			setCacheDirectoryInternal(cacheDirName, CACHE_DIR_SIZE);
		} else {
			log.w("No directory for cache is available!");
		}
	}

	private static boolean addMountRoot(Map<String, String> list, String pathname, int resourceId)
	{
		return addMountRoot(list, pathname, pathname); //mActivity.getResources().getString(resourceId));
	}
	
	public static boolean isStorageDir(String path) {
		if (path == null)
			return false;
		String normalized = pathCorrector.normalizeIfPossible(path);
		String sdpath = pathCorrector.normalizeIfPossible(Environment.getExternalStorageDirectory().getAbsolutePath());
		if (sdpath != null && sdpath.equals(normalized))
			return true;
		return false;
	}
	
	public static boolean isExternalStorageDir(String path) {
		if (path == null)
			return false;
		if (path.contains("/ext"))
			return true;
		return false;
	}
	
	private static boolean addMountRoot(Map<String, String> list, String path, String name) {
		if (list.containsKey(path))
			return false;
		if (path.equals("/storage/emulated/legacy")) {
			for (String key : list.keySet()) {
				if (key.equals("/storage/emulated/0"))
					return false; // don't add "/storage/emulated/legacy" after "/storage/emulated/0"
			}
		}
		String plink = folowLink(path);
		for (String key : list.keySet()) {
			//if (pathCorrector.normalizeIfPossible(path).equals(pathCorrector.normalizeIfPossible(key))) {
			if (plink.equals(folowLink(key))) { // path.startsWith(key + "/")
				log.w("Skipping duplicate path " + path + " == " + key);
				return false; // duplicate subpath
			}
		}
		try {
			File dir = new File(path);
			if (dir.isDirectory()) {
//				String[] d = dir.list();
//				if ((d!=null && d.length>0) || dir.canWrite()) {
					// Android 6 ext storage
					if (name.startsWith("/storage/") && name.length() == 18 && name.charAt(13) == '-')
						name = "EXT SD";
					log.i("Adding FS root: " + path + " " + name);
					list.put(path, name);
//					return true;
//				} else {
//					log.i("Skipping mount point " + path + " : no files or directories found here, and writing is disabled");
//				}
			}
		} catch (Exception e) {
			// ignore
		}
		return false;
	}
	
	public static HashSet<String> listStorageDir() {
	    final HashSet<String> out = new HashSet<String>();
	    File dir = new File("/storage");
	    try {
		    if (dir.exists() && dir.isDirectory()) {
		    	File[] files = dir.listFiles();
		    	for(File file : files) {
		    		if (file.isDirectory() && file.canRead() && !"/storage/emulated".equals(file.getName())) {
		    			log.d("listStorageDir path found: " + file.getAbsolutePath());
		    			out.add(file.getAbsolutePath());
		    		}
		    	}
		    }
	    } catch (Exception e) {
	    	// ignore
	    }
	    return out;
	}
	public static HashSet<String> getExternalMounts() {
	    final HashSet<String> out = new HashSet<String>();
	    try {
		    String reg = "(?i).*vold.*(vfat|ntfs|exfat|fat32|ext3|ext4).*rw.*";
		    String reg2 = "(?i).*fuse.*(vfat|ntfs|exfat|fat32|ext3|ext4).*rw.*";
		    String s = "";
		    try {
		        final Process process = new ProcessBuilder().command("mount")
		                .redirectErrorStream(true).start();
		        ProcessWithTimeout processWithTimeout = new ProcessWithTimeout(process);
		        int exitCode = processWithTimeout.waitForProcess(100);
		        if (exitCode == Integer.MIN_VALUE)
		        {
		            // Timeout
		            log.e("Timed out waiting for mount command output, " +
		                  "please add CoolReader to MagiskHide list!");
                            process.destroy();
                            return out;
		        }
		        final InputStream is = process.getInputStream();
		        final byte[] buffer = new byte[1024];
		        while (is.read(buffer) != -1) {
		            s = s + new String(buffer);
		        }
		        is.close();
		    } catch (final Exception e) {
		        e.printStackTrace();
		    }
	
		    // parse output
		    final String[] lines = s.split("\n");
		    for (String line : lines) {
		        if (!line.toLowerCase(Locale.US).contains("asec")) {
		        	log.d("mount entry: " + line);
		            if (line.matches(reg) || line.matches(reg2)) {
		                String[] parts = line.split(" ");
		                for (String part : parts) {
		                    if (part.startsWith("/"))
		                        if (!part.toLowerCase(Locale.US).contains("vold"))
		                            out.add(part);
		                }
		            }
		        }
		    }
	    } catch (Exception e) {
	    	// ignore
        	log.d("exception", e);
	    }
	    log.d("mount pathes: " + out);
	    return out;
	}	
	
    private static HashSet<String> readMountsFile() {
	    final HashSet<String> out = new HashSet<String>();
        /*
         * Scan the /proc/mounts file and look for lines like this:
         * /dev/block/vold/179:1 /mnt/sdcard vfat
         * rw,dirsync,nosuid,nodev,noexec,
         * relatime,uid=1000,gid=1015,fmask=0602,dmask
         * =0602,allow_utime=0020,codepage
         * =cp437,iocharset=iso8859-1,shortname=mixed,utf8,errors=remount-ro 0 0
         * 
         * When one is found, split it into its elements and then pull out the
         * path to the that mount point and add it to the arraylist
         */

        try {
			String s = loadFileUtf8(new File("/proc/mounts"));
			if (s != null) {
				String[] rows = s.split("\n");
				for(String line : rows) {
	                if (line.startsWith("/dev/block/vold/") || line.startsWith("/dev/fuse ")) {
	                    String[] lineElements = line.split(" ");
	                    String element = lineElements[1];
	                    if (element.startsWith("/"))
	                        out.add(element);	
	                }
				}
			}
        } catch (Exception e) {
        	// ignore
        }
        return out;
    }

	
	
	private static void initMountRoots() {

		log.i("initMountRoots()");
		HashSet<String> mountedPathsFromMountCmd = getExternalMounts();
		HashSet<String> mountedPathsFromMountFile = readMountsFile();
		HashSet<String> mountedPathsStorageDir = listStorageDir();
		log.i("mountedPathsFromMountCmd: " + mountedPathsFromMountCmd);
		log.i("mountedPathsFromMountFile: " + mountedPathsFromMountFile);
		log.i("mountedPathsStorageDir: " + mountedPathsStorageDir);
		
		Map<String, String> map = new LinkedHashMap<String, String>();

		// standard external directory
		String sdpath = DeviceInfo.EINK_NOOK ? "/media/" : Environment.getExternalStorageDirectory().getAbsolutePath();
		// dirty fix
		if ("/nand".equals(sdpath) && new File("/sdcard").isDirectory())
			sdpath = "/sdcard";
		//String sdlink = isLink(sdpath);
		//if (sdlink != null)
		//	sdpath = sdlink;
		
		// main storage
		addMountRoot(map, sdpath, "SD");

		// retrieve list of mount points from system
		String[] fstabLocations = new String[] {
			"/system/etc/vold.conf",
			"/system/etc/vold.fstab",
			"/etc/vold.conf",
			"/etc/vold.fstab",
		};
		String s = null;
		String fstabFileName = null;
		for (String fstabFile : fstabLocations) {
			fstabFileName = fstabFile;
			s = loadFileUtf8(new File(fstabFile));
			if (s != null)
				log.i("found fstab file " + fstabFile);
		}
		if (s == null)
			log.w("fstab file not found");
		if ( s!= null) {
			String[] rows = s.split("\n");
			int rulesFound = 0;
			for (String row : rows) {
				if (row != null && row.startsWith("dev_mount")) {
					log.d("mount rule: " + row);
					rulesFound++;
					String[] cols = Utils.splitByWhitespace(row);
					if (cols.length >= 5) {
						String name = Utils.ntrim(cols[1]);
						String point = Utils.ntrim(cols[2]);
						String mode = Utils.ntrim(cols[3]);
						String dev = Utils.ntrim(cols[4]);
						if (Utils.empty(name) || Utils.empty(point) || Utils.empty(mode) || Utils.empty(dev))
							continue;
						String label = null;
						boolean hasusb = dev.indexOf("usb") >= 0;
						boolean hasmmc = dev.indexOf("mmc") >= 0;
						log.i("*** mount point '" + name + "' *** " + point + "  (" + dev + ")");
						if ("auto".equals(mode)) {
							// assume AUTO is for externally automount devices
							if (hasusb)
								label = "USB Storage";
							else if (hasmmc)
								label = "External SD";
							else
								label = "External Storage";
						} else {
							if (hasmmc)
								label = "Internal SD";
							else
								label = "Internal Storage";
						}
						if (!point.equals(sdpath)) {
							// external SD
							addMountRoot(map, point, label + " (" + point + ")");
						}
					}
				}
			}
			if (rulesFound == 0)
				log.w("mount point rules not found in " + fstabFileName);
		}

		// TODO: probably, hardcoded list is not necessary after /etc/vold parsing 
		String[] knownMountPoints = new String[] {
			"/system/media/sdcard", // internal SD card on Nook
			"/media",
			"/nand",
			"/PocketBook701", // internal SD card on PocketBook 701 IQ
			"/mnt/extsd",
			"/mnt/external1",
			"/mnt/external_sd",
			"/mnt/udisk",
			"/mnt/sdcard2",
			"/mnt/ext.sd",
			"/ext.sd",
			"/extsd",
			"/storage/sdcard",
			"/storage/sdcard0",
			"/storage/sdcard1",
			"/storage/sdcard2",
			"/mnt/extSdCard",
			"/sdcard",
			"/sdcard2",
			"/mnt/udisk",
			"/sdcard-ext",
			"/sd-ext",
			"/mnt/external1",
			"/mnt/external2",
			"/mnt/sdcard1",
			"/mnt/sdcard2",
			"/mnt/usb_storage",
			"/mnt/external_SD",
			"/emmc",
			"/external",
			"/Removable/SD",
			"/Removable/MicroSD",
			"/Removable/USBDisk1",
			"/storage/sdcard1",
			"/mnt/sdcard/extStorages/SdCard",
			"/storage/extSdCard",
			"/storage/external_SD",
		};
		// collect mount points from all possible sources
		HashSet<String> mountPointsToAdd = new HashSet<String>();
		for (String point : knownMountPoints) {
			mountPointsToAdd.add(point);
		}
		mountPointsToAdd.addAll(mountedPathsFromMountCmd);
		mountPointsToAdd.addAll(mountedPathsFromMountFile);
		mountPointsToAdd.addAll(mountedPathsStorageDir);
		mountPointsToAdd.add(Environment.getExternalStorageDirectory().getAbsolutePath());
		String storageList = System.getenv("SECONDARY_STORAGE");
		if (storageList != null) {
			for (String point : storageList.split(":")) {
				if (point.startsWith("/"))
					mountPointsToAdd.add(point);
			}
		}
		// add mount points
		
		for (String point : mountPointsToAdd) {
			if (point == null)
				continue;
			point = point.trim();
			if (point.length() == 0)
				continue;
			File dir = new File(point);
			if (dir.isDirectory() && dir.canRead()) {
				String[] files = dir.list();
				if (files != null && files.length > 0) {
					String link = isLink(point);
					if (link != null) {
						log.d("found mount point path is link: " + point + " > " + link);
						addMountRoot(map, link, link);
					} else {
						addMountRoot(map, point, point);
					}
				}
			}
		}
		
		// auto detection
		//autoAddRoots(map, "/", SYSTEM_ROOT_PATHS);
		//autoAddRoots(map, "/mnt", new String[] {});
		
		mountedRootsMap = map;
		Collection<File> list = new ArrayList<File>();
		
		for (String f : map.keySet()) {
			File path = new File(f);
			list.add(path);
		}
		
		mountedRootsList = list.toArray(new File[] {});
		pathCorrector = new MountPathCorrector(mountedRootsList);

		for (String point : mountPointsToAdd) {
			String link = isLink(point);
			if (link != null) {
				log.d("pathCorrector.addRootLink(" + point + ", " + link + ")");
				pathCorrector.addRootLink(point, link);
			}
		}
		
		Log.i("cr3", "Root list: " + list + ", root links: " + pathCorrector);

		
		log.i("Mount ROOTS:");
		for (String f : map.keySet()) {
			File path = new File(f);
			String label = map.get(f);
			if (isStorageDir(f)) {
				label = "SD";
				map.put(f, label);
			} else if (isExternalStorageDir(f)) {
				label = "Ext SD";
				map.put(f, label);
			}
			
			log.i("*** " + f + " '" + label + "' isDirectory=" + path.isDirectory() + " canRead=" + path.canRead() + " canWrite=" + path.canRead() + " isLink=" + isLink(f));
		}
		
//		testPathNormalization("/sdcard/books/test.fb2");
//		testPathNormalization("/mnt/sdcard/downloads/test.fb2");
//		testPathNormalization("/mnt/sd/dir/test.fb2");
	}
	
//	private void testPathNormalization(String path) {
//		Log.i("cr3", "normalization: " + path + " => " + normalizePathUsingRootLinks(new File(path)));
//	}
	

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
//		log.i("Engine.uninit() is called for " + hashCode());
//		if (initialized) {
//			synchronized(this) {
//				uninitInternal();
//			}
//			initialized = false;
//		}
		instance = null;
	}

	protected void finalize() throws Throwable {
		log.i("Engine.finalize() is called for " + hashCode());
		// if ( initialized ) {
		// //uninitInternal();
		// initialized = false;
		// }
	}

	private static String[] findFonts() {
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
		Collections.sort(fontPaths);
		return fontPaths.toArray(new String[] {});
	}

	public static final ArrayList<String> getFontsDirs() {
		HashMap<String, Integer> dirsCapacity = new HashMap<String, Integer>();
		ArrayList<File> dirs = new ArrayList<File>();
		File[] dataDirs = getDataDirectories("fonts", false, false);
		for (File dir : dataDirs)
			dirs.add(dir);
		File[] rootDirs = getStorageDirectories(false);
		for (File dir : rootDirs)
			dirs.add(new File(dir, "fonts"));
		dirs.add(new File(Environment.getRootDirectory(), "fonts"));
		for (File fontDir : dirs) {
			if (fontDir.isDirectory())
				dirsCapacity.put(fontDir.getAbsolutePath(), Integer.valueOf(0));
		}
		String[] fontFileNameList = getFontFileNameList();
		for (String fontFileName: fontFileNameList) {
			log.d("enum registered font: " + fontFileName);
			for (File fontDir : dirs) {
				if (fontFileName.startsWith(fontDir.getAbsolutePath())) {
					Integer prevCount = dirsCapacity.get(fontDir.getAbsolutePath());
					if (null == prevCount)
						prevCount = Integer.valueOf(0);
					dirsCapacity.put(fontDir.getAbsolutePath(), new Integer(prevCount.intValue() + 1));
				}
			}
		}
		ArrayList<String> resArray = new ArrayList<String>();
		Map.Entry<String, Integer> entry;
		Iterator<Map.Entry<String, Integer>> it = dirsCapacity.entrySet().iterator();
		while (it.hasNext()) {
			entry = it.next();
			resArray.add(entry.getKey() + ": " + entry.getValue().toString() + " registered font(s)");
		}
		return resArray;
	}

	private String SO_NAME = "lib" + LIBRARY_NAME + ".so";
//	private static boolean force_install_library = false;

	private static void installLibrary() {
		try {
//			if (force_install_library)
//				throw new Exception("forcing install");
			// try loading library w/o manual installation
			log.i("trying to load library " + LIBRARY_NAME
					+ " w/o installation");
			System.loadLibrary(LIBRARY_NAME);
			// try invoke native method
			//log.i("trying execute native method ");
			//setHyphenationMethod(HYPH_NONE, new byte[] {});
			log.i(LIBRARY_NAME + " loaded successfully");
//		} catch (Exception ee) {
//			log.i(SO_NAME + " not found using standard paths, will install manually");
//			File sopath = mActivity.getDir("libs", Context.MODE_PRIVATE);
//			File soname = new File(sopath, SO_NAME);
//			try {
//				sopath.mkdirs();
//				File zip = new File(mActivity.getPackageCodePath());
//				ZipFile zipfile = new ZipFile(zip);
//				ZipEntry zipentry = zipfile.getEntry("lib/armeabi/" + SO_NAME);
//				if (!soname.exists() || zipentry.getSize() != soname.length()) {
//					InputStream is = zipfile.getInputStream(zipentry);
//					OutputStream os = new FileOutputStream(soname);
//					Log.i("cr3",
//							"Installing JNI library "
//									+ soname.getAbsolutePath());
//					final int BUF_SIZE = 0x10000;
//					byte[] buf = new byte[BUF_SIZE];
//					int n;
//					while ((n = is.read(buf)) > 0)
//						os.write(buf, 0, n);
//					is.close();
//					os.close();
//				} else {
//					log.i("JNI library " + soname.getAbsolutePath()
//							+ " is up to date");
//				}
//				System.load(soname.getAbsolutePath());
//				//setHyphenationMethod(HYPH_NONE, new byte[] {});
			} catch (Exception e) {
				log.e("cannot install " + LIBRARY_NAME + " library", e);
				throw new RuntimeException("Cannot load JNI library");
//			}
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
			new BackgroundTextureInfo("bg_paper2", "Paper 2",
					R.drawable.bg_paper2),
			new BackgroundTextureInfo("bg_paper2_dark", "Paper 2 (dark)",
					R.drawable.bg_paper2_dark),
			new BackgroundTextureInfo("tx_wood", "Wood", 
					DeviceInfo.getSDKLevel() == 3 ? R.drawable.tx_wood_v3 : R.drawable.tx_wood),
			new BackgroundTextureInfo("tx_wood_dark", "Wood (dark)",
					DeviceInfo.getSDKLevel() == 3 ? R.drawable.tx_wood_dark_v3 : R.drawable.tx_wood_dark),
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

	public static void findHyphDictionariesFromDirectory(File dir) {
		for (File f : dir.listFiles()) {
			if (f.isFile()) {
				if (HyphDict.fromFile(f))
					log.i("Registered external hyphenation dict " + f.getAbsolutePath());
			}
		}
	}

	public static void findExternalHyphDictionaries() {
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
			if (f.isFile()) {
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

	enum DataDirType {
		TexturesDirs,
		BackgroundsDirs,
		HyphsDirs
	}

	public static ArrayList<String> getDataDirs(DataDirType dirType) {
		ArrayList<String> res = new ArrayList<String>();
		for (File d : getStorageDirectories(false)) {
			File base = new File(d, ".cr3");
			if (!base.isDirectory())
				base = new File(d, "cr3");
			if (!base.isDirectory())
				continue;
			switch (dirType) {
				case TexturesDirs:
					File subdirTextures = new File(base, "textures");
					if (subdirTextures.isDirectory())
						res.add(subdirTextures.getAbsolutePath());
					else
						res.add(subdirTextures.getAbsolutePath() + " [not found]");
					break;
				case BackgroundsDirs:
					File subdirBackgrounds = new File(base, "backgrounds");
					if (subdirBackgrounds.isDirectory())
						res.add(subdirBackgrounds.getAbsolutePath());
					else
						res.add(subdirBackgrounds.getAbsolutePath() + " [not found]");
					break;
				case HyphsDirs:
					File subdirHyph = new File(base, "hyph");
					if (subdirHyph.isDirectory())
						res.add(subdirHyph.getAbsolutePath());
					else
						res.add(subdirHyph.getAbsolutePath() + " [not found]");
					break;
			}
		}
		return res;
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

	/**
	 * Create progress dialog control.
	 * @param resourceId is string resource Id of dialog title, 0 to disable progress
	 * @return created control object.
	 */
	public ProgressControl createProgress(int resourceId) {
		return new ProgressControl(resourceId);
	}
	private static final int PROGRESS_UPDATE_INTERVAL = DeviceInfo.EINK_SCREEN ? 4000 : 500;
	private static final int PROGRESS_SHOW_INTERVAL = DeviceInfo.EINK_SCREEN ? 4000 : 1500;
	public class ProgressControl {
		private final int resourceId;
		private long createTime = Utils.timeStamp();
		private long lastUpdateTime;
		private boolean shown;
		private ProgressControl(int resourceId) {
			this.resourceId = resourceId;
		}
		public void hide() {
			if (resourceId == 0)
				return; // disabled
			if (shown)
				hideProgress();
			shown = false;
		}
		public void setProgress(int percent) {
			if (resourceId == 0)
				return; // disabled
			if (Utils.timeInterval(createTime) < PROGRESS_SHOW_INTERVAL)
				return;
			if (Utils.timeInterval(lastUpdateTime) < PROGRESS_UPDATE_INTERVAL)
				return;
			shown = true;
			lastUpdateTime = Utils.timeStamp();
			showProgress(percent, resourceId);
		}
	}

	public MountPathCorrector getPathCorrector() {
		return pathCorrector;
	}

	private static File[] mountedRootsList;
	private static Map<String, String> mountedRootsMap;
	private static MountPathCorrector pathCorrector;
	private static String[] mFonts;

	// static initialization
	static {
		log.i("Engine() : static initialization");
		installLibrary();
		initMountRoots();
		mFonts = findFonts();
		findExternalHyphDictionaries();
		if (!initInternal(mFonts)) {
			log.i("Engine.initInternal failed!");
			throw new RuntimeException("Cannot initialize CREngine JNI");
		}
		initCacheDirectory();
		log.i("Engine() : initialization done");
	}
}
