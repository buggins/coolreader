package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.PhoneStateReceiver;
import org.coolreader.R;
import org.coolreader.crengine.CRToolBar.OnActionHandler;
import org.coolreader.crengine.CRToolBar.OnOverflowHandler;
import org.coolreader.crengine.SettingsManager.DictInfo;
import org.coolreader.crengine.TTS.OnTTSCreatedListener;
import org.coolreader.donations.BillingService;
import org.coolreader.donations.BillingService.RequestPurchase;
import org.coolreader.donations.BillingService.RestoreTransactions;
import org.coolreader.donations.Consts;
import org.coolreader.donations.Consts.PurchaseState;
import org.coolreader.donations.Consts.ResponseCode;
import org.coolreader.donations.PurchaseObserver;
import org.coolreader.donations.ResponseHandler;
import org.koekak.android.ebookdownloader.SonyBookSelector;

import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

public class ReaderActivity extends BaseActivity {

	static class PositionIndicator extends View {

		private final int INDICATOR_HEIGHT = 8;
		
		private int color = 0;
		private int percent = 0;
		
		public PositionIndicator(ReaderActivity context) {
			super(context);
			setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
			this.color = 0x808080; //SettingsManager.instance(context).get().getColor(Settings.PROP_STATUS_FONT_COLOR, 0);
			//setBackgroundColor(0xC0404040);
		}
		
		public void onThemeChange(InterfaceTheme theme) {
			color = theme.getStatusTextColor();
			invalidate();
		}
		
		@Override
		protected void onDraw(Canvas canvas) {
			Paint readPaint = Utils.createSolidPaint(0xC0000000 | color);
			Paint unreadPaint = Utils.createSolidPaint(0x40000000 | color);
			int w = getWidth();
			int h = getHeight();
			int pos = percent;
			int x = w * pos / 10000;
			canvas.drawRect(new Rect(getLeft(), h/2 - 3, getLeft() + x, h/2 + 0), readPaint);
			canvas.drawRect(new Rect(getLeft() + x, h/2 - 3, getLeft() + w, h/2 + 0), unreadPaint);
		}

		@Override
		protected void onLayout(boolean changed, int left, int top, int right,
				int bottom) {
			super.onLayout(changed, left, top, right, bottom);
		}

		@Override
		protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
			int w = MeasureSpec.getSize(widthMeasureSpec);
			int h = INDICATOR_HEIGHT;
			setMeasuredDimension(w, h);
		}
		
