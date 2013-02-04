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
