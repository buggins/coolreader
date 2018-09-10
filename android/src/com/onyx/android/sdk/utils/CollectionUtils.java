package com.onyx.android.sdk.utils;

// Commented function "public static boolean isNullOrEmpty(GAdapter adapter)"
// to decrease dependencies.

import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class CollectionUtils {
	public CollectionUtils() {
	}

	public static boolean isNullOrEmpty(Collection list) {
		return list == null || list.size() <= 0;
	}

	public static boolean isNullOrEmpty(Map map) {
		return map == null || map.size() <= 0;
	}

	/*
	public static boolean isNullOrEmpty(GAdapter adapter) {
		return adapter == null || isNullOrEmpty((Collection)adapter.getList());
	}
	*/

	public static boolean contains(Set<String> set, String value) {
		return set != null && set.size() > 0 ? set.contains(value) : true;
	}

	public static boolean contains(Set<String> source, Collection<String> target) {
		if (source == null && target == null) {
			return true;
		} else if (source == null || target == null) {
			return false;
		} else {
			Iterator it = target.iterator();
			String item;
			do {
				if (!it.hasNext()) {
					return false;
				}
				item = (String) it.next();
			} while (!source.contains(item));
			return true;
		}
	}

	public static boolean equals(Set firstSet, Set secondSet) {
		return firstSet != null && secondSet != null ? firstSet.equals(secondSet) : false;
	}

	public static boolean contains(List<String> list, String string) {
		if (list == null) {
			return true;
		} else {
			return list.contains(string);
		}
	}

	public static int getSize(Collection collection) {
		return collection == null ? 0 : collection.size();
	}

	public static boolean safelyContains(Set<String> set, String string) {
		return set != null && set.contains(string);
	}

	public static boolean safelyContains(List<String> list, String string) {
		if (list == null) {
			return true;
		} else {
			return list.contains(string);
		}
	}

	public static boolean safelyReverseContains(List<String> list, String string) {
		if (!StringUtils.isNullOrEmpty(string) && !isNullOrEmpty((Collection) list)) {
			Iterator it = list.iterator();

			String item;
			do {
				if (!it.hasNext()) {
					return false;
				}

				item = (String) it.next();
			} while (!string.contains(item));
			return true;
		} else {
			return false;
		}
	}

	public static void diff(Collection<String> origin, Collection<String> target, Collection<String> diff) {
		Iterator it = target.iterator();
		while (it.hasNext()) {
			String var4 = (String) it.next();
			if (!origin.contains(var4)) {
				diff.add(var4);
			}
		}
	}

	public static <T> void safeAddAll(Collection<T> originList, Collection<T> targetList) {
		if (originList != null && !isNullOrEmpty(targetList)) {
			originList.addAll(targetList);
		}
	}

	public static <T> void safeAddAll(Collection<T> originList, Collection<T> targetList, boolean clear) {
		if (originList != null) {
			if (clear) {
				originList.clear();
			}
			safeAddAll(originList, targetList);
		}
	}

	public static void clear(Collection list) {
		if (list != null) {
			list.clear();
		}

	}

	public static <T> void ensureAddAll(Collection<T> originList, Collection<T> targetList) {
		if (originList != null && !isNullOrEmpty(targetList)) {
			originList.addAll(targetList);
		}
	}
}
