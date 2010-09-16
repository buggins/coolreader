package org.coolreader.crengine;

public enum DocumentFormat {
	/// lvtinydom.h: source document formats
	//typedef enum {
	NONE("fb2.css"),// doc_format_none,
	FB2("fb2.css"), // doc_format_fb2,
	TXT("txt.css"), // doc_format_txt,
	RTF("rtf.css"), // doc_format_rtf,
	EPUB("epub.css"),// doc_format_epub,
	HTML("html.css"),// doc_format_html,
	TXT_BOOKMARK("fb2.css"), // doc_format_txt_bookmark, // coolreader TXT format bookmark
	CHM("chm.css"); //  doc_format_chm,
	    // don't forget update getDocFormatName() when changing this enum
	//} doc_format_t;
	
	public String getCssName()
	{
		return cssFileName;
	}
	
	static DocumentFormat byId( int i )
	{
		if ( i>=0 && i<=CHM.ordinal() )
			return values()[i];
		return null;
	}
	
	private DocumentFormat( String cssFileName )
	{
		this.cssFileName = cssFileName;
	}
	private String cssFileName;
}
