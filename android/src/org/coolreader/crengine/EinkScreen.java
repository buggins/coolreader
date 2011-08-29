package org.coolreader.crengine;


public class EinkScreen {

	public static int UpdateMode = -1;
	public static int UpdateModeInterval;
	public static int RefreshNumber = 0;
	
	public static void PrepareController() {
		if (DeviceInfo.EINK_NOOK) {
			if (UpdateMode == 1) {
//				System.err.println("Refresh #" + String.valueOf(RefreshNumber));
				if (UpdateModeInterval > 0 && RefreshNumber >= UpdateModeInterval) {
//					System.err.println("ResetPageNumber");
					RefreshNumber = 0;
					N2EpdController.setGL16Mode(1);
				} else {
//					System.err.println("setGL16Mode");
					N2EpdController.setGL16Mode(0);
					if (UpdateModeInterval > 0) {
						RefreshNumber++;
					}	
				}	
			}	
		}
	}
	public static void ResetController(int dept) {
		if (DeviceInfo.EINK_NOOK) {
			if (UpdateMode == 1) {
				N2EpdController.setGL16Mode(dept);
				RefreshNumber = 0;
			}	
		}
	}
}
