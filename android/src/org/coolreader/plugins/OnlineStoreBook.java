package org.coolreader.plugins;

import org.coolreader.crengine.Utils;
import org.coolreader.plugins.litres.LitresConnection;

public class OnlineStoreBook {
	public OnlineStoreAuthors authors = new OnlineStoreAuthors();
	public String bookTitle;
	public String id;
	public double basePrice;
	public double price;
	public int zipSize;
	public boolean hasTrial;
	public String trialUrl; // full URL to download trial version
	public String trialFileName; // generated filename to save trial book version as
	public String downloadUrl; // full download URL
	public String downloadFileName; // generated filename to save full book version as
	public String cover;
	public String coverPreview;
	public int rating;
	public String sequenceName;
	public int sequenceNumber;
	public String getAuthors() {
		StringBuilder buf = new StringBuilder();
		for (int i=0; i <authors.size(); i++) {
			OnlineStoreAuthor author = authors.get(i);
			if (buf.length() > 0)
				buf.append("|");
			String name = Utils.concatWs(author.firstName, author.lastName, " ");
			if (name.length() == 0)
				name = author.title;
			buf.append(name);
		}
		return buf.toString();
	}
	public String getSeries() {
		if (Utils.empty(sequenceName))
			return "";
		if (sequenceNumber <= 0)
			return sequenceName;
		return sequenceName + " #" + sequenceNumber;
	}
}
