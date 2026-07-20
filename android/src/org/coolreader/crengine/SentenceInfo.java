package org.coolreader.crengine;

import java.io.File;
import java.util.List;

public class SentenceInfo {
	public String text;
	public String startPos;

	public SentenceTiming sentenceTiming = new SentenceTiming();
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
