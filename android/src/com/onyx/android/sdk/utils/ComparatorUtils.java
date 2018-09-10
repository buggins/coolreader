package com.onyx.android.sdk.utils;

import com.onyx.android.sdk.data.SortOrder;

import java.text.Collator;
import java.util.Locale;

public class ComparatorUtils {
	public ComparatorUtils() {
	}

	public static int stringComparator(String lhs, String rhs, SortOrder ascOrder) {
		switch (ascOrder) {
			case Desc:
				return Collator.getInstance(Locale.getDefault()).compare(rhs, lhs);
			default:
				return Collator.getInstance(Locale.getDefault()).compare(lhs, rhs);
		}
	}

	public static int longComparator(long lhs, long rhs, SortOrder ascOrder) {
		switch (ascOrder) {
			case Desc:
				return rhs < lhs ? -1 : (lhs == rhs ? 0 : 1);
			default:
				return lhs < rhs ? -1 : (lhs == rhs ? 0 : 1);
		}
	}

	public static int integerComparator(int lhs, int rhs, SortOrder ascOrder) {
		switch (ascOrder) {
			case Desc:
				return rhs < lhs ? -1 : (lhs == rhs ? 0 : 1);
			default:
				return lhs < rhs ? -1 : (lhs == rhs ? 0 : 1);
		}
	}

	public static int booleanComparator(boolean lhs, boolean rhs, SortOrder ascOrder) {
		switch (ascOrder) {
			case Desc:
				return lhs == rhs ? 0 : (rhs ? 1 : -1);
			default:
				return lhs == rhs ? 0 : (lhs ? 1 : -1);
		}
	}
}
