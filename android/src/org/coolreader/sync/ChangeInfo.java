package org.coolreader.sync;

import java.io.UnsupportedEncodingException;

import org.coolreader.crengine.Bookmark;

public class ChangeInfo implements Comparable<ChangeInfo> {
	public Bookmark bookmark;
	public String fileName;
	public boolean deleted;
	public long timestamp;
	
	public Bookmark getBookmark() { return bookmark; }
	public String getFileName() { return fileName; }
	public boolean isDeleted() { return deleted; }
	public long getTimestamp() { return timestamp; }
	
	private ChangeInfo() {
	}
	
	public ChangeInfo(Bookmark bookmark, String fileName, boolean deleted) {
		this.bookmark = bookmark;
		this.fileName = fileName;
		this.deleted = deleted;
		timestamp = bookmark != null && bookmark.getTimeStamp() > 0 ? bookmark.getTimeStamp() : System.currentTimeMillis();
	}
	
	public final static String START_TAG = "# start record";
	public final static String END_TAG = "# end record";
	public final static byte[] START_TAG_BYTES = (START_TAG + "\n").getBytes();
	public final static byte[] END_TAG_BYTES = (END_TAG + "\n").getBytes();
	public final static String ACTION_TAG = "ACTION"; 
	public final static String ACTION_DELETE_TAG = "DELETE"; 
	public final static String ACTION_UPDATE_TAG = "UPDATE"; 
	public final static String FILE_TAG = "FILE"; 
	public final static String TYPE_TAG = "TYPE"; 
	public final static String START_POS_TAG = "STARTPOS"; 
	public final static String END_POS_TAG = "ENDPOS"; 
	public final static String TIMESTAMP_TAG = "TIMESTAMP"; 
	public final static String PERCENT_TAG = "PERCENT"; 
	public final static String SHORTCUT_TAG = "SHORTCUT"; 
	public final static String TITLE_TEXT_TAG = "TITLETEXT"; 
	public final static String POS_TEXT_TAG = "POSTEXT"; 
	public final static String COMMENT_TEXT_TAG = "COMMENTTEXT";
	
	@Override
	public String toString() {
		StringBuilder buf = new StringBuilder();
		buf.append(START_TAG);
		buf.append('\n');
		buf.append(FILE_TAG).append('=').append(fileName);
		buf.append('\n');
		buf.append(ACTION_TAG).append('=').append(deleted ? ACTION_DELETE_TAG : ACTION_UPDATE_TAG);
		buf.append('\n');
		buf.append(TIMESTAMP_TAG).append('=').append(timestamp);
		buf.append('\n');
		if (bookmark != null) {
			buf.append(TYPE_TAG).append('=').append(bookmark.getType());
			buf.append('\n');
			buf.append(START_POS_TAG).append('=').append(bookmark.getStartPos());
			buf.append('\n');
			buf.append(END_POS_TAG).append('=').append(bookmark.getEndPos());
			buf.append('\n');
			buf.append(PERCENT_TAG).append('=').append(bookmark.getPercent());
			buf.append('\n');
			buf.append(SHORTCUT_TAG).append('=').append(bookmark.getShortcut());
			buf.append('\n');
			buf.append(TITLE_TEXT_TAG).append('=').append(encodeText(bookmark.getTitleText()));
			buf.append('\n');
			buf.append(POS_TEXT_TAG).append('=').append(encodeText(bookmark.getPosText()));
			buf.append('\n');
			buf.append(COMMENT_TEXT_TAG).append('=').append(encodeText(bookmark.getCommentText()));
			buf.append('\n');
		}
		buf.append(END_TAG);
		buf.append('\n');
		return buf.toString();
	}
	
	public static String encodeText(String text) {
		if (text == null || text.length() == 0)
			return "";
		StringBuilder buf = new StringBuilder();
		for (char ch : text.toCharArray()) {
			switch (ch) {
			case '\\':
				buf.append(ch);
				buf.append(ch);
				break;
			case '\n':
				buf.append('\\');
				buf.append('n');
				break;
			case '\r':
				buf.append('\\');
				buf.append('r');
				break;
			case '\t':
				buf.append('\\');
				buf.append('t');
				break;
			default:
				buf.append(ch);
			}
		}
		return buf.toString();
	}
	
