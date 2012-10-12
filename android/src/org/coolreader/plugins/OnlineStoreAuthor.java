package org.coolreader.plugins;

import org.coolreader.crengine.Utils;

public class OnlineStoreAuthor {
	public String id;
	public String firstName;
	public String lastName;
	public String middleName;
	public String title;
	public String photo;
	public String getPrefix(int size) {
		String prefix = "";
		if (!Utils.empty(lastName)) {
			prefix = lastName.substring(0, size <= lastName.length() ? size : lastName.length());
		}
		while (prefix.length() < size)
			prefix = prefix + " ";
		return prefix.toUpperCase();
	}
	@Override
	public String toString() {
		return "LitresAuthor [id=" + id + ", lastName=" + lastName
				+ ", firstName=" + firstName + ", middleName=" + middleName
				+ ", title=" + title + ", photo=" + photo + "]";
	}
}