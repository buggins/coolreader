package org.coolreader.crengine;

import android.content.Context;
import android.os.Handler;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.PopupWindow;
import android.widget.TextView;

import org.coolreader.R;

import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * User: Victor Soskin
 * Date: 11/3/11
 * Time: 2:51 PM
 */
public class ToastView {
    private static class Toast {
        private View anchor;
        private String msg;
        private int duration;

        private Toast(View anchor, String msg, int duration) {
            this.anchor = anchor;
            this.msg = msg;
            this.duration = duration;
        }
    }


	private static View mReaderView;
    private static LinkedBlockingQueue<Toast> queue = new LinkedBlockingQueue<Toast>();
    private static AtomicBoolean showing = new AtomicBoolean(false);
    private static Handler mHandler = new Handler();
    private static PopupWindow window = null;

    private static Runnable handleDismiss = new Runnable() {
        @Override
        public void run() {
            if (window != null) {
                window.dismiss();
                show();
            }
        }
    };

    static int fontSize = 24;
    public static void showToast(View anchor, String msg, int duration, int textSize) {
    	mReaderView = anchor;
    	fontSize = textSize;
        try {
            queue.put(new Toast(anchor, msg, duration));
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        if (showing.compareAndSet(false, true)) {
            show();
        }
    }

    private static void show() {
        if (queue.size() == 0) {
            showing.compareAndSet(true, false);
            return;
        }
        Toast t = queue.poll();
        window = new PopupWindow(t.anchor.getContext());
        window.setWidth(WindowManager.LayoutParams.FILL_PARENT);
        window.setHeight(WindowManager.LayoutParams.WRAP_CONTENT);
        window.setTouchable(false);
        window.setFocusable(false);
        window.setOutsideTouchable(true);
        window.setBackgroundDrawable(null);
        /* LinearLayout ll = new LinearLayout(t.anchor.getContext());
        ll.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.FILL_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));

        TextView tv = new TextView(t.anchor.getContext());
        tv.setText(t.msg);
        ll.setGravity(Gravity.CENTER);
        ll.addView(tv);*/
        LayoutInflater inflater = (LayoutInflater) t.anchor.getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        window.setContentView(inflater.inflate(R.layout.custom_toast, null, true));
        TextView tv = (TextView) window.getContentView().findViewById(R.id.toast);
        tv.setTextSize(fontSize); //Integer.valueOf(Services.getSettings().getInt(ReaderView.PROP_FONT_SIZE, 20) ) );
        tv.setText(t.msg);
        tv.setGravity(Gravity.CENTER);
        window.showAtLocation(t.anchor, Gravity.NO_GRAVITY, 0, 0);
        mHandler.postDelayed(handleDismiss, t.duration == 0 ? 2000 : 3000);
    }
}
