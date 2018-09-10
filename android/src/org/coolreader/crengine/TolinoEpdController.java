package org.coolreader.crengine;

import android.graphics.Rect;
import android.view.View;
import de.telekom.epub.utils.ScreenHelper;

public class TolinoEpdController {
  private static final String LOG_TAG = TolinoEpdController.class.getSimpleName();

  public static boolean partialRefresh(View view, int mode, int waveform) {
    Rect r = new Rect();
    view.getGlobalVisibleRect(r);
    return partialRefresh(r.left, r.top, r.right, r.bottom, mode, waveform);
  }

  public static boolean partialRefresh(int left, int top, int right, int bottom, int mode, int waveform) {
    if ((right - left > 1) && (bottom - top > 1)) {
      return ScreenHelper.RegionalRefresh(left, top, right, bottom, mode, waveform) == 1;
    }
    return false;
  }

  public static void setMode(View view, int mode) {
    switch (mode) {
      case EinkScreen.CMODE_CLEAR:
	ScreenHelper.FullRefresh();
	break;
      case EinkScreen.CMODE_ONESHOT:
	partialRefresh(view, ScreenHelper.NATIVE_UPDATE_MODE_FULL, ScreenHelper.NATIVE_WAVEFORM_MODE_GC16);
	break;
      case EinkScreen.CMODE_ACTIVE:
	partialRefresh(view, ScreenHelper.NATIVE_UPDATE_MODE_FULL, ScreenHelper.NATIVE_WAVEFORM_MODE_A2);
	break;
    }  
  }
}
