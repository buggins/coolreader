package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.CoolReader;
import org.coolreader.crengine.CRToolBar.OnActionHandler;
import org.coolreader.crengine.CRToolBar.OnOverflowHandler;

import android.graphics.Rect;
import android.view.ViewGroup;

public class ReaderViewLayout extends ViewGroup implements Settings {
		private CoolReader activity;
		private ReaderView contentView;
		private StatusBar statusView;
		private CRToolBar toolbarView;
		private int statusBarLocation;
		private int toolbarLocation;
		private boolean hideToolbarInFullscren;
		private boolean fullscreen;
		private boolean nightMode;
		ReaderView.ToolbarBackgroundDrawable toolbarBackground;
		ReaderView.ToolbarBackgroundDrawable statusBackground;
	
		public CRToolBar getToolBar() {
			return toolbarView;
		}
		
		public StatusBar getStatusBar() {
			return statusView;
		}
		
		public void updateFullscreen(boolean fullscreen) {
			if (this.fullscreen == fullscreen)
				return;
			this.fullscreen = fullscreen;
			statusView.updateFullscreen(fullscreen);
			requestLayout();
		}
		
		public boolean isToolbarVisible() {
			return toolbarLocation != VIEWER_TOOLBAR_NONE && (!fullscreen || !hideToolbarInFullscren);
		}
		
		public boolean isStatusbarVisible() {
			return statusBarLocation == VIEWER_STATUS_BOTTOM || statusBarLocation == VIEWER_STATUS_TOP;
		}
		
		public void updateSettings(Properties settings) {
			CoolReader.log.d("CoolReader.updateSettings()");
			nightMode = settings.getBool(PROP_NIGHT_MODE, false);
			statusBarLocation = settings.getInt(PROP_STATUS_LOCATION, VIEWER_STATUS_TOP);
			toolbarLocation = settings.getInt(PROP_TOOLBAR_LOCATION, VIEWER_TOOLBAR_SHORT_SIDE);
			hideToolbarInFullscren = settings.getBool(PROP_TOOLBAR_HIDE_IN_FULLSCREEN, true);
			statusView.setVisibility(isStatusbarVisible() ? VISIBLE : GONE);
			statusView.updateSettings(settings);
			toolbarView.updateNightMode(nightMode);
			toolbarView.setVisibility(isToolbarVisible() ? VISIBLE : GONE);
			requestLayout();
		}
		
		public void showMenu() {
			if (isToolbarVisible())
				toolbarView.showOverflowMenu();
			else
				toolbarView.showAsPopup(this, new OnActionHandler() {
					@Override
					public boolean onActionSelected(ReaderAction item) {
						activity.getReaderView().onAction(item);
						return true;
					}
				}, null);
//			new OnOverflowHandler() {
//					@Override
//					public boolean onOverflowActions(ArrayList<ReaderAction> actions) {
//						toolbarView.showOverflowMenu();
////						activity.showActionsPopupMenu(actions, new OnActionHandler() {
////							@Override
////							public boolean onActionSelected(ReaderAction item) {
////								activity.getReaderView().onAction(item);
////								return true;
////							}
////						});
//						return false;
//					}
//				});
		}
		
