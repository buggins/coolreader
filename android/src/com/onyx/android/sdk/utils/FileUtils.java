package com.onyx.android.sdk.utils;

// Commented usage of class Benchmark
// Replaced Debug with Log

import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.media.MediaScannerConnection;
import android.media.MediaScannerConnection.OnScanCompletedListener;
import android.net.Uri;
import android.os.Build.VERSION;
import android.util.Log;

import com.onyx.android.sdk.data.SortBy;
import com.onyx.android.sdk.data.SortOrder;
import com.onyx.android.sdk.device.Device;
import com.onyx.android.sdk.device.EnvironmentUtil;

import java.io.BufferedReader;
import java.io.Closeable;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.RandomAccessFile;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.nio.channels.FileChannel;
import java.nio.charset.Charset;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.Stack;
import java.util.concurrent.atomic.AtomicBoolean;

public class FileUtils {
	private static final String TAG = FileUtils.class.getSimpleName();

	public FileUtils() {
	}

	public static boolean fileExist(String path) {
		if (StringUtils.isNullOrEmpty(path)) {
			return false;
		} else {
			File var1 = new File(path);
			return var1.exists();
		}
	}

	public static boolean mkdirs(String path) {
		File file = new File(path);
		return file.exists() ? file.isDirectory() : file.mkdirs();
	}

	public static void purgeDirectory(File dir) {
		File[] files = dir.listFiles();
		if (files != null && files.length > 0) {
			for (File file : files) {
				if (file.isFile()) {
					file.delete();
				}
			}
		}
	}

	public static String getFileExtension(String fileName) {
		if (StringUtils.isNullOrEmpty(fileName)) {
			return "";
		} else {
			int index = fileName.lastIndexOf('.');
			return index >= 0 ? fileName.substring(index + 1).toLowerCase(Locale.getDefault()) : "";
		}
	}

	public static String getFileExtension(File file) {
		return getFileExtension(file.getName());
	}

	public static boolean isPngExtension(File file) {
		return getFileExtension(file).toLowerCase().equals("png");
	}

	public static boolean isJpgExtension(File file) {
		String var1 = getFileExtension(file).toLowerCase();
		return var1.equals("jpg") || var1.equals("jpeg");
	}

	public static void collectFiles(String parentPath, Set<String> extensionFilters, boolean recursive, Collection<String> fileList) {
		File dir = new File(parentPath);
		File[] files = dir.listFiles();
		if (files != null && files.length > 0) {
			for (File file : files) {
				if (!file.isHidden()) {
					String absolutePath = file.getAbsolutePath();
					if (file.isFile()) {
						String ext = getFileExtension(absolutePath);
						if (extensionFilters == null || extensionFilters.contains(ext)) {
							fileList.add(absolutePath);
						}
					} else if (file.isDirectory() && recursive) {
						collectFiles(absolutePath, extensionFilters, recursive, fileList);
					}
				}
			}
		}
	}

	public static void collectDirs(String parentPath, boolean recursive, Collection<String> dirList) {
		File dir = new File(parentPath);
		File[] files = dir.listFiles();
		if (files != null && files.length > 0) {
			for (File file : files) {
				if (!file.isHidden() && !file.isFile()) {
					dirList.add(file.getAbsolutePath());
					if (recursive) {
						collectDirs(file.getAbsolutePath(), recursive, dirList);
					}
				}
			}
		}
	}

	public static void collectFileTree(File rootFile, List<File> flattenedFileList, AtomicBoolean abortHolder) {
		if (rootFile.exists()) {
			Comparator<File> comparator = new Comparator<File>() {
				public int compare(File var1, File var2) {
					return -var1.getName().compareToIgnoreCase(var2.getName());
				}
			};
			Stack<File> stack = new Stack<File>();
			stack.push(rootFile);
			while (true) {
				File[] files;
				do {
					File file;
					do {
						if (stack.isEmpty()) {
							return;
						}
						if (abortHolder.get()) {
							return;
						}
						file = (File) stack.pop();
						flattenedFileList.add(file);
					} while (!file.isDirectory());
					files = file.listFiles();
				} while (files == null);

				Arrays.sort(files, comparator);
				for (File file : files) {
					if (!file.getName().equals(".") && !file.getName().equals("..")) {
						stack.push(file);
					}
				}
			}
		}
	}

