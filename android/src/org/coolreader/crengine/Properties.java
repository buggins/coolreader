package org.coolreader.crengine;

import java.util.HashSet;
import java.util.Map;
import java.util.Set;

@SuppressWarnings("serial")
public class Properties extends java.util.Properties {
	public Properties() {
		super();
	}

	synchronized public void setAll(java.util.Properties props) {
		for (Map.Entry<Object, Object> entry : props.entrySet()) {
			setProperty((String) entry.getKey(), (String) entry.getValue());
		}
	}

	public Properties(java.util.Properties props) {
		synchronized(props) {
			setAll(props);
		}
	}

	private static int revBytes(int color) {
		return color & 0xFFFFFF;
		// return ((color & 0xFF)<<16)|((color & 0xFF00)<<0)|((color &
		// 0xFF0000)>>16);
	}

	public void setColor(String key, int color) {
		color &= 0xFFFFFF;
		color = revBytes(color);
		String value = Integer.toHexString(color);
		while (value.length() < 6)
			value = "0" + value;
		value = "0x" + value;
		setProperty(key, value);
	}

	public int getColor(String key, int defColor) {
		defColor = revBytes(defColor);
		String value = getProperty(key);
		try {
			if (value != null && value.length() > 2 && value.startsWith("0x")) {
				int cl = Integer.parseInt(value.substring(2), 16);
				cl = revBytes(cl);
				return cl | 0xFF000000;
			}
			if (value != null && value.length() > 1 && value.startsWith("#")) {
				int cl = Integer.parseInt(value.substring(1), 16);
				cl = revBytes(cl);
				return cl | 0xFF000000;
			}
		} catch (Exception e) {
		}
		return revBytes(defColor) | 0xFF000000;
	}

	public void setInt(String key, int v) {
		String value = String.valueOf(v);
		setProperty(key, value);
	}

	public int getInt(String key, int def) {
		String value = getProperty(key);
		int res = def;
		try {
			if (value != null)
				res = Integer.valueOf(value);
		} catch (Exception e) {
		}
		return res;
	}

	public void setBool(String key, boolean value) {
		setProperty(key, value ? "1" : "0");
	}

	public boolean getBool(String key, boolean defaultValue) {
		String value = getProperty(key);
		if (value == null)
			return defaultValue;
		if (value.equals("1") || value.equals("true") || value.equals("yes"))
			return true;
		if (value.equals("0") || value.equals("false") || value.equals("no"))
			return false;
		return defaultValue;
	}

	public void applyDefault(String prop, String defValue) {
		if (getProperty(prop) == null)
			setProperty(prop, defValue);
	}

	public void applyDefault(String prop, int defValue) {
		if (getProperty(prop) == null)
			setInt(prop, defValue);
	}

	public static boolean eq(Object obj1, Object obj2) {
		if (obj1 == null && obj2 == null)
			return true;
		if (obj1 == null || obj2 == null)
			return false;
		return obj1.equals(obj2);
	}

	synchronized public Properties diff(Properties oldValue) {
		Properties res = new Properties();
		for (Map.Entry<Object, Object> entry : entrySet()) {
			if (!oldValue.containsKey(entry.getKey())
					|| !eq(entry.getValue(), oldValue.get(entry.getKey()))) {
				res.setProperty((String) entry.getKey(),
						(String) entry.getValue());
			}
		}
		return res;
	}

	@Override
	synchronized public String getProperty(String name, String defaultValue) {
		return super.getProperty(name, defaultValue);
	}

	@Override
	synchronized public String getProperty(String name) {
		return super.getProperty(name);
	}

	@Override
	synchronized public Object setProperty(String name, String value) {
		return super.setProperty(name, value);
	}

	@Override
	public synchronized Object remove(Object key) {
		return super.remove(key);
	}

	@Override
	public synchronized Set<java.util.Map.Entry<Object, Object>> entrySet() {
		Set<java.util.Map.Entry<Object, Object>> res = new HashSet<java.util.Map.Entry<Object, Object>>();
		res.addAll(super.entrySet());
		return res;
	}
}
