package org.coolreader.crengine;

public class ImageInfo {
	int width;
	int height;
	int scaledWidth;
	int scaledHeight;
	int x;
	int y;
	int bufWidth;
	int bufHeight;
	int bufDpi;
	int rotation;

	public ImageInfo() {
		
	}
	
	public ImageInfo(ImageInfo v) {
		width = v.width;
		height = v.height;
		scaledWidth = v.scaledWidth;
		scaledHeight = v.scaledHeight;
		x = v.x;
		y = v.y;
		bufWidth = v.bufWidth;
		bufHeight = v.bufHeight;
		rotation = v.rotation;
		bufDpi = v.bufDpi;
	}
	
	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + bufHeight;
		result = prime * result + bufWidth;
		result = prime * result + height;
		result = prime * result + scaledHeight;
		result = prime * result + scaledWidth;
		result = prime * result + width;
		result = prime * result + x;
		result = prime * result + y;
		result = prime * result + rotation;
		result = prime * result + bufDpi;
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
		ImageInfo other = (ImageInfo) obj;
		if (bufHeight != other.bufHeight)
			return false;
		if (bufWidth != other.bufWidth)
			return false;
		if (height != other.height)
			return false;
		if (scaledHeight != other.scaledHeight)
			return false;
		if (scaledWidth != other.scaledWidth)
			return false;
		if (width != other.width)
			return false;
		if (x != other.x)
			return false;
		if (y != other.y)
			return false;
		if (bufDpi != other.bufDpi)
			return false;
		if (rotation != other.rotation)
			return false;
		return true;
	}
	
	
}
