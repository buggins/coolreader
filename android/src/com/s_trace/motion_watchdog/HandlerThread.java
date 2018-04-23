/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.s_trace.motion_watchdog;

import android.os.Build;
import android.os.Handler;
import android.os.Looper;

/**
 * Handy class for starting a new thread that has a looper. The looper can then be 
 * used to create handler classes. Note that start() must still be called.
 *
 * S-trace: Simpified version of android.os.HandlerThread class.
 * It is necessary here to avoid min SDK version bump from 3 to 18 (!)
 */
public class HandlerThread extends Thread {
    private static final int THREAD_PRIORITY_DEFAULT = 0;
    Looper mLooper;
    private Handler mHandler;

    public HandlerThread(String name) {
        super(name);
    }

    /**
     * Call back method that can be explicitly overridden if needed to execute some
     * setup before Looper loops.
     */
    protected void onLooperPrepared() {
    }

    @Override
    public void run() {
        Looper.prepare();
        synchronized (this) {
            mLooper = Looper.myLooper();
            notifyAll();
        }
        onLooperPrepared();
        Looper.loop();
    }
    
    /**
     * This method returns the Looper associated with this thread. If this thread not been started
     * or for any reason isAlive() returns false, this method will return null. If this thread
     * has been started, this method will block until the looper has been initialized.  
     * @return The looper.
     */
    public Looper getLooper() {
        if (!isAlive()) {
            return null;
        }
        
        // If the thread has been started, wait until the looper has been created.
        synchronized (this) {
            while (isAlive() && mLooper == null) {
                try {
                    wait();
                } catch (InterruptedException e) {
                }
            }
        }
        return mLooper;
    }

    /**
     * @return a shared {@link Handler} associated with this thread
     * @hide
     */
    public Handler getThreadHandler() {
        if (mHandler == null) {
            mHandler = new Handler(getLooper());
        }
        return mHandler;
    }

    /**
     * Quits the handler thread's looper.
     * <p>
     * Causes the handler thread's looper to terminate without processing any
     * more messages in the message queue.
     * </p><p>
     * Any attempt to post messages to the queue after the looper is asked to quit will fail.
     * For example, the Handler.sendMessage(Message) method will return false.
     * </p><p class="note">
     * Using this method may be unsafe because some messages may not be delivered
     * before the looper terminates.  Consider using {@link #quitSafely} instead to ensure
     * that all pending work is completed in an orderly manner.
     * </p>
     *
     * @return True if the looper looper has been asked to quit or false if the
     * thread had not yet started running.
     *
     * @see #quitSafely
     */
    public boolean quit() {
        Looper looper = getLooper();
        if (looper != null) {
            looper.quit();
            return true;
        }
        return false;
    }

    /**
     * Quits the handler thread's looper safely.
     * <p>
     * Causes the handler thread's looper to terminate as soon as all remaining messages
     * in the message queue that are already due to be delivered have been handled.
     * Pending delayed messages with due times in the future will not be delivered.
     * </p><p>
     * Any attempt to post messages to the queue after the looper is asked to quit will fail.
     * For example, the Handler.sendMessage(Message) method will return false.
     * </p><p>
     * If the thread has not been started or has finished (that is if
     * {@link #getLooper} returns null), then false is returned.
     * Otherwise the looper is asked to quit and true is returned.
     * </p>
     *
     * @return True if the looper looper has been asked to quit or false if the
     * thread had not yet started running.
     */
    public boolean quitSafely() {
        Looper looper = getLooper();
        if (looper != null) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
                looper.quitSafely();
            } else {
                looper.quit();
            }
            return true;
        }
        return false;
    }

}
