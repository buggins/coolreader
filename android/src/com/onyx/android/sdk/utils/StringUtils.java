package com.onyx.android.sdk.utils;

import android.graphics.Paint;
import android.os.Build;
import android.util.Log;
import android.util.Patterns;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.lang.Character.UnicodeBlock;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.regex.Pattern;

public class StringUtils {
	public static final String UTF16LE = "UTF-16LE";
	public static final String UTF16BE = "UTF-16BE";
	public static final String UTF16 = "UTF-16";
	public static String punctuation = "[`~!@#$%^&*()+=|{}':;',\\[\\].<>/?~！@#￥%……&*（）——+|{}【】‘；：”“’。，、？]";

	public StringUtils() {
	}

	public static boolean isNullOrEmpty(String string) {
		return string == null || string.trim().length() <= 0;
	}

	public static boolean isNotBlank(String string) {
		return string != null && string.trim().length() > 0;
	}

	public static boolean isBlank(String string) {
		return !isNotBlank(string);
	}

	public static boolean isInteger(String string) {
		if (isNullOrEmpty(string)) {
			return false;
		} else {
			String str = string;
			if (string.charAt(0) == '-') {
				if (string.length() <= 1) {
					return false;
				}
				str = string.substring(1, string.length() - 1);
			}
			for (int i = 0; i < str.length(); ++i) {
				if (!Character.isDigit(str.charAt(i))) {
					return false;
				}
			}
			return true;
		}
	}

	public static String utf16le(byte[] data) {
		String res = "";
		if (data == null) {
			return res;
		} else {
			try {
				res = new String(data, "UTF-16LE");
			} catch (Exception e) {
				Log.w("", e);
			}
			return res;
		}
	}

	public static String utf16(byte[] data) {
		String res = "";
		try {
			res = new String(data, "UTF-16");
		} catch (Exception e) {
		}
		return res;
	}

	public static byte[] utf16leBuffer(String text) {
		byte[] bytes = null;
		try {
			bytes = text.getBytes("UTF-16LE");
		} catch (Exception e) {
		}
		return bytes;
	}

	public static String join(Iterable<?> elements, String delimiter) {
		StringBuilder builder = new StringBuilder();
		Object element;
		for (Iterator it = elements.iterator(); it.hasNext(); builder.append(element)) {
			element = it.next();
			if (builder.length() > 0) {
				builder.append(delimiter);
			}
		}
		return builder.toString();
	}

	public static List<String> split(String string, String delimiter) {
		if (isNullOrEmpty(string)) {
			return new ArrayList<String>();
		} else {
			String[] list = string.split(delimiter);
			return Arrays.asList(list);
		}
	}

	public static String deleteNewlineSymbol(String content) {
		if (!isNullOrEmpty(content)) {
			content = content.replaceAll("\r\n", " ").replaceAll("\n", " ");
		}

		return content;
	}

	public static String leftTrim(String content) {
		int var1 = 0;

		int var2;
		for (var2 = content.length() - 1; var1 <= var2 && content.charAt(var1) <= ' '; ++var1) {
			;
		}

		return var1 == 0 ? content : content.substring(var1, var2 + 1);
	}

	public static String rightTrim(String content) {
		byte var1 = 0;
		int var2 = content.length() - 1;

		int var3;
		for (var3 = var2; var3 >= var1 && content.charAt(var3) <= ' '; --var3) {
			;
		}

		return var3 == var2 ? content : content.substring(var1, var3 + 1);
	}

	public static String trim(String input) {
		if (isNotBlank(input)) {
			input = input.trim();
			input = input.replace("\u0000", "");
			input = input.replace("\\u0000", "");
			input = input.replaceAll("\\u0000", "");
			input = input.replaceAll("\\\\u0000", "");
		}

		return input;
	}

