package com.onyx.android.sdk.utils;

import android.util.Log;

import com.onyx.android.sdk.data.RefValue;

import java.lang.reflect.Constructor;
import java.lang.reflect.Method;

public class ReflectUtil {
	private static final String TAG = "ReflectUtil";

	private static Object sDummyObject = new Object();

	public static boolean getConstructorSafely(RefValue<Constructor<?>> result, Class<?> cls, Class<?>... parameterTypes) {
		try {
			if (cls == null) {
				return false;
			}
			result.setValue(cls.getConstructor(parameterTypes));
			return true;
		} catch (SecurityException e) {
			Log.w(TAG, e);
		} catch (NoSuchMethodException e) {
			Log.w(TAG, e);
		}
		return false;
	}

	public static Constructor<?> getConstructorSafely(Class<?> cls, Class<?>... parameterTypes) {
		RefValue<Constructor<?>> result = new RefValue<Constructor<?>>();
		if (getConstructorSafely(result, cls, parameterTypes)) {
			return result.getValue();
		}
		return null;
	}

	public static Class<?> classForName(final String name) {
		Class<?> cls = null;
		try {
			cls = Class.forName(name);
		} catch (Exception e) {
			Log.e(TAG, "", e);
		}
		return cls;
	}

	public static boolean getMethodSafely(RefValue<Method> result, Class<?> cls, String name, Class<?>... parameterTypes) {
		try {
			if (cls == null) {
				return false;
			}
			result.setValue(cls.getMethod(name, parameterTypes));
			return true;
		} catch (SecurityException e) {
			Log.w(TAG, e);
		} catch (NoSuchMethodException e) {
			Log.w(TAG, e);
		}
		return false;
	}

	public static Method getMethodSafely(Class<?> cls, String name, Class<?>... parameterTypes) {
		RefValue<Method> result = new RefValue<Method>();
		if (getMethodSafely(result, cls, name, parameterTypes)) {
			return result.getValue();
		}
		return null;
	}

	public static boolean getStaticIntFieldSafely(RefValue<Integer> result, Class<?> cls, String name) {
		try {
			if (cls == null) {
				return false;
			}
			int n = cls.getField(name).getInt(null);
			result.setValue(Integer.valueOf(n));
			return true;
		} catch (IllegalArgumentException e) {
			Log.w(TAG, e);
		} catch (SecurityException e) {
			Log.w(TAG, e);
		} catch (IllegalAccessException e) {
			Log.w(TAG, e);
		} catch (NoSuchFieldException e) {
			Log.w(TAG, e);
		}
		return false;
	}

	public static int getStaticIntFieldSafely(Class<?> cls, String name) {
		RefValue<Integer> result = new RefValue<Integer>();
		if (getStaticIntFieldSafely(result, cls, name)) {
			return result.getValue();
		}
		return 0;
	}

	public static boolean getStaticFieldSafely(RefValue<Object> result, Class<?> cls, String name) {
		try {
			if (cls == null) {
				return false;
			}
			result.setValue(cls.getField(name).get(null));
			return true;
		} catch (IllegalArgumentException e) {
			Log.w(TAG, e);
		} catch (SecurityException e) {
			Log.w(TAG, e);
		} catch (IllegalAccessException e) {
			Log.w(TAG, e);
		} catch (NoSuchFieldException e) {
			Log.w(TAG, e);
		}
		return false;
	}

	public static Object getStaticFieldSafely(Class<?> cls, String name) {
		RefValue<Object> result = new RefValue<Object>();
		if (getStaticFieldSafely(result, cls, name)) {
			return result.getValue();
		}
		return null;
	}

	public static boolean constructObjectSafely(RefValue<Object> result, Constructor<?> constructor, Object... args) {
		if (constructor == null) {
			return false;
		}
		try {
			result.setValue(constructor.newInstance(args));
			return true;
		} catch (Throwable tr) {
			Log.w(TAG, "", tr);
		}
		return false;
	}

	public static Object newInstance(Constructor<?> constructor, Object... args) {
		RefValue<Object> result = new RefValue<Object>();
		if (constructObjectSafely(result, constructor, args)) {
			return result.getValue();
		}
		return null;
	}

	/**
	 * If this method is static, the receiver argument is ignored.
	 *
	 * @param result
	 * @param method
	 * @param receiver
	 * @param args
	 * @return
	 */
	public static boolean invokeMethodSafely(RefValue<Object> result, Method method, Object receiver, Object... args) {
		if (method == null) {
			return false;
		}
		try {
			result.setValue(method.invoke(receiver, args));
			return true;
		} catch (Throwable tr) {
			Log.w(TAG, tr);
		}
		return false;
	}

	/**
	 * If this method is static, the receiver argument is ignored.
	 *
	 * @param method
	 * @param receiver
	 * @param args
	 * @return
	 */
	public static Object invokeMethodSafely(Method method, Object receiver, Object... args) {
		RefValue<Object> result = new RefValue<Object>();
		if (invokeMethodSafely(result, method, receiver, args)) {
			if (result.getValue() != null) {
				return result.getValue();
			} else {
				return sDummyObject;
			}
		}
		return null;
	}

	public static Method getDeclaredMethodSafely(Class<?> cls, String name, Class<?>... parameterTypes) {
		RefValue<Method> result = new RefValue<Method>();
		if (getDeclaredMethod(result, cls, name, parameterTypes)) {
			return (Method) result.getValue();
		}
		return null;
	}

	public static boolean getDeclaredMethod(RefValue<Method> result, Class<?> cls, String name, Class<?>... parameterTypes) {
		try {
			if (cls == null) {
				return false;
			}
			Method method = cls.getDeclaredMethod(name, parameterTypes);
			method.setAccessible(true);
			result.setValue(method);
			return true;
		} catch (SecurityException localSecurityException1) {
		} catch (NoSuchMethodException localNoSuchMethodException1) {
		}
		return false;
	}
}
