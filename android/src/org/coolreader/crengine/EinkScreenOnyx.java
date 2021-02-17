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

public class EinkScreenOnyx implements EinkScreen {

	public static final Logger log = L.create("onyx", Log.VERBOSE);

	private EinkUpdateMode mUpdateMode = EinkUpdateMode.Unspecified;
	private int mUpdateInterval;
	private int mRefreshNumber = -1;
	private boolean mInFastMode = false;
	private boolean mInA2Mode = false;
	// Front light levels
	private List<Integer> mFrontLineLevels = null;
	private List<Integer> mWarmLightLevels = null;
	private UpdateMode mOnyxUpdateMode = UpdateMode.None;

	@Override
	public void setupController(EinkUpdateMode mode, int updateInterval, View view) {
		mUpdateInterval = updateInterval;
		if (mUpdateMode.equals(mode))
			return;
		log.d("EinkScreenOnyx.setupController(): mode=" + mode);
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
			case Regal:            // Regal
				if (mInA2Mode) {
					onyxEnableA2Mode(view, false);
					mInA2Mode = false;
				}
				if (mInFastMode) {
					EpdController.applyApplicationFastMode(CoolReader.class.getSimpleName(), false, true);
					mInFastMode = false;
				}
				mOnyxUpdateMode = UpdateMode.REGAL;
				break;
			case Clear:            // Quality
				if (mInA2Mode) {
					onyxEnableA2Mode(view, false);
					mInA2Mode = false;
				}
				if (mInFastMode) {
					EpdController.applyApplicationFastMode(CoolReader.class.getSimpleName(), false, true);
					mInFastMode = false;
				}
				mOnyxUpdateMode = UpdateMode.GU;
				break;
			case Fast:            // Fast
				if (mInA2Mode) {
					onyxEnableA2Mode(view, false);
					mInA2Mode = false;
				}
				// Enable fast mode (not implemented on RK3026, not tested)
				if (!mInFastMode) {
					EpdController.applyApplicationFastMode(CoolReader.class.getSimpleName(), true, true, UpdateMode.DU_QUALITY, Integer.MAX_VALUE);
					mInFastMode = true;
				}
				mOnyxUpdateMode = onyxFastUpdateMode;
				break;
			case A2:            // A2 mode
				if (mInFastMode) {
					EpdController.applyApplicationFastMode(CoolReader.class.getSimpleName(), false, true);
					mInFastMode = false;
				}
				if (!mInA2Mode) {
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
		mUpdateMode = mode;
	}

	@Override
	public void prepareController(View view, boolean isPartially) {
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

	@Override
	public void updateController(View view, boolean isPartially) {
		// do nothing...
	}

	@Override
	public void refreshScreen(View view) {
		onyxRepaintEveryThing(view, true);
		mRefreshNumber = 0;
	}

	@Override
	public EinkUpdateMode getUpdateMode() {
		return mUpdateMode;
	}

	@Override
	public int getUpdateInterval() {
		return mUpdateInterval;
	}

	@Override
	public int getFrontLightValue(Context context) {
		int res = 0;
		try {
			if (DeviceInfo.ONYX_HAVE_NATURAL_BACKLIGHT) {
				res = Device.currentDevice().getColdLightConfigValue(context);
			} else {
				res = Device.currentDevice().getFrontLightDeviceValue(context);
			}
		} catch (Exception ignored) {}
		return res;
	}

	@Override
	public boolean setFrontLightValue(Context context, int value) {
		boolean res = false;
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
		return res;
	}

	@Override
	public int getWarmLightValue(Context context) {
		int res = 0;
		try {
			if (DeviceInfo.ONYX_HAVE_NATURAL_BACKLIGHT) {
				res = Device.currentDevice().getWarmLightConfigValue(context);
			}
		} catch (Exception ignored) {}
		return res;
	}

	@Override
	public boolean setWarmLightValue(Context context, int value) {
		boolean res = false;
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
		return res;
	}

	@Override
	public List<Integer> getFrontLightLevels(Context context) {
		if (DeviceInfo.ONYX_HAVE_FRONTLIGHT || DeviceInfo.ONYX_HAVE_NATURAL_BACKLIGHT) {
			if (null == mFrontLineLevels) {
				try {
					mFrontLineLevels = Device.currentDevice().getFrontLightValueList(context);
				} catch (Exception ignored) { }
				if (null == mFrontLineLevels) {
					Integer[] values = Device.currentDevice().getColdLightValues(context);
					if (null != values) {
						mFrontLineLevels = Arrays.asList(values);
					}
				}
			}
		}
		return mFrontLineLevels;
	}

	@Override
	public List<Integer> getWarmLightLevels(Context context) {
		if (DeviceInfo.EINK_HAVE_NATURAL_BACKLIGHT) {
			if (null == mWarmLightLevels) {
				if (DeviceInfo.ONYX_HAVE_NATURAL_BACKLIGHT) {
					Integer[] values = Device.currentDevice().getWarmLightValues(context);
					if (null != values) {
						mWarmLightLevels = Arrays.asList(values);
					}
				}
			}
		}
		return mWarmLightLevels;
	}


	// private methods
	private void onyxRepaintEveryThing(View view, boolean invalidate) {
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

	private void onyxEnableA2Mode(View view, boolean enable) {
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
}
