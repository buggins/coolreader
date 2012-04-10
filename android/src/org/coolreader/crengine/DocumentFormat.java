package org.coolreader.crengine;

import org.coolreader.R;

public enum DocumentFormat {
	/// lvtinydom.h: source document formats
	//typedef enum {
	NONE("fb2.css", R.raw.fb2, R.drawable.cr3_browser_book, false, false, 0, 
			new String[] {},
			new String[] {}),// doc_format_none,
	FB2("fb2.css", R.raw.fb2, R.drawable.cr3_browser_book_fb2, true, true, 10, 
			new String[] {".fb2", ".fb2.zip"},
			new String[] {"application/fb2+zip"}), // doc_format_fb2,
	TXT("txt.css", R.raw.txt, R.drawable.cr3_browser_book_txt, false, false, 3, 
			new String[] {".txt", ".tcr", ".pml"},
			new String[] {"text/plain"}), // doc_format_txt,
	RTF("rtf.css", R.raw.rtf, R.drawable.cr3_browser_book_rtf, false, false, 7, 
			new String[] {".rtf"},
			new String[] {}), // doc_format_rtf,
	EPUB("epub.css", R.raw.epub, R.drawable.cr3_browser_book_epub, true, true, 9, 
			new String[] {".epub"},
			new String[] {"application/epub+zip"}),// doc_format_epub,
	HTML("htm.css", R.raw.htm, R.drawable.cr3_browser_book_html, false, false, 8, 
			new String[] {".htm", ".html", ".shtml", ".xhtml"},
			new String[] {"text/html"}),// doc_format_html,
	TXT_BOOKMARK("fb2.css", R.raw.fb2, R.drawable.cr3_browser_book_fb2, false, false, 0, 
			new String[] {".txt.bmk"},
			new String[] {}), // doc_format_txt_bookmark, // coolreader TXT format bookmark
	CHM("chm.css", R.raw.chm, R.drawable.cr3_browser_book_chm, false, false, 6, 
			new String[] {".chm"},
			new String[] {}), //  doc_format_chm,
	DOC("doc.css", R.raw.doc, R.drawable.cr3_browser_book_doc, false, false, 5, 
			new String[] {".doc"},
			new String[] {}), // doc_format_doc,
	PDB("htm.css", R.raw.htm, R.drawable.cr3_browser_book_pdb, false, true, 4, 
			new String[] {".pdb", ".prc", ".mobi", ".azw"},
			new String[] {}); // doc_format_txt/html/...,
    // don't forget update getDocFormatName() when changing this enum
	//} doc_format_t;
	
	public String getCssName()
	{
		return cssFileName;
	}
	
	public int getPriority()
	{
		return priority;
	}
	
	public String[] getExtensions()
	{
		return extensions;
	}
	
	public int getCSSResourceId()
	{
		return cssResourceId;
	}
	
	public int getIconResourceId()
	{
		return iconResourceId;
	}
	
	public String[] getMimeFormats()
	{
		return mimeFormats;
	}
	
	public String getMimeFormat()
	{
		return mimeFormats.length>0 ? mimeFormats[0] : null;
	}
	
	public boolean canParseProperties()
	{
		return canParseProperties;
	}
	
	public boolean canParseCoverpages()
	{
		return canParseCoverpages;
	}
	
	public boolean needCoverPageCaching()
	{
		return this == FB2;
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
	
	public boolean matchMimeType( String type )
	{
		for ( String s : mimeFormats ) {
			if ( type.equals(s) || type.startsWith(s+";"))
				return true;
		}
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
	
	public static DocumentFormat byMimeType( String format )
	{
		if ( format==null )
			return null;
		String s = format.toLowerCase();
		for ( int i=0; i<DocumentFormat.values().length; i++ )
			if ( values()[i].matchMimeType(s))
				return values()[i];
		return null;
	}
	
	private DocumentFormat( String cssFileName, int cssResourceId, int iconResourceId, boolean canParseProperties, boolean canParseCoverpages, int priority, String extensions[], String mimeFormats[] )
	{
		this.cssFileName = cssFileName;
		this.cssResourceId = cssResourceId;
		this.iconResourceId = iconResourceId;
		this.extensions = extensions;
		this.canParseProperties = canParseProperties;
		this.canParseCoverpages = canParseCoverpages;
		this.mimeFormats = mimeFormats;
		this.priority = priority;
	}
	final private String cssFileName;
	final private int cssResourceId;
	final private int iconResourceId;
	final private String[] extensions;
	final boolean canParseProperties;
	final boolean canParseCoverpages;
	final private String[] mimeFormats;
	final private int priority;
}
