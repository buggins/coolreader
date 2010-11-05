package org.coolreader.crengine;

public class Properties extends java.util.Properties {
	public Properties()
	{
		super();
	}
	public Properties(java.util.Properties props)
	{
		super(props);
	}
	private static int revBytes( int color )
	{
		return color & 0xFFFFFF;
		//return ((color & 0xFF)<<16)|((color & 0xFF00)<<0)|((color & 0xFF0000)>>16);
	}
	public void setColor( String key, int color )
	{
		color &= 0xFFFFFF;
		color = revBytes(color);
		String value = Integer.toHexString(color);
		while ( value.length()<6 )
			value = "0" + value;
		value = "0x" + value;
		setProperty(key, value);
	}
	public int getColor( String key, int defColor )
	{
		defColor = revBytes(defColor);
		String value = getProperty(key);
		try {
			if ( value!=null && value.length()>2 && value.startsWith("0x") ) {
				int cl = Integer.parseInt(value.substring(2), 16);
				cl = revBytes(cl);
				return cl | 0xFF000000;
			}
		} catch ( Exception e ) {
		}
		return revBytes(defColor) | 0xFF000000;
	}
	public void setBool( String key, boolean value )
	{
		setProperty( key, value ? "1" : "0" );
	}
	public boolean getBool( String key, boolean defaultValue )
	{
		String value = getProperty(key);
		if ( value==null )
			return defaultValue;
		if ( value.equals("1") || value.equals("true") || value.equals("yes") )
			return true;
		if ( value.equals("0") || value.equals("false") || value.equals("no") )
			return false;
		return defaultValue;
	}
	public void applyDefault( String prop, String defValue )
	{
		if ( getProperty(prop)==null )
			setProperty(prop, defValue);
	}
}
