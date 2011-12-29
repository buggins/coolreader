package org.coolreader.crengine;

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
}
