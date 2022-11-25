/*
 * CoolReader for Android
 * Copyright (C) 2011 Vadim Lopatin <coolreader.org@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

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
