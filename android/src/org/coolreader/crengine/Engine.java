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

	private final CoolReader mActivity;
	private final BackgroundThread mBackgroundThread;

	// private final View mMainView;
	// private final ExecutorService mExecutor =
	// Executors.newFixedThreadPool(1);

	/**
	 * Get storage root directories.
	 * 
	 * @return array of r/w storage roots
	 */
	public static File[] getStorageDirectories(boolean writableOnly) {
		ArrayList<File> res = new ArrayList<File>(2);
		File dir = Environment.getExternalStorageDirectory();
		if (dir.isDirectory() && (!writableOnly || dir.canWrite()))
			res.add(dir);
		File dir2 = new File("/system/media/sdcard");
		if (dir2.isDirectory() && (!writableOnly || dir2.canWrite()))
			res.add(dir2);
		File dir22 = new File("/sdcard2");
		if (dir22.isDirectory() && (!writableOnly || dir22.canWrite()))
			res.add(dir22);
		File dir3 = new File("/nand");
		if (dir3.isDirectory() && (!writableOnly || dir3.canWrite()))
			res.add(dir3);
		File dir4 = new File("/PocketBook701");
		if (dir4.isDirectory() && (!writableOnly || dir4.canWrite()))
			res.add(dir4);
		return res.toArray(new File[] {});
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
					Log.i("cr3", "running task.work() "
							+ task.getClass().getName());
				if (!initialized)
					throw new IllegalStateException("Engine not initialized");
				// run task
				task.work();
				if (LOG_ENGINE_TASKS)
					Log.i("cr3", "exited task.work() "
							+ task.getClass().getName());
				// post success callback
				mBackgroundThread.postGUI(new Runnable() {
					public void run() {
						if (LOG_ENGINE_TASKS)
							Log.i("cr3", "running task.done() "
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
				Log.e("cr3", "exception while running task "
						+ task.getClass().getName(), e);
				// post error callback
				mBackgroundThread.postGUI(new Runnable() {
					public void run() {
						Log.e("cr3", "running task.fail(" + e.getMessage()
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
			Log.d("cr3", "executing task " + task.getClass().getSimpleName());
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
			Log.d("cr3", "executing task " + task.getClass().getSimpleName());
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
	// Log.v("cr3", "showProgress() - in GUI thread");
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

	private void showProgress(final int mainProgress, final String msg) {
		final int progressId = ++nextProgressId;
		mProgressMessage = msg;
		mProgressPos = mainProgress;
		if (mainProgress == 10000) {
			Log.v("cr3", "mainProgress==10000 : calling hideProgress");
			hideProgress();
			return;
		}
		Log.v("cr3", "showProgress(" + mainProgress + ", \"" + msg
				+ "\") is called : " + Thread.currentThread().getName());
		if (enable_progress) {
			mBackgroundThread.executeGUI(new Runnable() {
				public void run() {
					// show progress
					Log.v("cr3", "showProgress() - in GUI thread");
					if (progressId != nextProgressId) {
						Log.v("cr3",
								"showProgress() - skipping duplicate progress event");
						return;
					}
					if (mProgress == null) {
						try {
							if (mActivity != null && mActivity.isStarted()) {
								Log.v("cr3",
										"showProgress() - in GUI thread : creating progress window");
								if (PROGRESS_STYLE == ProgressDialog.STYLE_HORIZONTAL) {
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
								} else {
									mProgress = ProgressDialog.show(mActivity,
											"Please Wait", msg);
									mProgress.setCancelable(false);
									mProgress.setProgress(mainProgress);
								}
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

	public void hideProgress() {
		final int progressId = ++nextProgressId;
		Log.v("cr3", "hideProgress() - is called : "
				+ Thread.currentThread().getName());
		// Log.v("cr3", "hideProgress() is called");
		mBackgroundThread.executeGUI(new Runnable() {
			public void run() {
				// hide progress
				Log.v("cr3", "hideProgress() - in GUI thread");
				if (progressId != nextProgressId) {
					Log.v("cr3",
							"hideProgress() - skipping duplicate progress event");
					return;
				}
				if (mProgress != null) {
					// if ( mProgress.isShowing() )
					// mProgress.hide();
					progressShown = false;
					progressIcon = null;
					mProgress.dismiss();
					mProgress = null;
					Log.v("cr3", "hideProgress() - in GUI thread, finished");
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
			Log.e("cr3", "cannot load resource from file " + file);
			return null;
		}
	}

	public String loadResourceUtf8(int id) {
		try {
			InputStream is = this.mActivity.getResources().openRawResource(id);
			return loadResourceUtf8(is);
		} catch (Exception e) {
			Log.e("cr3", "cannot load resource " + id);
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
			Log.e("cr3", "cannot load resource");
			return null;
		}
	}

	public byte[] loadResourceBytes(int id) {
		try {
			InputStream is = this.mActivity.getResources().openRawResource(id);
			return loadResourceBytes(is);
		} catch (Exception e) {
			Log.e("cr3", "cannot load resource");
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
			Log.e("cr3", "Cannot open file " + f);
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
			Log.e("cr3", "cannot load resource");
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
		// this.mMainView = mainView;
		//
//		Log.i("cr3", "Engine() : initializing Engine in UI thread");
//		if (!initialized) {
//			installLibrary();
//		}
		Log.i("cr3", "Engine() : scheduling init task");
		BackgroundThread.backgroundExecutor.execute(new Runnable() {
			public void run() {
				try {
					Log.i("cr3", "Engine() : running init() in engine thread");
					init();
					// android.view.ViewRoot.getRunQueue().post(new Runnable() {
					// public void run() {
					//
					// }
					// });
				} catch (final Exception e) {
					Log.e("cr3", "Exception while initializing Engine", e);
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

	public enum HyphDict {
		NONE(HYPH_NONE, 0, "[None]"), ALGORITHM(HYPH_ALGO, 0, "[Algorythmic]"), RUSSIAN(
				HYPH_DICT, R.raw.russian_enus_hyphen, "Russian"), ENGLISH(
				HYPH_DICT, R.raw.english_us_hyphen, "English US"), GERMAN(
				HYPH_DICT, R.raw.german_hyphen, "German"), UKRAINIAN(HYPH_DICT,
				R.raw.ukrain_hyphen, "Ukrainian"), SPANISH(HYPH_DICT,
				R.raw.spanish_hyphen, "Spanish"), FRENCH(HYPH_DICT,
				R.raw.french_hyphen, "French"), BULGARIAN(HYPH_DICT,
				R.raw.bulgarian_hyphen, "Bulgarian"), SWEDISH(HYPH_DICT,
				R.raw.swedish_hyphen, "Swedish"), POLISH(HYPH_DICT,
				R.raw.polish_hyphen, "Polish"), ;
		public final int type;
		public final int resource;
		public final String name;

		private HyphDict(int type, int resource, String name) {
			this.type = type;
			this.resource = resource;
			this.name = name;
		}

		public static HyphDict byCode(String code) {
			for (HyphDict dict : values())
				if (dict.toString().equals(code))
					return dict;
			return NONE;
		}
	};

	private HyphDict currentHyphDict = HyphDict.NONE;

	public boolean setHyphenationDictionary(final HyphDict dict) {
		Log.i("cr3", "setHyphenationDictionary( " + dict + " ) is called");
		if (currentHyphDict == dict)
			return false;
		currentHyphDict = dict;
		// byte[] image = loadResourceBytes(R.drawable.tx_old_book);
		mBackgroundThread.executeBackground(new Runnable() {
			public void run() {
				byte[] data = null;
				if (dict.type == HYPH_DICT && dict.resource != 0) {
					data = loadResourceBytes(dict.resource);
				}
				Log.i("cr3", "Setting engine's hyphenation dictionary to "
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

	final int CACHE_DIR_SIZE = 32000000;

	private String createCacheDir(File baseDir, String subDir) {
		String cacheDirName = null;
		if (baseDir.isDirectory()) {
			if (baseDir.canWrite()) {
				if (subDir != null) {
					baseDir = new File(baseDir, subDir);
					baseDir.mkdir();
				}
				if (baseDir.exists() && baseDir.canWrite()) {
					File cacheDir = new File(baseDir, ".cache");
					if (cacheDir.exists() || cacheDir.mkdirs()) {
						if (cacheDir.canWrite()) {
							cacheDirName = cacheDir.getAbsolutePath();
						}
					}
				}
			} else {
				Log.i("cr3", baseDir.toString() + " is read only");
			}
		} else {
			Log.i("cr3", baseDir.toString() + " is not found");
		}
		return cacheDirName;
	}

	private void initCacheDirectory() {
		String cacheDirName = null;
		// SD card
		cacheDirName = createCacheDir(
				Environment.getExternalStorageDirectory(), "Books");
		// internal SD card on Nook
		if (cacheDirName == null)
			cacheDirName = createCacheDir(new File("/system/media/sdcard"),
					"Books");
		// internal flash
		if (cacheDirName == null) {
			File cacheDir = mActivity.getCacheDir();
			if (!cacheDir.isDirectory())
				cacheDir.mkdir();
			// File cacheDir = mActivity.getDir("cache", Context.MODE_PRIVATE);
			if (cacheDir.isDirectory() && cacheDir.canWrite())
				cacheDirName = cacheDir.getAbsolutePath();
		}
		// set cache directory for engine
		if (cacheDirName != null) {
			Log.i("cr3", cacheDirName
					+ " will be used for cache, maxCacheSize=" + CACHE_DIR_SIZE);
			setCacheDirectoryInternal(cacheDirName, CACHE_DIR_SIZE);
		}
	}

	private void init() throws IOException {
		if (initialized)
			throw new IllegalStateException("Already initialized");
		installLibrary();
		String[] fonts = findFonts();
		if (!initInternal(fonts))
			throw new IOException("Cannot initialize CREngine JNI");
		// Initialization of cache directory
		initCacheDirectory();
		initialized = true;
	}

	// public void waitTasksCompletion()
	// {
	// Log.i("cr3", "waiting for engine tasks completion");
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
		Log.i("cr3", "Engine.uninit() is called");
		BackgroundThread.backgroundExecutor.execute(new Runnable() {
			public void run() {
				Log.i("cr3", "Engine.uninit() : in background thread");
				if (initialized) {
					uninitInternal();
					initialized = false;
				}
			}
		});
		// TODO:
		// waitTasksCompletion();
	}

	protected void finalize() throws Throwable {
		Log.i("cr3", "Engine.finalize() is called");
		// if ( initialized ) {
		// //uninitInternal();
		// initialized = false;
		// }
	}

	static private boolean initialized = false;

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
				Log.v("cr3", "Scanning directory " + fontDir.getAbsolutePath()
						+ " for font files");
				// get font names
				String[] fileList = fontDir.list(new FilenameFilter() {
					public boolean accept(File dir, String filename) {
						String lc = filename.toLowerCase();
						return (lc.endsWith(".ttf") || lc.endsWith(".otf")
								|| lc.endsWith(".pfb") || lc.endsWith(".pfa"))
								&& !filename.endsWith("Fallback.ttf");
					}
				});
				// append path
				for (int i = 0; i < fileList.length; i++) {
					String pathName = new File(fontDir, fileList[i])
							.getAbsolutePath();
					fontPaths.add(pathName);
					Log.v("cr3", "found font: " + pathName);
				}
			}
		}
		return fontPaths.toArray(new String[] {});
	}

	private String LIBRARY_NAME = "cr3engine-45-11";
	private String SO_NAME = "lib" + LIBRARY_NAME + ".so";
	private boolean force_install_library = false;

	private void installLibrary() {
		try {
			if (force_install_library)
				throw new Exception("forcing install");
			// try loading library w/o manual installation
			Log.i("cr3", "trying to load library " + LIBRARY_NAME
					+ " w/o installation");
			System.loadLibrary(LIBRARY_NAME);
			// try invoke native method
			//Log.i("cr3", "trying execute native method ");
			//setHyphenationMethod(HYPH_NONE, new byte[] {});
			Log.i("cr3", LIBRARY_NAME + " loaded successfully");
		} catch (Exception ee) {
			Log.i("cr3", SO_NAME + " not found using standard paths, will install manually");
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
					Log.i("cr3", "JNI library " + soname.getAbsolutePath()
							+ " is up to date");
				}
				System.load(soname.getAbsolutePath());
				//setHyphenationMethod(HYPH_NONE, new byte[] {});
			} catch (Exception e) {
				Log.e("cr3", "cannot install " + LIBRARY_NAME + " library", e);
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
	public static final String DEF_DAY_BACKGROUND_TEXTURE = "tx_fabric";
	public static final String DEF_NIGHT_BACKGROUND_TEXTURE = "tx_metall_old_blue_dark";

	public BackgroundTextureInfo[] getAvailableTextures() {
		ArrayList<BackgroundTextureInfo> list = new ArrayList<BackgroundTextureInfo>(
				internalTextures.length);
		list.add(NO_TEXTURE);
		findExternalTextures(list);
		for (int i = 1; i < internalTextures.length; i++)
			list.add(internalTextures[i]);
		return list.toArray(new BackgroundTextureInfo[] {});
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
