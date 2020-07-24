package com.onyx.android.sdk.utils;

import com.onyx.android.sdk.data.SortOrder;

import java.text.Collator;
import java.util.Locale;

public class ComparatorUtils {
	public ComparatorUtils() {
	}

	public static int stringComparator(String lhs, String rhs, SortOrder ascOrder) {
		if (ascOrder == SortOrder.Desc) {
			return Collator.getInstance(Locale.getDefault()).compare(rhs, lhs);
		}
		return Collator.getInstance(Locale.getDefault()).compare(lhs, rhs);
	}

	public static int longComparator(long lhs, long rhs, SortOrder ascOrder) {
		if (ascOrder == SortOrder.Desc) {
			return Long.compare(rhs, lhs);
		}
		return Long.compare(lhs, rhs);
	}

	public static int integerComparator(int lhs, int rhs, SortOrder ascOrder) {
		if (ascOrder == SortOrder.Desc) {
			return Integer.compare(rhs, lhs);
		}
		return Integer.compare(lhs, rhs);
	}

	public static int booleanComparator(boolean lhs, boolean rhs, SortOrder ascOrder) {
		if (ascOrder == SortOrder.Desc) {
			return Boolean.compare(rhs, lhs);
		}
		return Boolean.compare(lhs, rhs);
	}
}
