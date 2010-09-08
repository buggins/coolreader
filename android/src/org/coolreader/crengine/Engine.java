package org.coolreader.crengine;

/**
 * CoolReader Engine class.
 *
 * Only one instance is allowed.
 */
public class Engine {
	
	/**
	 * Initialize CoolReader Engine
	 * @param fontList is array of .ttf font pathnames to load
	 * @param hyphDir is directory containing hyphenation data
	 */
	public Engine( String[] fontList, String hyphDir )
	{
		if ( initialized )
			throw new IllegalStateException("Already initialized");
		initialized = true;
	}

	/**
	 * Uninitialize engine.
	 */
	public void uninit()
	{
		if ( !initialized )
			throw new IllegalStateException("Not initialized");
		initialized = false;
	}
	
	static private boolean initialized = false;
	
	protected void finalize() throws Throwable
	{
		if ( initialized )
			uninit();
	}
	
}