		public ReaderViewLayout(CoolReader context, ReaderView contentView) {
			super(context);
			this.activity = context;
			this.contentView = contentView;
			this.statusView = new StatusBar(context);
			statusBackground = contentView.createToolbarBackgroundDrawable();
			this.statusView.setBackgroundDrawable(statusBackground);
			toolbarBackground = contentView.createToolbarBackgroundDrawable();
			this.toolbarView = new CRToolBar(context, ReaderAction.createList(new ReaderAction[] {
				ReaderAction.GO_BACK,
				ReaderAction.TOC,
				ReaderAction.SEARCH,
				ReaderAction.OPTIONS,
				ReaderAction.BOOKMARKS,
				ReaderAction.FILE_BROWSER_ROOT,
				ReaderAction.TOGGLE_DAY_NIGHT,
				ReaderAction.TOGGLE_SELECTION_MODE,
				ReaderAction.GO_PAGE,
				ReaderAction.GO_PERCENT,
				ReaderAction.FILE_BROWSER,
				ReaderAction.TTS_PLAY,
				ReaderAction.GO_FORWARD,
				ReaderAction.RECENT_BOOKS,
				ReaderAction.OPEN_PREVIOUS_BOOK,
				ReaderAction.TOGGLE_AUTOSCROLL,
				ReaderAction.ABOUT,
				ReaderAction.EXIT,
			}), false);
			this.toolbarView.setBackgroundDrawable(toolbarBackground);
			this.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
			this.addView(toolbarView);
			this.addView(contentView.getSurface());
			this.addView(statusView);
			toolbarView.setFocusable(false);
			statusView.setFocusable(false);
			toolbarView.setFocusableInTouchMode(false);
			statusView.setFocusableInTouchMode(false);
			contentView.getSurface().setFocusable(true);
			contentView.getSurface().setFocusableInTouchMode(true);
			updateFullscreen(activity.isFullscreen());
			updateSettings(context.settings());
			onThemeChanged(activity.getCurrentTheme());
		}

		public void onThemeChanged(InterfaceTheme theme) {
//			if (DeviceInfo.EINK_SCREEN) {
//				statusView.setBackgroundColor(0xFFFFFFFF);
//				toolbarView.setBackgroundColor(0xFFFFFFFF);
//			} else if (nightMode) {
//				statusView.setBackgroundColor(0xFF000000);
//				toolbarView.setBackgroundColor(0xFF000000);
//			} else {
//				statusView.setBackgroundResource(theme.getReaderStatusBackground());
//				toolbarView.setBackgroundResource(theme.getReaderToolbarBackground(toolbarView.isVertical()));
//			}
			toolbarView.updateNightMode(nightMode);
			toolbarView.setButtonAlpha(theme.getToolbarButtonAlpha());
			toolbarView.onThemeChanged(theme);
			statusView.onThemeChanged(theme);
		}

		
		@Override
		protected void onLayout(boolean changed, int l, int t, int r, int b) {
			CoolReader.log.v("onLayout(" + l + ", " + t + ", " + r + ", " + b + ")");
			r -= l;
			b -= t;
			t = 0;
			l = 0;

			statusView.setVisibility(isStatusbarVisible() ? VISIBLE : GONE);
			toolbarView.setVisibility(isToolbarVisible() ? VISIBLE : GONE);
			
			boolean toolbarVisible = toolbarLocation != VIEWER_TOOLBAR_NONE && (!fullscreen || !hideToolbarInFullscren);
			boolean landscape = r > b;
			Rect toolbarRc = new Rect(l, t, r, b);
			if (toolbarVisible) {
				int location = toolbarLocation;
				if (location == VIEWER_TOOLBAR_SHORT_SIDE)
					location = landscape ? VIEWER_TOOLBAR_LEFT : VIEWER_TOOLBAR_TOP;
				else if (location == VIEWER_TOOLBAR_LONG_SIDE)
					location = landscape ? VIEWER_TOOLBAR_TOP : VIEWER_TOOLBAR_LEFT;
				switch (location) {
				case VIEWER_TOOLBAR_LEFT:
					//toolbarView.setBackgroundResource(activity.getCurrentTheme().getReaderToolbarBackground(true));
					toolbarRc.right = l + toolbarView.getMeasuredWidth();
					//toolbarView.layout(l, t, l + toolbarView.getMeasuredWidth(), b);
					l += toolbarView.getMeasuredWidth();
					break;
				case VIEWER_TOOLBAR_RIGHT:
					//toolbarView.setBackgroundResource(activity.getCurrentTheme().getReaderToolbarBackground(true));
					toolbarRc.left = r - toolbarView.getMeasuredWidth();
					//toolbarView.layout(r - toolbarView.getMeasuredWidth(), t, r, b);
					r -= toolbarView.getMeasuredWidth();
					break;
				case VIEWER_TOOLBAR_TOP:
					//toolbarView.setBackgroundResource(activity.getCurrentTheme().getReaderToolbarBackground(false));
					toolbarRc.bottom = t + toolbarView.getMeasuredHeight();
					//toolbarView.layout(l, t, r, t + toolbarView.getMeasuredHeight());
					t += toolbarView.getMeasuredHeight();
					break;
				case VIEWER_TOOLBAR_BOTTOM:
					//toolbarView.setBackgroundResource(activity.getCurrentTheme().getReaderToolbarBackground(false));
					toolbarRc.top = b - toolbarView.getMeasuredHeight();
					//toolbarView.layout(l, b - toolbarView.getMeasuredHeight(), r, b);
					b -= toolbarView.getMeasuredHeight();
					break;
				}
				toolbarBackground.setLocation(location);
			}
			Rect statusRc = new Rect(l, t, r, b);
			if (statusBarLocation == VIEWER_STATUS_TOP) {
				statusRc.bottom = t + statusView.getMeasuredHeight();
				//statusView.layout(l, t, r, t + statusView.getMeasuredHeight());
				t += statusView.getMeasuredHeight();
			} else if (statusBarLocation == VIEWER_STATUS_BOTTOM) {
				statusRc.top = b - statusView.getMeasuredHeight();
				//statusView.layout(l, b - statusView.getMeasuredHeight(), r, b);
				b -= statusView.getMeasuredHeight();
			}
			statusBackground.setLocation(statusBarLocation);
			contentView.getSurface().layout(l, t, r, b);
			toolbarView.layout(toolbarRc.left, toolbarRc.top, toolbarRc.right, toolbarRc.bottom);
			statusView.layout(statusRc.left, statusRc.top, statusRc.right, statusRc.bottom);
			
			if (activity.isFullscreen()) {
				BackgroundThread.instance().postGUI(new Runnable() {
					@Override
					public void run() {
						CoolReader.log.v("Invalidating toolbar ++++++++++");
						toolbarView.forceLayout();
						contentView.getSurface().invalidate();
						toolbarView.invalidate();
					}
				}, 100);
			}
			
			//			toolbarView.invalidate();
//			toolbarView.requestLayout();
			//invalidate();
		}
		
