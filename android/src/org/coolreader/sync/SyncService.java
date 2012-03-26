package org.coolreader.sync;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.text.SimpleDateFormat;
import java.util.Date;

import org.coolreader.crengine.Bookmark;
import org.coolreader.crengine.DeviceInfo;

import android.app.Service;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

public class SyncService extends Service {

	private final static String TAG = "cr3sync";
	
		
    @Override
    public void onCreate() {
    	Log.i(TAG, "onCreate()");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "Received start id " + startId + ": " + intent);
        // We want this service to continue running until it is explicitly
        // stopped, so return sticky.
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
    	Log.i(TAG, "onDestroy()");
    }

	/**
     * Class for clients to access.  Because we know this service always
     * runs in the same process as its clients, we don't need to deal with
     * IPC.
     */
    public class LocalBinder extends Binder {
        SyncService getService() {
            return SyncService.this;
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

	private static String PREF_FILE = "CR3Sync";
	private static String PREF_THIS_DEVICE_ID = "ThisDeviceId";
	private static String PREF_HELP_FILE = "HelpFile";
	private void readSettings()	{
		SharedPreferences pref = getSharedPreferences(PREF_FILE, 0);
		thisDeviceName = pref.getString(PREF_THIS_DEVICE_ID, null);
		if (thisDeviceName == null) {
			Log.i(TAG, "New device name generated: " + thisDeviceName);
			thisDeviceName = generateThisDeviceName();
			pref.edit().putString(PREF_THIS_DEVICE_ID, thisDeviceName);
		}
	}
	
    private String generateThisDeviceName() {
    	String name = DeviceInfo.MANUFACTURER + "_" + DeviceInfo.DEVICE;
    	StringBuilder res = new StringBuilder();
    	for (char c : name.toCharArray()) {
    		if (Character.isLetterOrDigit(c) || c=='-' || c=='_')
    			res.append(c);
    		else
    			res.append('_');
    	}
		res.append('_');
    	SimpleDateFormat fmt = new SimpleDateFormat("yyyyMMddHHmmss");
    	res.append(fmt.format(new Date()));
    	return res.toString();
    }

    private boolean setSyncDirectory(File dir) {
    	File configDir = new File(dir, ".cr3sync");
    	if (!configDir.isDirectory())
    		if (!configDir.mkdirs())
    			return false;
    	syncDir = dir.getAbsolutePath();
    	syncConfigDir = configDir.getAbsolutePath();
    	return true;
    }
    
    private String getCurrentLogFileName() {
    	SimpleDateFormat fmt = new SimpleDateFormat("yyyyMM");
    	return thisDeviceName + fmt.format(new Date());
    }
    
    private boolean saveBookmark(ChangeInfo ci) {
    	File f = new File(syncConfigDir, getCurrentLogFileName());
    	try {
			FileOutputStream os = new FileOutputStream(f, true);
			String data = ci.toString();
			try {
				byte[] bytes = data.getBytes("UTF8");
				os.write(bytes);
			} catch (UnsupportedEncodingException e) {
				return false;
			}
			os.close();
		} catch (FileNotFoundException e) {
			Log.e(TAG, "cannot write to log file " + f);
			return false;
		} catch (IOException e) {
			Log.e(TAG, "error while writing to log file " + f);
			return false;
		}
		return true;
    }

    // This is the object that receives interactions from clients.  See
    // RemoteService for a more complete example.
    private final IBinder mBinder = new LocalBinder();

    private String syncDir;
    
    private String syncConfigDir;

    private String thisDeviceName;
}
