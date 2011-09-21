package org.coolreader.crengine;

import org.coolreader.crengine.N2EpdController;
import android.view.View;

public class EinkScreen {

	public static int UpdateMode = -1;
	public static int UpdateModeInterval;
	public static int RefreshNumber = 0;

	public static void PrepareController(View view) {
		if (DeviceInfo.EINK_NOOK) {
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
		}
	}

	public static void ResetController(int mode, View view) {
		System.err.println("+++ResetController " + mode);
		if (DeviceInfo.EINK_NOOK) {
			UpdateMode = mode;
			RefreshNumber = 0;
			switch (mode) {
			case -1:
				// onPause
				N2EpdController.setMode(N2EpdController.REGION_APP_3,
										N2EpdController.WAVE_AUTO,
										N2EpdController.MODE_ACTIVE, view);
				break;
			case 0:
				// качественный режим
				//N2EpdController.setMode(N2EpdController.REGION_APP_3,
				//						N2EpdController.WAVE_AUTO,
				//						N2EpdController.MODE_ACTIVE, view);	// заливает любой экран
				N2EpdController.setMode(N2EpdController.REGION_APP_3,
										N2EpdController.WAVE_GU,
										N2EpdController.MODE_CLEAR_ALL, view);	// если меньше 50% на экране - не заливает
				break;
			case 1:
				// быстрый режим
				N2EpdController.setMode(N2EpdController.REGION_APP_3,
										N2EpdController.WAVE_GL16,
										N2EpdController.MODE_ACTIVE, view);
				//N2EpdController.setMode(N2EpdController.REGION_APP_3,
				//						N2EpdController.WAVE_GU,
				//						N2EpdController.MODE_ACTIVE_ALL, view); // качество хуже, кажется
				break;
			}
		}
	}
}
