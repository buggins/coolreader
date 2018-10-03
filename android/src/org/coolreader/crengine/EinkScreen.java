package org.coolreader.crengine;

import android.util.Log;
import android.view.View;

import com.onyx.android.sdk.api.device.epd.EpdController;
import com.onyx.android.sdk.api.device.epd.UpdateMode;

import org.coolreader.CoolReader;

public class EinkScreen {
	private static final String TAG = "EinkScreen";
	/// variables
	private static int mUpdateMode = -1;
	// 0 - CLEAR_ALL, set only for old_mode == 2
	// 1 - ONESHOT, always set in prepare
	// 2 - ACTIVE, set in prepare
	private static int mUpdateInterval;
	private static int mRefreshNumber = -1;
	private static boolean mIsSleep = false;
	private static boolean mIsSupportRegal = false;
	private static boolean mInFastMode = false;
	private static boolean mInA2Mode = false;
	// constants
	public final static int CMODE_CLEAR = 0;
	public final static int CMODE_ONESHOT = 1;
	public final static int CMODE_ACTIVE = 2;

	public static void Refresh() {
		mRefreshNumber = -1;
	}

	public static void PrepareController(View view, boolean isPartially) {
		if (DeviceInfo.EINK_NOOK || DeviceInfo.EINK_TOLINO) {
			//System.err.println("Sleep = " + isPartially);
			if (isPartially || mIsSleep != isPartially) {
				SleepController(isPartially, view);
//				if (isPartially) 
					return;
			}
			if (mRefreshNumber == -1) {
				switch (mUpdateMode) {
					case CMODE_CLEAR:
						SetMode(view, mUpdateMode);
						break;
					case CMODE_ACTIVE:
						if (mUpdateInterval == 0) {
							SetMode(view, mUpdateMode);
						}
						break;
				}
				mRefreshNumber = 0;
				return;
			}
			if (mUpdateMode == CMODE_CLEAR) {
				SetMode(view, CMODE_CLEAR);
				return;
			}
			if (mUpdateInterval > 0 || mUpdateMode == CMODE_ONESHOT) {
				if (mRefreshNumber == 0 || (mUpdateMode == CMODE_ONESHOT && mRefreshNumber < mUpdateInterval)) {
					switch (mUpdateMode) {
						case CMODE_ACTIVE:
							SetMode(view, CMODE_ACTIVE);
							break;
						case CMODE_ONESHOT:
							SetMode(view, CMODE_ONESHOT);
							break;
					}
				} else if (mUpdateInterval <= mRefreshNumber) {
					SetMode(view, CMODE_CLEAR);
					mRefreshNumber = -1;
				}
				if (mUpdateInterval > 0) {
					mRefreshNumber++;
				}
			}
			/*
			if (mUpdateMode == 1 && mUpdateInterval != 0) {
				if (mRefreshNumber == 0) {
					// быстрый режим, один раз устанавливается
					N2EpdController.setMode(N2EpdController.REGION_APP_3,
											N2EpdController.WAVE_GL16,
											N2EpdController.MODE_ACTIVE, view); // why not MODE_ACTIVE_ALL?
				} else if (mUpdateInterval == mRefreshNumber) {
					// одно качественное обновление для быстрого режима
					N2EpdController.setMode(N2EpdController.REGION_APP_3,
											N2EpdController.WAVE_GU,
											N2EpdController.MODE_CLEAR_ALL, view);
					mRefreshNumber = -1;
				}
				mRefreshNumber ++;
			}
			*/
		} else if (DeviceInfo.EINK_ONYX) {
			if (mRefreshNumber == -1) {
				mRefreshNumber = 0;
				EpdController.setViewDefaultUpdateMode(view, UpdateMode.GC);
				return;
			}
			if (mUpdateInterval > 0) {
				mRefreshNumber++;
				if (mRefreshNumber >= mUpdateInterval) {
					mRefreshNumber = 0;
					EpdController.setViewDefaultUpdateMode(view, UpdateMode.GC);
				}
			}
		}
	}

	public static void UpdateController(View view, boolean isPartially) {
		if (DeviceInfo.EINK_ONYX) {
			if (mRefreshNumber > 0 || mUpdateInterval == 0) {
				switch (mUpdateMode) {
					case CMODE_CLEAR:
						EpdController.setViewDefaultUpdateMode(view, mIsSupportRegal ? UpdateMode.REGAL : UpdateMode.GU);
						break;
					case CMODE_ONESHOT:
						EpdController.setViewDefaultUpdateMode(view, UpdateMode.DU);
						break;
					default:
				}
			}
		}
	}

