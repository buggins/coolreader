package org.coolreader.crengine;

import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class WordTimingAudiobookMatcher {
	public static final Logger log = L.create("wordtiming");

	private static final Pattern WORD_TIMING_REGEX = Pattern.compile(
		"^(\\d+|\\d*\\.\\d+),([^,]+),(.+)$"
	);
	private static class WordTiming {
		String word;
		Double startTime;
		String audioFile;

		public WordTiming(String word, Double startTime, String audioFile){
			this.word = word;
			this.startTime = startTime;
			this.audioFile = audioFile;
		}
	}

	private final File wordTimingsFile;
	private final List<SentenceInfo> allSentences;
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

		wordTimings = new ArrayList<>();
		for(String line : lines){
			Matcher m = WORD_TIMING_REGEX.matcher(line);
			if(m.matches()){
				wordTimings.add(new WordTiming(m.group(2), Double.parseDouble(m.group(1)), m.group(3)));
			}else{
				log.d("ERROR: could not parse word timings line: " + line);
			}
		}

		for(SentenceInfo s : allSentences){
			String text = s.text;
			text = text.replaceAll("’", "'");
			text = text.toLowerCase();
			String[] words = text.split("[^a-z0-9']");
			s.words = new ArrayList<>();
			for(String word : words){
				if(word.matches(".*\\w.*")){
					s.words.add(word);
				}
			}
		}

		if(wordTimings.size() == 0){
			return;
		}

		int wtIndex = 0;
		double prevStartTime = 0;
		String prevAudioFile = wordTimings.get(0).audioFile;
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
	}

	public SentenceInfo getSentence(double y, double x){
		for(SentenceInfo s : allSentences){
			if(s.startY > y || (s.startY == y && s.startX >= x)){
				return s;
			}
		}
		return null;
	}

	public File getAudioFile(String audioFileName){
		String dir = wordTimingsFile.getAbsoluteFile().getParent();
		return new File(dir + "/" + audioFileName);
	}
}
