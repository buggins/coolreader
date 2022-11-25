/*
 * CoolReader for Android
 * Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>
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

// document property names
public interface DocProperties {
	String DOC_PROP_AUTHORS = "doc.authors";
	String DOC_PROP_TITLE = "doc.title";
	String DOC_PROP_LANGUAGE = "doc.language";
	String DOC_PROP_DESCRIPTION = "doc.description";
	String DOC_PROP_KEYWORDS = "doc.keywords";
	String DOC_PROP_SERIES_NAME = "doc.series.name";
	String DOC_PROP_SERIES_NUMBER = "doc.series.number";
	String DOC_PROP_ARC_NAME = "doc.archive.name";
	String DOC_PROP_ARC_PATH = "doc.archive.path";
	String DOC_PROP_ARC_SIZE = "doc.archive.size";
	String DOC_PROP_ARC_FILE_COUNT = "doc.archive.file.count";
	String DOC_PROP_FILE_NAME = "doc.file.name";
	String DOC_PROP_FILE_PATH = "doc.file.path";
	String DOC_PROP_FILE_SIZE = "doc.file.size";
	String DOC_PROP_FILE_FORMAT = "doc.file.format";
	String DOC_PROP_FILE_FORMAT_ID = "doc.file.format.id";
	String DOC_PROP_FILE_CRC32 = "doc.file.crc32";
	String DOC_PROP_CODE_BASE = "doc.file.code.base";
	String DOC_PROP_COVER_FILE = "doc.cover.file";
}
