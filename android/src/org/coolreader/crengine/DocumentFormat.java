package org.coolreader.crengine;

public enum DocumentFormat {
	/// lvtinydom.h: source document formats
	//typedef enum {
	NONE,// doc_format_none,
	FB2, // doc_format_fb2,
	TXT, // doc_format_txt,
	RTF, // doc_format_rtf,
	EPUB,// doc_format_epub,
	HTML,// doc_format_html,
	TXT_BOOKMARK, // doc_format_txt_bookmark, // coolreader TXT format bookmark
	CHM; //  doc_format_chm,
	    // don't forget update getDocFormatName() when changing this enum
	//} doc_format_t;
	DocumentFormat byId( int i )
	{
		if ( i>=0 && i<=CHM.ordinal() )
			return values()[i];
		return null;
	}
}
