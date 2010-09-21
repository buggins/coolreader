package org.coolreader.crengine;

import org.coolreader.R;

public enum DocumentFormat {
	/// lvtinydom.h: source document formats
	//typedef enum {
	NONE("fb2.css", R.raw.fb2, R.drawable.cr3_browser_book, new String[] {}),// doc_format_none,
	FB2("fb2.css", R.raw.fb2, R.drawable.cr3_browser_book_fb2, new String[] {".fb2", ".fb2.zip"}), // doc_format_fb2,
	TXT("txt.css", R.raw.txt, R.drawable.cr3_browser_book_txt, new String[] {".txt", ".tcr"}), // doc_format_txt,
	RTF("rtf.css", R.raw.rtf, R.drawable.cr3_browser_book_rtf, new String[] {".rtf"}), // doc_format_rtf,
	EPUB("epub.css", R.raw.epub, R.drawable.cr3_browser_book_epub, new String[] {".epub"}),// doc_format_epub,
	HTML("htm.css", R.raw.htm, R.drawable.cr3_browser_book_html, new String[] {".htm", ".html", ".shtml", ".xhtml"}),// doc_format_html,
	TXT_BOOKMARK("fb2.css", R.raw.fb2, R.drawable.cr3_browser_book_fb2, new String[] {".txt.bmk"}), // doc_format_txt_bookmark, // coolreader TXT format bookmark
	CHM("chm.css", R.raw.chm, R.drawable.cr3_browser_book_chm, new String[] {".chm"}); //  doc_format_chm,
	    // don't forget update getDocFormatName() when changing this enum
	//} doc_format_t;
	
	public String getCssName()
	{
		return cssFileName;
	}
	
	public int getCSSResourceId()
	{
		return cssResourceId;
	}
	
	public int getIconResourceId()
	{
		return iconResourceId;
	}
	
	public static DocumentFormat byId( int i )
	{
		if ( i>=0 && i<=CHM.ordinal() )
			return values()[i];
		return null;
	}
	
	public boolean matchExtension( String filename )
	{
		for ( String ext : extensions )
			if ( filename.endsWith(ext) )
				return true;
		return false;
	}
	
	public static DocumentFormat byExtension( String filename )
	{
		String s = filename.toLowerCase();
		for ( int i=0; i<=CHM.ordinal(); i++ )
			if ( values()[i].matchExtension(s))
				return values()[i];
		return null;
	}
	
	private DocumentFormat( String cssFileName, int cssResourceId, int iconResourceId, String extensions[] )
	{
		this.cssFileName = cssFileName;
		this.cssResourceId = cssResourceId;
		this.iconResourceId = iconResourceId;
		this.extensions = extensions;
	}
	final private String cssFileName;
	final private int cssResourceId;
	final private int iconResourceId;
	final private String[] extensions;
}
