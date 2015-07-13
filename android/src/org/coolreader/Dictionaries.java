package org.coolreader;

import org.coolreader.crengine.DeviceInfo;
import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.SearchManager;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.PackageManager;

public class Dictionaries {

	private Activity mActivity;
	
	public Dictionaries(Activity activity) {
		mActivity = activity;
		currentDictionary = defaultDictionary();
	}
	
	DictInfo currentDictionary;
	
	public static class DictInfo {
		public final String id; 
		public final String name;
		public final String packageName;
		public final String className;
		public final String action;
		public final Integer internal;
		public String dataKey = SearchManager.QUERY; 
		public DictInfo ( String id, String name, String packageName, String className, String action, Integer internal ) {
			this.id = id;
			this.name = name;
			this.packageName = packageName;
			this.className = className;
			this.action = action;
			this.internal = internal;
		}
		public DictInfo setDataKey(String key) { this.dataKey = key; return this; }
	}

	static final DictInfo dicts[] = {
		new DictInfo("Fora", "Fora Dictionary", "com.ngc.fora", "com.ngc.fora.ForaDictionary", Intent.ACTION_SEARCH, 0),
		new DictInfo("ColorDict", "ColorDict", "com.socialnmobile.colordict", "com.socialnmobile.colordict.activity.Main", Intent.ACTION_SEARCH, 0),
		new DictInfo("ColorDictApi", "ColorDict new / GoldenDict", "com.socialnmobile.colordict", "com.socialnmobile.colordict.activity.Main", Intent.ACTION_SEARCH, 1),
		new DictInfo("AardDict", "Aard Dictionary", "aarddict.android", "aarddict.android.Article", Intent.ACTION_SEARCH, 0),
		new DictInfo("AardDictLookup", "Aard Dictionary Lookup", "aarddict.android", "aarddict.android.Lookup", Intent.ACTION_SEARCH, 0),
		new DictInfo("Aard2", "Aard 2 Dictionary", "itkach.aard2", "aard2.lookup", Intent.ACTION_SEARCH, 3),
		new DictInfo("Dictan", "Dictan Dictionary", "info.softex.dictan", null, Intent.ACTION_VIEW, 2),
		new DictInfo("FreeDictionary.org", "Free Dictionary . org", "org.freedictionary", "org.freedictionary.MainActivity", "android.intent.action.VIEW", 0),
		new DictInfo("ABBYYLingvo", "ABBYY Lingvo", "com.abbyy.mobile.lingvo.market", null /*com.abbyy.mobile.lingvo.market.MainActivity*/, "com.abbyy.mobile.lingvo.intent.action.TRANSLATE", 0).setDataKey("com.abbyy.mobile.lingvo.intent.extra.TEXT"),
		//new DictInfo("ABBYYLingvoLive", "ABBYY Lingvo Live", "com.abbyy.mobile.lingvolive", null, "com.abbyy.mobile.lingvo.intent.action.TRANSLATE", 0).setDataKey("com.abbyy.mobile.lingvo.intent.extra.TEXT"),
		new DictInfo("LingoQuizLite", "Lingo Quiz Lite", "mnm.lite.lingoquiz", "mnm.lite.lingoquiz.ExchangeActivity", "lingoquiz.intent.action.ADD_WORD", 0).setDataKey("EXTRA_WORD"),
		new DictInfo("LingoQuiz", "Lingo Quiz", "mnm.lingoquiz", "mnm.lingoquiz.ExchangeActivity", "lingoquiz.intent.action.ADD_WORD", 0).setDataKey("EXTRA_WORD"),
		new DictInfo("LEODictionary", "LEO Dictionary", "org.leo.android.dict", "org.leo.android.dict.LeoDict", "android.intent.action.SEARCH", 0).setDataKey("query"),
		new DictInfo("PopupDictionary", "Popup Dictionary", "com.barisatamer.popupdictionary", "com.barisatamer.popupdictionary.MainActivity", "android.intent.action.VIEW", 0),
	};

	public static final String DEFAULT_DICTIONARY_ID = "com.ngc.fora";
	
	static DictInfo findById(String id) {
		for(DictInfo d: dicts) {
			if (d.id.equals(id))
				return d;
		}
		return null;
	}
	
	static DictInfo defaultDictionary() {
		return findById(DEFAULT_DICTIONARY_ID);
	}
		
	
	public static DictInfo[] getDictList() {
		return dicts;
	}

	public void setDict( String id ) {
		DictInfo d = findById(id);
		if (d != null)
			currentDictionary = d;
	}
	
