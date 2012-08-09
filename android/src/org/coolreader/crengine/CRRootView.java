package org.coolreader.crengine;

import java.io.File;
import java.util.ArrayList;

import org.coolreader.CoolReader;
import org.coolreader.R;
import org.coolreader.crengine.CoverpageManager.CoverpageReadyListener;
import org.coolreader.db.CRDBService;
import org.coolreader.db.CRDBService.OPDSCatalogsLoadingCallback;
import org.coolreader.plugins.AsyncResponse;
import org.coolreader.plugins.litres.LitresConnection;
import org.coolreader.plugins.litres.LitresPlugin;
import org.coolreader.plugins.litres.LitresConnection.ResultHandler;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class CRRootView extends ViewGroup {

	public static final Logger log = L.create("cr");

	private final CoolReader mActivity;
	private ViewGroup mView;
	private LinearLayout mRecentBooksScroll;
	private LinearLayout mFilesystemScroll;
	private LinearLayout mLibraryScroll;
	private LinearLayout mOnlineCatalogsScroll;
	private CoverpageManager mCoverpageManager;
	private int coverWidth;
	private int coverHeight;
	private BookInfo currentBook;
	private CoverpageReadyListener coverpageListener;
	public CRRootView(CoolReader activity) {
		super(activity);
		this.mActivity = activity;
		this.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
		this.mCoverpageManager = Services.getCoverpageManager();


		int screenHeight = mActivity.getWindowManager().getDefaultDisplay().getHeight();
		int screenWidth = mActivity.getWindowManager().getDefaultDisplay().getWidth();
		int h = screenHeight / 4;
		int w = screenWidth / 4;
		if (h > w)
			h = w;
    	w = h * 3 / 4;
    	coverWidth = w;
    	coverHeight = h;
		createViews();
		
	}
	
	public void onThemeChange(InterfaceTheme theme) {
		createViews();
	}
	
	public void onClose() {
		this.mCoverpageManager.removeCoverpageReadyListener(coverpageListener);
		coverpageListener = null;
		super.onDetachedFromWindow();
	}

	private void setBookInfoItem(ViewGroup baseView, int viewId, String value) {
		TextView view = (TextView)baseView.findViewById(viewId);
		if (view != null) {
			if (value != null && value.length() > 0) {
				view.setText(value);
			} else {
				view.setText("");
			}
		}
	}
	
	private void updateCurrentBook(BookInfo book) {
    	currentBook = book;
    	
    	// set current book cover page
		ImageView cover = (ImageView)mView.findViewById(R.id.book_cover);
		if (currentBook != null) {
			FileInfo item = currentBook.getFileInfo();
			cover.setImageDrawable(mCoverpageManager.getCoverpageDrawableFor(mActivity.getDB(), item, coverWidth, coverHeight));
			cover.setMinimumHeight(coverHeight);
			cover.setMinimumWidth(coverWidth);
			cover.setMaxHeight(coverHeight);
			cover.setMaxWidth(coverWidth);

			setBookInfoItem(mView, R.id.lbl_book_author, Utils.formatAuthors(item.authors));
			setBookInfoItem(mView, R.id.lbl_book_title, currentBook.getFileInfo().title);
			setBookInfoItem(mView, R.id.lbl_book_series, Utils.formatSeries(item.series, item.seriesNumber));
			String state = Utils.formatReadingState(mActivity, item);
			state = state + " " + Utils.formatFileInfo(item) + " ";
			state = state + " " + Utils.formatLastPosition(Services.getHistory().getLastPos(item));
			setBookInfoItem(mView, R.id.lbl_book_info, state);
		} else {
			log.w("No current book in history");
			cover.setImageDrawable(null);
			cover.setMinimumHeight(0);
			cover.setMinimumWidth(0);
			cover.setMaxHeight(0);
			cover.setMaxWidth(0);

			setBookInfoItem(mView, R.id.lbl_book_author, "");
			setBookInfoItem(mView, R.id.lbl_book_title, "No last book"); // TODO: i18n
			setBookInfoItem(mView, R.id.lbl_book_series, "");
		}
	}	
	
	private final static int MAX_RECENT_BOOKS = 12;
	private void updateRecentBooks(ArrayList<BookInfo> books) {
		ArrayList<FileInfo> files = new ArrayList<FileInfo>();
		for (int i = 1; i <= MAX_RECENT_BOOKS && i < books.size(); i++)
			files.add(books.get(i).getFileInfo());
		files.add(Services.getScanner().createRecentRoot());
		LayoutInflater inflater = LayoutInflater.from(mActivity);
		mRecentBooksScroll.removeAllViews();
		for (final FileInfo item : files) {
			final View view = inflater.inflate(R.layout.root_item_recent_book, null);
			ImageView cover = (ImageView)view.findViewById(R.id.book_cover);
			TextView label = (TextView)view.findViewById(R.id.book_name);
			cover.setMinimumHeight(coverHeight);
			cover.setMaxHeight(coverHeight);
			cover.setMaxWidth(coverWidth);
			if (item.isRecentDir()) {
				cover.setImageResource(R.drawable.cr3_button_next);
				if (label != null) {
					label.setText("");
				}
				view.setOnClickListener(new OnClickListener() {
					@Override
					public void onClick(View v) {
						Activities.showRecentBooks();
					}
				});
			} else {
				cover.setMinimumWidth(coverWidth);
				cover.setImageDrawable(mCoverpageManager.getCoverpageDrawableFor(mActivity.getDB(), item, coverWidth, coverHeight));
				if (label != null) {
					String title = item.title;
					String authors = Utils.formatAuthors(item.authors);
					String s = item.getFileNameToDisplay();
					if (!Utils.empty(title) && !Utils.empty(authors))
						s = title + " - " + authors;
					else if (!Utils.empty(title))
						s = title;
					else if (!Utils.empty(authors))
						s = authors;
					label.setText(s != null ? s : "");
					label.setMaxWidth(coverWidth);
				}
				view.setOnClickListener(new OnClickListener() {
					@Override
					public void onClick(View v) {
						Activities.loadDocument(item);
					}
				});
				view.setOnLongClickListener(new OnLongClickListener() {
					@Override
					public boolean onLongClick(View v) {
						Activities.editBookInfo(mActivity, Services.getScanner().createRecentRoot(), item);
						return true;
					}
				});
			}
			mRecentBooksScroll.addView(view);
		}
		mRecentBooksScroll.invalidate();
	}

	public void refreshRecentBooks() {
		BackgroundThread.instance().postGUI(new Runnable() {
			@Override
			public void run() {
				Services.getHistory().getOrLoadRecentBooks(mActivity.getDB(), new CRDBService.RecentBooksLoadingCallback() {
					@Override
					public void onRecentBooksListLoaded(ArrayList<BookInfo> bookList) {
						updateCurrentBook(bookList != null && bookList.size() > 0 ? bookList.get(0) : null);
						updateRecentBooks(bookList);
					}
				});
			}
		});
	}

	public void refreshOnlineCatalogs() {
		mActivity.getDB().loadOPDSCatalogs(new OPDSCatalogsLoadingCallback() {
			@Override
			public void onOPDSCatalogsLoaded(ArrayList<FileInfo> catalogs) {
				updateOnlineCatalogs(catalogs);
			}
		});
	}
	
	private void updateOnlineCatalogs(ArrayList<FileInfo> catalogs) {
		catalogs.add(0, Services.getScanner().createOnlineLibraryPluginItem("org.coolreader.plugins.litres", "LitRes"));
		catalogs.add(Services.getScanner().createOPDSRoot());
		LayoutInflater inflater = LayoutInflater.from(mActivity);
		mOnlineCatalogsScroll.removeAllViews();
		for (final FileInfo item : catalogs) {
			final View view = inflater.inflate(R.layout.root_item_online_catalog, null);
			ImageView icon = (ImageView)view.findViewById(R.id.item_icon);
			TextView label = (TextView)view.findViewById(R.id.item_name);
			if (item.isOPDSRoot()) {
				icon.setImageResource(R.drawable.cr3_browser_folder_opds_add);
				label.setText("Add");
				view.setOnClickListener(new OnClickListener() {
					@Override
					public void onClick(View v) {
						Activities.editOPDSCatalog(null);
					}
				});
			} else if (item.isOnlineCatalogPluginDir()) {
				icon.setImageResource(R.drawable.plugins_logo_litres);
				label.setText(item.filename);
				view.setOnClickListener(new OnClickListener() {
					@Override
					public void onClick(View v) {
						Activities.showBrowser(FileInfo.ONLINE_CATALOG_PLUGIN_PREFIX + LitresPlugin.PACKAGE_NAME);
//						LitresConnection.instance().loadGenres(new ResultHandler() {
//							@Override
//							public void onResponse(LitresResponse response) {
//								if (response instanceof LitresConnection.LitresGenre) {
//									LitresConnection.LitresGenre result = (LitresConnection.LitresGenre)response;
//									log.d("genres found: " + result.getChildCount() + " on top level");
//								}
//							}
//						});
//						LitresConnection.instance().authorize("login", "password", new ResultHandler() {
//							@Override
//							public void onResponse(LitresResponse response) {
//								if (response instanceof LitresConnection.LitresAuthInfo) {
//									LitresConnection.LitresAuthInfo result = (LitresConnection.LitresAuthInfo)response;
//									log.d("authorization successful: " + result);
//								} else {
//									log.d("authorization failed");
//								}
//							}
//						});
//						LitresConnection.instance().loadAuthorsByLastName("л", new ResultHandler() {
//							@Override
//							public void onResponse(LitresResponse response) {
//								if (response instanceof LitresConnection.LitresAuthors) {
//									LitresConnection.LitresAuthors result = (LitresConnection.LitresAuthors)response;
//									log.d("authors found: " + result.size());
//									for (int i=0; i<result.size() && i<10; i++) {
//										log.d(result.get(i).toString());
//									}
//								}
//							}
//						});
//						mActivity.showToast("TODO");
					}
				});
			} else {
				if (label != null) {
					label.setText(item.getFileNameToDisplay());
					label.setMaxWidth(coverWidth * 3 / 2);
				}
				view.setOnClickListener(new OnClickListener() {
					@Override
					public void onClick(View v) {
						Activities.showCatalog(item);
					}
				});
				view.setOnLongClickListener(new OnLongClickListener() {
					@Override
					public boolean onLongClick(View v) {
						Activities.editOPDSCatalog(item);
						return true;
					}
				});
			}
			mOnlineCatalogsScroll.addView(view);
		}
		mOnlineCatalogsScroll.invalidate();
	}
	
	private void updateFilesystems(ArrayList<FileInfo> dirs) {

		LayoutInflater inflater = LayoutInflater.from(mActivity);
		mFilesystemScroll.removeAllViews();
		for (int i = 0; i < dirs.size(); i++) {
			final FileInfo item = dirs.get(i);
			final View view = inflater.inflate(R.layout.root_item_dir, null);
			ImageView icon = (ImageView)view.findViewById(R.id.item_icon);
			TextView label = (TextView)view.findViewById(R.id.item_name);
			if (i == dirs.size() - 1)
				icon.setImageResource(R.drawable.cr3_browser_folder_user);
			else
				icon.setImageResource(R.drawable.cr3_browser_folder_database);
			label.setText(item.pathname);
			label.setMaxWidth(coverWidth * 25 / 10);
			view.setOnClickListener(new OnClickListener() {
				@Override
				public void onClick(View v) {
					Activities.showDirectory(item);
				}
			});
			mFilesystemScroll.addView(view);
		}
		mFilesystemScroll.invalidate();
	}

	private void updateLibraryItems(ArrayList<FileInfo> dirs) {
		LayoutInflater inflater = LayoutInflater.from(mActivity);
		mLibraryScroll.removeAllViews();
		for (final FileInfo item : dirs) {
			final View view = inflater.inflate(R.layout.root_item_library, null);
			ImageView image = (ImageView)view.findViewById(R.id.item_icon);
			TextView label = (TextView)view.findViewById(R.id.item_name);
			if (item.isSearchShortcut())
				image.setImageResource(R.drawable.cr3_browser_find);
			else if (item.isBooksByAuthorRoot() || item.isBooksByTitleRoot() || item.isBooksBySeriesRoot())
				image.setImageResource(R.drawable.cr3_browser_folder_authors);
			if (label != null) {
				label.setText(item.filename);
				label.setMinWidth(coverWidth);
				label.setMaxWidth(coverWidth * 2);
			}
			view.setOnClickListener(new OnClickListener() {
				@Override
				public void onClick(View v) {
					Activities.showDirectory(item);
				}
			});
			mLibraryScroll.addView(view);
		}
		mLibraryScroll.invalidate();
	}

//	private HorizontalListView createHScroll(int layoutId, OnLongClickListener longClickListener) {
//		LinearLayout layout = (LinearLayout)mView.findViewById(layoutId);
//		layout.removeAllViews();
//		HorizontalListView view = new HorizontalListView(mActivity, null);
//		view.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
////		view.setFadingEdgeLength(10);
////		view.setHorizontalFadingEdgeEnabled(true);
//		layout.addView(view);
//		if (longClickListener != null)
//			layout.setOnLongClickListener(longClickListener); 
//		return view;
//	}
	
	private void updateDelimiterTheme(int viewId) {
		View view = mView.findViewById(viewId);
		InterfaceTheme theme = mActivity.getCurrentTheme();
		view.setBackgroundResource(theme.getRootDelimiterResourceId());
		view.setMinimumHeight(theme.getRootDelimiterHeight());
		view.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.FILL_PARENT, theme.getRootDelimiterHeight()));
	}
	
	private void createViews() {
		LayoutInflater inflater = LayoutInflater.from(mActivity);
		View view = inflater.inflate(R.layout.root_window, null);
		mView = (ViewGroup)view;
		
		updateDelimiterTheme(R.id.delimiter1);
		updateDelimiterTheme(R.id.delimiter2);
		updateDelimiterTheme(R.id.delimiter3);
		updateDelimiterTheme(R.id.delimiter4);
		updateDelimiterTheme(R.id.delimiter5);
		
		mRecentBooksScroll = (LinearLayout)mView.findViewById(R.id.scroll_recent_books);
		
		mFilesystemScroll = (LinearLayout)mView.findViewById(R.id.scroll_filesystem);

		mLibraryScroll = (LinearLayout)mView.findViewById(R.id.scroll_library);
		
		mOnlineCatalogsScroll = (LinearLayout)mView.findViewById(R.id.scroll_online_catalogs);

		updateCurrentBook(Services.getHistory().getLastBook());
		
//		((ImageButton)mView.findViewById(R.id.btn_recent_books)).setOnClickListener(new OnClickListener() {
//			@Override
//			public void onClick(View v) {
//				Activities.showRecentBooks();
//			}
//		});
//
//		((ImageButton)mView.findViewById(R.id.btn_online_catalogs)).setOnClickListener(new OnClickListener() {
//			@Override
//			public void onClick(View v) {
//				Activities.showOnlineCatalogs();
//			}
//		});
		
		mView.findViewById(R.id.current_book).setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				if (currentBook != null) {
					Activities.loadDocument(currentBook.getFileInfo());
				}
				
			}
		});
		mView.findViewById(R.id.current_book).setOnLongClickListener(new OnLongClickListener() {
			@Override
			public boolean onLongClick(View v) {
				if (currentBook != null)
					Activities.editBookInfo(mActivity, Services.getScanner().createRecentRoot(), currentBook.getFileInfo());
				return true;
			}
		});

		refreshRecentBooks();

		coverpageListener =	new CoverpageReadyListener() {
			@Override
			public void onCoverpagesReady(ArrayList<CoverpageManager.ImageItem> files) {
				mView.invalidate();
			}
		};
		this.mCoverpageManager.addCoverpageReadyListener(coverpageListener);

		BackgroundThread.instance().postGUI(new Runnable() {
			@Override
			public void run() {
				refreshOnlineCatalogs();
			}
		});

		BackgroundThread.instance().postGUI(new Runnable() {
			@Override
			public void run() {
				ArrayList<FileInfo> dirs = new ArrayList<FileInfo>();
				File[] roots = Engine.getStorageDirectories(false);
				for (File f : roots) {
					FileInfo dir = new FileInfo(f);
					dirs.add(dir);
				}
				dirs.add(Services.getScanner().getDownloadDirectory());
				updateFilesystems(dirs);
				
				updateLibraryItems(Services.getScanner().getLibraryItems());
			}
		});
		
		removeAllViews();
		addView(mView);
	}

	@Override
	protected void onLayout(boolean changed, int l, int t, int r, int b) {
		r -= l;
		b -= t;
		t = 0;
		l = 0;
		mView.layout(l, t, r, b);
	}

	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		mView.measure(widthMeasureSpec, heightMeasureSpec);
        setMeasuredDimension(mView.getMeasuredWidth(), mView.getMeasuredHeight());
	}
	
	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		super.onSizeChanged(w, h, oldw, oldh);
	}
}
