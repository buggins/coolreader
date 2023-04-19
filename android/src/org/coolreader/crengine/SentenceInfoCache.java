package org.coolreader.crengine;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class SentenceInfoCache {
	public static final Logger log = L.create("sentenceinfocache");

	private final File sentenceInfoCacheFile;

	public static List<SentenceInfo> maybeReadCache(File sentenceInfoCacheFile) {
		try{
			return new SentenceInfoCache(sentenceInfoCacheFile).readCache();
		}catch(Exception e){
			log.e("ERROR: could not read sentence info cache file: " + sentenceInfoCacheFile + " " + e);
			return null;
		}
	}
	public static void maybeWriteCache(File sentenceInfoCacheFile, List<SentenceInfo> allSentences) {
		try{
			new SentenceInfoCache(sentenceInfoCacheFile).writeCache(allSentences);
		}catch(Exception e){
			log.e("ERROR: could not write sentence info cache file: " + sentenceInfoCacheFile + " " + e);
		}
	}


	public SentenceInfoCache(File sentenceInfoCacheFile) {
		this.sentenceInfoCacheFile = sentenceInfoCacheFile;
	}

	public List<SentenceInfo> readCache() throws IOException {
		List<SentenceInfo> allSentences = new ArrayList<>();
		try (
			BufferedReader br = new BufferedReader(new FileReader(sentenceInfoCacheFile));
		) {
			String line;
			while ((line = br.readLine()) != null) {
				SentenceInfo sentenceInfo = parseSentenceInfoLine(line);
				if(sentenceInfo == null){
					log.e("ERROR: could not parse sentence info cache line: " + line);
					return null;
				}
				allSentences.add(sentenceInfo);
			}
		}
		if(allSentences.isEmpty()){
			return null;
		}
		return allSentences;
	}

	public void writeCache(List<SentenceInfo> allSentences) throws IOException {
		FileWriter fw = new FileWriter(sentenceInfoCacheFile);
		for(SentenceInfo s : allSentences){
			fw.write(formatSentenceInfo(s));
		}
		fw.close();
	}

	private SentenceInfo parseSentenceInfoLine(String line){
		int sep = line.indexOf(',');
		if(sep < 0 || sep >= line.length()){
			return null;
		}
		SentenceInfo sentenceInfo = new SentenceInfo();
		sentenceInfo.startPos = line.substring(0, sep);
		sentenceInfo.text = line.substring(sep+1);
		return sentenceInfo;
	}

	private String formatSentenceInfo(SentenceInfo sentenceInfo){
		return sentenceInfo.startPos + "," + sentenceInfo.text + "\n";
	}
}
