/*
 * CoolReader for Android
 * Copyright (C) 2013 Vadim Lopatin <coolreader.org@gmail.com>
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

public interface OPDSConst {
	/*
	 * This is a list of OPDS catalogs which may have problems with copyright.
	 * Added by request of LitRes.
	 * http://tr.reddit.com/r/Android/comments/17rtk0/moon_reader_was_blocked_from_play_market_by/
	 */
	final static int BLACK_LIST_MODE_NONE = 0;
	final static int BLACK_LIST_MODE_WARN = 1;
	final static int BLACK_LIST_MODE_FORCE = 2;
	static int BLACK_LIST_MODE = BLACK_LIST_MODE_NONE;
	final static String[] BLACK_LIST = {
		"http://109.163.230.117/opds",
		"http://213.5.65.159/opds",
		"http://flibusta.net/opds",
		"http://dimonvideo.ru/lib.xml",
		"http://lib.rus.ec/opds",
		"http://books.vnuki.org/opds.xml",
		"http://coollib.net/opds",
		"http://iflip.ru/xml/",
		"http://www.zone4iphone.ru/catalog.php" 
	};
}
