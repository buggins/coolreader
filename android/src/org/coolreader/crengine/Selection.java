package org.coolreader.crengine;

public class Selection {
	// IN
	public int startX;
	public int startY;
	public int endX;
	public int endY;
	// OUT
	public String startPos; 
	public String endPos;
	public String text;
	public String chapter;
	public int percent;
	
	boolean isEmpty() { return startPos==null || endPos==null; }
}
