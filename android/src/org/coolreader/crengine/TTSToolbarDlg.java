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

public class TTSToolbarDlg {
	PopupWindow mWindow;
	View mAnchor;
	CoolReader mCoolReader;
	ReaderView mReaderView;
	View mPanel;
	TTS mTTS;
	static public void showDialog( CoolReader coolReader, ReaderView readerView, TTS tts)
	{
		TTSToolbarDlg dlg = new TTSToolbarDlg(coolReader, readerView, tts);
		//dlg.mWindow.update(dlg.mAnchor, width, height)
		Log.d("cr3", "popup: " + dlg.mWindow.getWidth() + "x" + dlg.mWindow.getHeight());
		//dlg.update();
		//dlg.showAtLocation(readerView, Gravity.LEFT|Gravity.TOP, readerView.getLeft()+50, readerView.getTop()+50);
		//dlg.showAsDropDown(readerView);
		//dlg.update();
	}

	private boolean closed; 
	public void stopAndClose() {
		if (closed)
			return;
		closed = true;
		BackgroundThread.instance().executeGUI(new Runnable() {
			@Override
			public void run() {
				if ( mTTS!=null )
					mTTS.stop();
				mReaderView.clearSelection();
				mWindow.dismiss();
			}
		});
	}
	
	public TTSToolbarDlg( CoolReader coolReader, ReaderView readerView, TTS tts )
	{
		mCoolReader = coolReader;
		mReaderView = readerView;
		mAnchor = readerView;
		mTTS = tts;

		View panel = (LayoutInflater.from(coolReader.getApplicationContext()).inflate(R.layout.tts_toolbar, null));
		panel.measure(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
		
		//mReaderView.getS
		
		mWindow = new PopupWindow( mAnchor.getContext() );
		mWindow.setTouchInterceptor(new OnTouchListener() {
			
			@Override
			public boolean onTouch(View v, MotionEvent event) {
				if ( event.getAction()==MotionEvent.ACTION_OUTSIDE ) {
					stopAndClose();
					return true;
				}
				return false;
			}
		});
		//super(panel);
		mPanel = panel;
		mPanel.findViewById(R.id.tts_play_pause).setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
//				mReaderView.copyToClipboard(selection.text);
//				mReaderView.clearSelection();
				mWindow.dismiss();
			}
		});
		mPanel.findViewById(R.id.tts_back).setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
//				mCoolReader.findInDictionary( sel.text );
//				mReaderView.clearSelection();
				mWindow.dismiss();
			}
		});
		mPanel.findViewById(R.id.tts_forward).setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
//				mReaderView.showNewBookmarkDialog(sel);
				mWindow.dismiss();
			}
		});
		mPanel.findViewById(R.id.tts_stop).setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				stopAndClose();
			}
		});
		mPanel.setFocusable(true);
		mPanel.setOnKeyListener( new OnKeyListener() {

			public boolean onKey(View v, int keyCode, KeyEvent event) {
				if ( event.getAction()==KeyEvent.ACTION_UP ) {
					switch ( keyCode ) {
					case KeyEvent.KEYCODE_BACK:
						mReaderView.clearSelection();
						mWindow.dismiss();
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

		mWindow.showAtLocation(mAnchor, Gravity.TOP | Gravity.CENTER_HORIZONTAL, location[0], location[1] + mAnchor.getHeight() - mPanel.getHeight());
//		if ( mWindow.isShowing() )
//			mWindow.update(mAnchor, 50, 50);
		//dlg.mWindow.showAsDropDown(dlg.mAnchor);
	
	}
	
}