		@Override
		protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
			int w = MeasureSpec.getSize(widthMeasureSpec);
			int h = MeasureSpec.getSize(heightMeasureSpec);
	        setMeasuredDimension(w, h);

			boolean statusVisible = statusBarLocation == VIEWER_STATUS_BOTTOM || statusBarLocation == VIEWER_STATUS_TOP;
			boolean toolbarVisible = toolbarLocation != VIEWER_TOOLBAR_NONE && (!fullscreen || !hideToolbarInFullscren);
			boolean landscape = w > h;
			if (toolbarVisible) {
				int location = toolbarLocation;
				if (location == VIEWER_TOOLBAR_SHORT_SIDE)
					location = landscape ? VIEWER_TOOLBAR_LEFT : VIEWER_TOOLBAR_TOP;
				else if (location == VIEWER_TOOLBAR_LONG_SIDE)
					location = landscape ? VIEWER_TOOLBAR_TOP : VIEWER_TOOLBAR_LEFT;
				switch (location) {
				case VIEWER_TOOLBAR_LEFT:
				case VIEWER_TOOLBAR_RIGHT:
					toolbarView.setVertical(true);
					toolbarView.measure(MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, w), 
							MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, h));
					w -= toolbarView.getMeasuredWidth();
					break;
				case VIEWER_TOOLBAR_TOP:
				case VIEWER_TOOLBAR_BOTTOM:
					toolbarView.setVertical(false);
					toolbarView.measure(MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, w), 
							MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, h));
					h -= toolbarView.getMeasuredHeight();
					break;
				}
			}
			if (statusVisible) {
				statusView.measure(MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, w), 
						MeasureSpec.makeMeasureSpec(MeasureSpec.UNSPECIFIED, 0));
				h -= statusView.getMeasuredHeight();
			}
			
			contentView.getSurface().measure(MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, w), 
					MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, h));
		}

		@Override
		protected void onSizeChanged(int w, int h, int oldw, int oldh) {
			super.onSizeChanged(w, h, oldw, oldh);
			requestLayout();
		}
	}