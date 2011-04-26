package org.coolreader.crengine;

import org.coolreader.R;

public enum DocumentFormat {
	/// lvtinydom.h: source document formats
	//typedef enum {
	NONE("fb2.css", R.raw.fb2, R.drawable.cr3_browser_book, false, new String[] {}),// doc_format_none,
	FB2("fb2.css", R.raw.fb2, R.drawable.cr3_browser_book_fb2, true, new String[] {".fb2", ".fb2.zip"}), // doc_format_fb2,
	TXT("txt.css", R.raw.txt, R.drawable.cr3_browser_book_txt, false, new String[] {".txt", ".tcr", ".pml"}), // doc_format_txt,
	PDB("htm.css", R.raw.htm, R.drawable.cr3_browser_book_pdb, false, new String[] {".pdb", ".prc", ".mobi", ".azw"}), // doc_format_txt/html/...,
	RTF("rtf.css", R.raw.rtf, R.drawable.cr3_browser_book_rtf, false, new String[] {".rtf"}), // doc_format_rtf,
	EPUB("epub.css", R.raw.epub, R.drawable.cr3_browser_book_epub, true, new String[] {".epub"}),// doc_format_epub,
	HTML("htm.css", R.raw.htm, R.drawable.cr3_browser_book_html, false, new String[] {".htm", ".html", ".shtml", ".xhtml"}),// doc_format_html,
	TXT_BOOKMARK("fb2.css", R.raw.fb2, R.drawable.cr3_browser_book_fb2, false, new String[] {".txt.bmk"}), // doc_format_txt_bookmark, // coolreader TXT format bookmark
	CHM("chm.css", R.raw.chm, R.drawable.cr3_browser_book_chm, false, new String[] {".chm"}), //  doc_format_chm,
	DOC("doc.css", R.raw.doc, R.drawable.cr3_browser_book_doc, false, new String[] {".doc"}); // doc_format_doc,
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
	
	public boolean canParseProperties()
	{
		return canParseProperties;
	}
	
	public static DocumentFormat byId( int i )
	{
		if ( i>=0 && i<DocumentFormat.values().length )
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
		for ( int i=0; i<DocumentFormat.values().length; i++ )
			if ( values()[i].matchExtension(s))
				return values()[i];
		return null;
	}
	
	private DocumentFormat( String cssFileName, int cssResourceId, int iconResourceId, boolean canParseProperties, String extensions[] )
	{
		this.cssFileName = cssFileName;
		this.cssResourceId = cssResourceId;
		this.iconResourceId = iconResourceId;
		this.extensions = extensions;
		this.canParseProperties = canParseProperties;
	}
	final private String cssFileName;
	final private int cssResourceId;
	final private int iconResourceId;
	final private String[] extensions;
	final boolean canParseProperties;
}
