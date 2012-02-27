// 
// This program is free software; you can redistribute it and/or modify it any way you want
//

package org.koekak.android.ebookdownloader;

import java.io.File;
import java.lang.reflect.*;

import android.app.Activity;
import android.content.ContentValues;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;

/**
 * @author Michael Berganovsky [mike0berg at gmail.com]
 */

public class SonyBookSelector
{
    public final static String  packageTag = "SonyBookSelector";

    private Activity            m_activity;
    
    final private static String m_extsd    = getExtSDDir();
    final private static String m_sdcard   = getSDDir();

    private static String getExtSDDir()
    {
        try {
            Method getExternalExtSDStorageDirectory = Class.forName("android.os.Environment").getMethod("getExternalExtSDStorageDirectory", (Class[])null);
            File dir = (File)getExternalExtSDStorageDirectory.invoke(null,(Object[])null);
            return dir.getPath();
        } catch(Exception e) {
            Log.e(packageTag, "getExtSDDir", e);
        }
        return "/mnt/extsd"; 
    }
    
    private static String getSDDir()
    {
        try {
            Method getExternalSDStorageDirectory = Class.forName("android.os.Environment").getMethod("getExternalSDStorageDirectory", (Class[])null);
            File dir = (File)getExternalSDStorageDirectory.invoke(null,(Object[])null);
            return dir.getPath();
        } catch(Exception e) {
            Log.e(packageTag, "getSDDir", e);
        }
        return "/mnt/sdcard"; 
    }

    public SonyBookSelector(Activity activity)
    {
        m_activity = activity;
    }

    public long getContentId(String filename)
    {
        long res = 0;
        Cursor cursor = null;
        String name = null;
        String src = "";

        Log.d(packageTag, "getContentId: file name = " + filename);

        File f = new File(filename);
        if( !f.exists() ) {
            Log.w(packageTag, "getContentId: file does not exist in fs - " + filename);
            return res;
        }

        String fname = f.getAbsolutePath();
        try {
            fname = f.getCanonicalPath();
        } catch( Exception e ) {
            Log.e(packageTag, "getContentId", e);
        }

        Log.d(packageTag, "getContentId: canonical file name = " + fname);

        try {
            if( fname.startsWith(m_extsd) ) {
                name = fname.substring(m_extsd.length() + 1);
                src = "1";
            } else if( fname.startsWith(m_sdcard) ) {
                name = fname.substring(m_sdcard.length() + 1);
                src = "0";
            }

            if( name != null ) {
                Log.d(packageTag, "getContentId: name = " + name);
                Uri uri = Uri.parse("content://com.sony.drbd.ebook.provider/books");
                cursor = m_activity.getContentResolver().query(uri, null, "file_path=? AND source_id=?", new String[] { name, src }, null);
                if( cursor != null ) {
                    if( cursor.moveToFirst() ) {
                        res = cursor.getLong(cursor.getColumnIndex("_id"));
                        Log.w(packageTag, "getContentId: id = " + res);
                    } else {
                        Log.w(packageTag, "getContentId: database error - " + fname);
                    }
                    cursor.close();
                } else {
                    Log.w(packageTag, "getContentId: database error - " + fname);
                }
            } else {
                Log.w(packageTag, "getContentId: wrong file requested - " + fname);
            }
        } catch( Exception e ) {
            if( cursor != null ) {
                cursor.close();
            }
            Log.e(packageTag, "getContentId", e);
        }
        return res;
    }

    public void notifyScanner(String filename)
    {
        File f = new File(filename);
        if( f.exists() ) {
            String fname = f.getAbsolutePath();
            try {
                fname = f.getCanonicalPath();
            } catch( Exception e ) {
                Log.e(packageTag, "notifyScanner", e);
            }
            Intent intent = new Intent("com.sony.drbd.ebook.mediascanner.MediaScannerService");
            Bundle bundle = new Bundle();
            bundle.putString("file_path", fname);
            // bundle.putString("mime_type", "");
            intent.putExtras(bundle);
            m_activity.startService(intent);
            Log.d(packageTag, "notifyScanner: " + fname);
        }
    }

    public void setReadingTime(long id)
    {
        Uri baseUri = Uri.parse("content://com.sony.drbd.ebook.provider/books");
        Uri uri = Uri.withAppendedPath(baseUri, Long.toString(id));
        ContentValues contentvalues = new ContentValues();
        contentvalues.put("reading_time", Long.valueOf(System.currentTimeMillis()));
        m_activity.getContentResolver().update(uri, contentvalues, null, null);
        Log.d(packageTag, "setReadingTime: id = " + id);
    }

    public void requestBookSelection(long id)
    {
        Intent intent = new Intent("android.intent.action.book_selected");
        intent.putExtra("_id", id);
        m_activity.sendBroadcast(intent);
        Log.d(packageTag, "requestBookSelection: id = " + id);
    }
}