	public static String getParent(String path) {
		File file = new File(path);
		return file.getParent();
	}

	public static String getFileName(String path) {
		File file = new File(path);
		return file.getName();
	}

	public static String getBaseName(File file) {
		return file == null ? "" : getBaseName(file.getAbsolutePath());
	}

	public static String getBaseName(String path) {
		String fileName = getFileName(path);
		int index = fileName.lastIndexOf('.');
		return index < 0 ? fileName : fileName.substring(0, index);
	}

	public static void closeQuietly(Cursor cursor) {
		try {
			if (cursor != null) {
				cursor.close();
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public static void closeQuietly(Closeable closeable) {
		try {
			if (closeable != null) {
				closeable.close();
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public static String canonicalPath(String ref, String path) {
		String res = path;
		int index = ref.lastIndexOf('/');
		if (index > 0 && path.indexOf('/') < 0) {
			res = ref.substring(0, index + 1) + path;
		}
		return res;
	}

	public static long getLastChangeTime(File file) {
		return file.lastModified();
	}

	public static boolean isImageFile(String fileName) {
		fileName = fileName.toLowerCase(Locale.getDefault());
		return fileName.endsWith(".bmp") || fileName.endsWith(".jpg") || fileName.endsWith(".jpeg") || fileName.endsWith(".png") || fileName.endsWith(".gif");
	}

	public static boolean isPdfFile(String fileName) {
		fileName = fileName.toLowerCase(Locale.getDefault());
		return fileName.endsWith(".pdf");
	}

	public static boolean isZipFile(String fileName) {
		fileName = fileName.toLowerCase(Locale.getDefault());
		return fileName.endsWith(".zip") || fileName.endsWith(".cbz");
	}

	public static boolean isRarFile(String fileName) {
		fileName = fileName.toLowerCase(Locale.getDefault());
		return fileName.endsWith(".rar") || fileName.endsWith(".cbr");
	}

	public static String readContentOfFile(File fileForRead) {
		FileInputStream inputStream = null;
		InputStreamReader streamReader = null;
		BufferedReader reader = null;
		try {
			inputStream = new FileInputStream(fileForRead);
			streamReader = new InputStreamReader(inputStream, "utf-8");
			reader = new BufferedReader(streamReader);
			String sep = System.getProperty("line.separator");
			StringBuilder builder = new StringBuilder();

			String line;
			boolean firstLine;
			for (firstLine = true; (line = reader.readLine()) != null; builder.append(line)) {
				if (firstLine) {
					firstLine = false;
				} else {
					builder.append(sep);
				}
			}
			return builder.toString();
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			closeQuietly((Closeable) reader);
			closeQuietly((Closeable) streamReader);
			closeQuietly((Closeable) inputStream);
		}
		return null;
	}

	public static boolean saveContentToFile(String content, File fileForSave) {
		boolean res = true;
		FileOutputStream outputStream = null;
		try {
			outputStream = new FileOutputStream(fileForSave);
			outputStream.write(content.getBytes("utf-8"));
		} catch (Exception e) {
			e.printStackTrace();
			res = false;
		} finally {
			closeQuietly((Closeable) outputStream);
		}
		return res;
	}

	public static boolean appendContentToFile(String content, File fileForSave) {
		boolean res = true;
		FileOutputStream outputStream = null;
		try {
			outputStream = new FileOutputStream(fileForSave, true);
			outputStream.write(content.getBytes("utf-8"));
		} catch (Exception e) {
			e.printStackTrace();
			res = false;
		} finally {
			closeQuietly((Closeable) outputStream);
		}
		return res;
	}

	public static boolean saveContentToFile(byte[] data, File fileForSave) {
		boolean res = true;
		FileOutputStream outputStream = null;
		try {
			outputStream = new FileOutputStream(fileForSave);
			outputStream.write(data);
		} catch (Exception e) {
			res = false;
		} finally {
			closeQuietly((Closeable) outputStream);
		}
		return res;
	}

	public static boolean saveBitmapToFile(Bitmap bitmap, File fileForSave, CompressFormat format, int quality) {
		boolean res = true;
		FileOutputStream outputStream = null;
		try {
			outputStream = new FileOutputStream(fileForSave);
			bitmap.compress(format, quality, outputStream);
		} catch (Exception e) {
			e.printStackTrace();
			res = false;
		} finally {
			closeQuietly((Closeable) outputStream);
		}
		return res;
	}

	public static String getRealFilePathFromUri(Context context, Uri uri) {
		String res = null;
		if (uri != null) {
			if ("content".equals(uri.getScheme())) {
				if (VERSION.SDK_INT >= 23) {
					res = a(context, uri);
				} else {
					res = a(context, uri, "_data");
				}
			} else {
				res = uri.getPath();
			}
		}

		return res;
	}

	private static String a(Context context, Uri uri) {
		String res = "";
		if (uri.getAuthority().equals("com.google.android.bluetooth.fileprovider")) {
			String urlstr = null;
			try {
				urlstr = URLDecoder.decode(uri.getEncodedPath(), "UTF-8");
			} catch (UnsupportedEncodingException e) {
				e.printStackTrace();
			}
			String var4 = (String) urlstr.subSequence(urlstr.lastIndexOf(File.separator), urlstr.length());
			res = Device.currentDevice().getBluetoothRootDirectory().getPath() + var4;
		} else {
			res = a(context, uri, "_data");
		}
		return res;
	}

	private static String a(Context context, Uri uri, String str) {
		String res = "";
		Cursor cursor = context.getContentResolver().query(uri, new String[]{str}, (String) null, (String[]) null, (String) null);
		if (cursor == null) {
			return res;
		} else {
			cursor.moveToFirst();
			res = cursor.getString(0);
			cursor.close();
			return res;
		}
	}

	public static String computeMD5(File file) throws IOException, NoSuchAlgorithmException {
		if (!file.exists()) {
			throw new FileNotFoundException();
		} else if (!file.isFile()) {
			throw new IllegalArgumentException();
		} else {
			byte[] var1 = getDigestBuffer(file);
			MessageDigest digest = MessageDigest.getInstance("MD5");
			digest.update(var1);
			return hexToString(digest.digest());
		}
	}

	public static byte[] getDigestBuffer(File file) throws IOException {
		RandomAccessFile randomAccessFile = null;
		byte[] bytes;
		try {
			randomAccessFile = new RandomAccessFile(file, "r");
			long size = randomAccessFile.length();
			if (size <= 1536L) {
				bytes = new byte[(int) size];
				randomAccessFile.read(bytes);
			} else {
				bytes = new byte[1536];
				randomAccessFile.seek(0L);
				randomAccessFile.read(bytes, 0, 512);
				randomAccessFile.seek(size / 2L - 256L);
				randomAccessFile.read(bytes, 512, 512);
				randomAccessFile.seek(size - 512L);
				randomAccessFile.read(bytes, 1024, 512);
			}
		} finally {
			if (randomAccessFile != null) {
				randomAccessFile.close();
			}
		}
		return bytes;
	}

	public static String computeMD5(String content) {
		return StringUtils.isNullOrEmpty(content) ? null : computeMD5(content.getBytes(Charset.defaultCharset()));
	}

	public static String computeMD5(byte[] buffer) {
		if (buffer == null) {
			return null;
		} else {
			String res = null;
			try {
				MessageDigest digest = MessageDigest.getInstance("MD5");
				digest.update(buffer, 0, buffer.length);
				res = hexToString(digest.digest());
			} catch (NoSuchAlgorithmException e) {
				e.printStackTrace();
			}
			return res;
		}
	}

	public static String computeFullMD5Checksum(File file) throws IOException, NoSuchAlgorithmException {
		FileInputStream inputStream = null;
		try {
			inputStream = new FileInputStream(file);
			byte[] buffer = new byte[65536];
			MessageDigest digest = MessageDigest.getInstance("MD5");
			int ret;
			do {
				ret = inputStream.read(buffer);
				if (ret > 0) {
					digest.update(buffer, 0, ret);
				}
			} while (ret != -1);
			String var5 = hexToString(digest.digest());
			return var5;
		} finally {
			closeQuietly((Closeable) inputStream);
		}
	}

	public static String hexToString(byte[] out) {
		char[] digits = new char[]{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
		char[] outbuff = new char[out.length * 2];
		for (int i = 0; i < out.length; ++i) {
			outbuff[2*i] = digits[(out[i] >> 4) & 15];
			outbuff[2*i + 1] = digits[out[i] & 15];
		}
		return String.valueOf(outbuff);
	}

	public static boolean deleteFile(String path) {
		return deleteFile(new File(path));
	}

	public static boolean deleteFile(File file) {
		if (file.exists()) {
			return file.delete();
		} else {
			return false;
		}
	}

	public static boolean deleteFile(File file, boolean recursive) {
		if (file.isFile()) {
			return deleteFile(file);
		} else {
			File[] files = file.listFiles();
			if (files != null && files.length > 0) {
				for (File item : files) {
					if (recursive) {
						deleteFile(item, true);
					} else {
						deleteFile(item);
					}
				}
				return deleteFile(file);
			} else {
				return deleteFile(file);
			}
		}
	}

	public static boolean ensureFileExists(String path) {
		File file = new File(path);
		if (file.exists()) {
			return true;
		} else {
			int index = path.indexOf('/', 1);
			if (index < 1) {
				return false;
			} else {
				String dirname = path.substring(0, index);
				File dir = new File(dirname);
				if (!dir.exists()) {
					return false;
				} else {
					File parentDir = file.getParentFile();
					if (!parentDir.exists() && !parentDir.mkdirs()) {
						Log.e(TAG, "create folder failed: " + parentDir.getAbsolutePath());
						return false;
					} else {
						try {
							return file.createNewFile();
						} catch (IOException e) {
							Log.e(TAG, "File creation failed", e);
							return false;
						}
					}
				}
			}
		}
	}

	public static void findFileByKey(List<File> fileList, String searchKey) {
		findFileByKey(fileList, EnvironmentUtil.getExternalStorageDirectory(), searchKey);
		findFileByKey(fileList, EnvironmentUtil.getRemovableSDCardDirectory(), searchKey);
	}

	public static void findFileByKey(List<File> fileList, File targetDir, String searchKey) {
		if (targetDir.canRead()) {
			File[] files = targetDir.listFiles();
			for (File file : files) {
				if (!file.isHidden()) {
					if (file.isDirectory()) {
						if (file.getName().contains(searchKey)) {
							fileList.add(file);
						}
						findFileByKey(fileList, file, searchKey);
					}
					if (file.isFile() && file.getName().contains(searchKey)) {
						fileList.add(file);
					}
				}
			}
		}
	}

	public static String fixNotAllowFileName(String fileName) {
		if (StringUtils.isBlank(fileName)) {
			return null;
		} else {
			int index = fileName.lastIndexOf(".");
			if (index == -1) {
				return null;
			} else {
				String regex = "([.*/^()?|<>\\]\\[])";
				String var2 = fileName.replaceAll(regex, " ");
				var2 = var2.replace(var2.substring(index), fileName.substring(index));
				var2 = var2.replace(":", "：");
				boolean var4 = false;
				while (var2.indexOf("\"") != -1) {
					if (!var4) {
						var2 = var2.replaceFirst("\"", "“");
						var4 = true;
					} else {
						var2 = var2.replaceFirst("\"", "”");
						var4 = false;
					}
				}
				return var2;
			}
		}
	}

	public static String filterFileName(String fileName) {
		return fileName.replaceAll("([.*/^()?|<>\\]\\[])", " ");
	}

	public static String readContentOfFile(String path) {
		BufferedReader bufferedReader = null;
		FileInputStream inputStream = null;
		try {
			File file = new File(path);
			bufferedReader = new BufferedReader(new InputStreamReader(inputStream = new FileInputStream(file)));
			StringBuilder builder = new StringBuilder();
			String line;
			while ((line = bufferedReader.readLine()) != null) {
				builder.append(line);
			}
			String res = builder.toString();
			return res;
		} catch (Exception e) {
		} finally {
			closeQuietly((Closeable) bufferedReader);
			closeQuietly((Closeable) inputStream);
		}
		return null;
	}

	public static String getFileNameFromUrl(String url) {
		if (!StringUtils.isUrl(url)) {
			return null;
		} else {
			int index = url.lastIndexOf('/');
			if (index < 0)
				return null;
			return url.substring(index + 1, url.length());
		}
	}

	public static String getFileSize(long size) {
		DecimalFormat format = new DecimalFormat("###.##");
		float megabytes = (float) size / 1048576.0F;
		if ((double) megabytes < 1.0D) {
			float kilobytes = (float) size / 1024.0F;
			return format.format((new Float(kilobytes)).doubleValue()) + "KB";
		} else {
			return format.format((new Float(megabytes)).doubleValue()) + "M";
		}
	}

	public static void transferFile(String currentFilePath, String newFilePath) throws Exception {
		File srcFile = new File(currentFilePath);
		File dstFile = new File(newFilePath);
		FileChannel srcChannel = (new FileInputStream(srcFile)).getChannel();
		FileChannel dstChannel = (new FileOutputStream(dstFile)).getChannel();
		dstChannel.transferFrom(srcChannel, 0L, srcChannel.size());
		srcChannel.close();
		dstChannel.close();
	}

	public static boolean compareFileMd5(String file1, String file2) throws IOException, NoSuchAlgorithmException {
		String var2 = computeFullMD5Checksum(new File(file1));
		String var3 = computeFullMD5Checksum(new File(file2));
		return var2.equals(var3);
	}

	public static boolean copyFile(File sourceFile, File targetFile) {
		FileChannel srcChannel = null;
		FileChannel dstChannel = null;
		boolean res;
		try {
			if (!targetFile.exists() && !targetFile.createNewFile()) {
				return false;
			}
			srcChannel = (new FileInputStream(sourceFile)).getChannel();
			dstChannel = (new FileOutputStream(targetFile)).getChannel();
			srcChannel.transferTo(0L, srcChannel.size(), dstChannel);
			res = true;
		} catch (Exception e) {
			e.printStackTrace();
			res = false;
		} finally {
			closeQuietly((Closeable) srcChannel);
			closeQuietly((Closeable) dstChannel);
		}
		return res;
	}

	public static void sortListByName(List<File> fileList, final SortOrder sortOrder) {
		Collections.sort(fileList, new Comparator<File>() {
			public int compare(File var1, File var2) {
				int var3 = ComparatorUtils.booleanComparator(var1.isDirectory(), var2.isDirectory(), SortOrder.Desc);
				return var3 == 0 ? ComparatorUtils.stringComparator(var1.getName(), var2.getName(), sortOrder) : var3;
			}
		});
	}

	public static void sortListByCreationTime(List<File> fileList, final SortOrder sortOrder) {
		Collections.sort(fileList, new Comparator<File>() {
			public int compare(File var1, File var2) {
				int var3 = ComparatorUtils.booleanComparator(var1.isDirectory(), var2.isDirectory(), SortOrder.Desc);
				return var3 == 0 ? ComparatorUtils.longComparator(var1.lastModified(), var2.lastModified(), sortOrder) : var3;
			}
		});
	}

	public static void sortListBySize(List<File> fileList, final SortOrder sortOrder) {
		Collections.sort(fileList, new Comparator<File>() {
			public int compare(File var1, File var2) {
				int var3 = ComparatorUtils.booleanComparator(var1.isDirectory(), var2.isDirectory(), SortOrder.Desc);
				return var3 == 0 ? ComparatorUtils.longComparator(var1.length(), var2.length(), sortOrder) : var3;
			}
		});
	}

	public static void sortListByFileType(List<File> fileList, final SortOrder sortOrder) {
		Collections.sort(fileList, new Comparator<File>() {
			public int compare(File var1, File var2) {
				int var3 = ComparatorUtils.booleanComparator(var1.isDirectory(), var2.isDirectory(), SortOrder.Desc);
				return var3 == 0 ? ComparatorUtils.stringComparator(FileUtils.getFileExtension(var1), FileUtils.getFileExtension(var2), sortOrder) : var3;
			}
		});
	}

	public static boolean onSameSDCard(File a, File b) {
		if (a.getAbsolutePath().contains(EnvironmentUtil.getExternalStorageDirectory().getAbsolutePath())) {
			if (b.getAbsolutePath().contains(EnvironmentUtil.getExternalStorageDirectory().getAbsolutePath())) {
				return true;
			}
		} else if (!b.getAbsolutePath().contains(EnvironmentUtil.getExternalStorageDirectory().getAbsolutePath())) {
			return true;
		}

		return false;
	}

	public static void updateMtpDb(Context context, File file) {
		MediaScannerConnection.scanFile(context, new String[]{file.getAbsolutePath()}, (String[]) null, new OnScanCompletedListener() {
			public void onScanCompleted(String path, Uri uri) {
				//Debug.i(FileUtils.class, "file " + path + " was scanned successfully: " + uri, new Object[0]);
				Log.i(TAG, "file " + path + " was scanned successfully: " + uri);
			}
		});
	}

	public static String readableFileSize(long size) {
		if (size <= 0L) {
			return "0";
		} else {
			String[] var2 = new String[]{"B", "KB", "MB", "GB", "TB"};
			int e = (int) (Math.log10((double) size) / Math.log10(1024.0D));
			return (new DecimalFormat("#,##0.#")).format((double) size / Math.pow(1024.0D, (double) e)) + " " + var2[e];
		}
	}

	public static long getFileSize(File file) {
		if (file.isDirectory()) {
			long size = 0L;
			try {
				File[] files = file.listFiles();
				for (File var7 : files) {
					if (var7.isDirectory()) {
						size += getFileSize(var7);
					} else {
						size += var7.length();
					}
				}
			} catch (Exception e) {
				e.printStackTrace();
			}
			return size;
		} else {
			if (file.exists() && file.isFile())
				return file.length();
			return -1L;
		}
	}

	public static void removeUnSupportFormatFiles(Collection<String> originList, Collection<String> extensionFilterList) {
		if (!CollectionUtils.isNullOrEmpty(originList) && !CollectionUtils.isNullOrEmpty(extensionFilterList)) {
			Iterator var2 = originList.iterator();

			while (var2.hasNext()) {
				String var3 = (String) var2.next();
				if (!extensionFilterList.contains(getFileExtension(StringUtils.safelyGetStr(var3)))) {
					var2.remove();
				}
			}

		}
	}

	public static List<File> loadStorageFileList(File targetDir, final boolean skipHiddenFile) {
		File[] files = targetDir.listFiles(new FilenameFilter() {
			public boolean accept(File dir, String filename) {
				return !skipHiddenFile || !(new File(dir, filename)).isHidden();
			}
		});
		if (files != null && files.length > 0)
			return new ArrayList<File>(Arrays.asList(files));
		return new ArrayList<File>();
	}

	public static void sortFileList(List<File> fileList, SortBy sortBy, SortOrder sortOrder) {
		if (!CollectionUtils.isNullOrEmpty(fileList)) {
			//Benchmark var3 = new Benchmark();
			switch (sortBy) {
				case Name:
					sortListByName(fileList, sortOrder);
					break;
				case CreationTime:
					sortListByCreationTime(fileList, sortOrder);
					break;
				case FileType:
					sortListByFileType(fileList, sortOrder);
					break;
				case Size:
					sortListBySize(fileList, sortOrder);
			}
			//Log.w(TAG, "Sort duration:" + var3.duration() + "ms");
		}
	}

	public static boolean isStorageRoot(File targetDirectory) {
		return targetDirectory != null ? EnvironmentUtil.getStorageRootDirectory().getAbsolutePath().contains(targetDirectory.getAbsolutePath()) : true;
	}

	public static List<String> readStringListOfFile(File fileForRead) {
		FileInputStream inputStream = null;
		InputStreamReader streamReader = null;
		BufferedReader bufferedReader = null;
		ArrayList<String> list = new ArrayList<String>();
		try {
			inputStream = new FileInputStream(fileForRead);
			streamReader = new InputStreamReader(inputStream, "utf-8");
			bufferedReader = new BufferedReader(streamReader);
			new StringBuilder();
			String line = null;
			while ((line = bufferedReader.readLine()) != null) {
				list.add(line);
			}
			return list;
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			closeQuietly((Closeable) bufferedReader);
			closeQuietly((Closeable) streamReader);
			closeQuietly((Closeable) inputStream);
		}
		return null;
	}
}
