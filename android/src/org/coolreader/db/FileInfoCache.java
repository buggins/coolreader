package org.coolreader.db;

import java.util.ArrayList;

import org.coolreader.crengine.FileInfo;

public class FileInfoCache {

	private final int maxSize;
	private int currentSize;

	private ArrayList<FileInfo> list = new ArrayList<FileInfo>(); 
	
	public FileInfoCache(int maxSize) {
		this.maxSize = maxSize;
	}
	
	public FileInfo remove(FileInfo entry) {
		int index = findByPath(entry.getPathName());
		if (index == -1)
			index = findById(entry.id);
		if (index == -1)
			return null;
		FileInfo removed = list.get(index);
		list.remove(index);
		currentSize--;
		return removed;
	}
	
	public void put(FileInfo entry) {
		int index = findByPath(entry.getPathName());
		if (index == -1)
			index = findById(entry.id);
		if (index == -1) {
			list.add(entry);
			currentSize++;
			checkSize();
			return;
		}
		list.set(index, entry);
		moveOnTop(index);
	}
	
	public FileInfo get(String path) {
		int index = findByPath(path);
		if (index == -1)
			return null;
		FileInfo item = list.get(index);
		moveOnTop(index);
		return item;
	}
	
	public FileInfo get(Long id) {
		if (id == null)
			return null;
		int index = findById(id);
		if (index == -1)
			return null;
		FileInfo item = list.get(index);
		moveOnTop(index);
		return item;
	}
	
	public void clear() {
		list.clear();
		currentSize = 0;
	}
	
	private void moveOnTop(int index) {
		if (index >= list.size() - 1)
			return;
		FileInfo item = list.get(index);
		list.remove(index);
		list.add(item);
	}
	
	private int findByPath(String path) {
		if (path == null)
			return -1;
		for (int i=0; i<list.size(); i++) {
			if (path.equals(list.get(i).getPathName()))
				return i;
		}
		return -1;
	}

	private int findById(Long id) {
		if (id == null)
			return -1;
		for (int i=0; i<list.size(); i++) {
			if (id.equals(list.get(i).id))
				return i;
		}
		return -1;
	}

	private void checkSize() {
		int itemsToRemove = currentSize - maxSize;
		if (itemsToRemove < maxSize / 10)
			return;
		for (int i=itemsToRemove; i>=0; i--) {
			list.remove(i);
		}
	}
}