		public void setPosition(int percent) {
			this.percent = percent;
			if (isShown())
				invalidate();
		}
	}
	
	static class StatusBar extends LinearLayout implements Settings {
		private ReaderActivity activity;
		private LinearLayout content;
		private TextView title;
		private TextView position;
		private PositionIndicator indicator;
		private int textSize = 14;
		private int color = 0;
		
		public void updateSettings(Properties props) {
			this.textSize = props.getInt(Settings.PROP_STATUS_FONT_SIZE, 16);
		}
		
		public StatusBar(ReaderActivity context) {
			super(context);
			this.activity = context;
			setOrientation(VERTICAL);
			
			this.color = 0x808080; //SettingsManager.instance(context).get().getColor(Settings.PROP_STATUS_FONT_COLOR, 0);
			
			LayoutInflater inflater = LayoutInflater.from(activity);
			content = (LinearLayout)inflater.inflate(R.layout.reader_status_bar, null);
			title = (TextView)content.findViewById(R.id.title);
			position = (TextView)content.findViewById(R.id.position);

			title.setText("Book Name - Book Author (series #1)");
			title.setTextSize(TypedValue.COMPLEX_UNIT_PX, textSize);
			title.setTextColor(0xFF000000 | color);

			position.setText("24/124  [35%]");
			position.setTextSize(TypedValue.COMPLEX_UNIT_PX, textSize);
			position.setTextColor(0xFF000000 | color);
			
			addView(content);
			indicator = new PositionIndicator(activity);
			addView(indicator);
			//content.addView(indicator);
			setBackgroundResource(context.getCurrentTheme().getReaderStatusBackground());
			updateSettings(SettingsManager.instance(context).get());
		}

		public void onThemeChanged(InterfaceTheme theme) {
			color = theme.getStatusTextColor();
			title.setTextColor(0xFF000000 | color);
			position.setTextColor(0xFF000000 | color);
			indicator.onThemeChange(theme);
			if (isShown())
				invalidate();
		}
		
		public void updateCurrentPositionStatus(FileInfo book, Bookmark position, PositionProperties props) {
			String title = "";
			if (book != null) {
				String authors = Utils.formatAuthors(book.authors);
				if (Utils.empty(book.title)) {
					if (Utils.empty(authors)) {
						title = book.getFileNameToDisplay();
					} else {
						title = authors;
					}
				} else {
					if (Utils.empty(authors)) {
						title = book.title;
					} else {
						title = book.title + " - " + authors;
					}
				}
				String pos = "";
				if (props != null) {
					pos = pos + props.pageNumber + " / " + props.pageCount;
				}
				if (position != null) {
					pos = pos + " "; 
					String percent = position.getPercent() > 0 ? Utils.formatPercent(position.getPercent()) : "0%";
					pos = pos + percent;
				}
				this.position.setText(pos);
			}
			this.title.setText(title);
			indicator.setPosition(position.getPercent());
		}
	
	}

	static class ReaderViewLayout extends FrameLayout implements Settings {
		private ReaderActivity activity;
		private ReaderView contentView;
		private StatusBar statusView;
		private CRToolBar toolbarView;
		private int statusBarLocation;
		private int toolbarLocation;
		private boolean hideToolbarInFullscren;
		private boolean fullscreen;
	
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
			requestLayout();
		}
		
		public boolean isToolbarVisible() {
			return toolbarLocation != VIEWER_TOOLBAR_NONE && (!fullscreen || !hideToolbarInFullscren);
		}
		
		public boolean isStatusbarVisible() {
			return statusBarLocation == VIEWER_STATUS_BOTTOM || statusBarLocation == VIEWER_STATUS_TOP;
		}
		
		public void updateSettings(Properties settings) {
			statusBarLocation = settings.getInt(PROP_STATUS_LOCATION, VIEWER_STATUS_TOP);
			toolbarLocation = settings.getInt(PROP_TOOLBAR_LOCATION, VIEWER_TOOLBAR_SHORT_SIDE);
			hideToolbarInFullscren = settings.getBool(PROP_TOOLBAR_HIDE_IN_FULLSCREEN, true);
			statusView.setVisibility(isStatusbarVisible() ? VISIBLE : GONE);
			toolbarView.setVisibility(isToolbarVisible() ? VISIBLE : GONE);
			requestLayout();
		}
		
		public void showMenu() {
			if (toolbarView.getVisibility() == VISIBLE)
				toolbarView.showOverflowMenu();
			else
				toolbarView.showAsPopup(this, new OnActionHandler() {
					@Override
					public boolean onActionSelected(ReaderAction item) {
						activity.getReaderView().onAction(item);
						return true;
					}
				}, new OnOverflowHandler() {
					@Override
					public boolean onOverflowActions(ArrayList<ReaderAction> actions) {
						activity.showActionsPopupMenu(actions, new OnActionHandler() {
							@Override
							public boolean onActionSelected(ReaderAction item) {
								activity.getReaderView().onAction(item);
								return true;
							}
						});
						return false;
					}
				});
		}
		
		public ReaderViewLayout(ReaderActivity context, ReaderView contentView) {
			super(context);
			this.activity = context;
			this.contentView = contentView;
			this.statusView = new StatusBar(context);
			this.toolbarView = new CRToolBar(context, ReaderAction.createList(new ReaderAction[] {
				ReaderAction.GO_BACK,
				ReaderAction.TOC,
				ReaderAction.GO_PAGE,
				ReaderAction.GO_PERCENT,
				ReaderAction.SEARCH,
				ReaderAction.OPTIONS,
				ReaderAction.BOOKMARKS,
				ReaderAction.FILE_BROWSER_ROOT,
				ReaderAction.FILE_BROWSER,
				ReaderAction.TTS_PLAY,
				ReaderAction.GO_FORWARD,
				ReaderAction.EXIT,
			}));
			this.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
			this.addView(contentView);
			this.addView(statusView);
			this.addView(toolbarView);
			updateFullscreen(activity.isFullscreen());
			onThemeChanged(activity.getCurrentTheme());
			updateSettings(SettingsManager.instance(context).get());
		}

		public void onThemeChanged(InterfaceTheme theme) {
			statusView.setBackgroundResource(theme.getReaderStatusBackground());
			toolbarView.setBackgroundResource(theme.getReaderToolbarBackground(toolbarView.isVertical()));
			toolbarView.onThemeChanged(theme);
			statusView.onThemeChanged(theme);
		}

		
		@Override
		protected void onLayout(boolean changed, int l, int t, int r, int b) {
			r -= l;
			b -= t;
			t = 0;
			l = 0;

			statusView.setVisibility(isStatusbarVisible() ? VISIBLE : GONE);
			toolbarView.setVisibility(isToolbarVisible() ? VISIBLE : GONE);
			
			boolean toolbarVisible = toolbarLocation != VIEWER_TOOLBAR_NONE && (!fullscreen || !hideToolbarInFullscren);
			boolean landscape = r > b;
			if (toolbarVisible) {
				int location = toolbarLocation;
				if (location == VIEWER_TOOLBAR_SHORT_SIDE)
					location = landscape ? VIEWER_TOOLBAR_LEFT : VIEWER_TOOLBAR_TOP;
				else if (location == VIEWER_TOOLBAR_LONG_SIDE)
					location = landscape ? VIEWER_TOOLBAR_TOP : VIEWER_TOOLBAR_LEFT;
				switch (location) {
				case VIEWER_TOOLBAR_LEFT:
					toolbarView.setBackgroundResource(activity.getCurrentTheme().getReaderToolbarBackground(true));
					toolbarView.layout(l, t, l + toolbarView.getMeasuredWidth(), b);
					l += toolbarView.getMeasuredWidth();
					break;
				case VIEWER_TOOLBAR_RIGHT:
					toolbarView.setBackgroundResource(activity.getCurrentTheme().getReaderToolbarBackground(true));
					toolbarView.layout(r - toolbarView.getMeasuredWidth(), t, r, b);
					r -= toolbarView.getMeasuredWidth();
					break;
				case VIEWER_TOOLBAR_TOP:
					toolbarView.setBackgroundResource(activity.getCurrentTheme().getReaderToolbarBackground(false));
					toolbarView.layout(l, t, r, t + toolbarView.getMeasuredHeight());
					t += toolbarView.getMeasuredHeight();
					break;
				case VIEWER_TOOLBAR_BOTTOM:
					toolbarView.setBackgroundResource(activity.getCurrentTheme().getReaderToolbarBackground(false));
					toolbarView.layout(l, b - toolbarView.getMeasuredHeight(), r, b);
					b -= toolbarView.getMeasuredHeight();
					break;
				}
			}
			if (statusBarLocation == VIEWER_STATUS_TOP) {
				statusView.layout(l, t, r, t + statusView.getMeasuredHeight());
				t += statusView.getMeasuredHeight();
			} else if (statusBarLocation == VIEWER_STATUS_BOTTOM) {
				statusView.layout(l, b - statusView.getMeasuredHeight(), r, b);
				b -= statusView.getMeasuredHeight();
			}
			contentView.layout(l, t, r, b);
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
			
			contentView.measure(MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, w), 
					MeasureSpec.makeMeasureSpec(MeasureSpec.AT_MOST, h));
		}

		@Override
		protected void onSizeChanged(int w, int h, int oldw, int oldh) {
			super.onSizeChanged(w, h, oldw, oldh);
			requestLayout();
		}
	}
	
	public ReaderView getReaderView() {
		return mReaderView;
	}
	
	private ReaderView mReaderView;
	private Engine mEngine;
	private ReaderViewLayout mFrame;
	
	
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
		
		mEngine = Engine.getInstance(this);
		mReaderView = new ReaderView(this, mEngine, SettingsManager.instance(this).get());
		mFrame = new ReaderViewLayout(this, mReaderView);
        mFrame.getToolBar().setOnActionHandler(new OnActionHandler() {
			@Override
			public boolean onActionSelected(ReaderAction item) {
				mReaderView.onAction(item);
				return true;
			}
		});

		//==========================================
		// Donations related code
		try {
	        mHandler = new Handler();
	        mPurchaseObserver = new CRPurchaseObserver(mHandler);
	        mBillingService = new BillingService();
	        mBillingService.setContext(this);
	
	        //mPurchaseDatabase = new PurchaseDatabase(this);
	
	        // Check if billing is supported.
	        ResponseHandler.register(mPurchaseObserver);
	        billingSupported = mBillingService.checkBillingSupported();
		} catch (VerifyError e) {
			log.e("Exception while trying to initialize billing service for donations");
		}
        if (!billingSupported) {
        	log.i("Billing is not supported");
        } else {
        	log.i("Billing is supported");
        }

		intentReceiver = new BroadcastReceiver() {

			@Override
			public void onReceive(Context context, Intent intent) {
				int level = intent.getIntExtra("level", 0);
				if ( mReaderView!=null )
					mReaderView.setBatteryState(level);
				else
					initialBatteryState = level;
			}
			
		};
		registerReceiver(intentReceiver, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
        
		setVolumeControlStream(AudioManager.STREAM_MUSIC);
		
		if ( initialBatteryState>=0 )
			mReaderView.setBatteryState(initialBatteryState);
		
		setContentView(mFrame);
		
	}

	public final static boolean CLOSE_BOOK_ON_STOP = false;
	
	@Override
	protected void onDestroy() {

		if ( !CLOSE_BOOK_ON_STOP )
			mReaderView.close();

		if ( tts!=null ) {
			tts.shutdown();
			tts = null;
			ttsInitialized = false;
			ttsError = false;
		}
		
		if ( intentReceiver!=null ) {
			unregisterReceiver(intentReceiver);
			intentReceiver = null;
		}
		
		mBillingService.unbind();
		Activities.setReader(null);

		if (mReaderView != null) {
			mReaderView.destroy();
		}
		mReaderView = null;
		super.onDestroy();
	}

	@Override
	protected void onNewIntent(Intent intent) {
		// TODO Auto-generated method stub
		super.onNewIntent(intent);
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

	@Override
	protected void onStart() {
		// Donations support code
		if (billingSupported)
			ResponseHandler.register(mPurchaseObserver);
		
		Intent intent = getIntent();
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

		PhoneStateReceiver.setPhoneActivityHandler(new Runnable() {
			@Override
			public void run() {
				if (mReaderView != null) {
					mReaderView.stopTTS();
					mReaderView.save();
				}
			}
		});

		super.onStart();
	}

	@Override
	protected void onStop() {
		// Donations support code
		if (billingSupported)
			ResponseHandler.unregister(mPurchaseObserver);
		super.onStop();
		if ( CLOSE_BOOK_ON_STOP )
			mReaderView.close();
	}

	@Override
    protected void setDimmingAlpha(int dimmingAlpha) {
    	mReaderView.setDimmingAlpha(dimmingAlpha);
    }




	public void findInDictionary( String s ) {
		if ( s!=null && s.length()!=0 ) {
			s = s.trim();
			for ( ;s.length()>0; ) {
				char ch = s.charAt(s.length()-1);
				if ( ch>=128 )
					break;
				if ( ch>='0' && ch<='9' || ch>='A' && ch<='Z' || ch>='a' && ch<='z' )
					break;
				s = s.substring(0, s.length()-1);
			}
			if ( s.length()>0 ) {
				//
				final String pattern = s;
				BackgroundThread.instance().postBackground(new Runnable() {
					@Override
					public void run() {
						BackgroundThread.instance().postGUI(new Runnable() {
							@Override
							public void run() {
								findInDictionaryInternal(pattern);
							}
						}, 100);
					}
				});
			}
		}
	}
	
	private final static int DICTAN_ARTICLE_REQUEST_CODE = 100;
	
	private final static String DICTAN_ARTICLE_WORD = "article.word";
	
	private final static String DICTAN_ERROR_MESSAGE = "error.message";

	private final static int FLAG_ACTIVITY_CLEAR_TASK = 0x00008000;
	
	private void findInDictionaryInternal(String s) {
		switch (currentDict.internal) {
		case 0:
			Intent intent0 = new Intent(currentDict.action).setComponent(new ComponentName(
				currentDict.packageName, currentDict.className
				)).addFlags(DeviceInfo.getSDKLevel() >= 7 ? FLAG_ACTIVITY_CLEAR_TASK : Intent.FLAG_ACTIVITY_NEW_TASK);
			if (s!=null)
				intent0.putExtra(currentDict.dataKey, s);
			try {
				startActivity( intent0 );
			} catch ( ActivityNotFoundException e ) {
				showToast("Dictionary \"" + currentDict.name + "\" is not installed");
			}
			break;
		case 1:
			final String SEARCH_ACTION  = "colordict.intent.action.SEARCH";
			final String EXTRA_QUERY   = "EXTRA_QUERY";
			final String EXTRA_FULLSCREEN = "EXTRA_FULLSCREEN";
			final String EXTRA_HEIGHT  = "EXTRA_HEIGHT";
			final String EXTRA_WIDTH   = "EXTRA_WIDTH";
			final String EXTRA_GRAVITY  = "EXTRA_GRAVITY";
			final String EXTRA_MARGIN_LEFT = "EXTRA_MARGIN_LEFT";
			final String EXTRA_MARGIN_TOP  = "EXTRA_MARGIN_TOP";
			final String EXTRA_MARGIN_BOTTOM = "EXTRA_MARGIN_BOTTOM";
			final String EXTRA_MARGIN_RIGHT = "EXTRA_MARGIN_RIGHT";

			Intent intent1 = new Intent(SEARCH_ACTION);
			if (s!=null)
				intent1.putExtra(EXTRA_QUERY, s); //Search Query
			intent1.putExtra(EXTRA_FULLSCREEN, true); //
			try
			{
				startActivity(intent1);
			} catch ( ActivityNotFoundException e ) {
				showToast("Dictionary \"" + currentDict.name + "\" is not installed");
			}
			break;
		case 2:
			// Dictan support
			Intent intent2 = new Intent("android.intent.action.VIEW");
			// Add custom category to run the Dictan external dispatcher
            intent2.addCategory("info.softex.dictan.EXTERNAL_DISPATCHER");
            
   	        // Don't include the dispatcher in activity  
            // because it doesn't have any content view.	      
            intent2.setFlags(Intent.FLAG_ACTIVITY_NO_HISTORY);
		  
	        intent2.putExtra(DICTAN_ARTICLE_WORD, s);
			  
	        try {
	        	startActivityForResult(intent2, DICTAN_ARTICLE_REQUEST_CODE);
	        } catch (ActivityNotFoundException e) {
				showToast("Dictionary \"" + currentDict.name + "\" is not installed");
	        }
			break;
		}
	}

	public void showDictionary() {
		findInDictionaryInternal(null);
	}
	
	@Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent) {
        if (requestCode == DICTAN_ARTICLE_REQUEST_CODE) {
	       	switch (resultCode) {
	        	
	        	// The article has been shown, the intent is never expected null
			case RESULT_OK:
				break;
					
			// Error occured
			case RESULT_CANCELED: 
				String errMessage = "Unknown Error.";
				if (intent != null) {
					errMessage = "The Requested Word: " + 
					intent.getStringExtra(DICTAN_ARTICLE_WORD) + 
					". Error: " + intent.getStringExtra(DICTAN_ERROR_MESSAGE);
				}
				showToast(errMessage);
				break;
					
			// Must never occur
			default: 
				showToast("Unknown Result Code: " + resultCode);
				break;
			}
        }
    }
	
	private DictInfo currentDict = SettingsManager.getDictList()[0];
	
	public void setDict( String id ) {
		for ( DictInfo d : SettingsManager.getDictList() ) {
			if ( d.id.equals(id) ) {
				currentDict = d;
				return;
			}
		}
	}

	public void sendBookFragment(BookInfo bookInfo, String text) {
        final Intent emailIntent = new Intent(android.content.Intent.ACTION_SEND);
        emailIntent.setType("text/plain");
    	emailIntent.putExtra(android.content.Intent.EXTRA_SUBJECT, bookInfo.getFileInfo().getAuthors() + " " + bookInfo.getFileInfo().getTitle());
        emailIntent.putExtra(android.content.Intent.EXTRA_TEXT, text);
		startActivity(Intent.createChooser(emailIntent, null));	
	}


	public void openURL(String url) {
		try {
			Intent i = new Intent(Intent.ACTION_VIEW);  
			i.setData(Uri.parse(url));  
			startActivity(i);
		} catch (Exception e) {
			log.e("Exception " + e + " while trying to open URL " + url);
			showToast("Cannot open URL " + url);
		}
	}

	
	
	public void showAboutDialog() {
		AboutDialog dlg = new AboutDialog(this);
		dlg.show();
	}
	

	
	
	private AudioManager am;
	private int maxVolume;
	public AudioManager getAudioManager() {
		if ( am==null ) {
			am = (AudioManager)getSystemService(AUDIO_SERVICE);
			maxVolume = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
		}
		return am;
	}
	
	public int getVolume() {
		AudioManager am = getAudioManager();
		if (am!=null) {
			return am.getStreamVolume(AudioManager.STREAM_MUSIC) * 100 / maxVolume;
		}
		return 0;
	}
	
	public void setVolume( int volume ) {
		AudioManager am = getAudioManager();
		if (am!=null) {
			am.setStreamVolume(AudioManager.STREAM_MUSIC, volume * maxVolume / 100, 0);
		}
	}
	

	
	TTS tts;
	boolean ttsInitialized;
	boolean ttsError;
	
	public boolean initTTS(final OnTTSCreatedListener listener) {
		if ( ttsError || !TTS.isFound() ) {
			if ( !ttsError ) {
				ttsError = true;
				showToast("TTS is not available");
			}
			return false;
		}
		if ( ttsInitialized && tts!=null ) {
			BackgroundThread.instance().executeGUI(new Runnable() {
				@Override
				public void run() {
					listener.onCreated(tts);
				}
			});
			return true;
		}
		if ( ttsInitialized && tts!=null ) {
			showToast("TTS initialization is already called");
			return false;
		}
		showToast("Initializing TTS");
    	tts = new TTS(this, new TTS.OnInitListener() {
			@Override
			public void onInit(int status) {
				//tts.shutdown();
				L.i("TTS init status: " + status);
				if ( status==TTS.SUCCESS ) {
					ttsInitialized = true;
					BackgroundThread.instance().executeGUI(new Runnable() {
						@Override
						public void run() {
							listener.onCreated(tts);
						}
					});
				} else {
					ttsError = true;
					BackgroundThread.instance().executeGUI(new Runnable() {
						@Override
						public void run() {
							showToast("Cannot initialize TTS");
						}
					});
				}
			}
		});
		return true;
	}
	
	
	public void showOptionsDialog(final OptionsDialog.Mode mode)
	{
		BackgroundThread.instance().postBackground(new Runnable() {
			public void run() {
				final String[] mFontFaces = Engine.getFontFaceList();
				BackgroundThread.instance().executeGUI(new Runnable() {
					public void run() {
						OptionsDialog dlg = new OptionsDialog(ReaderActivity.this, mReaderView, mFontFaces, mode);
						dlg.show();
					}
				});
			}
		});
	}
	

	public void showBookmarksDialog()
	{
		BackgroundThread.instance().executeGUI(new Runnable() {
			@Override
			public void run() {
				BookmarksDlg dlg = new BookmarksDlg(ReaderActivity.this, mReaderView);
				dlg.show();
			}
		});
	}
	
	
	
	@Override
	public void applyAppSetting( String key, String value )
	{
		super.applyAppSetting(key, value);
		boolean flg = "1".equals(value);
        if ( key.equals(PROP_APP_KEY_BACKLIGHT_OFF) ) {
			setKeyBacklightDisabled(flg);
        } else if ( key.equals(PROP_APP_SCREEN_BACKLIGHT_LOCK) ) {
        	int n = 0;
        	try {
        		n = Integer.parseInt(value);
        	} catch (NumberFormatException e) {
        		// ignore
        	}
			setScreenBacklightDuration(n);
        } else if ( key.equals(PROP_APP_DICTIONARY) ) {
        	setDict(value);
        }
        //
	}
	
	

	//==============================================================
	// 
	// Donations related code
	// (from Dungeons sample) 
    private static final int DIALOG_CANNOT_CONNECT_ID = 1;
    private static final int DIALOG_BILLING_NOT_SUPPORTED_ID = 2;
    /**
     * Used for storing the log text.
     */
    private static final String LOG_TEXT_KEY = "DUNGEONS_LOG_TEXT";

    /**
     * The SharedPreferences key for recording whether we initialized the
     * database.  If false, then we perform a RestoreTransactions request
     * to get all the purchases for this user.
     */
    private static final String DB_INITIALIZED = "db_initialized";

	
    /**
     * Each product in the catalog is either MANAGED or UNMANAGED.  MANAGED
     * means that the product can be purchased only once per user (such as a new
     * level in a game). The purchase is remembered by Android Market and
     * can be restored if this application is uninstalled and then
     * re-installed. UNMANAGED is used for products that can be used up and
     * purchased multiple times (such as poker chips). It is up to the
     * application to keep track of UNMANAGED products for the user.
     */
    private enum Managed { MANAGED, UNMANAGED }

    private CRPurchaseObserver mPurchaseObserver;
    private BillingService mBillingService;
    private Handler mHandler;
    private DonationListener mDonationListener = null;
    private boolean billingSupported = false;
    private double mTotalDonations = 0;
    
    public boolean isDonationSupported() {
    	return billingSupported;
    }
    public void setDonationListener(DonationListener listener) {
    	mDonationListener = listener;
    }
    public static interface DonationListener {
    	void onDonationTotalChanged(double total);
    }
    public double getTotalDonations() {
    	return mTotalDonations;
    }
    public boolean makeDonation(double amount) {
		final String itemName = "donation" + (amount >= 1 ? String.valueOf((int)amount) : String.valueOf(amount));
    	log.i("makeDonation is called, itemName=" + itemName);
    	if (!billingSupported)
    		return false;
    	String mPayloadContents = null;
    	String mSku = itemName;
        if (!mBillingService.requestPurchase(mSku, mPayloadContents)) {
        	showToast("Purchase is failed");
        }
    	return true;
    }
    

	private static String DONATIONS_PREF_FILE = "cr3donations";
	private static String DONATIONS_PREF_TOTAL_AMOUNT = "total";
    /**
     * A {@link PurchaseObserver} is used to get callbacks when Android Market sends
     * messages to this application so that we can update the UI.
     */
    private class CRPurchaseObserver extends PurchaseObserver {
    	
    	private String TAG = "cr3Billing";
        public CRPurchaseObserver(Handler handler) {
            super(ReaderActivity.this, handler);
        }

        @Override
        public void onBillingSupported(boolean supported) {
            if (Consts.DEBUG) {
                Log.i(TAG, "supported: " + supported);
            }
            if (supported) {
            	billingSupported = true;
        		SharedPreferences pref = getSharedPreferences(DONATIONS_PREF_FILE, 0);
        		try {
        			mTotalDonations = pref.getFloat(DONATIONS_PREF_TOTAL_AMOUNT, 0.0f);
        		} catch (Exception e) {
        			log.e("exception while reading total donations from preferences", e);
        		}
            	// TODO:
//                restoreDatabase();
            }
        }

        @Override
        public void onPurchaseStateChange(PurchaseState purchaseState, String itemId,
                int quantity, long purchaseTime, String developerPayload) {
            if (Consts.DEBUG) {
                Log.i(TAG, "onPurchaseStateChange() itemId: " + itemId + " " + purchaseState);
            }

            if (developerPayload == null) {
                logProductActivity(itemId, purchaseState.toString());
            } else {
                logProductActivity(itemId, purchaseState + "\n\t" + developerPayload);
            }

            if (purchaseState == PurchaseState.PURCHASED) {
            	double amount = 0;
            	try {
	            	if (itemId.startsWith("donation"))
	            		amount = Double.parseDouble(itemId.substring(8));
            	} catch (NumberFormatException e) {
            		//
            	}

            	mTotalDonations += amount;
        		SharedPreferences pref = getSharedPreferences(DONATIONS_PREF_FILE, 0);
        		pref.edit().putString(DONATIONS_PREF_TOTAL_AMOUNT, String.valueOf(mTotalDonations)).commit();

            	if (mDonationListener != null)
            		mDonationListener.onDonationTotalChanged(mTotalDonations);
                //mOwnedItems.add(itemId);
            }
//            mCatalogAdapter.setOwnedItems(mOwnedItems);
//            mOwnedItemsCursor.requery();
        }

        @Override
        public void onRequestPurchaseResponse(RequestPurchase request,
                ResponseCode responseCode) {
            if (Consts.DEBUG) {
                Log.d(TAG, request.mProductId + ": " + responseCode);
            }
            if (responseCode == ResponseCode.RESULT_OK) {
                if (Consts.DEBUG) {
                    Log.i(TAG, "purchase was successfully sent to server");
                }
                logProductActivity(request.mProductId, "sending purchase request");
            } else if (responseCode == ResponseCode.RESULT_USER_CANCELED) {
                if (Consts.DEBUG) {
                    Log.i(TAG, "user canceled purchase");
                }
                logProductActivity(request.mProductId, "dismissed purchase dialog");
            } else {
                if (Consts.DEBUG) {
                    Log.i(TAG, "purchase failed");
                }
                logProductActivity(request.mProductId, "request purchase returned " + responseCode);
            }
        }

        @Override
        public void onRestoreTransactionsResponse(RestoreTransactions request,
                ResponseCode responseCode) {
            if (responseCode == ResponseCode.RESULT_OK) {
                if (Consts.DEBUG) {
                    Log.d(TAG, "completed RestoreTransactions request");
                }
                // Update the shared preferences so that we don't perform
                // a RestoreTransactions again.
                SharedPreferences prefs = getPreferences(Context.MODE_PRIVATE);
                SharedPreferences.Editor edit = prefs.edit();
                edit.putBoolean(DB_INITIALIZED, true);
                edit.commit();
            } else {
                if (Consts.DEBUG) {
                    Log.d(TAG, "RestoreTransactions error: " + responseCode);
                }
            }
        }
    }
    private void logProductActivity(String product, String activity) {
    	// TODO: some logging
    	Log.i(LOG_TEXT_KEY, activity);
    }

	int initialBatteryState = -1;
	BroadcastReceiver intentReceiver;

	public void updateCurrentPositionStatus(FileInfo book, Bookmark position, PositionProperties props) {
		mFrame.getStatusBar().updateCurrentPositionStatus(book, position, props);
	}

	@Override
	public void setFullscreen( boolean fullscreen )
	{
		super.setFullscreen(fullscreen);
		if (mFrame != null)
			mFrame.updateFullscreen(fullscreen);
	}
	
	public void onSettingsChanged(Properties props) {
		if (mFrame != null)
			mFrame.updateSettings(props);
	}
	
	public void showMenu() {
		//
		if (mFrame != null) {
			mFrame.showMenu();
		}
	}
}
