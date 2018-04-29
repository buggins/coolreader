package com.s_trace.motion_watchdog;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.media.AudioManager;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

import org.coolreader.CoolReader;
import org.coolreader.crengine.TTSToolbarDlg;

/**
 * Created by s-trace on 13.03.18.
 * Handler for motion events
 */

public class MotionWatchdogHandler extends Handler implements SensorEventListener {
    private static final int MSG_MOTION_DETECTED  = 0;
    private static final int MSG_MOTION_TIMEOUT   = 1;
    private static final int MSG_HANDLE_STOP_STEP = 2;
    private static final String TAG = MotionWatchdogHandler.class.getSimpleName();
    private static final long STEP_TIME = 5 * 1000; // 5 seconds
    private final SensorManager mSensorManager;
    private final CoolReader mCoolReader;
    private final TTSToolbarDlg mTTSToolbarDlg;
    private boolean mIsStopping;
    private boolean mIsStopped;
    private AudioManager mAudioService;
    private int mOriginalVolume;
    private int mCurrentVolume;
    private HandlerThread mHandlerThread;
    private static final double MOTION_THRESHOLD = 1;
    private final double[] mLastValues = new double[3];
    private final double[] mDelta = new double[3];
    private final int mTimeout;

    public MotionWatchdogHandler(TTSToolbarDlg ttsToolbarDlg, CoolReader coolReader,
                                 com.s_trace.motion_watchdog.HandlerThread handlerThread, int timeout) {
        mHandlerThread = handlerThread;
        mCoolReader = coolReader;
        mTTSToolbarDlg = ttsToolbarDlg;
        mSensorManager = (SensorManager) mCoolReader.getSystemService(Context.SENSOR_SERVICE);
        mTimeout = timeout;

        // Force first sensor event to always fire MSG_MOTION_DETECTED
        mLastValues[0] = MOTION_THRESHOLD + 1000;
        mLastValues[1] = MOTION_THRESHOLD + 1000;
        mLastValues[2] = MOTION_THRESHOLD + 1000;
        try {
            if (mSensorManager != null) {
                Sensor mAccel = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
                int delay = SensorManager.SENSOR_DELAY_NORMAL;
                mSensorManager.registerListener(this, mAccel, delay, this);
            }
        } catch (Exception e) {
            Log.e(TAG, "run: exception " + e);
        }
    }

    @Override
    public void handleMessage(Message msg) {
        if (mHandlerThread.isInterrupted()) {
            Log.i(TAG, "handleMessage: mHandlerThread.isInterrupted() for msg=" + msg);
            handleInterrupt();
            return;
        }
        Log.d(TAG, "handleMessage: msg=" + msg);
        if (mCoolReader == null || mIsStopped) {
            return;
        }
        switch (msg.what) {
            case MSG_MOTION_DETECTED:
                mIsStopping = false;
                if (mAudioService != null) {
                    mAudioService.setStreamVolume(AudioManager.STREAM_MUSIC, mOriginalVolume, 0);
                    mAudioService = null;
                }
                this.removeMessages(MSG_MOTION_TIMEOUT);
                this.removeMessages(MSG_HANDLE_STOP_STEP);

                Message message = Message.obtain();
                message.what = MSG_MOTION_TIMEOUT;
                this.sendMessageDelayed(message, mTimeout);
                break;
            case MSG_MOTION_TIMEOUT:
                mIsStopping = true;
                handleStop();
                break;
            case MSG_HANDLE_STOP_STEP:
                if (mIsStopping) {
                    handleStop();
                }
                break;
        }
    }

    private void handleStop() {
        Log.e(TAG, "handleStop: mCurrentVolume=" + mCurrentVolume);
        if (mHandlerThread.isInterrupted()) {
            Log.i(TAG, "handleStop: mHandlerThread.isInterrupted()");
            handleInterrupt();
            return;
        }
        if (mAudioService == null) {
            mAudioService = (AudioManager) mCoolReader.getSystemService(Context.AUDIO_SERVICE);
            if (mAudioService == null) {
                Log.e(TAG, "handleStop: mAudioService == null! ");
                return;
            }
            mOriginalVolume = mAudioService.getStreamVolume(AudioManager.STREAM_MUSIC);
            mCurrentVolume = mOriginalVolume;
            Message message = Message.obtain();
            message.what = MSG_HANDLE_STOP_STEP;
            this.sendMessageDelayed(message, STEP_TIME);
            return;
        }
        mAudioService.setStreamVolume(AudioManager.STREAM_MUSIC, --mCurrentVolume, 0);
        if (mCurrentVolume > 0) {
            Message message = Message.obtain();
            message.what = MSG_HANDLE_STOP_STEP;
            this.sendMessageDelayed(message, STEP_TIME);
            return;
        }

        Log.i(TAG, "Final stop");
        mIsStopped = true;
        mIsStopping = false;
        mTTSToolbarDlg.stopAndClose();
        try {
            Thread.sleep(2000);
        } catch (InterruptedException e) {
            handleInterrupt();
            return;
        }
        handleInterrupt();
        mHandlerThread.interrupt();
    }

    private void handleInterrupt() {
        Log.i(TAG, "handleInterrupt()");
        mSensorManager.unregisterListener(this);
        if (mAudioService != null) {
            mAudioService.setStreamVolume(AudioManager.STREAM_MUSIC, mOriginalVolume, 0);
        }
        removeMessages(MotionWatchdogHandler.MSG_MOTION_TIMEOUT);
        removeMessages(MotionWatchdogHandler.MSG_HANDLE_STOP_STEP);
        removeMessages(MotionWatchdogHandler.MSG_MOTION_DETECTED);
        mHandlerThread.quitSafely();
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
//        Log.d(TAG, "onSensorChanged: event=" + event + " isInterrupted() ==" + mHandlerThread.isInterrupted());
        if (mHandlerThread.isInterrupted()) {
            Log.d(TAG, "onSensorChanged: isInterrupted()");
            handleInterrupt();
            return;
        }

        mDelta[0] = Math.abs(Math.abs(event.values[0]) - Math.abs(mLastValues[0]));
        mDelta[1] = Math.abs(Math.abs(event.values[1]) - Math.abs(mLastValues[1]));
        mDelta[2] = Math.abs(Math.abs(event.values[2]) - Math.abs(mLastValues[2]));

        mLastValues[0] = event.values[0];
        mLastValues[1] = event.values[1];
        mLastValues[2] = event.values[2];

//        Log.d(TAG, "onSensorChanged:"
//                + " x=" + mDelta[0]
//                + " y=" + mDelta[1]
//                + " z=" + mDelta[2]
//        );

        if ((mDelta[0] > MOTION_THRESHOLD) || (mDelta[1] > MOTION_THRESHOLD) || (mDelta[2] > MOTION_THRESHOLD)) {
            Log.d(TAG, "Got significant motion");
            Message message = Message.obtain();
            message.what = MotionWatchdogHandler.MSG_MOTION_DETECTED;
            sendMessage(message);
        }

    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int i) {
        Log.d(TAG, "onAccuracyChanged: sensor=" + sensor + " i=" + i +
                " isInterrupted() ==" + mHandlerThread.isInterrupted());
        if (mHandlerThread.isInterrupted()) {
            Log.d(TAG, "onAccuracyChanged: isInterrupted()");
            handleInterrupt();
        }
    }

}
