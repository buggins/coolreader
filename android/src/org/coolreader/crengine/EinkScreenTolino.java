package org.coolreader.crengine;

import android.view.View;

public class EinkScreenTolino extends EinkScreenNook {

	@Override
	public void prepareController(View view, boolean isPartially) {
		//System.err.println("Sleep = " + isPartially);
		if (isPartially || mIsSleep != isPartially) {
			tolinoSleepController(isPartially, view);
//			if (isPartially)
			return;
		}
		if (mRefreshNumber == -1) {
			switch (mUpdateMode) {
				case Clear:
					tolinoSetMode(view, mUpdateMode);
					break;
				case Active:
					if (mUpdateInterval == 0) {
						tolinoSetMode(view, mUpdateMode);
					}
					break;
			}
			mRefreshNumber = 0;
			return;
		}
		if (mUpdateMode == EinkUpdateMode.Clear) {
			tolinoSetMode(view, mUpdateMode);
			return;
		}
		if (mUpdateInterval > 0 || mUpdateMode == EinkUpdateMode.Fast) {
			if (mRefreshNumber == 0 || (mUpdateMode == EinkUpdateMode.Fast && mRefreshNumber < mUpdateInterval)) {
				switch (mUpdateMode) {
					case Active:
						tolinoSetMode(view, mUpdateMode);
						break;
					case Fast:
						tolinoSetMode(view, mUpdateMode);
						break;
				}
			} else if (mUpdateInterval <= mRefreshNumber) {
				tolinoSetMode(view, EinkUpdateMode.Clear);
				mRefreshNumber = -1;
			}
			if (mUpdateInterval > 0) {
				mRefreshNumber++;
			}
		}
	}


	// private methods
	private void tolinoSleepController(boolean toSleep, View view) {
		if (toSleep != mIsSleep) {
			log.d("+++SleepController " + toSleep);
			mIsSleep = toSleep;
			if (mIsSleep) {
				switch (mUpdateMode) {
					case Clear:
						break;
					case Fast:
						break;
					case Active:
						tolinoSetMode(view, EinkUpdateMode.Clear);
						mRefreshNumber = -1;
				}
			} else {
				setupController(mUpdateMode, mUpdateInterval, view);
			}
		}
	}

	private void tolinoSetMode(View view, EinkUpdateMode mode) {
		TolinoEpdController.setMode(view, mode);
	}
}
