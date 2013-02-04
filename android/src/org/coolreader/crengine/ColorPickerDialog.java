package org.coolreader.crengine;

// based on color picker from 
// http://www.anddev.org/announce_color_picker_dialog-t10771.html

import org.coolreader.R;

import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.graphics.BlurMaskFilter;
import android.graphics.BlurMaskFilter.Blur;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff.Mode;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import android.os.SystemClock;
import android.util.StateSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.Transformation;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

public class ColorPickerDialog extends BaseDialog implements OnSeekBarChangeListener {

    public interface OnColorChangedListener {
        public void colorChanged(int color);
    }

	private SeekBar mR;
	private SeekBar mG;
	private SeekBar mB;
	private SeekBar mHue;
	private SeekBar mSaturation;
	private SeekBar mValue;
	private TextView mLabel;
	private OnColorChangedListener mListener;
	private int mColor;
	private GradientDrawable mPreviewDrawable;

	public ColorPickerDialog(BaseActivity activity, OnColorChangedListener listener, int color, String title) {
		super(activity, title, false, true);
		mListener = listener;

		Resources res = activity.getResources();
		setTitle(title);
		View root = LayoutInflater.from(activity).inflate(R.layout.color_picker, null);
		setView(root);
		
		View preview = root.findViewById(R.id.preview);
		mPreviewDrawable = new GradientDrawable();
		// 2 pix more than color_picker_frame's radius
		mPreviewDrawable.setCornerRadius(7);
		Drawable[] layers;
		layers = new Drawable[] {
				mPreviewDrawable,
				res.getDrawable(R.drawable.color_picker_frame),
		};
		preview.setBackgroundDrawable(new LayerDrawable(layers));
		
		mR = (SeekBar) root.findViewById(R.id.r);
		mG = (SeekBar) root.findViewById(R.id.g);
		mB = (SeekBar) root.findViewById(R.id.b);
		mHue = (SeekBar) root.findViewById(R.id.hue);
		mSaturation = (SeekBar) root.findViewById(R.id.saturation);
		mValue = (SeekBar) root.findViewById(R.id.value);
		mLabel = (TextView) root.findViewById(R.id.value_label);
		
		mColor = color;
		int r = Color.red(mColor);
		int g = Color.green(mColor);
		int b = Color.blue(mColor);
		float[] hsv = new float[3];
		Color.colorToHSV(color, hsv);
		int h = (int) (hsv[0] * mHue.getMax() / 360);
		int s = (int) (hsv[1] * mSaturation.getMax());
		int v = (int) (hsv[2] * mValue.getMax());
		setupSeekBar(mR, R.string.options_color_r, r, res);
		setupSeekBar(mG, R.string.options_color_g, g, res);
		setupSeekBar(mB, R.string.options_color_b, b, res);
		setupSeekBar(mHue, R.string.options_color_hue, h, res);
		setupSeekBar(mSaturation, R.string.options_color_saturation, s, res);
		setupSeekBar(mValue, R.string.options_color_brightness, v, res);
		
		updatePreview(color);
	}
	
	private void setupSeekBar(SeekBar seekBar, int id, int value, Resources res) {
		seekBar.setProgressDrawable(new TextSeekBarDrawable(res, id, value < seekBar.getMax() / 2));
		seekBar.setProgress(value);
		seekBar.setOnSeekBarChangeListener(this);
	}

	private void updateHSV() {
		float[] hsv = {
			360 * mHue.getProgress() / (float) mHue.getMax(),
			mSaturation.getProgress() / (float) mSaturation.getMax(),
			mValue.getProgress() / (float) mValue.getMax(),
		};
		mColor = Color.HSVToColor(hsv);
		mR.setProgress(Color.red(mColor));
		mG.setProgress(Color.green(mColor));
		mB.setProgress(Color.blue(mColor));
		updatePreview(mColor);
	}
	
	private void updateRGB() {
		mColor = Color.rgb(mR.getProgress(), mG.getProgress(), mB.getProgress());
		float[] hsv = new float[3];
		Color.colorToHSV(mColor, hsv);
		int h = (int) (hsv[0] * mHue.getMax() / 360);
		int s = (int) (hsv[1] * mSaturation.getMax());
		int v = (int) (hsv[2] * mValue.getMax());
		mHue.setProgress(h);
		mSaturation.setProgress(s);
		mValue.setProgress(v);
		updatePreview(mColor);
	}
	
