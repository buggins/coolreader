package org.coolreader.crengine;

import org.coolreader.crengine.N2EpdController;
import android.view.View;

public class EinkScreen {
	
	/// variables
	public static int UpdateMode = -1; 
	// 0 - CLEAR_ALL, set only for old_mode == 2 
	// 1 - ONESHOT, always set in prepare 
	// 2 - ACTIVE, set in prepare
	public static int UpdateModeInterval;
	public static int RefreshNumber = -1;
	public static boolean IsSleep = false;
	// constants
	public final static int cmodeClear = 0;
	public final static int cmodeOneshot = 1;
	public final static int cmodeActive = 2;

	public static void Refresh() {
		RefreshNumber = -1;
	}
	
	public static void PrepareController(View view, boolean isPartially) {
		if (DeviceInfo.EINK_NOOK) {
			//System.err.println("Sleep = " + isPartially);
			if (isPartially || IsSleep != isPartially) {
				SleepController(isPartially, view);
//				if (isPartially) 
					return;
			} 
			if (RefreshNumber == -1) {
				switch (UpdateMode) {
					case cmodeClear:
						SetMode(view, cmodeClear);
						break;
					case cmodeActive:
						if (UpdateModeInterval == 0) {
							SetMode(view, cmodeActive);
						}
						break;
				}
				RefreshNumber = 0;
				return;
			}
			if (UpdateMode == cmodeClear) {
				SetMode(view, cmodeClear);
				return;
			}
			if (UpdateMode > 0 && (UpdateModeInterval > 0 || UpdateMode == 1)) {
				if (RefreshNumber == 0 || (UpdateMode == cmodeOneshot && RefreshNumber < UpdateModeInterval)) {
					switch (UpdateMode) {
						case cmodeActive:
							SetMode(view, cmodeActive);
							break;
						case cmodeOneshot:
							SetMode(view, cmodeOneshot);
							break;
					}
				} else if (UpdateModeInterval <= RefreshNumber) {
					SetMode(view, cmodeClear);
					RefreshNumber = -1;
				}
				if (UpdateModeInterval > 0) {
					RefreshNumber++;
				}
			}
			
			return;
			/*
			if (UpdateMode == 1 && UpdateModeInterval != 0) {
				if (RefreshNumber == 0) {
					// быстрый режим, один раз устанавливается
					N2EpdController.setMode(N2EpdController.REGION_APP_3,
											N2EpdController.WAVE_GL16,
											N2EpdController.MODE_ACTIVE, view); // why not MODE_ACTIVE_ALL?
				} else if (UpdateModeInterval == RefreshNumber) {
					// одно качественное обновление для быстрого режима
					N2EpdController.setMode(N2EpdController.REGION_APP_3,
											N2EpdController.WAVE_GU,
											N2EpdController.MODE_CLEAR_ALL, view);
					RefreshNumber = -1;
				}
				RefreshNumber ++;
			}
			*/
		}
	}

	public static void ResetController(int mode, View view) {
		if (!DeviceInfo.EINK_NOOK) { return; }
		System.err.println("+++ResetController " + mode);
		switch (mode) {
			case cmodeClear:
				if (UpdateMode == cmodeActive) {
					RefreshNumber = -1;
				} else {
					RefreshNumber = 0;
				}
				break;
			case cmodeOneshot:
				RefreshNumber = 0;
				break;
			default:
				RefreshNumber = -1;
		}
		
		UpdateMode = mode;
	}
	public static void ResetController(View view) {
		if (!DeviceInfo.EINK_NOOK || UpdateMode == cmodeClear) { return; }
		System.err.println("+++Soft reset Controller ");
		SetMode(view, cmodeClear);
		RefreshNumber = -1;
	}

	public static void SleepController(boolean toSleep, View view) {
		if (!DeviceInfo.EINK_NOOK || toSleep == IsSleep) {
			return;
		}
		System.err.println("+++SleepController " + toSleep);
		IsSleep = toSleep;
		if (IsSleep) {
			switch (UpdateMode) {
			case cmodeClear:
				break;
			case cmodeOneshot:
				break;
			case cmodeActive:
				SetMode(view, cmodeClear);
				RefreshNumber = -1;
			}
		} else {
			ResetController(UpdateMode, view);
		}
		return;
	}
	
	private static void SetMode(View view, int mode) {
		switch (mode) {
		case cmodeClear:	
			N2EpdController.setMode(N2EpdController.REGION_APP_3,
				N2EpdController.WAVE_GC,
				N2EpdController.MODE_ONESHOT_ALL);
//				N2EpdController.MODE_CLEAR, view);
			break;
		case cmodeOneshot:	
			N2EpdController.setMode(N2EpdController.REGION_APP_3,
					N2EpdController.WAVE_GU,
					N2EpdController.MODE_ONESHOT_ALL);
//			N2EpdController.MODE_ONESHOT_ALL, view);
			break;
		case cmodeActive:	
			N2EpdController.setMode(N2EpdController.REGION_APP_3,
				N2EpdController.WAVE_GL16,
				N2EpdController.MODE_ACTIVE_ALL);
//			N2EpdController.MODE_ACTIVE_ALL, view);
			break;
		}  
	}	
}
