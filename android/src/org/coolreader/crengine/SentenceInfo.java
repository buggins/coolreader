package org.coolreader.crengine;

import java.io.File;
import java.util.List;

public class SentenceInfo {
	public String text;
	public String startPos;

	public double startTime;
	public double startTimeInBook;
	public double totalBookDuration;
	public boolean isFirstSentenceInAudioFile = false;
	public File audioFile;
	public List<String> words;
	public SentenceInfo nextSentence;

	public SentenceInfo() {
	}

	public void setStartPos(String startPos){
		this.startPos = startPos;
	}
	public void setText(String text){
		this.text = text;
	}
}
