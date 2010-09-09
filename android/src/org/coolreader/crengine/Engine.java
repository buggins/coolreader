package org.coolreader.crengine;

import java.io.IOException;

/**
 * CoolReader Engine class.
 *
 * Only one instance is allowed.
 */
public class Engine {
	
	/**
	 * Initialize CoolReader Engine
	 * @param fontList is array of .ttf font pathnames to load
	 */
	public Engine( String[] fontList ) throws IOException
	{
		if ( initialized )
			throw new IllegalStateException("Already initialized");
		if ( !initInternal( fontList ) )
			throw new IOException("Cannot initialize CREngine JNI");
		initialized = true;
	}

	private native boolean initInternal( String[] fontList );
	private native void uninitInternal();
	private native String[] getFontFaceListInternal();
	
	public String[] getFontFaceList()
	{
		if ( !initialized )
			throw new IllegalStateException("CREngine is not initialized");
		return getFontFaceListInternal();
	}
	
	/**
	 * Uninitialize engine.
	 */
	public void uninit()
	{
		if ( !initialized )
			throw new IllegalStateException("Not initialized");
		uninitInternal();
		initialized = false;
	}
	
	protected void finalize() throws Throwable
	{
		if ( initialized )
			uninit();
	}
	
	static private boolean initialized = false;
	
}
