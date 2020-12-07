/*
 *   Copyright (C) 2020 by Chernov A.A.
 *   valexlin@gmail.com
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.coolreader.sync2;

import java.util.Date;
import java.util.HashMap;
import java.util.Map;

public final class FileMetadata implements Cloneable {

	public static final String CUSTOM_PROP_FINGERPRINT = "cr3.fingerprint";
	public static final String CUSTOM_PROP_SOURCE_SIZE = "cr3.filesize";

	public String id;
	public String fileName;
	public String description;
	public long fileSize;
	public Date createdDate;
	public Date modifiedDate;
	public String mimeType;
	public boolean isFolder;
	public boolean isTrashed;
	public boolean isShared;
	public Map<String, String> appProperties;
	public FileMetadata parent;

	public FileMetadata() {
	}

	public FileMetadata(String id, String fileName, boolean isFolder) {
		this.id = id;
		this.fileName = fileName;
		this.isFolder = isFolder;
	}

	public boolean isNull() {
		return null == fileName;
	}

	public String getCustomPropFingerprint() {
		String fingerprint = "";
		if (null != appProperties) {
			String str = appProperties.get(CUSTOM_PROP_FINGERPRINT);
			if (null != str) {
				fingerprint = str;
			}
		}
		return fingerprint;
	}

	public void setCustomPropFingerprint(String fingerprint) {
		if (null == appProperties)
			appProperties = new HashMap<String, String>();
		appProperties.put(CUSTOM_PROP_FINGERPRINT, fingerprint);
	}

	public int getCustomPropSourceSize() {
		int size = -1;
		if (null != appProperties) {
			String str = appProperties.get(CUSTOM_PROP_SOURCE_SIZE);
			if (null != str) {
				try {
					size = Integer.parseInt(str, 10);
				} catch (Exception ignored) {
				}
			}
		}
		return size;
	}

	public void setCustomPropSourceSize(int size) {
		if (null == appProperties)
			appProperties = new HashMap<String, String>();
		appProperties.put(CUSTOM_PROP_SOURCE_SIZE, Integer.toString(size, 10));
	}

	public Object clone() {
		FileMetadata meta = new FileMetadata();
		meta.id = id;
		meta.fileName = fileName;
		meta.description = description;
		meta.fileSize = fileSize;
		meta.createdDate = createdDate;
		meta.modifiedDate = modifiedDate;
		meta.mimeType = mimeType;
		meta.isFolder = isFolder;
		meta.isTrashed = isTrashed;
		meta.isShared = isShared;
		meta.appProperties = appProperties;
		meta.parent = parent;
		return meta;
	}

	public String toString() {
		String result;
		if (isFolder)
			result = "[dir]  ";
		else
			result = "[file] ";
		if (null != fileName)
			result += fileName;
		else
			result += "null";
		if (!isFolder)
			result += " [size=" + fileSize + "]";
		return result;
	}
}
