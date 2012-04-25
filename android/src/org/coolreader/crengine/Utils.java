package org.coolreader.crengine;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;

import android.util.Log;

public class Utils {
	public static long timeStamp() {
		return android.os.SystemClock.uptimeMillis();
	}
	
	public static long timeInterval(long startTime) {
		return android.os.SystemClock.uptimeMillis() - startTime;
	}
	
	public static String cleanupHtmlTags(String src) {
		StringBuilder buf = new StringBuilder();
		boolean insideTag = false;
		for (char ch : src.toCharArray()) {
			if (ch=='<') {
				insideTag = true;
			} else if (ch=='>') {
				insideTag = false;
				buf.append(' ');
			} else if (!insideTag) {
				buf.append(ch);
			}
		}
		return buf.toString();
	}
	
	public static String authorNameFileAs(String name) {
		if (name == null || name.length() == 0)
			return name;
		int lastSpace = name.lastIndexOf(' ');
		if (lastSpace >= 0 && lastSpace < name.length() - 1)
			return name.substring(lastSpace + 1) + " " + name.substring(0, lastSpace);
		return name;
	}

	public static boolean moveFile(File oldPlace, File newPlace) {
		return moveFile(oldPlace, newPlace, true);
	}

	public static boolean copyFile(File oldPlace, File newPlace) {
		return moveFile(oldPlace, newPlace, false);
	}

	private static boolean moveFile(File oldPlace, File newPlace, boolean removeOld) {
		boolean removeNewFile = true;
		Log.i("cr3", "Moving file " + oldPlace.getAbsolutePath() + " to " + newPlace.getAbsolutePath());
		if ( !oldPlace.exists() ) {
			Log.i("cr3", "File " + oldPlace.getAbsolutePath() + " does not exist!");
			return false;
		}
		FileOutputStream os = null;
		FileInputStream is = null;
		try {
			if (!newPlace.createNewFile())
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
	
	public static boolean restoreFromBackup(File f) {
		File backup = new File(f.getAbsolutePath() + ".good.bak.2");
		if (f.exists())
			f.delete();
		if (backup.exists()) {
			if (backup.renameTo(f))
				return true; 
		}
		return false;
	}
	
	public static void backupFile(File f) {
		if (!f.exists())
			return;
		File backup = getBackupFileName(f, true);
		L.i("Creating backup of file " + f + " as " + backup);
		if (Utils.copyFile(f, backup)) {
			L.w("copying of DB has been failed");
		}
		f.renameTo(backup);
	}
	
	public static void moveCorruptedFileToBackup(File f) {
		if (!f.exists())
			return;
		Log.e("cr3", "Moving corrupted file " + f + " to backup.");
		File backup = getBackupFileName(f, false);
		f.renameTo(backup);
	}
	
	private final static int MAX_BACKUP_FILES = 5;
	private static File getBackupFileName(File f, boolean isGoodBackup) {
		File f2 = null;
		String prefix = f.getAbsolutePath() + (isGoodBackup ? ".good.bak." : ".corrupted.bak.");
		for (int i=MAX_BACKUP_FILES - 1; i > 2; i--) {
			File to = new File(prefix + i); 
			File from = new File(prefix + (i-1));
			if (to.exists())
				to.delete();
			if (from.exists()) {
				if (!from.renameTo(to))
					Log.e("cr3", "Cannot rename DB file " + from + " to " + to);
			}
		}
		f2 = new File(prefix + 2);
		if (f2.exists())
			if (!f2.delete())
				Log.e("cr3", "Cannot remove DB file " + f2);
		return f2;
	}

	private final static String LATIN_C0 =
		// 0xC0 .. 0xFF
		  "aaaaaaaceeeeiiiidnoooooxouuuuyps" 
		+ "aaaaaaaceeeeiiiidnoooooxouuuuypy";
	
	private static char convertCharCaseForSearch(char ch) {
		if (ch >= 'A' && ch <= 'Z')
			return (char)(ch - 'A' + 'a');
		if ( ch>=0xC0 && ch<=0xFF )
			return LATIN_C0.charAt(ch - 0xC0);
    	if ( ch>=0x410 && ch<=0x42F )
    		return (char)(ch + 0x20);
    	if ( ch>=0x390 && ch<=0x3aF )
    		return (char)(ch + 0x20);
    	if ( (ch >> 8)==0x1F ) { // greek
	        int n = ch & 255;
	        if (n<0x70) {
	            return (char)(ch & (~8));
	        } else if (n<0x80) {
	
	        } else if (n<0xF0) {
	            return (char)(ch & (~8));
	        }
	    }
		return ch;
	}
	
	public static boolean matchPattern(String text, String pattern) {
		if (pattern == null)
			return true;
		if (text == null)
			return false;
		int textlen = text.length();
		int patternlen = pattern.length();
		if (textlen < patternlen)
			return false;
		for (int i=0; i <= textlen - patternlen; i++) {
			if (i > 0 && text.charAt(i-1) != ' ')
				continue; // match only beginning of words
			boolean eq = true;
			for (int j=0; j<patternlen; j++) {
				if (convertCharCaseForSearch(text.charAt(i + j)) != convertCharCaseForSearch(pattern.charAt(j))) {
					eq = false;
					break;
				}
			}
			if (eq)
				return true;
		}
		return false;
	}
	
	public static String[] splitByWhitespace(String str) {
		ArrayList<String> list = new ArrayList<String>();
		StringBuilder buf = new StringBuilder();
		for (int i=0; i<str.length(); i++) {
			char ch = str.charAt(i);
			if (ch == ' ' || ch == '\t') {
				if (buf.length() > 0) {
					list.add(buf.toString());
					buf = new StringBuilder();
				}
			} else {
				buf.append(ch);
			}
		}
		if (buf.length() > 0)
			list.add(buf.toString());
		return list.toArray(new String[list.size()]);
	}

	public static boolean eq(String s1, String s2) {
		if (s1 == null) {
			return s2 == null;
		}
		return s1.equals(s2);
	}
}
