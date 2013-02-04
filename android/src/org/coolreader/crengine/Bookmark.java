package org.coolreader.crengine;


public class Bookmark {

	public Bookmark()
	{
	}
	
	public Bookmark( Bookmark v )
	{
		id=v.id;
		type=v.type;
		percent=v.percent;
		shortcut=v.shortcut;
		startPos=v.startPos;
		endPos=v.endPos;
		titleText=v.titleText;
		posText=v.posText;
		commentText=v.commentText;
		timeStamp=v.timeStamp;
		timeElapsed = v.timeElapsed;
	}
	
	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result
				+ ((commentText == null) ? 0 : commentText.hashCode());
		result = prime * result + ((endPos == null) ? 0 : endPos.hashCode());
		result = prime * result + ((id == null) ? 0 : id.hashCode());
		result = prime * result + percent;
		result = prime * result + ((posText == null) ? 0 : posText.hashCode());
		result = prime * result + shortcut;
		result = prime * result
				+ ((startPos == null) ? 0 : startPos.hashCode());
		result = prime * result + (int) (timeStamp ^ (timeStamp >>> 32));
		result = prime * result
				+ ((titleText == null) ? 0 : titleText.hashCode());
		result = prime * result + type;
		return result;
	}

	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		Bookmark other = (Bookmark) obj;
		if (commentText == null) {
			if (other.commentText != null)
				return false;
		} else if (!commentText.equals(other.commentText))
			return false;
		if (endPos == null) {
			if (other.endPos != null)
				return false;
		} else if (!endPos.equals(other.endPos))
			return false;
		if (id == null) {
			if (other.id != null)
				return false;
		} else if (!id.equals(other.id))
			return false;
		if (percent != other.percent)
			return false;
		if (posText == null) {
			if (other.posText != null)
				return false;
		} else if (!posText.equals(other.posText))
			return false;
		if (shortcut != other.shortcut)
			return false;
		if (startPos == null) {
			if (other.startPos != null)
				return false;
		} else if (!startPos.equals(other.startPos))
			return false;
		if (timeStamp != other.timeStamp)
			return false;
		if (timeElapsed != other.timeElapsed)
			return false;
		if (titleText == null) {
			if (other.titleText != null)
				return false;
		} else if (!titleText.equals(other.titleText))
			return false;
		if (type != other.type)
			return false;
		return true;
	}

	public String getUniqueKey() {
		switch (type) {
		case TYPE_LAST_POSITION:
			return "l";
		case TYPE_POSITION:
			return shortcut > 0 ? String.valueOf(shortcut) : "p" + startPos;
		case TYPE_COMMENT:
			return "c" + startPos + "-" + endPos;
		case TYPE_CORRECTION:
			return "r" + startPos + "-" + endPos;
		default:
			return "unknown";
		}
	}
	
	public boolean equalUniqueKey(Bookmark bm) {
		if (type != bm.type)
			return false;
		switch (type) {
		case TYPE_LAST_POSITION:
			return true;
		case TYPE_POSITION:
			return shortcut > 0 ? shortcut == bm.shortcut : Utils.eq(startPos, bm.startPos);
		case TYPE_COMMENT:
		case TYPE_CORRECTION:
			return Utils.eq(startPos, bm.startPos) && Utils.eq(endPos, bm.endPos);
		default:
			return false;
		}
	}
	
	public int getType() {
		return type;
	}
	public boolean setType(int type) {
		if (this.type == type)
			return false;
		this.type = type;
		return true;
	}
	public int getPercent() {
		return percent;
	}
	public void setPercent(int percent) {
		this.percent = percent;
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
	private boolean changed( String v1, String v2 ) {
		if ( v1==null && v2==null )
			return false;
		if ( v1==null || v2==null )
			return true;
		return !v1.equals(v2);
	}
	public boolean setCommentText(String commentText) {
		if ( !changed(this.commentText, commentText) )
			return false;
		this.commentText = commentText;
		return true;
	}
	public String getTitleText() {
		return titleText;
	}
	public String getPosText() {
		return posText;
	}
	
	public void setTitleText(String titleText) {
		this.titleText = titleText;
	}
	public void setPosText(String posText) {
		this.posText = posText;
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
		if ( this.timeStamp == timeStamp )
			return;
		this.timeStamp = timeStamp;
	}
	public long getTimeElapsed() {
		return timeElapsed;
	}
	public void setTimeElapsed(long timeElapsed) {
		if ( this.timeElapsed == timeElapsed )
			return;
		this.timeElapsed = timeElapsed;
	}	
	public Long getId() {
		return id;
	}
	public void setId(Long id) {
		this.id = id;
	}

	public boolean isValid() {
		if (startPos == null || startPos.length() == 0)
			return false;
		if (type < TYPE_LAST_POSITION || type > TYPE_CORRECTION)
			return false;
		if ((endPos == null || endPos.length() == 0) && (type == TYPE_COMMENT || type == TYPE_CORRECTION))
			return false;
		return true;
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
	private long timeStamp = System.currentTimeMillis(); // UTC timestamp
	private long timeElapsed;
	@Override
	public String toString() {
		return "Bookmark[t=" + type + ", start=" + startPos + "]";
	}
	
}
