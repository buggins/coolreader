package org.coolreader.crengine;

import java.io.File;

import org.coolreader.R;
import org.coolreader.crengine.CoverpageManager.CoverpageBitmapReadyListener;
import org.coolreader.plugins.BookInfoCallback;
import org.coolreader.plugins.DownloadBookCallback;
import org.coolreader.plugins.OnlineStoreBook;
import org.coolreader.plugins.OnlineStoreBookInfo;
import org.coolreader.plugins.OnlineStorePluginManager;
import org.coolreader.plugins.OnlineStoreWrapper;
import org.coolreader.plugins.PurchaseBookCallback;

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
	private OnlineStoreWrapper mPlugin;
	private File downloadDir;
	private File downloadTrialDir;
	private File downloadFilename;
	private File downloadTrialFilename;
	
	private ViewGroup mContentView;
	
	public OnlineStoreBookInfoDialog(BaseActivity activity, OnlineStoreBookInfo book, FileInfo fileInfo)
	{
		super(activity, null, false, false);
		DisplayMetrics outMetrics = new DisplayMetrics();
		activity.getWindowManager().getDefaultDisplay().getMetrics(outMetrics);
		this.mWindowSize = outMetrics.widthPixels < outMetrics.heightPixels ? outMetrics.widthPixels : outMetrics.heightPixels;
		this.mActivity = activity;
		this.mBookInfo = book;
		this.mFileInfo = fileInfo;
		this.mPlugin = OnlineStorePluginManager.getPlugin(fileInfo.getOnlineCatalogPluginPackage());
		File baseDir = new File(Services.getScanner().getDownloadDirectory().pathname);
		this.downloadDir = new File(baseDir, mPlugin.getDescription());
		this.downloadFilename = new File(downloadDir, book.book.downloadFileName);
		this.downloadTrialDir = new File(baseDir, mPlugin.getDescription() + "-trials");
		this.downloadTrialFilename = new File(downloadTrialDir, book.book.trialFileName);
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
        mContentView = view;
        
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
		progress = new ProgressPopup(mActivity, mContentView);
        updateInfo();
        setView(view);
	}
	
	private void updateInfo() {
		lblTitle.setText(mBookInfo.book.bookTitle);
		lblAuthors.setText(Utils.formatAuthorsNormalNames(mBookInfo.book.getAuthors()));
		lblSeries.setText(mBookInfo.book.getSeries());
        lblLogin.setText(mBookInfo.isLoggedIn ? mBookInfo.login : "please log in");
        lblBalance.setText(mBookInfo.isLoggedIn ? "balance: " + mBookInfo.accountBalance : "");
        lblStatus.setText(mBookInfo.isPurchased ? "purchased" : "");
        lblPrice.setText(mBookInfo.book.price > 0 ? "price: " + String.valueOf(mBookInfo.book.price) : "free!");
        lblNormalPrice.setText(mBookInfo.book.price != mBookInfo.book.basePrice ? String.valueOf(mBookInfo.book.basePrice) : "");
        lblNormalPrice.setPaintFlags(lblNormalPrice.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);
        lblFileInfo.setText(Utils.formatSize(mBookInfo.book.zipSize));
        if (mBookInfo.book.trialUrl == null)
        	btnPreview.setVisibility(View.GONE);
        else {
        	if (bookFileExists(true))
        		btnPreview.setText("Open preview");
        	else
        		btnPreview.setText("Download preview");
        }
        if (bookFileExists(false)) {
			btnBuyOrDownload.setText("Open");
        } else if (mBookInfo.isLoggedIn) {
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
		if (bookFileExists(false)) {
			openBook(false);
		} else if (mBookInfo.isLoggedIn) {
			if (mBookInfo.isPurchased) {
				download(false);
			} else {
				// buy
				mActivity.askConfirmation("Price is " + mBookInfo.book.price + " Do you want to purchase this book?", new Runnable() {
					@Override
					public void run() {
						String bookId = mFileInfo.getOnlineCatalogPluginId();
						progress.show();
						mPlugin.purchaseBook(bookId, new PurchaseBookCallback() {
							@Override
							public void onError(int errorCode, String errorMessage) {
								progress.hide();
								mActivity.showToast("Purchase error: " + errorMessage);
							}
							
							@Override
							public void onBookPurchased(String bookId, double newAccountBalance) {
								progress.hide();
								mBookInfo.accountBalance = newAccountBalance;
								mBookInfo.isPurchased = true;
								updateInfo();
								mActivity.showToast("Book has been purchased. New balance: " + newAccountBalance);
							}
						});
					}
				});
			}
		} else {
			// LOGIN
			OnlineStoreLoginDialog dlg = new OnlineStoreLoginDialog(mActivity, mPlugin, new Runnable() {
				@Override
				public void run() {
					reloadBookInfo();
				}
			});
			dlg.show();
		}
	}
	
	private ProgressPopup progress;
	
	private void reloadBookInfo() {
		String bookId = mFileInfo.getOnlineCatalogPluginId();
		progress.show();
		mPlugin.loadBookInfo(bookId, new BookInfoCallback() {
			@Override
			public void onError(int errorCode, String errorMessage) {
				progress.hide();
				mActivity.showToast("Error while loading book info");
			}
			
			@Override
			public void onBookInfoReady(OnlineStoreBookInfo bookInfo) {
				progress.hide();
				mBookInfo = bookInfo;
				updateInfo();
			}
		});
	}
	
	protected void onPreviewButtonClick() {
		if (bookFileExists(true))
			openBook(true);
		else
			download(true);
	}
	
	private boolean ensureDownloadDirectoryExists(boolean trial) {
		File dir = (trial ? downloadTrialDir : downloadDir);
		if (dir.isDirectory())
			return true;
		return dir.mkdirs();
	}
	
	private File getBookFile(boolean trial) {
		return (trial ? downloadTrialFilename : downloadFilename);
	}
	
	private boolean bookFileExists(boolean trial) {
		return getBookFile(trial).exists();
	}
	
	private void download(final boolean trial) {
		File f = getBookFile(trial);
		if (!ensureDownloadDirectoryExists(trial)) {
			mActivity.showToast("Cannot create download directory " + f.getAbsolutePath());
			return;
		}
		progress.show();
		mPlugin.downloadBook(mBookInfo.book, trial, f, new DownloadBookCallback() {
			@Override
			public void onError(int errorCode, String errorMessage) {
				progress.hide();
				mActivity.showToast("Error while downloading book: " + errorMessage);
			}
			
			@Override
			public void onBookDownloaded(OnlineStoreBook book, boolean trial,
					File savedFileName) {
				progress.hide();
				openBook(trial);
			}
		});
	}
	
	private void openBook(boolean trial) {
		File book = getBookFile(trial);
//		FileInfo fileInfo = new FileInfo(book);
//		FileInfo parent = Services.getScanner().findParent(fileInfo, Services.getScanner().getRoot());
//		FileInfo bookFileInfo = parent.findItemByPathName(book.getAbsolutePath());
		dismiss();
		Activities.loadDocument(book.getAbsolutePath(), null);
	}
}

