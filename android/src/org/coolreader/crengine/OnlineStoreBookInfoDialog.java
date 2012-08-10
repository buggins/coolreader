package org.coolreader.crengine;

import org.coolreader.R;
import org.coolreader.crengine.CoverpageManager.CoverpageBitmapReadyListener;
import org.coolreader.plugins.OnlineStoreBookInfo;

import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Paint;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.RatingBar;
import android.widget.TextView;

public class OnlineStoreBookInfoDialog extends BaseDialog {
	private BaseActivity mActivity;
	private OnlineStoreBookInfo mBookInfo;
	private FileInfo mFileInfo;
	private LayoutInflater mInflater;
	private int mWindowSize;
	public OnlineStoreBookInfoDialog(BaseActivity activity, OnlineStoreBookInfo book, FileInfo fileInfo)
	{
		super(activity, null, false, false);
		DisplayMetrics outMetrics = new DisplayMetrics();
		activity.getWindowManager().getDefaultDisplay().getMetrics(outMetrics);
		this.mWindowSize = outMetrics.widthPixels < outMetrics.heightPixels ? outMetrics.widthPixels : outMetrics.heightPixels;
		this.mActivity = activity;
		this.mBookInfo = book;
		this.mFileInfo = fileInfo;
	}

	@Override
	protected void onCreate() {
        setCancelable(true);
        setCanceledOnTouchOutside(true);

        super.onCreate();
		L.v("OptionsDialog is created");
	}
	
	RatingBar rbBookRating;
    Button btnBuyOrDownload;
    Button btnPreview;
    TextView lblTitle;
    TextView lblSeries;
    TextView lblAuthors;
    TextView lblFileInfo;
    TextView lblLogin;
    TextView lblStatus;
    TextView lblBalance;
    TextView lblPrice;
    TextView lblNormalPrice;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

        mInflater = LayoutInflater.from(getContext());
        ViewGroup view = (ViewGroup)mInflater.inflate(R.layout.online_store_book_info_dialog, null);
        
        ImageButton btnBack = (ImageButton)view.findViewById(R.id.base_dlg_btn_back);
        btnBack.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				onNegativeButtonClick();
			}
		});
        btnBuyOrDownload = (Button)view.findViewById(R.id.btn_buy);
        btnBuyOrDownload.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				onBuyButtonClick();
			}
		});
        btnPreview = (Button)view.findViewById(R.id.btn_preview);
        btnPreview.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				onPreviewButtonClick();
			}
		});
        lblTitle = (TextView)view.findViewById(R.id.lbl_book_title);
        lblSeries = (TextView)view.findViewById(R.id.lbl_book_series);
        lblAuthors = (TextView)view.findViewById(R.id.lbl_book_author);
        lblFileInfo = (TextView)view.findViewById(R.id.lbl_book_file_info);
        lblLogin = (TextView)view.findViewById(R.id.lbl_login);
        lblStatus = (TextView)view.findViewById(R.id.lbl_status);
        lblBalance = (TextView)view.findViewById(R.id.lbl_balance);
        lblPrice = (TextView)view.findViewById(R.id.lbl_price);
        lblNormalPrice = (TextView)view.findViewById(R.id.lbl_normal_price);
        rbBookRating = (RatingBar)view.findViewById(R.id.book_rating);

        final ImageView image = (ImageView)view.findViewById(R.id.book_cover);
        int w = mWindowSize * 4 / 10;
        int h = w * 4 / 3;
        image.setMinimumHeight(h);
        image.setMaxHeight(h);
        image.setMinimumWidth(w);
        image.setMaxWidth(w);
        Bitmap bmp = Bitmap.createBitmap(w, h, Config.RGB_565);
        Services.getCoverpageManager().drawCoverpageFor(mActivity.getDB(), mFileInfo, bmp, new CoverpageBitmapReadyListener() {
			@Override
			public void onCoverpageReady(CoverpageManager.ImageItem file, Bitmap bitmap) {
		        BitmapDrawable drawable = new BitmapDrawable(bitmap);
				image.setImageDrawable(drawable);
			}
		}); 

        if (mBookInfo.book.rating > 0)
        	rbBookRating.setRating(mBookInfo.book.rating / 2.0f);
        else
        	rbBookRating.setVisibility(View.GONE);
        updateInfo();
        setView(view);
	}
	
	private void updateInfo() {
		lblTitle.setText(mBookInfo.book.bookTitle);
		lblAuthors.setText(Utils.formatAuthorsNormalNames(mBookInfo.book.getAuthors()));
		lblSeries.setText(mBookInfo.book.getSeries());
        lblLogin.setText(mBookInfo.isLoggedIn ? "account: " + mBookInfo.login : "please log in");
        lblBalance.setText(mBookInfo.isLoggedIn ? "balance: " + mBookInfo.accountBalance : "");
        lblStatus.setText(mBookInfo.isPurchased ? "(already purchased)" : "");
        lblPrice.setText(mBookInfo.book.price > 0 ? "price: " + String.valueOf(mBookInfo.book.price) : "free!");
        lblNormalPrice.setText(mBookInfo.book.price != mBookInfo.book.basePrice ? String.valueOf(mBookInfo.book.basePrice) : "");
        lblNormalPrice.setPaintFlags(lblNormalPrice.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);
        lblFileInfo.setText(Utils.formatSize(mBookInfo.book.zipSize));
        if (mBookInfo.book.trialUrl == null)
        	btnPreview.setVisibility(View.GONE);
		if (mBookInfo.isLoggedIn) {
			if (mBookInfo.isPurchased) {
				btnBuyOrDownload.setText("Download");
			} else {
				btnBuyOrDownload.setText("Buy");
			}
		} else {
			btnBuyOrDownload.setText("Login");
		}
	}
	
	@Override
	protected void onPositiveButtonClick() {
		super.onPositiveButtonClick();
	}

	@Override
	protected void onNegativeButtonClick() {
		super.onNegativeButtonClick();
	}

	protected void onBuyButtonClick() {
		mActivity.showToast("Buy button pressed");
	}
	
	protected void onPreviewButtonClick() {
		mActivity.showToast("Preview button pressed");
	}
}

