package org.eink_onyx_reflections;

import java.lang.reflect.Method;

public final class Utils {

	public static Class<?> getClassForName(String name) {
		Class<?> clazz = null;
		try {
			clazz = Class.forName(name);
		} catch (ClassNotFoundException ignored) {
		}
		return clazz;
	}

	public static Method getMethod(Class<?> clazz, String name, Class<?>... parameters) {
		if (null == clazz)
			return null;
		Method method = null;
		try {
			method = clazz.getMethod(name, parameters);
		} catch (NoSuchMethodException ignored) {
		}
		return method;
	}

	public static Method getDeclaredMethod(Class<?> clazz, String name, Class<?>... parameters) {
		if (null == clazz)
			return null;
		Method method = null;
		try {
			method = clazz.getDeclaredMethod(name, parameters);
			method.setAccessible(true);
		} catch (NoSuchMethodException ignored) {
		}
		return method;
	}

	public static int getStaticIntField(Class<?> clazz, String field) {
		if (clazz == null) {
			return 0;
		}
		try {
			return clazz.getField(field).getInt(null);
		} catch (Exception ignored) {
		}
		return 0;
	}

	public static Object invokeMethod(Method method, Object receiver, Object... args) {
		if (null == method) {
			return null;
		}
		try {
			return method.invoke(receiver, args);
		} catch (Exception ignored) {
		}
		return null;
	}
}
