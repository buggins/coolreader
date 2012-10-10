package org.coolreader.crengine;


import org.coolreader.PhoneStateReceiver;
import org.coolreader.crengine.CRToolBar.OnActionHandler;
import org.coolreader.crengine.TTS.OnTTSCreatedListener;
import org.coolreader.donations.BillingService;
import org.coolreader.donations.ResponseHandler;
import org.koekak.android.ebookdownloader.SonyBookSelector;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;

public class ReaderActivity extends BaseActivity {

	static final Logger log = L.create("ra");
	
	private ReaderView mReaderView;
	private ReaderViewLayout mReaderFrame;
	
	
	private boolean justCreated = false; 
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Activities.setReader(this);
		
		super.onCreate(savedInstanceState);

		waitForCRDBService(new Runnable() {
			@Override
			public void run() {
				// TO do on DB ready
			}
		});

		justCreated = true;
		

		
		setContentView(mReaderFrame);
		mReaderFrame.bringToFront();
	}

	@Override
	protected void onDestroy() {

		Activities.setReader(null);

		if (mReaderView != null) {
			mReaderView.destroy();
		}
		mReaderView = null;
		super.onDestroy();
	}

	@Override
	protected void onNewIntent(Intent intent) {
		super.onNewIntent(intent);
		log.v("onNewIntent()");
		processIntent(intent);
	}

	@Override
	protected void onPause() {
		super.onPause();
		log.i("CoolReader.onPause() : saving reader state");
		mReaderView.onAppPause();
	}

	@Override
	protected void onResume() {
		log.i("CoolReader.onResume()");
		super.onResume();
		Properties props = mReaderView.getSettings();
		
		if (DeviceInfo.EINK_SCREEN) {
            if (DeviceInfo.EINK_SONY) {
                SharedPreferences pref = getSharedPreferences(PREF_FILE, 0);
                String res = pref.getString(PREF_LAST_BOOK, null);
                if( res != null && res.length() > 0 ) {
                    SonyBookSelector selector = new SonyBookSelector(this);
                    long l = selector.getContentId(res);
                    if(l != 0) {
                       selector.setReadingTime(l);
                       selector.requestBookSelection(l);
                    }
                }
            }
		}
//		BackgroundThread.instance().postGUI(new Runnable() {
//			@Override
//			public void run() {
//				if (mFrame != null) {
//					log.v("requesting layout");
//					mFrame.requestLayout();
//					mFrame.getToolBar().invalidate();
//				}
//			}
//		}, 400);
	}
	
	@Override
	protected void onPostResume() {
		// TODO Auto-generated method stub
		super.onPostResume();
	}

	@Override
	protected void onRestart() {
		// TODO Auto-generated method stub
		super.onRestart();
	}
	
	private void processIntent(Intent intent) {
		log.d("intent=" + intent);
		if (intent != null) {
			final String fileToOpen = intent.getExtras().getString(Activities.OPEN_FILE_PARAM);
			if (fileToOpen != null) {
				log.d("FILE_TO_OPEN = " + fileToOpen);
				waitForCRDBService(new Runnable() {
					@Override
					public void run() {
						mReaderView.loadDocument(fileToOpen, null);
					}
				});
			}
		}
	}

	@Override
	protected void onStart() {
		super.onStart();

		if (justCreated) {
			justCreated = false;
			processIntent(getIntent());
		}
		
//		BackgroundThread.instance().postGUI(new Runnable() {
//
//			@Override
//			public void run() {
//				log.v("Invalidating toolbar ***************");
//				mFrame.getToolBar().invalidate();
//				mFrame.requestLayout();
//				//mFrame.invalidate();
//			}
//		}, 10000);

		PhoneStateReceiver.setPhoneActivityHandler(new Runnable() {
			@Override
			public void run() {
				if (mReaderView != null) {
					mReaderView.stopTTS();
					mReaderView.save();
				}
			}
		});
	}

	@Override
	protected void onStop() {
		super.onStop();
	}





	
	

	
	

	
	

	
	
	

	@Override
	protected boolean wantHideNavbarInFullscreen() {
		return true;
	}

}