	public static void ResetController(int mode, int updateInterval, View view) {
		mUpdateInterval = updateInterval;
		ResetController(mode, view);
	}

	public static void ResetController(int mode, View view) {
		if (DeviceInfo.EINK_NOOK || DeviceInfo.EINK_TOLINO) {
			System.err.println("+++ResetController " + mode);
			switch (mode) {
				case CMODE_CLEAR:
					if (mUpdateMode == CMODE_ACTIVE) {
						mRefreshNumber = -1;
					} else {
						mRefreshNumber = 0;
					}
					break;
				case CMODE_ONESHOT:
					mRefreshNumber = 0;
					break;
				default:
					mRefreshNumber = -1;
			}
		} else if (DeviceInfo.EINK_ONYX) {
			mIsSupportRegal = EpdController.supportRegal();
			mRefreshNumber = 0;
			if (mUpdateInterval == 0)
				EpdController.setViewDefaultUpdateMode(view, UpdateMode.GC);
			switch (mode) {
				case CMODE_CLEAR:			// Quality
					if (mInA2Mode) {
						EpdController.disableA2ForSpecificView(view);
						mInA2Mode = false;
					}
					if (mInFastMode) {
						EpdController.applyApplicationFastMode(CoolReader.class.getSimpleName(), false, true);
						mInFastMode = false;
					}
					break;
				case CMODE_ONESHOT:			// Fast
					if (mInA2Mode) {
						EpdController.disableA2ForSpecificView(view);
						mInA2Mode = false;
					}
					// Enable fast mode (not implemented on RK3026, not tested)
					if (!mInFastMode) {
						EpdController.applyApplicationFastMode(CoolReader.class.getSimpleName(), true, true);
						mInFastMode = true;
					}
					break;
				case CMODE_ACTIVE:			// Fast 2
					if (mInFastMode) {
						EpdController.applyApplicationFastMode(CoolReader.class.getSimpleName(), false, true);
						mInFastMode = false;
					}
					if (!mInA2Mode) {
						EpdController.enableA2ForSpecificView(view);
						mInA2Mode = true;
					}
					break;
			}
			Log.d(TAG, "EinkScreen: Regal is " + (mIsSupportRegal ? "" : "NOT ") + "supported");
		}
		mUpdateMode = mode;
	}

	public static void ResetController(View view) {
		if (DeviceInfo.EINK_NOOK || DeviceInfo.EINK_TOLINO) {
			if (mUpdateInterval != CMODE_CLEAR) {
				System.err.println("+++Soft reset Controller ");
				SetMode(view, CMODE_CLEAR);
				mRefreshNumber = -1;
			}
		}
	}

	private static void SleepController(boolean toSleep, View view) {
		if (DeviceInfo.EINK_NOOK || DeviceInfo.EINK_TOLINO) {
			if (toSleep != mIsSleep) {
				System.err.println("+++SleepController " + toSleep);
				mIsSleep = toSleep;
				if (mIsSleep) {
					switch (mUpdateMode) {
						case CMODE_CLEAR:
							break;
						case CMODE_ONESHOT:
							break;
						case CMODE_ACTIVE:
							SetMode(view, CMODE_CLEAR);
							mRefreshNumber = -1;
					}
				} else {
					ResetController(mUpdateMode, view);
				}
			}
		}
	}

	private static void SetMode(View view, int mode) {
		if (DeviceInfo.EINK_TOLINO) {
			TolinoEpdController.setMode(view, mode);
		} else if (DeviceInfo.EINK_NOOK) {
			switch (mode) {
				case CMODE_CLEAR:
					N2EpdController.setMode(N2EpdController.REGION_APP_3,
							N2EpdController.WAVE_GC,
							N2EpdController.MODE_ONESHOT_ALL);
//					N2EpdController.MODE_CLEAR, view);
					break;
				case CMODE_ONESHOT:
					N2EpdController.setMode(N2EpdController.REGION_APP_3,
							N2EpdController.WAVE_GU,
							N2EpdController.MODE_ONESHOT_ALL);
//					N2EpdController.MODE_ONESHOT_ALL, view);
					break;
				case CMODE_ACTIVE:
					N2EpdController.setMode(N2EpdController.REGION_APP_3,
							N2EpdController.WAVE_GL16,
							N2EpdController.MODE_ACTIVE_ALL);
//					N2EpdController.MODE_ACTIVE_ALL, view);
					break;
			}
		}
	}

	public static int getUpdateMode() {
		return mUpdateMode;
	}

	public static int getUpdateInterval() {
		return mUpdateInterval;
	}
}
