/*
 * CoolReader for Android
 * Copyright (C) 2010-2012 Vadim Lopatin <coolreader.org@gmail.com>
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

public class PositionProperties {
	public int x;
	public int y;
	public int fullHeight;
	public int pageHeight;
	public int pageWidth;
	public int pageNumber;
	public int pageCount;
	public int pageMode; // 1, 2 for page mode, 0 for scroll mode
	public int charCount;
	public int imageCount;
	public String pageText;
	
	
	public PositionProperties(PositionProperties v) {
		x = v.x;
		y = v.y;
		fullHeight = v.fullHeight;
		pageHeight = v.pageHeight;
		pageWidth = v.pageWidth;
		pageNumber = v.pageNumber;
		pageCount = v.pageCount;
		pageMode = v.pageMode;
		charCount = v.charCount;
		imageCount = v.imageCount;
	}

	public PositionProperties() {
		
	}
	
	public int getPercent() {
		if (fullHeight - pageHeight <= 0)
			return 0;
		int p = 10000 * y / (fullHeight - pageHeight);
		if (p < 0)
			p = 0;
		if (p > 10000)
			p = 10000;
		return p;
	}
	
	@Override
	public Object clone() throws CloneNotSupportedException {
		return super.clone();
	}

	public boolean canMoveToNextPage() {
		if (pageMode == 0) {
			return fullHeight > pageHeight && y < fullHeight - pageHeight;
		}
		return pageNumber < pageCount - pageMode;
	}
	
	@Override
	public String toString() {
		return "PositionProperties [pageMode=" + pageMode + ", pageNumber="
				+ pageNumber + ", pageCount=" + pageCount + ", x=" + x + ", y="
				+ y + ", pageHeight=" + pageHeight + ", pageWidth=" + pageWidth
				+ ", fullHeight=" + fullHeight + "]";
	}
	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + fullHeight;
		result = prime * result + pageCount;
		result = prime * result + pageHeight;
		result = prime * result + pageMode;
		result = prime * result + pageNumber;
		result = prime * result + pageWidth;
		result = prime * result + x;
		result = prime * result + y;
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
		PositionProperties other = (PositionProperties) obj;
		if (fullHeight != other.fullHeight)
			return false;
		if (pageCount != other.pageCount)
			return false;
		if (pageHeight != other.pageHeight)
			return false;
		if (pageMode != other.pageMode)
			return false;
		if (pageNumber != other.pageNumber)
			return false;
		if (pageWidth != other.pageWidth)
			return false;
		if (x != other.x)
			return false;
		if (y != other.y)
			return false;
		return true;
	}
	
	
}