	public boolean isPackageInstalled(String packageName) {
        PackageManager pm = mActivity.getPackageManager();
        try
        {
            pm.getPackageInfo(packageName, 0); //PackageManager.GET_ACTIVITIES);
            return true;
        }
        catch (PackageManager.NameNotFoundException e)
        {
            return false;
        }
    }

	private final static int DICTAN_ARTICLE_REQUEST_CODE = 100;
	
	private final static String DICTAN_ARTICLE_WORD = "article.word";
	
	private final static String DICTAN_ERROR_MESSAGE = "error.message";

	private final static int FLAG_ACTIVITY_CLEAR_TASK = 0x00008000;
	
	public static final Logger log = L.create("cr3dict");
	
	@SuppressWarnings("serial")
	public static class DictionaryException extends Exception {
		public DictionaryException(String msg) {
			super(msg);
		}
	}
	
	@SuppressLint("NewApi")
	public void findInDictionary(String s) throws DictionaryException {
		log.d("lookup in dictionary: " + s);
		switch (currentDictionary.internal) {
		case 0:
			Intent intent0 = new Intent(currentDictionary.action);
			if (currentDictionary.className != null || DeviceInfo.getSDKLevel() == 3) {
				intent0.setComponent(new ComponentName(
					currentDictionary.packageName, currentDictionary.className));
			} else {
				intent0.setPackage(currentDictionary.packageName);
			}
			intent0.addFlags(DeviceInfo.getSDKLevel() >= 7 ? FLAG_ACTIVITY_CLEAR_TASK : Intent.FLAG_ACTIVITY_NEW_TASK);
			if (s!=null)
				intent0.putExtra(currentDictionary.dataKey, s);
			try {
				mActivity.startActivity( intent0 );
			} catch ( ActivityNotFoundException e ) {
				throw new DictionaryException("Dictionary \"" + currentDictionary.name + "\" is not installed");
			}
			break;
		case 1:
			final String SEARCH_ACTION  = "colordict.intent.action.SEARCH";
			final String EXTRA_QUERY   = "EXTRA_QUERY";
			final String EXTRA_FULLSCREEN = "EXTRA_FULLSCREEN";
//			final String EXTRA_HEIGHT  = "EXTRA_HEIGHT";
//			final String EXTRA_WIDTH   = "EXTRA_WIDTH";
//			final String EXTRA_GRAVITY  = "EXTRA_GRAVITY";
//			final String EXTRA_MARGIN_LEFT = "EXTRA_MARGIN_LEFT";
//			final String EXTRA_MARGIN_TOP  = "EXTRA_MARGIN_TOP";
//			final String EXTRA_MARGIN_BOTTOM = "EXTRA_MARGIN_BOTTOM";
//			final String EXTRA_MARGIN_RIGHT = "EXTRA_MARGIN_RIGHT";

			Intent intent1 = new Intent(SEARCH_ACTION);
			if (s!=null)
				intent1.putExtra(EXTRA_QUERY, s); //Search Query
			intent1.putExtra(EXTRA_FULLSCREEN, true); //
			try
			{
				mActivity.startActivity(intent1);
			} catch ( ActivityNotFoundException e ) {
				throw new DictionaryException("Dictionary \"" + currentDictionary.name + "\" is not installed");
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
	        	mActivity.startActivityForResult(intent2, DICTAN_ARTICLE_REQUEST_CODE);
	        } catch (ActivityNotFoundException e) {
				throw new DictionaryException("Dictionary \"" + currentDictionary.name + "\" is not installed");
	        }
			break;
		case 3:
			Intent intent3 = new Intent("aard2.lookup");
			intent3.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
			intent3.putExtra(SearchManager.QUERY, s);
			try
			{
				mActivity.startActivity(intent3);
			} catch ( ActivityNotFoundException e ) {
				throw new DictionaryException("Dictionary \"" + currentDictionary.name + "\" is not installed");
			}
			break;
		}
	}

    public void onActivityResult(int requestCode, int resultCode, Intent intent) throws DictionaryException {
        if (requestCode == DICTAN_ARTICLE_REQUEST_CODE) {
	       	switch (resultCode) {
	        	
	        	// The article has been shown, the intent is never expected null
			case Activity.RESULT_OK:
				break;
					
			// Error occured
			case Activity.RESULT_CANCELED: 
				String errMessage = "Unknown Error.";
				if (intent != null) {
					errMessage = "The Requested Word: " + 
					intent.getStringExtra(DICTAN_ARTICLE_WORD) + 
					". Error: " + intent.getStringExtra(DICTAN_ERROR_MESSAGE);
				}
				throw new DictionaryException(errMessage);
					
			// Must never occur
			default: 
				throw new DictionaryException("Unknown Result Code: " + resultCode);
			}
        }
	}
	
}
