package org.coolreader.crengine;

import org.coolreader.R;

public enum DocumentFormat {
	/// lvtinydom.h: source document formats
	//typedef enum {
	NONE("fb2.css", R.raw.fb2),// doc_format_none,
	FB2("fb2.css", R.raw.fb2), // doc_format_fb2,
	TXT("txt.css", R.raw.txt), // doc_format_txt,
	RTF("rtf.css", R.raw.rtf), // doc_format_rtf,
	EPUB("epub.css", R.raw.epub),// doc_format_epub,
	HTML("htm.css", R.raw.htm),// doc_format_html,
	TXT_BOOKMARK("fb2.css", R.raw.fb2), // doc_format_txt_bookmark, // coolreader TXT format bookmark
	CHM("chm.css", R.raw.chm); //  doc_format_chm,
	    // don't forget update getDocFormatName() when changing this enum
	//} doc_format_t;
	
	public String getCssName()
	{
		return cssFileName;
	}
	
	public int getResourceId()
	{
		return resourceId;
	}
	
	static DocumentFormat byId( int i )
	{
		if ( i>=0 && i<=CHM.ordinal() )
			return values()[i];
		return null;
	}
	
	private DocumentFormat( String cssFileName, int resourceId )
	{
		this.cssFileName = cssFileName;
		this.resourceId = resourceId;
	}
	final private String cssFileName;
	final private int resourceId;
}
