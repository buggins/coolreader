package org.coolreader.plugins;

import org.coolreader.crengine.Utils;
import org.coolreader.plugins.litres.LitresConnection;
import org.coolreader.plugins.litres.LitresConnection.LitresAuthor;
import org.coolreader.plugins.litres.LitresConnection.LitresAuthors;

public class OnlineStoreBook {
	public LitresAuthors authors = new LitresAuthors();
	public String bookTitle;
	public String id;
	public double basePrice;
	public double price;
	public int zipSize;
	public boolean hasTrial;
	public String trialId;
	public String cover;
	public String coverPreview;
	public int rating;
	public String sequenceName;
	public int sequenceNumber;
	public String getAuthors() {
		StringBuilder buf = new StringBuilder();
		for (int i=0; i <authors.size(); i++) {
			LitresAuthor author = authors.get(i);
			if (buf.length() > 0)
				buf.append("|");
			String name = Utils.concatWs(author.firstName, author.lastName, " ");
			if (name.length() == 0)
				name = author.title;
			buf.append(name);
		}
		return buf.toString();
	}
}
