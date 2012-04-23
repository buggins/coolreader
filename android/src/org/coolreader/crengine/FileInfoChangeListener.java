package org.coolreader.crengine;

public interface FileInfoChangeListener {
	/**
	 * Notify about file or directory change.
	 * @param object object which has been changed
	 * @param onlyProperties is true if only book info is changed, but no item count changed
	 */
	void onChange(FileInfo object, boolean onlyProperties);
}
