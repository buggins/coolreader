/*
 * CoolReader for Android
 * Copyright (C) 2010,2012 Vadim Lopatin <coolreader.org@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.coolreader.crengine;

import java.io.File;

public class BackgroundTextureInfo {
	public String id; // filepath for external image or unique symbolic name for resource
	public String name;
	public int resourceId;
	public boolean tiled;
	public BackgroundTextureInfo(String id, String name, int resourceId) {
		this.id = id;
		this.name = name;
		this.resourceId = resourceId;
		this.tiled = id.startsWith("tx_") || id.indexOf("/textures/")>0;
	}

	public static BackgroundTextureInfo fromFile( String filename ) {
		if ( filename==null )
			return null;
		File f = new File(filename);
		if ( !f.isFile() || !f.exists() )
			return null;
		String nm = new File(filename).getName();
		String fnlc = nm.toLowerCase();
		if ( fnlc.endsWith(".png") || fnlc.endsWith(".jpg") || fnlc.endsWith(".jpeg") || fnlc.endsWith(".gif") ) {
			return new BackgroundTextureInfo(filename, nm.substring(0, nm.lastIndexOf('.')), 0);
		}
		return null;
	}

	public static final String NO_TEXTURE_ID = "(NONE)";
	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + ((id == null) ? 0 : id.hashCode());
		result = prime * result + ((name == null) ? 0 : name.hashCode());
		result = prime * result + resourceId;
		return result;
	}
	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		BackgroundTextureInfo other = (BackgroundTextureInfo) obj;
		if (id == null) {
			if (other.id != null)
				return false;
		} else if (!id.equals(other.id))
			return false;
		if (name == null) {
			if (other.name != null)
				return false;
		} else if (!name.equals(other.name))
			return false;
		if (resourceId != other.resourceId)
			return false;
		return true;
	}
	@Override
	public String toString() {
		return "BackgroundTextureInfo[id=" + id + ", name=" + name + ", tiled=" + tiled + "]";
	}
	public boolean isNone() {
		return id==null || id.equals(NO_TEXTURE_ID);
	}
}
