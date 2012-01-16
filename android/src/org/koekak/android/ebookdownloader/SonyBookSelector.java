// 
// This program is free software; you can redistribute it and/or modify it any way you want
//

package org.koekak.android.ebookdownloader;

import android.app.Activity;
import android.content.ContentValues;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.util.Log;

//import android.os.Environment;

/**
  * @author Michael Berganovsky [mike0berg at gmail.com] 
  */

public class SonyBookSelector
{

    public final static String  packageTag = "SonyBookSelector";

    private Activity            m_activity;
    private final static String m_extsd    = "/mnt/extsd";      // Environment.getExternalExtSDStorageDirectory().getPath()
    private final static String m_sdcard   = "/mnt/sdcard";     // Environment.getExternalSDStorageDirectory().getPath()

    SonyBookSelector(Activity activity)
    {
        m_activity = activity;
    }

    public long getContentId(String fname)
    {
        long res = 0;
        Cursor cursor = null;
        String name = null;
        String src = "";

        Log.d(packageTag, "getContentId: file name = " + fname);

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
                    }
                    else {
                        Log.w(packageTag, "getContentId: file not found - " + fname);                    
                    }
                    cursor.close();
                }
                else {
                    Log.w(packageTag, "getContentId: file not found - " + fname);                    
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
