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

import org.coolreader.crengine.Bookmark;
import org.coolreader.crengine.FileInfo;
import org.xml.sax.Attributes;
import org.xml.sax.ContentHandler;
import org.xml.sax.Locator;
import org.xml.sax.SAXException;

import java.util.ArrayList;
import java.util.List;
import java.util.Stack;

// Simple content handler for XMLReader to parse bookmarks data on cloud
class BookmarksContentHandler implements ContentHandler {

	enum ItemType {
		Unset,
		Root,
		FileInfo,
		Bookmarks,
		Bookmark
	}

	private int m_version = -1;
	private FileInfo m_fileInfo;
	private List<Bookmark> m_bookmarks;
	private Bookmark m_bookmark;

	private ItemType m_itemType = ItemType.Unset;
	private Stack<String> m_tagStack = new Stack<>();
	private String m_text;

	public int getVersion() {
		return m_version;
	}
	public FileInfo getFileInfo() {
		return m_fileInfo;
	}
	public List<Bookmark> getBookmarks() {
		return m_bookmarks;
	}

	@Override
	public void setDocumentLocator(Locator locator) {
	}

	@Override
	public void startDocument() throws SAXException {
		m_itemType = ItemType.Unset;
		m_tagStack = new Stack<>();
	}

	@Override
	public void endDocument() throws SAXException {
	}

	@Override
	public void startPrefixMapping(String prefix, String uri) throws SAXException {
	}

	@Override
	public void endPrefixMapping(String prefix) throws SAXException {
	}

	@Override
	public void startElement(String uri, String localName, String qName, Attributes attrs) throws SAXException {
		if (null == qName)
			return;
		String parent = "";
		if (!m_tagStack.empty())
			parent = m_tagStack.peek();
		m_tagStack.push(qName);
		switch (parent) {
			case "":
				switch (qName) {
					case "root":
						m_version = atoi(attrs.getValue("version"));
						break;
				}
				break;
			case "root":
				switch (qName) {
					case "fileinfo":
						m_itemType = ItemType.FileInfo;
						m_fileInfo = new FileInfo();
						break;
					case "bookmarks":
						m_itemType = ItemType.Bookmarks;
						m_bookmarks = new ArrayList<>();
						break;
				}
				break;
			case "bookmarks":
				switch (qName) {
					case "bookmark":
						m_itemType = ItemType.Bookmark;
						m_bookmark = new Bookmark();
						m_bookmark.setId(atol(attrs.getValue("id")));
						m_bookmark.setType(atoi(attrs.getValue("type")));
						m_bookmark.setPercent(atoi(attrs.getValue("percent")));
						m_bookmark.setShortcut(atoi(attrs.getValue("shortcut")));
				}
				break;
		}
		m_text = "";
	}

	@Override
	public void endElement(String uri, String localName, String qName) throws SAXException {
		if (null == qName)
			return;
		String tag = "";
		if (!m_tagStack.empty())
			tag = m_tagStack.pop();
		if (tag != qName) {
			throw new SAXException("end element '" + qName + "' not equal to start element '" + tag + "'");
		}
		switch (tag) {
			case "fileinfo":
			case "bookmarks":
				m_itemType = ItemType.Root;
				break;
			case "bookmark":
				m_itemType = ItemType.Bookmarks;
		}

		switch (m_itemType) {
			case FileInfo:
				if (null == m_fileInfo)
					break;
				switch (tag) {
					case "filename":
						m_fileInfo.filename = m_text;
						break;
					case "authors":
						m_fileInfo.authors = m_text;
						break;
					case "title":
						m_fileInfo.title = m_text;
						break;
					case "series":
						m_fileInfo.series = m_text;
						break;
					case "seriesNumber":
						m_fileInfo.seriesNumber = atoi(m_text);
						break;
					case "size":
						m_fileInfo.size = atoi(m_text);
						break;
					case "crc32":
						m_fileInfo.crc32 = atoi(m_text);
						break;
				}
				break;
			case Bookmark:
				if (null == m_bookmark)
					break;
				switch (tag) {
					case "startpos":
						m_bookmark.setStartPos(m_text);
						break;
					case "endpos":
						m_bookmark.setEndPos(m_text);
						break;
					case "title":
						m_bookmark.setTitleText(m_text);
						break;
					case "pos":
						m_bookmark.setPosText(m_text);
						break;
					case "comment":
						m_bookmark.setCommentText(m_text);
						break;
					case "timestamp":
						m_bookmark.setTimeStamp(atol(m_text));
						break;
					case "elapsed":
						m_bookmark.setTimeElapsed(atol(m_text));
						break;
				}
				break;
			case Bookmarks:
				if (null == m_bookmarks)
					break;
				switch (tag) {
					case "bookmark":
						m_bookmarks.add(m_bookmark);
						m_bookmark = null;
						break;
				}
				break;
		}
	}

	@Override
	public void characters(char[] ch, int start, int length) throws SAXException {
		StringBuilder stringBuilder = new StringBuilder();
		for (int i = 0; i < length; i++)
			stringBuilder.append(ch[start + i]);
		m_text += stringBuilder.toString();
	}

	@Override
	public void ignorableWhitespace(char[] ch, int start, int length) throws SAXException {
	}

	@Override
	public void processingInstruction(String target, String data) throws SAXException {
	}

	@Override
	public void skippedEntity(String name) throws SAXException {
	}

	private static long atol(String s) {
		long res;
		try {
			res = Long.parseLong(s);
		} catch (NumberFormatException e) {
			res = 0;
		}
		return res;
	}

	private static int atoi(String s) {
		int res;
		try {
			res = Integer.parseInt(s);
		} catch (NumberFormatException e) {
			res = 0;
		}
		return res;
	}

}
