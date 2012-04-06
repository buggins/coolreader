package org.coolreader.sync;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.coolreader.crengine.DeviceInfo;

import android.app.Service;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

public class SyncService extends Service {

	private final static String TAG = "cr3sync";
    public final static String SYNC_LOG_DIR_NAME = ".cr3sync"; 
		
    @Override
    public void onCreate() {
    	Log.i(TAG, "onCreate()");
        readSettings();
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
        public SyncService getService() {
            return SyncService.this;
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

	private static String PREF_FILE = "CR3Sync";
	private static String PREF_THIS_DEVICE_ID = "ThisDeviceId";
	private static String PREF_LAST_POSITION_PREFIX = "LastPos";
	private static String PREF_LAST_SIZE_PREFIX = "LastSize";
	private void readSettings()	{
		SharedPreferences pref = getSharedPreferences(PREF_FILE, 0);
		thisDeviceName = pref.getString(PREF_THIS_DEVICE_ID, null);
		if (thisDeviceName == null) {
			thisDeviceName = generateThisDeviceName();
			pref.edit().putString(PREF_THIS_DEVICE_ID, thisDeviceName);
			Log.i(TAG, "New device name generated: " + thisDeviceName);
		}
		Log.i(TAG, "My device name: " + thisDeviceName);
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

    public synchronized boolean setSyncDirectory(File dir) {
    	File configDir = new File(dir, SYNC_LOG_DIR_NAME);
    	if (!configDir.isDirectory())
    		if (!configDir.mkdirs())
    			return false;
    	syncDir = dir.getAbsolutePath();
    	syncLogDir = configDir.getAbsolutePath();
		Log.i(TAG, "Sync directory: " + syncDir);
		Log.i(TAG, "Sync logs directory: " + syncLogDir);
    	return true;
    }
    
    private static final String PART_SUFFIX_FORMAT = "yyyyMM";
    private static final String PART_SUFFIX_DELIMITER = "-";
    private String getCurrentLogFileName() {
    	SimpleDateFormat fmt = new SimpleDateFormat(PART_SUFFIX_FORMAT);
    	return thisDeviceName + PART_SUFFIX_DELIMITER + fmt.format(new Date());
    }
    
    synchronized public boolean saveBookmarks(Collection<ChangeInfo> src) {
    	if (thisDeviceName == null)
    		return false;
    	// filter file names, cut directory name
    	Collection<ChangeInfo> list = new ArrayList<ChangeInfo>(src.size());
    	String prefix = syncDir + "/";
    	for (ChangeInfo ci : src) {
    		if (ci.fileName.startsWith(prefix)) {
    			ci.fileName = ci.fileName.substring(prefix.length());
    			list.add(ci);
    		}
    	}
    	if (list.size() == 0)
    		return true;
    	// save
    	File f = new File(syncLogDir, getCurrentLogFileName());
    	try {
			FileOutputStream os = new FileOutputStream(f, true);
			for (ChangeInfo ci : list) {
				String data = ci.toString();
				try {
					byte[] bytes = data.getBytes("UTF8");
					os.write(bytes);
				} catch (UnsupportedEncodingException e) {
					return false;
				}
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

	public void sync(List<ChangeInfo> changes, int maxRecords) {
		Log.v(TAG, "sync readers");
		syncReaders();
		ArrayList<LogReader> readers = new ArrayList<LogReader>();
		synchronized(readerMap) {
			readers.addAll(readerMap.values());
		}
		Collections.sort(readers);
		Log.v(TAG, "sync data for " + readers.size() + " readers");
		for (LogReader reader : readers) {
			reader.sync(changes, maxRecords);
			if (changes.size() >= maxRecords)
				break;
		}
		Collections.sort(changes);
		// add directory prefix
		for (ChangeInfo ci : changes) {
			ci.fileName = syncDir + "/" + ci.fileName;
		}
	}
	
	public final static int MAX_RECORDS_NUMBER = 10000;
    
    private boolean syncReaders() {
    	File logdir = new File(syncLogDir);
    	if (!logdir.isDirectory()) {
    		Log.e(TAG, "cannot read directory " + syncLogDir);
    		return false;
    	}
    	SimpleDateFormat fmt = new SimpleDateFormat(PART_SUFFIX_FORMAT);
    	File[] files = logdir.listFiles();
    	for (File f : files) {
    		String name = f.getName();
    		if (name == null || name.startsWith(thisDeviceName))
    			continue;
    		int p = name.lastIndexOf(PART_SUFFIX_DELIMITER);
    		if (p != name.length() - PART_SUFFIX_FORMAT.length() - PART_SUFFIX_DELIMITER.length())
    			continue;
    		String deviceId = name.substring(0, p); // unique device id 
    		String partSuffix = name.substring(p + 1); // year+month suffix
			Date minDate = new Date(System.currentTimeMillis() - 1000L*60*60*24*30*3); // -3 months
			String minPart = fmt.format(minDate);
			if (minPart.compareTo(partSuffix) > 0) {
				Log.i(TAG, "will remove obsolete log file " + name);
				f.delete();
				continue;
			}
			Log.v(TAG, "checking log file " + name + " deviceId=" + deviceId + ", part=" + partSuffix);
			syncReader(deviceId, partSuffix, f);
    	}
    	
    	return true;
    }
    
    private Map<String, LogReader> readerMap = new HashMap<String, LogReader>();
    
    private void syncReader(String deviceId, String partId, File logfile) {
    	LogReader reader = null;
    	synchronized(readerMap) {
    		reader = readerMap.get(logfile.getName());
    		if (reader == null) {
    			reader = new LogReader(deviceId, partId, logfile);
    			readerMap.put(logfile.getName(), reader);
    		}
    	}
    }
    
    private class LogReader implements Comparable<LogReader> {
    	public LogReader(String deviceId, String partId, File logFile) {
    		this.deviceId = deviceId;
    		this.logfile = logFile;
    		this.partId = partId;
			//long fileSize = f.length();
    		SharedPreferences pref = getSharedPreferences(PREF_FILE, 0);
    		String suffix = "." + logfile.getName();
    		lastReadPosition = pref.getLong(PREF_LAST_POSITION_PREFIX + suffix, 0);
    		lastSeenFileSize = pref.getLong(PREF_LAST_SIZE_PREFIX + suffix, 0);
    		if (lastReadPosition > 0 || lastSeenFileSize > 0)
    			Log.i(TAG, deviceId + ": resuming reading position " + lastReadPosition);
    		else
    			Log.i(TAG, deviceId + ": new log file found");
    	}

    	public void sync(List<ChangeInfo> changes, int maxRecords) {
    		if (!isFileChanged())
    			return;
    		Log.i(TAG, "file is changed: " + logfile);
    		long newSeenFileSize = logfile.length();
    		long newReadPosition = lastReadPosition;
    		int lastRecordCount = changes.size();
    		try {
				FileInputStream is = new FileInputStream(logfile);
				long pos = 0;
				if (lastReadPosition > 0) {
					is.skip(lastReadPosition);
					pos += lastReadPosition;
				}
				int avail = is.available();
				if (avail > 0) {
					if (avail > 0x40000)
						avail = 0x40000;
					byte[] buf = new byte[avail];
					int bytesRead = is.read(buf);
					int bytesParsed = parseChanges(buf, bytesRead, changes, maxRecords);
					if (bytesParsed > 0) {
						newReadPosition += bytesParsed;
					}
				}
				is.close();
			} catch (FileNotFoundException e) {
	    		Log.i(TAG, "cannot open file for reading: " + logfile);
			} catch (IOException e) {
	    		Log.i(TAG, "error while reading file: " + logfile);
			}
			if (newReadPosition != lastReadPosition || newSeenFileSize != lastSeenFileSize) {
				int recordsRead = changes.size() - lastRecordCount;
				Log.i(TAG, deviceId + ": read " + recordsRead + " (" + (newReadPosition-lastReadPosition) + " bytes)");
				lastReadPosition = newReadPosition;
				lastSeenFileSize = newSeenFileSize;
	    		SharedPreferences pref = getSharedPreferences(PREF_FILE, 0);
	    		String suffix = "." + logfile.getName();
	    		pref.edit().putLong(PREF_LAST_POSITION_PREFIX + suffix, lastReadPosition);
	    		pref.edit().putLong(PREF_LAST_SIZE_PREFIX + suffix, lastSeenFileSize);
			}
    	}

    	private int parseChanges(byte[] buf, int len, List<ChangeInfo> changes, int maxRecords) {
    		int pos = 0;
    		for (;;) {
				if (changes.size() >= maxRecords)
					return pos;
	    		int[] found = ChangeInfo.findNextRecordBounds(buf, pos, len);
	    		if (found == null)
	    			return pos;
	    		ChangeInfo ci = ChangeInfo.fromBytes(buf, found[0], found[1]);
	    		if (ci != null)
	    			changes.add(ci);
	    		pos = found[1];
    		}
    	}

    	private boolean isFileChanged() {
    		if (!logfile.exists()) {
    			// log file is removed
    	    	synchronized(readerMap) {
    	    		readerMap.remove(logfile.getName());
    	    		return false;
    	    	}
    		}
    		if (lastSeenFileSize < logfile.length())
    			return true;
    		return false;
    	}

    	@Override
		public int compareTo(LogReader another) {
    		return partId.compareTo(another.partId);
		}
    	
    	private final String deviceId;
    	private final String partId;
    	private final File logfile;
    	private long lastReadPosition;
    	private long lastSeenFileSize;

    }

    
//    public void test() {
//    	
//    	// remove directory
//    	File dir = new File("/mnt/sdcard/Books/" + SYNC_LOG_DIR_NAME);
//    	File[] files = dir.listFiles();
//    	if (files != null)
//    		for (File f : files)
//    			f.delete();
//    	dir.delete();
//    	
//    	SyncService svc = this; //new SyncService();
//    	svc.setSyncDirectory(new File("/mnt/sdcard/Books"));
//    	
//    	
//    	Log.i(TAG, "generating test data");
//    	svc.thisDeviceName = "test1dev";
//    	Collection<ChangeInfo> data1 = genChanges(200);
//    	svc.saveBookmarks(data1);
//    	svc.thisDeviceName = "test2dev";
//    	Collection<ChangeInfo> data2 = genChanges(100);
//    	svc.saveBookmarks(data2);
//    	svc.thisDeviceName = "test3dev";
//    	Collection<ChangeInfo> data3 = genChanges(300);
//    	svc.saveBookmarks(data3);
//    	svc.thisDeviceName = "mydevice";
//    	Log.i(TAG, "test: 600 records written");
//
//    	List<ChangeInfo> changes = new ArrayList<ChangeInfo>();
//    	svc.sync(changes, 10000);
//    	Log.i(TAG, "test: " + changes.size() + " records read");
//    }
//    
//    private static Random rnd = new Random();
//    private static Collection<ChangeInfo> genChanges(int count) {
//    	ArrayList<ChangeInfo> list = new ArrayList<ChangeInfo>();
//    	for (int i=0; i<count; i++) {
//    		long ts = System.currentTimeMillis() - rnd.nextInt(60000);
//    		Bookmark bmk = new Bookmark();
//			bmk.setTitleText("bla bla title " + rnd.nextInt(1000000));
//			bmk.setPosText("pos text " + rnd.nextInt(1000000));
//			bmk.setCommentText("comment text " + rnd.nextInt(1000000));
//			bmk.setTimeStamp(ts);
//    		switch (rnd.nextInt(4)) {
//    		case 0:
//    			bmk.setType(0);
//    			bmk.setStartPos("/test/start/pos/last/position/" + rnd.nextInt(5));
//    			break;
//    		case 1:
//    			bmk.setType(1);
//    			bmk.setStartPos("/test/start/pos" + rnd.nextInt(5));
//    			break;
//    		case 2:
//    			bmk.setType(1);
//    			bmk.setStartPos("/test/start/pos" + rnd.nextInt(5));
//    			bmk.setStartPos("/test/end/pos" + rnd.nextInt(5));
//    			break;
//    		case 3:
//    			bmk.setType(1);
//    			bmk.setStartPos("/test/start/pos" + rnd.nextInt(5));
//    			bmk.setStartPos("/test/end/pos" + rnd.nextInt(5));
//    			break;
//    		}
//    		String fn = "/mnt/sdcard/Books/test1/book" + rnd.nextInt(30) + ".txt";
//    		ChangeInfo ci = new ChangeInfo(bmk, fn, false);
//    		list.add(ci);
//    	}
//    	return list;
//    }
    
    // This is the object that receives interactions from clients.  See
    // RemoteService for a more complete example.
    private final IBinder mBinder = new LocalBinder();

    private String syncDir;
    
    private String syncLogDir;

    private String thisDeviceName;
}