	private static String byteToHex(int n) {
		String s = Integer.toHexString(n & 255);
		if (s.length()<2)
			s = "0" + s;
		return s;
	}
	private static String colorToHex(int n) {
		return ("#" + byteToHex(Color.red(n))
			 + byteToHex(Color.green(n))
			 + byteToHex(Color.blue(n))).toUpperCase();
	}
	private void updatePreview(int color) {
		mPreviewDrawable.setColor(color);
		mPreviewDrawable.invalidateSelf();
		mLabel.setText(colorToHex(mColor));
	}

	public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
		if ( fromUser ) {
			if ( seekBar==mR || seekBar==mG || seekBar==mB )
				updateRGB();
			else
				updateHSV();
		}
	}

	public void onStartTrackingTouch(SeekBar seekBar) {
	}

	public void onStopTrackingTouch(SeekBar seekBar) {
	}

	@Override
	protected void onPositiveButtonClick() {
		mListener.colorChanged(mColor);
		super.onPositiveButtonClick();
	}

	@Override
	protected void onNegativeButtonClick() {
		mListener.colorChanged(mColor);
		super.onPositiveButtonClick();
	}

//	@Override
//	protected void onNegativeButtonClick() {
//		onPositiveButtonClick();
//	}

	static class IconPreviewDrawable extends Drawable {
		private Bitmap mBitmap;
		private Bitmap mTmpBitmap;
		private Canvas mTmpCanvas;
		private int mTintColor;
		

		public IconPreviewDrawable(Resources res, int id) {
			Bitmap b;
			try {
				b = BitmapFactory.decodeResource(res, id);
				if (b == null) {
					b = BitmapFactory.decodeResource(res, R.drawable.color_picker_icon);
				}
			} catch (NotFoundException e) {
				b = BitmapFactory.decodeResource(res, R.drawable.color_picker_icon);
			}
			mBitmap = b;
			mTmpBitmap = Bitmap.createBitmap(b.getWidth(), b.getHeight(), Config.ARGB_8888);
			mTmpCanvas = new Canvas(mTmpBitmap);
		}
		
		@Override
		public void draw(Canvas canvas) {
			Rect b = getBounds();
			float x = (b.width() - mBitmap.getWidth()) / 2.0f;
			float y = 0.75f * b.height() - mBitmap.getHeight() / 2.0f;
			
			mTmpCanvas.drawColor(0, Mode.CLEAR);
			mTmpCanvas.drawBitmap(mBitmap, 0, 0, null);
			mTmpCanvas.drawColor(mTintColor, Mode.SRC_ATOP);
			canvas.drawBitmap(mTmpBitmap, x, y, null);
		}

		@Override
		public int getOpacity() {
			return PixelFormat.TRANSLUCENT;
		}

		@Override
		public void setAlpha(int alpha) {
		}

		@Override
		public void setColorFilter(ColorFilter cf) {
		}
		
		@Override
		public void setColorFilter(int color, Mode mode) {
			mTintColor = color;
		}
	}
	
	static final int[] STATE_FOCUSED = {android.R.attr.state_focused};
	static final int[] STATE_PRESSED = {android.R.attr.state_pressed};
	
	static class TextSeekBarDrawable extends Drawable implements Runnable {
		
		private static final String TAG = "TextSeekBarDrawable";
		private static final long DELAY = 50;
		private String mText;
		private Drawable mProgress;
		private Paint mPaint;
		private Paint mOutlinePaint;
		private float mTextWidth;
		private boolean mActive;
		private float mTextXScale;
		private int mDelta;
		private ScrollAnimation mAnimation;

		public TextSeekBarDrawable(Resources res, int id, boolean labelOnRight) {
			mText = res.getString(id);
			mProgress = res.getDrawable(android.R.drawable.progress_horizontal);
			mPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
			mPaint.setTypeface(Typeface.DEFAULT_BOLD);
			mPaint.setTextSize(16);
			mPaint.setColor(0xff000000);
			mOutlinePaint = new Paint(mPaint);
			mOutlinePaint.setStyle(Style.STROKE);
			mOutlinePaint.setStrokeWidth(3);
			mOutlinePaint.setColor(0xbbffc300);
			mOutlinePaint.setMaskFilter(new BlurMaskFilter(1, Blur.NORMAL));
			mTextWidth = mOutlinePaint.measureText(mText);
			mTextXScale = labelOnRight? 1 : 0;
			mAnimation = new ScrollAnimation();
		}

		@Override
		protected void onBoundsChange(Rect bounds) {
			mProgress.setBounds(bounds);
		}
		
		@Override
		protected boolean onStateChange(int[] state) {
			mActive = StateSet.stateSetMatches(STATE_FOCUSED, state) | StateSet.stateSetMatches(STATE_PRESSED, state);
			invalidateSelf();
			return false;
		}
		
		@Override
		public boolean isStateful() {
			return true;
		}
		
		@Override
		protected boolean onLevelChange(int level) {
//			Log.d(TAG, "onLevelChange " + level);
			if (level < 4000 && mDelta <= 0) {
//				Log.d(TAG, "onLevelChange scheduleSelf ++");
				mDelta = 1;
				mAnimation.startScrolling(mTextXScale, 1);
				scheduleSelf(this, SystemClock.uptimeMillis() + DELAY);
			} else
			if (level > 6000 && mDelta >= 0) {
//				Log.d(TAG, "onLevelChange scheduleSelf --");
				mDelta = -1;
				mAnimation.startScrolling(mTextXScale, 0);
				scheduleSelf(this, SystemClock.uptimeMillis() + DELAY);
			}
			return mProgress.setLevel(level);
		}
		
		@Override
		public void draw(Canvas canvas) {
			mProgress.draw(canvas);

			if (mAnimation.hasStarted() && !mAnimation.hasEnded()) {
				// pending animation
				mAnimation.getTransformation(AnimationUtils.currentAnimationTimeMillis(), null);
				mTextXScale = mAnimation.getCurrent();
//				Log.d(TAG, "draw " + mTextX + " " + SystemClock.uptimeMillis());
			}
			
			Rect bounds = getBounds();
			float x = 6 + mTextXScale * (bounds.width() - mTextWidth - 6 - 6);
			float y = (bounds.height() + mPaint.getTextSize()) / 2;
			mOutlinePaint.setAlpha(mActive? 255 : 255 / 2);
			mPaint.setAlpha(mActive? 255 : 255 / 2);
			canvas.drawText(mText, x, y, mOutlinePaint);
			canvas.drawText(mText, x, y, mPaint);
		}

		@Override
		public int getOpacity() {
			return PixelFormat.TRANSLUCENT;
		}

		@Override
		public void setAlpha(int alpha) {
		}

		@Override
		public void setColorFilter(ColorFilter cf) {
		}

		public void run() {
			mAnimation.getTransformation(AnimationUtils.currentAnimationTimeMillis(), null);
			// close interpolation of mTextX
			mTextXScale = mAnimation.getCurrent();
			if (!mAnimation.hasEnded()) {
				scheduleSelf(this, SystemClock.uptimeMillis() + DELAY);
			}
			invalidateSelf();
//			Log.d(TAG, "run " + mTextX + " " + SystemClock.uptimeMillis());
		}
	}
	
	static class ScrollAnimation extends Animation {
		private static final String TAG = "ScrollAnimation";
		private static final long DURATION = 750;
		private float mFrom;
		private float mTo;
		private float mCurrent;
		
		public ScrollAnimation() {
			setDuration(DURATION);
			setInterpolator(new DecelerateInterpolator());
		}
		
		public void startScrolling(float from, float to) {
			mFrom = from;
			mTo = to;
			startNow();
		}
		
		@Override
		protected void applyTransformation(float interpolatedTime, Transformation t) {
			mCurrent = mFrom + (mTo - mFrom) * interpolatedTime;
//			Log.d(TAG, "applyTransformation " + mCurrent);
		}
		
		public float getCurrent() {
			return mCurrent;
		}
	}
}
