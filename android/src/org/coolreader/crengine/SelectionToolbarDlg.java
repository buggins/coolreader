package org.coolreader.crengine;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.graphics.drawable.BitmapDrawable;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnKeyListener;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.PopupWindow;
import android.widget.PopupWindow.OnDismissListener;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;

public class SelectionToolbarDlg {
	PopupWindow mWindow;
	View mAnchor;
	CoolReader mCoolReader;
	ReaderView mReaderView;
	View mPanel;
	Selection selection;
	static public void showDialog( CoolReader coolReader, ReaderView readerView, final Selection selection )
	{
		SelectionToolbarDlg dlg = new SelectionToolbarDlg(coolReader, readerView, selection);
		//dlg.mWindow.update(dlg.mAnchor, width, height)
		Log.d("cr3", "popup: " + dlg.mWindow.getWidth() + "x" + dlg.mWindow.getHeight());
		//dlg.update();
		//dlg.showAtLocation(readerView, Gravity.LEFT|Gravity.TOP, readerView.getLeft()+50, readerView.getTop()+50);
		//dlg.showAsDropDown(readerView);
		//dlg.update();
	}

	private boolean pageModeSet;
	private boolean changedPageMode;
	private void setReaderMode()
	{
		if (pageModeSet)
			return;
		//if (DeviceInfo.EINK_SCREEN) { return; } // switching to scroll view doesn't work well on eink screens
		
		String oldViewSetting = mReaderView.getSetting( ReaderView.PROP_PAGE_VIEW_MODE );
		if ( "1".equals(oldViewSetting) ) {
			changedPageMode = true;
			mReaderView.setSetting(ReaderView.PROP_PAGE_VIEW_MODE, "0");
		}
		pageModeSet = true;
	}
	
	private void restoreReaderMode()
	{
		if ( changedPageMode ) {
			mReaderView.setSetting(ReaderView.PROP_PAGE_VIEW_MODE, "1");
		}
	}
	
	private void changeSelectionBound(boolean start, int delta) {
		L.d("changeSelectionBound(" + (start?"start":"end") + ", " + delta + ")");
		setReaderMode();
		ReaderCommand cmd = start ? ReaderCommand.DCMD_SELECT_MOVE_LEFT_BOUND_BY_WORDS : ReaderCommand.DCMD_SELECT_MOVE_RIGHT_BOUND_BY_WORDS; 
		mReaderView.moveSelection(cmd, delta, new ReaderView.MoveSelectionCallback() {
			
			@Override
			public void onNewSelection(Selection selection) {
				Log.d("cr3", "onNewSelection: " + selection.text);
				SelectionToolbarDlg.this.selection = selection;
			}
			
			@Override
			public void onFail() {
				Log.d("cr3", "fail()");
				//currentSelection = null;
			}
		});
	}
	
	private final static int SELECTION_CONTROL_STEP = 10; 
	private class BoundControlListener implements OnSeekBarChangeListener {

		public BoundControlListener(SeekBar sb, boolean start) {
			this.start = start;
			this.sb = sb;
			sb.setOnSeekBarChangeListener(this);
		}
		final boolean start;
		final SeekBar sb;
		int lastProgress = 50;
		
		@Override
		public void onStopTrackingTouch(SeekBar seekBar) {
			sb.setProgress(50);
			lastProgress = 50;
		}
		
		@Override
		public void onStartTrackingTouch(SeekBar seekBar) {
			sb.setProgress(50);
		}
		
		@Override
		public void onProgressChanged(SeekBar seekBar, int progress,
				boolean fromUser) {
			if (!fromUser)
				return;
			int diff = (progress - lastProgress) / SELECTION_CONTROL_STEP * SELECTION_CONTROL_STEP;
			if (diff!=0) {
				lastProgress += diff;
				changeSelectionBound(start, diff/SELECTION_CONTROL_STEP);
			}
		}
	};
	
	private void closeDialog(boolean clearSelection) {
		if (clearSelection)
			mReaderView.clearSelection();
		restoreReaderMode();
		mWindow.dismiss();
	}
	
