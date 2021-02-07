package org.coolreader.crengine;

import android.content.Context;
import android.util.Log;
import android.view.View;

import com.onyx.android.sdk.api.device.epd.EpdController;
import com.onyx.android.sdk.api.device.epd.UpdateMode;
import com.onyx.android.sdk.device.Device;

import org.coolreader.CoolReader;

import java.util.Arrays;
import java.util.List;

public class EinkScreen {
	private static final String TAG = "EinkScreen";

	public enum EinkUpdateMode {
		Unspecified(-1),
		Regal(3),				// also known as 'SNOW Field'
		Clear(0),				// old name CMODE_CLEAR
		Fast(1),				// old name CMODE_ONESHOT
		Active(2),			// old name CMODE_ACTIVE
		A2(4),				// Fast 'A2' mode
		;

		public static EinkUpdateMode byCode(int code) {
			for (EinkUpdateMode mode : EinkUpdateMode.values()) {
				if (mode.code == code)
					return mode;
			}
			return EinkUpdateMode.Unspecified;
		}

		EinkUpdateMode(int code) {
			this.code = code;
		}

		public final int code;
	}

	/// variables
	//private static int mUpdateMode = -1;
	private static EinkUpdateMode mUpdateMode = EinkUpdateMode.Unspecified;
	// 0 - Clear, set only for old_mode == 2
	// 1 - Fast, always set in prepare
	// 2 - Active, set in prepare
	private static int mUpdateInterval;
	private static int mRefreshNumber = -1;
	private static boolean mIsSleep = false;
	private static boolean mInFastMode = false;
	private static boolean mInA2Mode = false;
	// Front light levels
	private static List<Integer> mFrontLineLevels = null;
	private static List<Integer> mWarmLightLevels = null;
	private static UpdateMode mOnyxUpdateMode = UpdateMode.None;

