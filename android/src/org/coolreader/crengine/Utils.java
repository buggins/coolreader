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
	
	public static String authorNameFileAs(String name) {
		if (name == null || name.length() == 0)
			return name;
		int lastSpace = name.lastIndexOf(' ');
		if (lastSpace >= 0 && lastSpace < name.length() - 1)
			return name.substring(lastSpace + 1) + " " + name.substring(0, lastSpace);
		return name;
	}
}
