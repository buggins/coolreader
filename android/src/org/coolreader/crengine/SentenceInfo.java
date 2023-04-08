package org.coolreader.crengine;

import java.io.File;
import java.util.List;

public class SentenceInfo {
	public int startX;
	public int startY;
	public String text;

	public double startTime;
	public File audioFile;
	public List<String> words;
	public SentenceInfo nextSentence;

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