	public static String trimPunctuation(String input) {
		input = trim(input);
		if (isNullOrEmpty(input)) {
			return input;
		} else {
			int var1;
			for (var1 = 0; var1 < input.length() && punctuation.contains(String.valueOf(input.charAt(var1))); ++var1) {
				;
			}

			if (var1 > input.length() - 1) {
				return "";
			} else {
				int var2;
				for (var2 = input.length() - 1; var2 > var1 && punctuation.contains(String.valueOf(input.charAt(var2))); --var2) {
					;
				}

				input = input.substring(var1, var2 + 1);
				return input;
			}
		}
	}

	public static boolean isAlpha(char ch) {
		return 'A' <= ch && ch <= 'z' || 192 <= ch && ch <= 214 || 216 <= ch && ch <= 246 || 248 <= ch && ch <= 255 || 256 <= ch && ch <= 383 || 384 <= ch && ch <= 591 || 902 == ch || 904 <= ch && ch <= 1023 || 1024 <= ch && ch <= 1153 || 1162 <= ch && ch <= 1279 || 1280 <= ch && ch <= 1327 || 7680 <= ch && ch <= 7935;
	}

	public static boolean isUrl(String url) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.FROYO) {
			return !isNullOrEmpty(url) && Patterns.WEB_URL.matcher(url).matches();
		}
		return false;
	}

	public static String safelyGetStr(String origin) {
		return isNullOrEmpty(origin) ? "" : origin;
	}

	public static boolean safelyEquals(String firstStr, String secondStr) {
		if (firstStr == null && secondStr == null) {
			return true;
		} else {
			return firstStr != null && secondStr != null ? firstStr.equals(secondStr) : false;
		}
	}

	public static int getTextWidth(Paint paint, String str) {
		int var2 = 0;
		if (str != null && str.length() > 0) {
			int var3 = str.length();
			float[] var4 = new float[var3];
			paint.getTextWidths(str, var4);

			for (int var5 = 0; var5 < var3; ++var5) {
				var2 += (int) Math.ceil((double) var4[var5]);
			}
		}

		return var2;
	}

	public static boolean isChinese(char c) {
		UnicodeBlock var1 = UnicodeBlock.of(c);
		return var1 == UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS || var1 == UnicodeBlock.CJK_COMPATIBILITY_IDEOGRAPHS || var1 == UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A || var1 == UnicodeBlock.GENERAL_PUNCTUATION || var1 == UnicodeBlock.CJK_SYMBOLS_AND_PUNCTUATION || var1 == UnicodeBlock.HALFWIDTH_AND_FULLWIDTH_FORMS;
	}

	public static boolean isChinese(String s) {
		if (isNullOrEmpty(s)) {
			return false;
		} else {
			for (int var1 = 0; var1 < s.length(); ++var1) {
				if (isChinese(s.charAt(var1))) {
					return true;
				}
			}

			return false;
		}
	}

	public static boolean isEquals(String s1, String s2) {
		return !isNullOrEmpty(s1) && !isNullOrEmpty(s2) ? s1.equals(s2) : false;
	}

	public static String getBlankStr(String origin) {
		return isNullOrEmpty(origin) ? "" : origin;
	}

	public static String getHtmlFormatString(String content) {
		return content == null ? null : content.replaceAll("\\<.*?>|\\n", "");
	}

	public static boolean isMatchCaseInsensitive(String string, String pattern) {
		String var2 = "(?i)" + pattern;
		return Pattern.compile(var2).matcher(string).find();
	}

	public static String readLine(String filename) throws IOException {
		BufferedReader reader = new BufferedReader(new FileReader(filename), 256);
		String line;
		try {
			line = reader.readLine();
		} finally {
			reader.close();
		}
		return line;
	}

	public static int calculateSpaceNumForString(String str) {
		if (str == null) {
			return 0;
		} else if (str.contains(" ")) {
			String[] var1 = str.split(" ");
			return var1.length > 0 ? var1.length - 1 : 0;
		} else {
			return 0;
		}
	}
}
