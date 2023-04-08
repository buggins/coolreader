package org.coolreader.crengine;

import java.util.List;

public class SentenceInfo {
	public int startX;
	public int startY;
	public String text;

	public double startTime;
	public String audioFile;
	public List<String> words;

	public SentenceInfo() {
	}

	public void setStartX(int startX){
		this.startX = startX;
	}
	public void setStartY(int startY){
		this.startY = startY;
	}
	public void setText(String text){
		this.text = text;
	}
}
