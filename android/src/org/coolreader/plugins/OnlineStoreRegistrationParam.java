/*
 * CoolReader for Android
 * Copyright (C) 2012,2014 Vadim Lopatin <coolreader.org@gmail.com>
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

package org.coolreader.plugins;

public class OnlineStoreRegistrationParam {
	public final static String NEW_ACCOUNT_PARAM_LOGIN = "login";
	public final static String NEW_ACCOUNT_PARAM_PASSWORD = "password";
	public final static String NEW_ACCOUNT_PARAM_EMAIL = "email";
	public final static String NEW_ACCOUNT_PARAM_FIRST_NAME = "firstName";
	public final static String NEW_ACCOUNT_PARAM_MIDDLE_NAME = "middleName";
	public final static String NEW_ACCOUNT_PARAM_LAST_NAME = "lastName";
	public final static String NEW_ACCOUNT_PARAM_CITY = "city";
	public final static String NEW_ACCOUNT_PARAM_PHONE = "phone";
	public final static String NEW_ACCOUNT_PARAM_BIRTH_DAY = "birthDay";
	public final static String NEW_ACCOUNT_PARAM_GENDER = "gender";
	public final static String NEW_ACCOUNT_PARAM_SUBSCRIBE = "subscribe";
	
	public final String id;
	public final String name;
	public final String description;
	public final boolean mandatory;
	
	public OnlineStoreRegistrationParam(String id, String name,
			String description, boolean mandatory) {
		super();
		this.id = id;
		this.name = name;
		this.description = description;
		this.mandatory = mandatory;
	}

	
}
