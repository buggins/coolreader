package org.coolreader.crengine;

import java.util.HashSet;
import java.util.Set;

public class FileInfoChangeSource implements FileInfoChangeListener {
	Set<FileInfoChangeListener> listeners = new HashSet<FileInfoChangeListener>();
	void addListener(FileInfoChangeListener listener) {
		listeners.add(listener);
	}
	void removeListener(FileInfoChangeListener listener) {
		listeners.remove(listener);
	}
	@Override
	public void onChange(FileInfo object, boolean filePropsOnlyChange) {
		for (FileInfoChangeListener listener : listeners)
			listener.onChange(object, filePropsOnlyChange);
	}
}
