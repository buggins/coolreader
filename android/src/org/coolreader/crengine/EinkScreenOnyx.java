/*
 * CoolReader for Android
 * Copyright (C) 2018,2020,2021 Aleksey Chernov <valexlin@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.coolreader.crengine;

import android.content.Context;
import android.util.Log;
import android.view.View;

import org.coolreader.CoolReader;
import org.eink_onyx_reflections.OnyxDevice;
import org.eink_onyx_reflections.OnyxEinkDeviceImpl;
import org.eink_onyx_reflections.UpdateMode;

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
	private int mExtraDelayFullRefresh = 0;
	private boolean mIsAppOptimizationEnabled = false;
	private boolean mNeedCallByPass = false;

	@Override
	public void setupController(EinkUpdateMode mode, int updateInterval, View view) {
		OnyxEinkDeviceImpl onyxEinkDevice = OnyxDevice.currentDevice();
		mIsAppOptimizationEnabled = onyxEinkDevice.isAppOptimizationEnabled();
		if (mIsAppOptimizationEnabled) {
			log.i("ONYX App Optimization is enabled");
		} else {
			log.i("ONYX App Optimization is disabled");
		}
		mUpdateInterval = updateInterval;
		if (mUpdateMode.equals(mode))
			return;
		log.d("EinkScreenOnyx.setupController(): mode=" + mode);
		onyxEinkDevice.enableScreenUpdate(view, true);
		mRefreshNumber = 0;
		onyxEinkDevice.clearApplicationFastMode();
		mNeedCallByPass = false;
		UpdateMode onyxFastUpdateMode = UpdateMode.DU;
		switch (onyxEinkDevice.deviceType()) {
			case rk32xx:
			case rk33xx:
			case sdm:
				onyxFastUpdateMode = UpdateMode.DU_QUALITY;
				mNeedCallByPass = !mIsAppOptimizationEnabled;
				break;
		}
		switch (onyxEinkDevice.deviceType()) {
			// TODO: check other ONYX devices & platforms
			case sdm:
				// Hack, use additional delay before full screen update
				mExtraDelayFullRefresh = 40;
				break;
		}
		switch (mode) {
			case Regal:            // Regal
				if (mInA2Mode) {
					onyxEnableA2Mode(view, false);
					mInA2Mode = false;
				}
				if (mInFastMode) {
					onyxEinkDevice.applyApplicationFastMode(CoolReader.class.getSimpleName(), false, true);
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
					onyxEinkDevice.applyApplicationFastMode(CoolReader.class.getSimpleName(), false, true);
					mInFastMode = false;
				}
				mOnyxUpdateMode = UpdateMode.GU;
				break;
			case Fast:            // Fast
				if (mInA2Mode) {
					onyxEnableA2Mode(view, false);
					mInA2Mode = false;
				}
				// Enable fast mode (not implemented on RK3026)
				if (!mInFastMode) {
					onyxEinkDevice.applyApplicationFastMode(CoolReader.class.getSimpleName(), true, true, UpdateMode.DU_QUALITY, Integer.MAX_VALUE);
					mInFastMode = true;
				}
				mOnyxUpdateMode = onyxFastUpdateMode;
				break;
			case A2:            // A2 mode
				if (mInFastMode) {
					onyxEinkDevice.applyApplicationFastMode(CoolReader.class.getSimpleName(), false, true);
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
			onyxEinkDevice.setViewDefaultUpdateMode(view, mOnyxUpdateMode);
			BackgroundThread.instance().executeGUI(view::invalidate);
		}
		mUpdateMode = mode;
	}

	@Override
	public void prepareController(View view, boolean isPartially) {
		if (mIsAppOptimizationEnabled)
			return;
		if (isPartially)
			return;
		if (mRefreshNumber == -1) {
			mRefreshNumber = 0;
			onyxRepaintEveryThing(view, false);
			return;
		}
		if (mUpdateInterval > 0) {
			mRefreshNumber++;
			if (mRefreshNumber >= mUpdateInterval) {
				mRefreshNumber = 0;
				return;
			}
		}
		if (mRefreshNumber > 0 || mUpdateInterval == 0) {
			OnyxDevice.currentDevice().setViewDefaultUpdateMode(view, mOnyxUpdateMode);
			if (mNeedCallByPass) {
				// Hack, without it, Regal NOT work (if app optimization is disabled).
				// But if app optimization is enabled this cause flickering: after screen drawn - screen cleared and then image restored
				// Also, without it, on rk3288 with firmware 2.1 & 3.0 the image will not updated.
				OnyxDevice.currentDevice().byPass(0);
			}
		}
	}

	@Override
	public void updateController(View view, boolean isPartially) {
		if (mIsAppOptimizationEnabled)
			return;
		if (isPartially)
			return;
		if (0 == mRefreshNumber && mUpdateInterval > 0) {
			if (mExtraDelayFullRefresh > 0) {
				// Hack, on ONYX devices with SDM platform without this delay full screen refresh runs too early
				// (before new page appears on screen)
				// This functions called after android.view.SurfaceHolder.unlockCanvasAndPost()
				//   See https://developer.android.com/reference/android/view/SurfaceHolder#unlockCanvasAndPost(android.graphics.Canvas)
				// which guarantees that by this time the new image will be on the screen
				// But in fact on com.onyx.android.sdk.device.Device.DeviceIndex.SDM need extra delay.
				try {
					Thread.sleep(mExtraDelayFullRefresh);
				} catch (InterruptedException ignored) {
				}
			}
			onyxRepaintEveryThing(view, false);
		}
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
				res = OnyxDevice.currentDevice().getColdLightConfigValue(context);
			} else {
				res = OnyxDevice.currentDevice().getFrontLightDeviceValue(context);
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
						res = OnyxDevice.currentDevice().setColdLightDeviceValue(context, alignedValue);
					} else {
						if (OnyxDevice.currentDevice().setFrontLightDeviceValue(context, alignedValue))
							res = OnyxDevice.currentDevice().setFrontLightConfigValue(context, alignedValue);
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
				res = OnyxDevice.currentDevice().getWarmLightConfigValue(context);
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
					res = OnyxDevice.currentDevice().setWarmLightDeviceValue(context, alignedValue);
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
					mFrontLineLevels = OnyxDevice.currentDevice().getFrontLightValueList(context);
				} catch (Exception ignored) { }
				if (null == mFrontLineLevels || mFrontLineLevels.size() == 0) {
					mFrontLineLevels = OnyxDevice.currentDevice().getColdLightValues(context);
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
					mWarmLightLevels = OnyxDevice.currentDevice().getWarmLightValues(context);
				}
			}
		}
		return mWarmLightLevels;
	}

	@Override
	public boolean isAppOptimizationEnabled() {
		return mIsAppOptimizationEnabled;
	}

	private void onyxRepaintEveryThing(View view, boolean invalidate) {
		switch (OnyxDevice.currentDeviceType()) {
			case rk31xx:
			case rk32xx:
			case rk33xx:
			case sdm:
				OnyxDevice.currentDevice().repaintEveryThing(UpdateMode.GC);
				break;
			default:
				if (null != view) {
					OnyxDevice.currentDevice().setViewDefaultUpdateMode(view, UpdateMode.GC);
					if (invalidate)
						view.postInvalidate();
				}
				break;
		}
	}

	private void onyxEnableA2Mode(View view, boolean enable) {
		switch (OnyxDevice.currentDeviceType()) {
			case rk3026:
			case imx6:
			case imx7:
				if (enable)
					OnyxDevice.currentDevice().enableA2ForSpecificView(view);
				else
					OnyxDevice.currentDevice().disableA2ForSpecificView(view);
				break;
			default:
				OnyxDevice.currentDevice().clearApplicationFastMode();
				if (enable)
					OnyxDevice.currentDevice().applyApplicationFastMode(CoolReader.class.getSimpleName(), true, true, UpdateMode.ANIMATION_QUALITY, Integer.MAX_VALUE);
				else
					OnyxDevice.currentDevice().applyApplicationFastMode(CoolReader.class.getSimpleName(), false, true);
				break;
		}
	}
}
