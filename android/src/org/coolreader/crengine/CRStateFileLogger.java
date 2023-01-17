package org.coolreader.crengine;

import android.content.Context;
import android.text.format.DateFormat;

import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import java.util.Date;

public class CRStateFileLogger {
    private static final String fileName = "cr-state-log.txt";
    private static File appFilesDir;

    public static final Logger log = L.create("state-logger");

    public static void init(Context applicationContext){
        appFilesDir = applicationContext.getFilesDir();
    }

    public static void clear(){
        File file = new File(appFilesDir, fileName);
        file.delete();
    }

    public static void appendState(String state){
        log.i("writing " + state + " to " + fileName);

        if (appFilesDir == null) {
            log.e("ERROR: " + fileName + " is not initialized");
            return;
        }

        String dtm = DateFormat.format("yyyy-MM-ddTHH:mm:ss", new Date()).toString();
        String msg = dtm + " - " + state.trim() + "\n";

        try {
            File file = new File(appFilesDir, fileName);
            FileOutputStream stream = new FileOutputStream(file, true);
            stream.write(msg.getBytes());
            stream.close();
        } catch (IOException e){
            log.e("ERROR: could not log to " + fileName);
        }
    }
}
