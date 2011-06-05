package org.coolreader.crengine;

public interface Logger {
	public void i(String msg); 
	public void i(String msg, Exception e); 
	public void w(String msg); 
	public void w(String msg, Exception e); 
	public void e(String msg); 
	public void e(String msg, Exception e); 
	public void d(String msg); 
	public void d(String msg, Exception e); 
	public void v(String msg); 
	public void v(String msg, Exception e);
	public void setLevel( int level );
}