	public SelectionToolbarDlg(CoolReader coolReader, ReaderView readerView, Selection sel )
	{
		this.selection = sel;
		mCoolReader = coolReader;
		mReaderView = readerView;
		mAnchor = readerView.getSurface();

		View panel = (LayoutInflater.from(coolReader.getApplicationContext()).inflate(R.layout.selection_toolbar, null));
		panel.measure(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
		
		//mReaderView.getS
		
		mWindow = new PopupWindow( mAnchor.getContext() );
		mWindow.setTouchInterceptor(new OnTouchListener() {
			
			@Override
			public boolean onTouch(View v, MotionEvent event) {
				if ( event.getAction()==MotionEvent.ACTION_OUTSIDE ) {
					closeDialog(true);
					return true;
				}
				return false;
			}
		});
		//super(panel);
		mPanel = panel;
		mPanel.findViewById(R.id.selection_copy).setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				mReaderView.copyToClipboard(selection.text);
				closeDialog(true);
			}
		});
		mPanel.findViewById(R.id.selection_dict).setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				mCoolReader.findInDictionary( selection.text );
				closeDialog(!mReaderView.getSettings().getBool(ReaderView.PROP_APP_SELECTION_PERSIST, false));
			}
		});
		mPanel.findViewById(R.id.selection_bookmark).setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				mReaderView.showNewBookmarkDialog(selection);
				closeDialog(true);
			}
		});
		mPanel.findViewById(R.id.selection_email).setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				mReaderView.sendQuotationInEmail(selection);
				closeDialog(true);
			}
		});
		mPanel.findViewById(R.id.selection_find).setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				mReaderView.showSearchDialog(selection.text.trim());
				closeDialog(true);
			}
		});
		mPanel.findViewById(R.id.selection_cancel).setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				closeDialog(true);
			}
		});
		new BoundControlListener((SeekBar)mPanel.findViewById(R.id.selection_left_bound_control), true);
		new BoundControlListener((SeekBar)mPanel.findViewById(R.id.selection_right_bound_control), false);
		mPanel.setFocusable(true);
		mPanel.setOnKeyListener( new OnKeyListener() {

			public boolean onKey(View v, int keyCode, KeyEvent event) {
				if ( event.getAction()==KeyEvent.ACTION_UP ) {
					switch ( keyCode ) {
					case KeyEvent.KEYCODE_BACK:
						closeDialog(true);
						return true;
//					case KeyEvent.KEYCODE_DPAD_LEFT:
//					case KeyEvent.KEYCODE_DPAD_UP:
//						//mReaderView.findNext(pattern, true, caseInsensitive);
//						return true;
//					case KeyEvent.KEYCODE_DPAD_RIGHT:
//					case KeyEvent.KEYCODE_DPAD_DOWN:
//						//mReaderView.findNext(pattern, false, caseInsensitive);
//						return true;
					}
				} else if ( event.getAction()==KeyEvent.ACTION_DOWN ) {
						switch ( keyCode ) {
//						case KeyEvent.KEYCODE_BACK:
//						case KeyEvent.KEYCODE_DPAD_LEFT:
//						case KeyEvent.KEYCODE_DPAD_UP:
//						case KeyEvent.KEYCODE_DPAD_RIGHT:
//						case KeyEvent.KEYCODE_DPAD_DOWN:
//							return true;
						}
					}
				if ( keyCode == KeyEvent.KEYCODE_BACK) {
					return true;
				}
				return false;
			}
			
		});

		mWindow.setOnDismissListener(new OnDismissListener() {
			@Override
			public void onDismiss() {
				restoreReaderMode();
				mReaderView.clearSelection();
			}
		});
		
		mWindow.setBackgroundDrawable(new BitmapDrawable());
		//mWindow.setAnimationStyle(android.R.style.Animation_Toast);
		mWindow.setWidth(WindowManager.LayoutParams.WRAP_CONTENT);
		mWindow.setHeight(WindowManager.LayoutParams.WRAP_CONTENT);
//		setWidth(panel.getWidth());
//		setHeight(panel.getHeight());
		
		mWindow.setFocusable(true);
		mWindow.setTouchable(true);
		mWindow.setOutsideTouchable(true);
		mWindow.setContentView(panel);
		
		
		int [] location = new int[2];
		mAnchor.getLocationOnScreen(location);
		//mWindow.update(location[0], location[1], mPanel.getWidth(), mPanel.getHeight() );
		//mWindow.setWidth(mPanel.getWidth());
		//mWindow.setHeight(mPanel.getHeight());

		int popupY = location[1] + mAnchor.getHeight() - mPanel.getHeight();
		mWindow.showAtLocation(mAnchor, Gravity.TOP | Gravity.CENTER_HORIZONTAL, location[0], popupY);
//		if ( mWindow.isShowing() )
//			mWindow.update(mAnchor, 50, 50);
		//dlg.mWindow.showAsDropDown(dlg.mAnchor);
		int y = sel.startY;
		if (y > sel.endY)
			y = sel.endY;
		int maxy = mReaderView.getSurface().getHeight() * 4 / 5; 
		if (y > maxy) {
			setReaderMode(); // selection is overlapped by toolbar: set scroll mode and move
			BackgroundThread.instance().postGUI(new Runnable() {
				@Override
				public void run() {
					//mReaderView.doEngineCommand(ReaderCommand.DCMD_REQUEST_RENDER, 0);
					BackgroundThread.instance().postBackground(new Runnable() {
						@Override
						public void run() {
							BackgroundThread.instance().postGUI(new Runnable() {
								@Override
								public void run() {
									mReaderView.doEngineCommand(ReaderCommand.DCMD_SCROLL_BY, mReaderView.getSurface().getHeight() / 3);
									mReaderView.redraw();
								}
							});
						}
					});
				}
			});
		}
	}
	
}