	public static String decodeText(String text) {
		if (text == null || text.length() == 0)
			return "";
		StringBuilder buf = new StringBuilder();
		boolean lastControl = false;
		for (char ch : text.toCharArray()) {
			if (lastControl) {
				switch (ch) {
				case 'r':
					buf.append('\r');
					break;
				case 'n':
					buf.append('\n');
					break;
				case 't':
					buf.append('\t');
					break;
				default:
					buf.append(ch);
				}
				lastControl = false;
				continue;
			}
			if (ch == '\\') {
				lastControl = true;
				continue;
			}
			buf.append(ch);
		}
		return buf.toString();
	}
	
	public static ChangeInfo fromString(String s) {
		String[] rows = s.split("\n");
		if (rows.length<3 || !START_TAG.equals(rows[0]) || !END_TAG.equals(rows[rows.length - 1]))
			return null;
		ChangeInfo ci = new ChangeInfo();
		try {
			Bookmark bmk = new Bookmark();
			for (int i=1; i<rows.length - 1; i++) {
				String row = rows[i];
				int p = row.indexOf('=');
				if (p<1)
					continue;
				String name = row.substring(0, p);
				String value = row.substring(p + 1);
				if (ACTION_TAG.equals(name)) {
					ci.deleted = ACTION_DELETE_TAG.equals(value);
				} else if (FILE_TAG.equals(name)) { 
					ci.fileName = value;
				} else if (TYPE_TAG.equals(name)) { 
					bmk.setType(Integer.valueOf(value));
				} else if (START_POS_TAG.equals(name)) { 
					bmk.setStartPos(value);
				} else if (END_POS_TAG.equals(name)) { 
					bmk.setEndPos(value);
				} else if (TIMESTAMP_TAG.equals(name)) { 
					ci.timestamp = Long.parseLong(value);
					bmk.setTimeStamp(ci.timestamp);
				} else if (PERCENT_TAG.equals(name)) { 
					bmk.setPercent(Integer.valueOf(value));
				} else if (SHORTCUT_TAG.equals(name)) { 
					bmk.setShortcut(Integer.valueOf(value));
				} else if (TITLE_TEXT_TAG.equals(name)) { 
					bmk.setTitleText(decodeText(value));
				} else if (POS_TEXT_TAG.equals(name)) { 
					bmk.setPosText(decodeText(value));
				} else if (COMMENT_TEXT_TAG.equals(name)) {
					bmk.setCommentText(decodeText(value));
				}
			}
			if (bmk.isValid())
				ci.bookmark = bmk;
			if (ci.fileName == null)
				return null;
			if (ci.timestamp == 0)
				return null;
			if (ci.bookmark == null && !ci.deleted)
				return null;
		} catch (Exception e) {
			return null;
		}
		return ci;
	}

	public static ChangeInfo fromBytes(byte[] buf, int start, int end) {
		try {
			String s = new String(buf, start, end - start, "UTF8");
			return fromString(s);
		} catch (UnsupportedEncodingException e) {
			return null;
		}
	}

	public static int[] findNextRecordBounds(byte[] buf, int start, int end) {
		int startTagPos = findBytes(buf, start, end, START_TAG_BYTES);
		if (startTagPos < 0)
			return null;
		int endTagPos = findBytes(buf, startTagPos, end, END_TAG_BYTES);
		if (endTagPos < 0)
			return null;
		int[] res = {startTagPos, endTagPos + END_TAG_BYTES.length};
		return res;
	}

	private static int findBytes(byte[] buf, int start, int end, byte[] pattern) {
		int len = pattern.length;
		for (int i = start; i <= end - len; i++) {
			int j = 0;
			for (; j < len; j++) {
				if (buf[i+j] != pattern[j])
					break;
			}
			if (j == len)
				return i;
		}
		return -1;
	}
	
	@Override
	public int compareTo(ChangeInfo another) {
		if (timestamp < another.timestamp)
			return -1;
		if (timestamp > another.timestamp)
			return 1;
		return 0;
	}
}
