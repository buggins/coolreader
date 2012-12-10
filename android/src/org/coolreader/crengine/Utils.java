package org.coolreader.crengine;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.TimeZone;

import org.coolreader.R;
import org.coolreader.crengine.FileInfo.SortOrder;

import android.app.Activity;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.text.format.DateFormat;
import android.util.Log;
import android.view.View;

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

	public static int copyStreamContent(OutputStream os, InputStream is) throws IOException {
		int totalSize = 0;
		byte[] buf = new byte[0x10000];
		for (;;) {
			int bytesRead = is.read(buf);
			if ( bytesRead<=0 )
				break;
			totalSize += bytesRead;
			os.write(buf, 0, bytesRead);
		}
		return totalSize;
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
			copyStreamContent(os, is);
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
	
	public static String formatAuthors( String authors ) {
		if ( authors==null || authors.length()==0 )
			return null;
		String[] list = authors.split("\\|");
		StringBuilder buf = new StringBuilder(authors.length());
		for ( String a : list ) {
			if ( buf.length()>0 )
				buf.append(", ");
			buf.append(Utils.authorNameFileAs(a));
//			String[] items = a.split(" ");
//			if ( items.length==3 && items[1]!=null && items[1].length()>=1 )
//				buf.append(items[0] + " " + items[1].charAt(0) + ". " + items[2]);
//			else
//				buf.append(a);
		}
		return buf.toString();
	}

	public static String formatAuthorsNormalNames( String authors ) {
		if ( authors==null || authors.length()==0 )
			return null;
		String[] list = authors.split("\\|");
		StringBuilder buf = new StringBuilder(authors.length());
		for (String a : list) {
			if (buf.length() > 0)
				buf.append(", ");
			buf.append(a);
//			String[] items = a.split(" ");
//			if ( items.length==3 && items[1]!=null && items[1].length()>=1 )
//				buf.append(items[0] + " " + items[1].charAt(0) + ". " + items[2]);
//			else
//				buf.append(a);
		}
		return buf.toString();
	}

	public static String ntrim(String str) {
		if (str == null)
			return null;
		str = str.trim();
		if (str.length() == 0)
			return null;
		return str;
	}

	public static String concatWs(String str1, String str2, String ws) {
		if (empty(str1)) {
			if (empty(str2))
				return "";
			return str2;
		}
		if (empty(str2))
			return str1;
		return str1 + ws + str2;
	}
	
	public static boolean empty(String str) {
		if (str == null || str.length() == 0)
			return true;
		if (str.trim().length() == 0)
			return true;
		return false;
		
	}
	
	public static Drawable solidColorDrawable(final int color) {
		GradientDrawable d = new GradientDrawable();
		d.setColor(color);
		return d;
//		RectShape s = new RectShape();
//		
//		d.setShape(s);
//		return new Drawable() {
//			@Override
//			public void setColorFilter(ColorFilter cf) {
//			}
//			
//			@Override
//			public void setAlpha(int alpha) {
//			}
//			
//			@Override
//			public int getOpacity() {
//				return 0;
//			}
//			
//			@Override
//			public void draw(Canvas canvas) {
//				canvas.drawColor(0xFF000000 | color);
//			}
//		};
	}

	public static String formatSeries( String name, int number )
	{
		if ( name==null || name.length()==0 )
			return null;
		if ( number>0 )
			return "#" + number + " " + name;
		else
			return name;
	}
	
	public static String formatPercent( int percent )
	{
		if ( percent<=0 )
			return null;
		return String.valueOf(percent/100) + "." + String.valueOf(percent/10%10) + "%";
	}

	public static String formatTime( Activity activity, long timeStamp )
	{
		if ( timeStamp<5000*60*60*24*1000 )
			return "";
		TimeZone tz = java.util.TimeZone.getDefault();
		Calendar c = Calendar.getInstance(tz);
		c.setTimeInMillis(timeStamp);
		return DateFormat.getTimeFormat(activity.getApplicationContext()).format(c.getTime());
	}
	
	public static String formatDate( Activity activity, long timeStamp )
	{
		if ( timeStamp<5000*60*60*24*1000 )
			return "";
		TimeZone tz = java.util.TimeZone.getDefault();
		Calendar now = Calendar.getInstance(tz);
		Calendar c = Calendar.getInstance(tz);
		c.setTimeInMillis(timeStamp);
		if ( c.get(Calendar.YEAR)<1980 )
			return "";
		if ( c.get(Calendar.YEAR)==now.get(Calendar.YEAR)
				&& c.get(Calendar.MONTH)==now.get(Calendar.MONTH)
				&& c.get(Calendar.DAY_OF_MONTH)==now.get(Calendar.DAY_OF_MONTH)) {
			return formatTime(activity, timeStamp);
		} else {
			return DateFormat.getDateFormat(activity.getApplicationContext()).format(c.getTime());
		}
	}
	
//	static private ThreadLocal<SimpleDateFormat> dateFormatThreadLocal = new ThreadLocal<SimpleDateFormat>(); 
//	static private ThreadLocal<SimpleDateFormat> timeFormatThreadLocal = new ThreadLocal<SimpleDateFormat>();
//	static private SimpleDateFormat dateFormat() {
//		if (dateFormatThreadLocal.get() == null)
//			dateFormatThreadLocal.set(new SimpleDateFormat("dd.MM.yy", Locale.getDefault()));
//		return dateFormatThreadLocal.get();
//	}
//	
//	static private SimpleDateFormat timeFormat() {
//		if (timeFormatThreadLocal.get() == null)
//			timeFormatThreadLocal.set(new SimpleDateFormat("HH:mm", Locale.getDefault()));
//		return timeFormatThreadLocal.get();
//	}
	
	public static String formatTimeElapsed( long timeElapsed )
	{
		int hours = (int) timeElapsed/(60*60*1000);
		int min = (int) timeElapsed % (60*60*1000) / (60*1000);
//		int sec = (int) timeElapsed % (60*60*1000) % (60 * 1000) / 1000;
//		return String.format("%02d:%02d:%02d", hours, min, sec);
		return String.format("%d:%02d", hours, min);
	}
	public static String formatSize( int size )
	{
		if ( size==0 )
			return "";
		if ( size<10000 )
			return String.valueOf(size);
		else if ( size<1000000 )
			return String.valueOf(size/1000) + "K";
		else if ( size<10000000 )
			return String.valueOf(size/1000000) + "." + String.valueOf(size%1000000/100000) + "M";
		else
			return String.valueOf(size/1000000) + "M";
	}

	public static String formatFileInfo(Activity activity, FileInfo item) {
		return formatSize(item.size) + " " + (item.format!=null ? item.format.name().toLowerCase() : "") + " " + formatDate(activity, item.createTime);
	}

	public static String formatLastPosition(Activity activity, Bookmark pos) {
		if ( pos!=null && pos.getPercent() > 0 && pos.getTimeStamp() > 0) {
			//return formatPercent(pos.getPercent()) + " " + formatDate(activity, pos.getTimeStamp());
			return formatPercent(pos.getPercent()) + " " + formatDate(activity, pos.getTimeStamp()) + " (" + formatTimeElapsed(pos.getTimeElapsed()) + ")";
		} else {
			return "";
		}
	}

	public static String formatReadingState(Activity activity, FileInfo item) {
		String state = "";
		if (item.getRate() > 0 && item.getRate() <= 5) {
			String[] stars = new String[] {
					"*",
					"**",
					"***",
					"****",
					"*****",
			};
			state = state + stars[item.getRate() - 1] + " ";
		}
		if (item.getReadingState() > 0) {
			String stateName = "";
			int n = item.getReadingState();
			if (n == FileInfo.STATE_READING)
				stateName = activity.getString(R.string.book_state_reading);
			else if (n == FileInfo.STATE_TO_READ)
				stateName = activity.getString(R.string.book_state_toread);
			else if (n == FileInfo.STATE_FINISHED)
				stateName = activity.getString(R.string.book_state_finished);
			state = state + "[" + stateName + "] ";
		}
		return state;
	}
	
	public static void drawFrame(Canvas canvas, Rect rect, Paint paint) {
		canvas.drawRect(new Rect(rect.left, rect.top, rect.right, rect.top + 1), paint);
		canvas.drawRect(new Rect(rect.left, rect.top + 1, rect.left + 1, rect.bottom), paint);
		canvas.drawRect(new Rect(rect.right - 1, rect.top + 1, rect.right, rect.bottom), paint);
		canvas.drawRect(new Rect(rect.left + 1, rect.bottom - 1, rect.right - 1, rect.bottom), paint);
	}
	
	public static Paint createSolidPaint(int color) {
		Paint res = new Paint();
		res.setStyle(Paint.Style.FILL);
		res.setColor(color);
		return res;
	}

	/**
	 * Compares two strings - with numbers sorted by value.
	 * @param str1
	 * @param str2
	 * @return
	 */
	public static int cmp( String str1, String str2 )
	{
		if ( str1==null && str2==null )
			return 0;
		if ( str1==null )
			return -1;
		if ( str2==null )
			return 1;
	
		str1 = str1.toLowerCase();
		str2 = str2.toLowerCase();
		int p1 = 0;
		int p2 = 0;
		for ( ;; ) {
			if ( p1>=str1.length() ) {
				if ( p2>=str2.length() )
					return 0;
				return 1;
			}
			if ( p2>=str2.length() )
				return -1;
			char ch1 = str1.charAt(p1);
			char ch2 = str2.charAt(p2);
			if ( ch1>='0' && ch1<='9' && ch2>='0' && ch2<='9' ) {
				int n1 = 0;
				int n2 = 0;
				while ( ch1>='0' && ch1<='9' ) {
					p1++;
					n1 = n1 * 10 + (ch1-'0');
					if ( p1>=str1.length() )
						break;
					ch1 = str1.charAt(p1);
				}
				while ( ch2>='0' && ch2<='9' ) {
					p2++;
					n2 = n2 * 10 + (ch2-'0');
					if ( p2>=str2.length() )
						break;
					ch2 = str2.charAt(p2);
				}
				int c = SortOrder.cmp(n1, n2);
				if ( c!=0 )
					return c;
			} else {
				if ( ch1<ch2 )
					return -1;
				if ( ch1>ch2 )
					return 1;
				p1++;
				p2++;
			}
		}
	}

	public static String transcribeFileName( String fileName ) {
		StringBuilder buf = new StringBuilder(fileName.length());
		for ( char ch : fileName.toCharArray() ) {
			boolean found = false;
			if ( ((ch>='a' && ch<='z') || (ch>='A' && ch<='Z') || (ch>='0' && ch<='9') || ch=='-' || ch=='_' || ch=='(' || ch==')')) {
				buf.append(ch);
				continue;
			}
			for ( OPDSUtil.SubstTable t : Utils.substTables ) {
				if ( t.isInRange(ch) ) {
					buf.append(t.get(ch));
					found = true;
				}
			}
			if ( found )
				continue;
			buf.append("_");
		}
		return buf.toString();
	}

	final static OPDSUtil.SubstTable[] substTables = { 
		new OPDSUtil.SubstTable(0x430, new String[]{"a", "b", "v", "g", "d", "e", "zh", "z", "i", "j", "k", "l", "m", "n", "o", "p", "r", "s", "t", "u", "f", "h", "c", "ch", "sh", "sch", "'", "y", "i", "e", "yu", "ya"}),
		new OPDSUtil.SubstTable(0x410, new String[]{"A", "B", "V", "G", "D", "E", "Zh", "Z", "I", "J", "K", "L", "M", "N", "O", "P", "R", "S", "T", "U", "F", "H", "C", "Ch", "Sh", "Sch", "'", "Y", "I", "E", "Yu", "Ya"}),
	};
	public static String transcribeWithLimit(String str, int maxLen) {
		str = transcribeFileName(str);
		if (str.length() > maxLen)
			str = str.substring(0, maxLen);
		return str;
	}

	// to support API LEVEL 3: View.setContentDescription() has been added only since API LEVEL 4
	public static void setContentDescription(View view, CharSequence text) {
		if (DeviceInfo.getSDKLevel() >= 4) {
			Method m;
			try {
				m = view.getClass().getMethod("setContentDescription", CharSequence.class);
				m.invoke(view, text);
			} catch (NoSuchMethodException e) {
				// Ignore
			} catch (IllegalArgumentException e) {
				// Ignore
			} catch (IllegalAccessException e) {
				// Ignore
			} catch (InvocationTargetException e) {
				// Ignore
			}
		}
	}
}