	public static void Refresh(View view) {
		if (DeviceInfo.EINK_NOOK || DeviceInfo.EINK_TOLINO) {
			mRefreshNumber = -1;
		} else if (DeviceInfo.EINK_ONYX) {
			onyxRepaintEveryThing(view, true);
			mRefreshNumber = 0;
		}
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
					case Clear:
						SetMode(view, mUpdateMode);
						break;
					case Active:
						if (mUpdateInterval == 0) {
							SetMode(view, mUpdateMode);
						}
						break;
				}
				mRefreshNumber = 0;
				return;
			}
			if (mUpdateMode == EinkUpdateMode.Clear) {
				SetMode(view, mUpdateMode);
				return;
			}
			if (mUpdateInterval > 0 || mUpdateMode == EinkUpdateMode.Fast) {
				if (mRefreshNumber == 0 || (mUpdateMode == EinkUpdateMode.Fast && mRefreshNumber < mUpdateInterval)) {
					switch (mUpdateMode) {
						case Active:
							SetMode(view, mUpdateMode);
							break;
						case Fast:
							SetMode(view, mUpdateMode);
							break;
					}
				} else if (mUpdateInterval <= mRefreshNumber) {
					SetMode(view, EinkUpdateMode.Clear);
					mRefreshNumber = -1;
				}
				if (mUpdateInterval > 0) {
					mRefreshNumber++;
				}
			}
		} else if (DeviceInfo.EINK_ONYX) {
			if (mRefreshNumber == -1) {
				mRefreshNumber = 0;
				onyxRepaintEveryThing(view, false);
				return;
			}
			if (mUpdateInterval > 0) {
				mRefreshNumber++;
				if (mRefreshNumber >= mUpdateInterval) {
					mRefreshNumber = 0;
					onyxRepaintEveryThing(view, false);
					return;
				}
			}
			if (mRefreshNumber > 0 || mUpdateInterval == 0) {
				EpdController.setViewDefaultUpdateMode(view, mOnyxUpdateMode);
				if (Device.DeviceIndex.Rk32xx == Device.currentDeviceIndex()) {
					// I don't know what exactly this line does, but without it, the image on rk3288 will not updated.
					// Found by brute force.
					EpdController.byPass(0);
				}
			}
		}
	}

	public static void UpdateController(View view, boolean isPartially) {
	}

	private static void onyxRepaintEveryThing(View view, boolean invalidate) {
		switch (Device.currentDeviceIndex()) {
			case Rk31xx:
			case Rk32xx:
			case Rk33xx:
			case SDM:
				EpdController.repaintEveryThing(UpdateMode.GC);
				break;
			default:
				if (null != view) {
					EpdController.setViewDefaultUpdateMode(view, UpdateMode.GC);
					if (invalidate)
						view.postInvalidate();
				}
				break;
		}
	}

	private static void onyxEnableA2Mode(View view, boolean enable) {
		switch (Device.currentDeviceIndex()) {
			case Rk3026:
			case imx6:
			case imx7:
				if (enable)
					EpdController.enableA2ForSpecificView(view);
				else
					EpdController.disableA2ForSpecificView(view);
				break;
			default:
				EpdController.clearApplicationFastMode();
				if (enable)
					EpdController.applyApplicationFastMode(CoolReader.class.getSimpleName(), true, true, UpdateMode.ANIMATION_QUALITY, Integer.MAX_VALUE);
				else
					EpdController.applyApplicationFastMode(CoolReader.class.getSimpleName(), false, true);
				break;
		}
	}

	public static void ResetController(EinkUpdateMode mode, int updateInterval, View view) {
		mUpdateInterval = updateInterval;
		if (mUpdateMode.equals(mode))
			return;
		Log.d(TAG, "EinkScreen.ResetController(): mode=" + mode);
		if (DeviceInfo.EINK_NOOK || DeviceInfo.EINK_TOLINO) {
			switch (mode) {
				case Clear:
					if (mUpdateMode == EinkUpdateMode.Active) {
						mRefreshNumber = -1;
					} else {
						mRefreshNumber = 0;
					}
					break;
				case Fast:
					mRefreshNumber = 0;
					break;
				default:
					mRefreshNumber = -1;
			}
		} else if (DeviceInfo.EINK_ONYX) {
			EpdController.enableScreenUpdate(view, true);
			mRefreshNumber = 0;
			EpdController.clearApplicationFastMode();
			UpdateMode onyxFastUpdateMode = UpdateMode.DU;
			switch (Device.currentDeviceIndex()) {
				case Rk32xx:
				case Rk33xx:
				case SDM:
					onyxFastUpdateMode = UpdateMode.DU_QUALITY;
					break;
			}
			switch (mode) {
				case Regal:			// Regal
					if (mInA2Mode) {
						Log.d(TAG, "disable A2 mode");
						onyxEnableA2Mode(view, false);
						mInA2Mode = false;
					}
					if (mInFastMode) {
						Log.d(TAG, "disable Fast mode");
						EpdController.applyApplicationFastMode(CoolReader.class.getSimpleName(), false, true);
						mInFastMode = false;
					}
					mOnyxUpdateMode = UpdateMode.REGAL;
					break;
				case Clear:			// Quality
					if (mInA2Mode) {
						Log.d(TAG, "disable A2 mode");
						onyxEnableA2Mode(view, false);
						mInA2Mode = false;
					}
					if (mInFastMode) {
						Log.d(TAG, "disable Fast mode");
						EpdController.applyApplicationFastMode(CoolReader.class.getSimpleName(), false, true);
						mInFastMode = false;
					}
					mOnyxUpdateMode = UpdateMode.GU;
					break;
				case Fast:			// Fast
					if (mInA2Mode) {
						Log.d(TAG, "disable A2 mode");
						onyxEnableA2Mode(view, false);
						mInA2Mode = false;
					}
					// Enable fast mode (not implemented on RK3026, not tested)
					if (!mInFastMode) {
						Log.d(TAG, "enable Fast mode");
						EpdController.applyApplicationFastMode(CoolReader.class.getSimpleName(), true, true, UpdateMode.DU_QUALITY, Integer.MAX_VALUE);
						mInFastMode = true;
					}
					mOnyxUpdateMode = onyxFastUpdateMode;
					break;
				case A2:			// A2 mode
					if (mInFastMode) {
						Log.d(TAG, "disable Fast mode");
						EpdController.applyApplicationFastMode(CoolReader.class.getSimpleName(), false, true);
						mInFastMode = false;
					}
					if (!mInA2Mode) {
						Log.d(TAG, "enable A2 mode");
						onyxEnableA2Mode(view, true);
						mInA2Mode = true;
					}
					mOnyxUpdateMode = onyxFastUpdateMode;
					break;
				default:
					mOnyxUpdateMode = UpdateMode.GU;
			}
			if (null != view) {
				EpdController.setViewDefaultUpdateMode(view, mOnyxUpdateMode);
				BackgroundThread.instance().executeGUI(view::invalidate);
			}
		}
		mUpdateMode = mode;
	}

	private static void SleepController(boolean toSleep, View view) {
		if (DeviceInfo.EINK_NOOK || DeviceInfo.EINK_TOLINO) {
			if (toSleep != mIsSleep) {
				System.err.println("+++SleepController " + toSleep);
				mIsSleep = toSleep;
				if (mIsSleep) {
					switch (mUpdateMode) {
						case Clear:
							break;
						case Fast:
							break;
						case Active:
							SetMode(view, EinkUpdateMode.Clear);
							mRefreshNumber = -1;
					}
				} else {
					ResetController(mUpdateMode, mUpdateInterval, view);
				}
			}
		}
	}

	private static void SetMode(View view, EinkUpdateMode mode) {
		if (DeviceInfo.EINK_TOLINO) {
			TolinoEpdController.setMode(view, mode);
		} else if (DeviceInfo.EINK_NOOK) {
			switch (mode) {
				case Clear:
					N2EpdController.setMode(N2EpdController.REGION_APP_3,
							N2EpdController.WAVE_GC,
							N2EpdController.MODE_ONESHOT_ALL);
//					N2EpdController.MODE_CLEAR, view);
					break;
				case Fast:
					N2EpdController.setMode(N2EpdController.REGION_APP_3,
							N2EpdController.WAVE_GU,
							N2EpdController.MODE_ONESHOT_ALL);
//					N2EpdController.MODE_ONESHOT_ALL, view);
					break;
				case Active:
					N2EpdController.setMode(N2EpdController.REGION_APP_3,
							N2EpdController.WAVE_GL16,
							N2EpdController.MODE_ACTIVE_ALL);
//					N2EpdController.MODE_ACTIVE_ALL, view);
					break;
			}
		}
	}

	public static EinkUpdateMode getUpdateMode() {
		return mUpdateMode;
	}

	public static int getUpdateInterval() {
		return mUpdateInterval;
	}

	public static boolean setFrontLightValue(Context context, int value) {
		boolean res = false;
		if (DeviceInfo.EINK_ONYX) {
			if (DeviceInfo.ONYX_HAVE_FRONTLIGHT) {
				if (value >= 0) {
					Integer alignedValue = Utils.findNearestValue(getFrontLightLevels(context), value);
					if (null != alignedValue) {
						if (DeviceInfo.ONYX_HAVE_NATURAL_BACKLIGHT) {
							res = Device.currentDevice().setColdLightDeviceValue(context, alignedValue);
						} else {
							if (Device.currentDevice().setFrontLightDeviceValue(context, alignedValue))
								res = Device.currentDevice().setFrontLightConfigValue(context, alignedValue);
						}
					}
				} else {
					// system default, just ignore
				}
			}
		}
		return res;
	}

	public static boolean setWarmLightValue(Context context, int value) {
		boolean res = false;
		if (DeviceInfo.EINK_ONYX) {
			if (DeviceInfo.ONYX_HAVE_NATURAL_BACKLIGHT) {
				if (value >= 0) {
					Integer alignedValue = Utils.findNearestValue(getWarmLightLevels(context), value);
					if (null != alignedValue) {
						res = Device.currentDevice().setWarmLightDeviceValue(context, alignedValue);
					}
				} else {
					// system default, just ignore
				}
			}
		}
		return res;
	}

	public static List<Integer> getFrontLightLevels(Context context) {
		if (null == mFrontLineLevels) {
			if (DeviceInfo.EINK_HAVE_FRONTLIGHT) {
				if (DeviceInfo.ONYX_HAVE_FRONTLIGHT) {
					try {
						mFrontLineLevels = Device.currentDevice().getFrontLightValueList(context);
					} catch (Exception ignored) {}
					if (null == mFrontLineLevels) {
						Integer[] values = Device.currentDevice().getColdLightValues(context);
						if (null != values) {
							mFrontLineLevels = Arrays.asList(values);
						}
					}
				}
				// TODO: other e-ink devices with front light support...
			}
		}
		return mFrontLineLevels;
	}

	public static List<Integer> getWarmLightLevels(Context context) {
		if (null == mWarmLightLevels) {
			if (DeviceInfo.EINK_HAVE_NATURAL_BACKLIGHT) {
				if (DeviceInfo.ONYX_HAVE_NATURAL_BACKLIGHT) {
					Integer[] values = Device.currentDevice().getWarmLightValues(context);
					if (null != values) {
						mWarmLightLevels = Arrays.asList(values);
					}
				}
				// TODO: other e-ink devices with front light support...
			}
		}
		return mWarmLightLevels;
	}
}
