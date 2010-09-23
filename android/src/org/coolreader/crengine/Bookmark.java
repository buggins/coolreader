package org.coolreader.crengine;

import android.os.Parcel;
import android.os.Parcelable;

public class Bookmark implements Parcelable {
	
	
	
	public int getType() {
		return type;
	}
	public void setType(int type) {
		this.type = type;
	}
	public int getPercent() {
		return percent;
	}
	public void setPercent(int percent) {
		this.percent = percent;
	}
	public int getPage() {
		return page;
	}
	public void setPage(int page) {
		this.page = page;
	}
	public String getStartPos() {
		return startPos;
	}
	public void setStartPos(String startPos) {
		this.startPos = startPos;
	}
	public String getEndPos() {
		return endPos;
	}
	public void setEndPos(String endPos) {
		this.endPos = endPos;
	}
	public String getCommentText() {
		return commentText;
	}
	public void setCommentText(String commentText) {
		this.commentText = commentText;
	}
	public String getTitleText() {
		return titleText;
	}
	public String getPosText() {
		return posText;
	}
	
	public int getShortcut() {
		return shortcut;
	}
	public void setShortcut(int shortcut) {
		this.shortcut = shortcut;
	}
	public long getTimeStamp() {
		return timeStamp;
	}
	public void setTimeStamp(long timeStamp) {
		this.timeStamp = timeStamp;
	}

	public static final int TYPE_LAST_POSITION = 0;
	public static final int TYPE_POSITION = 1;
	public static final int TYPE_COMMENT = 2;
	public static final int TYPE_CORRECTION = 3;
	private int type;
	private int percent;
	private int page;
	private int shortcut;
	private String startPos;
	private String endPos;
	private String titleText;
	private String posText;
	private String commentText;
	private long timeStamp; // UTC timestamp
	
	public final static Parcelable.Creator<Bookmark> CREATOR = new Parcelable.Creator<Bookmark>() {

		public Bookmark createFromParcel(Parcel source) {
			try {
				Bookmark res = new Bookmark(source);
				return res;
			} catch ( Exception e ) {
				return null;
			}
		}

		public Bookmark[] newArray(int size) {
			return new Bookmark[size];
		}
	};
	
	public int describeContents() {
		return 0;
	}

	public Bookmark()
	{
		
	}
	
	private static final int FORMAT_VERSION = 1;
	private Bookmark(Parcel source) throws Exception
	{
		if (source.readInt()!=FORMAT_VERSION)
			throw new Exception("Invalid FileInfo format");
		type = source.readInt();
		percent = source.readInt();
		page = source.readInt();
		shortcut = source.readInt();
		startPos = source.readString();
		endPos = source.readString();
		titleText = source.readString();
		posText = source.readString();
		commentText = source.readString();
		timeStamp = source.readLong();
	}
	
	public void writeToParcel(Parcel dest, int flags) {
		dest.writeInt(FORMAT_VERSION);
		dest.writeInt(type);
		dest.writeInt(percent);
		dest.writeInt(page);
		dest.writeInt(shortcut);
		dest.writeString(startPos);
		dest.writeString(endPos);
		dest.writeString(titleText);
		dest.writeString(posText);
		dest.writeString(commentText);
		dest.writeLong(timeStamp);
	}
	
	
}
