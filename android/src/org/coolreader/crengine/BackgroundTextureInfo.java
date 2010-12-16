package org.coolreader.crengine;

public class BackgroundTextureInfo {
	public String id;
	public String name;
	public int resourceId;
	public String filepath;
	public BackgroundTextureInfo(String id, String name, int resourceId) {
		this.id = id;
		this.name = name;
		this.resourceId = resourceId;
	}
	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result
				+ ((filepath == null) ? 0 : filepath.hashCode());
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
		if (filepath == null) {
			if (other.filepath != null)
				return false;
		} else if (!filepath.equals(other.filepath))
			return false;
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
		return "BackgroundTextureInfo [id=" + id + ", name=" + name + "]";
	}
	
}
