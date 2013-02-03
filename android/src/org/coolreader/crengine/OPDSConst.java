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
