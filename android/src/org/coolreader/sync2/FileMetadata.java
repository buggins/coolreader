/*
 *   Copyright (C) 2020 by Chernov A.A.
 *   valexlin@gmail.com
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
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

public final class FileMetadata implements Cloneable {

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

	public Object clone() throws CloneNotSupportedException {
		return super.clone();
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
