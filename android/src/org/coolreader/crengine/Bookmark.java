package org.coolreader.crengine;


public class Bookmark {

	public Bookmark()
	{
	}
	
	public int getType() {
		return type;
	}
	public void setType(int type) {
		this.type = type;
		modified = true;
	}
	public int getPercent() {
		return percent;
	}
	public void setPercent(int percent) {
		this.percent = percent;
		modified = true;
	}
	public String getStartPos() {
		return startPos;
	}
	public void setStartPos(String startPos) {
		this.startPos = startPos;
		modified = true;
	}
	public String getEndPos() {
		return endPos;
	}
	public void setEndPos(String endPos) {
		this.endPos = endPos;
		modified = true;
	}
	public String getCommentText() {
		return commentText;
	}
	public void setCommentText(String commentText) {
		this.commentText = commentText;
		modified = true;
	}
	public String getTitleText() {
		return titleText;
	}
	public String getPosText() {
		return posText;
	}
	
	public void setTitleText(String titleText) {
		this.titleText = titleText;
		modified = true;
	}
	public void setPosText(String posText) {
		this.posText = posText;
		modified = true;
	}
	public int getShortcut() {
		return shortcut;
	}
	public void setShortcut(int shortcut) {
		modified = true;
		this.shortcut = shortcut;
	}
	public long getTimeStamp() {
		return timeStamp;
	}
	public void setTimeStamp(long timeStamp) {
		this.timeStamp = timeStamp;
		modified = true;
	}
	
	public Long getId() {
		return id;
	}
	public void setId(Long id) {
		this.id = id;
	}

	public boolean isModified() {
		return modified || id==null;
	}
	public void setModified(boolean modified) {
		this.modified = modified;
	}

	public static final int TYPE_LAST_POSITION = 0;
	public static final int TYPE_POSITION = 1;
	public static final int TYPE_COMMENT = 2;
	public static final int TYPE_CORRECTION = 3;
	private Long id;
	private int type;
	private int percent;
	private int shortcut;
	private String startPos;
	private String endPos;
	private String titleText;
	private String posText;
	private String commentText;
	private long timeStamp; // UTC timestamp
	private boolean modified;
	
}
