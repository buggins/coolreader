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
