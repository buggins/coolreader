package org.coolreader.crengine;

import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class WordTimingAudiobookMatcher {
	public static final Logger log = L.create("wordtiming");

	private static class WordTiming {
		String word;
		Double startTime;
		File audioFile;

		public WordTiming(String word, Double startTime, File audioFile){
			this.word = word;
			this.startTime = startTime;
			this.audioFile = audioFile;
		}
	}

	private final File wordTimingsFile;
	private final List<SentenceInfo> allSentences;
	private final Map<String, File> fileCache = new HashMap<>();
	private String wordTimingsDir;
	private List<WordTiming> wordTimings;

	public WordTimingAudiobookMatcher(File wordTimingsFile, List<SentenceInfo> allSentences) {
		this.wordTimingsFile = wordTimingsFile;
		this.allSentences = allSentences;
	}

	public void parseWordTimingsFile(){
		List<String> lines = new ArrayList<>();
		try {
			BufferedReader br = new BufferedReader(new FileReader(wordTimingsFile));
			String line;
			while ((line = br.readLine()) != null) {
				lines.add(line);
			}
		} catch(Exception e) {
			log.d("ERROR: could not read  word timings file: " + wordTimingsFile + " " + e);
			lines = new ArrayList<>();
		}

		this.wordTimingsDir = wordTimingsFile.getAbsoluteFile().getParent();

		wordTimings = new ArrayList<>();
		for(String line : lines){
			WordTiming wordTiming = parseWordTimingsLine(line);
			if(wordTiming == null){
				log.d("ERROR: could not parse word timings line: " + line);
			}
			wordTimings.add(wordTiming);
		}

		for(int i=0; i<allSentences.size(); i++){
			SentenceInfo s = allSentences.get(i);
			SentenceInfo nextSentence;
			if(i+1<allSentences.size()){
				nextSentence = allSentences.get(i+1);
			}else{
				nextSentence = null;
			}
			s.nextSentence = nextSentence;
		}

		for(SentenceInfo s : allSentences){
			s.words = splitSentenceIntoWords(s.text);
		}

		if(wordTimings.size() == 0){
			return;
		}

		int wtIndex = 0;
		double prevStartTime = 0;
		File prevAudioFile = wordTimings.get(0).audioFile;
		for(SentenceInfo s : allSentences){
			if(s.words.size() == 0){
				s.startTime = prevStartTime;
				s.audioFile = prevAudioFile;
				continue;
			}
			boolean matchFailed = false;
			WordTiming firstWordTiming = null;
			int sentenceWtIndex = wtIndex;
			for(String wordInSentence : s.words){
				int wordWtIndex = sentenceWtIndex;
				boolean wordFound = false;
				while(wordWtIndex <= wordTimings.size()){
					if(wordInSentence.equals(wordTimings.get(wordWtIndex).word)){
						wordFound = true;
						break;
					}else if(wordWtIndex - sentenceWtIndex > 20){
						break;
					}else{
						wordWtIndex++;
					}
				}
				if(wordFound){
					if(firstWordTiming == null){
						firstWordTiming = wordTimings.get(wordWtIndex);
					}
					sentenceWtIndex = wordWtIndex + 1;
				}else{
					matchFailed = true;
					break;
				}
			}
			if(matchFailed){
				s.startTime = prevStartTime;
				s.audioFile = prevAudioFile;
			}else{
				wtIndex = sentenceWtIndex;
				s.startTime = firstWordTiming.startTime;
				s.audioFile = firstWordTiming.audioFile;
				prevStartTime = s.startTime;
				prevAudioFile = s.audioFile;
			}
		}

		//start first sentence of all audio files at 0.0
		// prevents skipping intros
		File curAudioFile = null;
		for(SentenceInfo s : allSentences){
			if(curAudioFile == null || s.audioFile != curAudioFile){
				s.isFirstSentenceInAudioFile = true;
				s.startTime = 0;
				curAudioFile = s.audioFile;
			}
		}
	}

	public SentenceInfo getSentence(double y, double x){
		for(SentenceInfo s : allSentences){
			if(s.startY > y || (s.startY == y && s.startX >= x)){
				return s;
			}
		}
		return null;
	}

	private WordTiming parseWordTimingsLine(String line){
		int sep1 = line.indexOf(',');
		int sep2 = line.indexOf(',', sep1+1);
		if(sep1 < 0 || sep2 < 0 || sep1 >= line.length() || sep2 >= line.length()){
			return null;
		}
		String word = line.substring(sep1+1, sep2);
		Double startTime = Double.parseDouble(line.substring(0, sep1));
		String audioFileName = line.substring(sep2+1);
		if(!fileCache.containsKey(audioFileName)){
			fileCache.put(audioFileName, new File(wordTimingsDir + "/" + audioFileName));
		}
		File audioFile = fileCache.get(audioFileName);
		return new WordTiming(word, startTime, audioFile);
	}

	private List<String> splitSentenceIntoWords(String sentence){
		List<String> words = new ArrayList<String>();

		StringBuilder str = null;
		boolean wordContainsLetterOrNumber = false;
		for(int i=0; i<sentence.length(); i++){
			char ch = sentence.charAt(i);
			if(ch == '’'){
				ch = '\'';
			}
			ch = Character.toLowerCase(ch);

			boolean isWordChar;
			if('a' <= ch && ch <= 'z'){
				isWordChar = true;
				wordContainsLetterOrNumber = true;
			}else if('0' <= ch && ch <= '9'){
				isWordChar = true;
				wordContainsLetterOrNumber = true;
			}else if(ch == '\''){
				isWordChar = true;
			}else{
				isWordChar = false;
			}

			if(isWordChar){
				if(str == null){
					str = new StringBuilder();
				}
				str.append(ch);
			}

			if(str != null && (!isWordChar || i == sentence.length()-1)){
				if(wordContainsLetterOrNumber){
					words.add(str.toString());
				}
				str = null;
				wordContainsLetterOrNumber = false;
			}
		}

		return words;
	}
}
