package org.coolreader.crengine;

import java.util.ArrayList;

public class TOCItem {
	private TOCItem mParent;
	private int mLevel;
	private int mIndex;
	private int mGlobalIndex=-1;
	private int mPage;
	private int mPercent;
	private boolean mExpanded;
	private String mName;
	private String mPath;
	private ArrayList<TOCItem> mChildren;
	// create root item
	public TOCItem() {
	}
	// create child item
	public TOCItem addChild() {
		if ( mChildren==null )
			mChildren = new ArrayList<TOCItem>();
		TOCItem item = new TOCItem();
		item.mParent = this;
		item.mIndex = mChildren.size();
		mChildren.add(item);
		return item;
	}
	public int getChildCount(){
		return mChildren!=null ? mChildren.size() : 0;
	}
	public TOCItem getChild( int index)	{
		return mChildren.get(index);
	}
	public TOCItem getParent() {
		return mParent;
	}
	public int getLevel() {
		return mLevel;
	}
	public int getIndex() {
		return mIndex;
	}
	public int getGlobalIndex() {
		return mGlobalIndex;
	}
	public void setGlobalIndex( int index ) {
		mGlobalIndex = index;
	}
	public boolean getExpanded() {
		return mExpanded;
	}
	public void setExpanded( boolean expanded ) {
		mExpanded = expanded;
	}
	public int getPage() {
		return mPage;
	}
	public int getPercent() {
		return mPercent;
	}
	public String getName() {
		return mName;
	}
	public String getPath() {
		return mPath;
	}
	public TOCItem getChapterAtPage(int page)
	{
		if ( this.getChildCount() > 1) {

			TOCItem curChapter = null;
			for ( int i=this.getChildCount()-1; i>=0; i-- ) {
				curChapter = this.getChild(i);
				if ( curChapter.getPage()<=page )
					if (curChapter.getChildCount() > 1)
						return curChapter.getChapterAtPage(page);
					else
						return curChapter;
			}
		}
		return this;
	}
	public TOCItem getNextChapter()
	{
		if (this.getParent() == null)
			return null;

		TOCItem parent = this.getParent();
		int pos = parent.mChildren.indexOf(this);
		if (pos < parent.getChildCount()-1)
			return parent.getChild(pos+1);
		
		return parent.getNextChapter();
	}
}
