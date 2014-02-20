package org.coolreader.crengine;

import android.view.*;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import org.coolreader.CoolReader;
import org.coolreader.R;
import org.coolreader.crengine.CRToolBar.OnActionHandler;
import org.coolreader.crengine.CoverpageManager.CoverpageReadyListener;
import org.coolreader.db.CRDBService;
import org.coolreader.db.CRDBService.OPDSCatalogsLoadingCallback;
import org.coolreader.plugins.OnlineStorePluginManager;
import org.coolreader.plugins.OnlineStoreWrapper;
import org.coolreader.plugins.litres.LitresPlugin;

import java.util.ArrayList;
import java.util.List;

import static android.view.ContextMenu.ContextMenuInfo;

public class CRRootView extends ViewGroup implements CoverpageReadyListener {

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
    	setFocusable(true);
    	setFocusableInTouchMode(true);
		createViews();
		
	}
	
	
	
	private long menuDownTs = 0;
	private long backDownTs = 0;

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (event.getKeyCode() == KeyEvent.KEYCODE_MENU) {
			//L.v("CRRootView.onKeyDown(" + keyCode + ")");
			if (event.getRepeatCount() == 0)
				menuDownTs = Utils.timeStamp();
			return true;
		}
		if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
			//L.v("CRRootView.onKeyDown(" + keyCode + ")");
			if (event.getRepeatCount() == 0)
				backDownTs = Utils.timeStamp();
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if (event.getKeyCode() == KeyEvent.KEYCODE_MENU) {
			long duration = Utils.timeInterval(menuDownTs);
			L.v("CRRootView.onKeyUp(" + keyCode + ") duration = " + duration);
			if (duration > 700 && duration < 10000)
				mActivity.showBrowserOptionsDialog();
			else
				showMenu();
			return true;
		}
		if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
			long duration = Utils.timeInterval(backDownTs);
			L.v("CRRootView.onKeyUp(" + keyCode + ") duration = " + duration);
			if (duration > 700 && duration < 10000 || !mActivity.isBookOpened()) {
				mActivity.finish();
				return true;
			} else {
				mActivity.showReader();
				return true;
			}
		}
		return super.onKeyUp(keyCode, event);
	}



	private InterfaceTheme lastTheme;
	public void onThemeChange(InterfaceTheme theme) {
		if (lastTheme != theme) {
			lastTheme = theme;
			createViews();
		}
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
			cover.setTag(new CoverpageManager.ImageItem(item, coverWidth, coverHeight));

			setBookInfoItem(mView, R.id.lbl_book_author, Utils.formatAuthors(item.authors));
			setBookInfoItem(mView, R.id.lbl_book_title, currentBook.getFileInfo().title);
			setBookInfoItem(mView, R.id.lbl_book_series, Utils.formatSeries(item.series, item.seriesNumber));
			String state = Utils.formatReadingState(mActivity, item);
			state = state + " " + Utils.formatFileInfo(mActivity, item) + " ";
			if (Services.getHistory() != null)
				state = state + " " + Utils.formatLastPosition(mActivity, Services.getHistory().getLastPos(item));
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
		if (books.size() > MAX_RECENT_BOOKS)
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
					label.setText("More...");
				}
				view.setOnClickListener(new OnClickListener() {
					@Override
					public void onClick(View v) {
						mActivity.showRecentBooks();
					}
				});
			} else {
				cover.setMinimumWidth(coverWidth);
				cover.setTag(new CoverpageManager.ImageItem(item, coverWidth, coverHeight));
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
						mActivity.loadDocument(item);
					}
				});
				view.setOnLongClickListener(new OnLongClickListener() {
					@Override
					public boolean onLongClick(View v) {
						mActivity.editBookInfo(Services.getScanner().createRecentRoot(), item);
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
				if (Services.getHistory() != null && mActivity.getDB() != null)
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
		mActivity.waitForCRDBService(new Runnable() {
			@Override
			public void run() {
				mActivity.getDB().loadOPDSCatalogs(new OPDSCatalogsLoadingCallback() {
					@Override
					public void onOPDSCatalogsLoaded(ArrayList<FileInfo> catalogs) {
						updateOnlineCatalogs(catalogs);
					}
				});
			}
		});
	}

    public void refreshFileSystemFolders() {
        ArrayList<FileInfo> folders = Services.getFileSystemFolders().getFileSystemFolders();
        updateFilesystems(folders);
    }

	ArrayList<FileInfo> lastCatalogs = new ArrayList<FileInfo>();
	private void updateOnlineCatalogs(ArrayList<FileInfo> catalogs) {
		String lang = mActivity.getCurrentLanguage();
		boolean defEnableLitres = lang.toLowerCase().startsWith("ru") && !DeviceInfo.POCKETBOOK;
		boolean enableLitres = mActivity.settings().getBool(Settings.PROP_APP_PLUGIN_ENABLED + "." + OnlineStorePluginManager.PLUGIN_PKG_LITRES, defEnableLitres);
		if (enableLitres)
			catalogs.add(0, Scanner.createOnlineLibraryPluginItem(OnlineStorePluginManager.PLUGIN_PKG_LITRES, "LitRes"));
		if (Services.getScanner() == null)
			return;
		FileInfo opdsRoot = Services.getScanner().getOPDSRoot();
		if (opdsRoot.dirCount() == 0)
			opdsRoot.addItems(catalogs);
		catalogs.add(0, opdsRoot);
		
//		if (lastCatalogs.equals(catalogs)) {
//			return; // not changed
//		}
		lastCatalogs = catalogs;
		
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
						mActivity.editOPDSCatalog(null);
					}
				});
			} else if (item.isOnlineCatalogPluginDir()) {
				icon.setImageResource(R.drawable.plugins_logo_litres);
				label.setText(item.filename);
				view.setOnLongClickListener(new OnLongClickListener() {
					@Override
					public boolean onLongClick(View v) {
						OnlineStoreWrapper plugin = OnlineStorePluginManager.getPlugin(mActivity, FileInfo.ONLINE_CATALOG_PLUGIN_PREFIX + LitresPlugin.PACKAGE_NAME);
						if (plugin != null) {
							OnlineStoreLoginDialog dlg = new OnlineStoreLoginDialog(mActivity, plugin, new Runnable() {
								@Override
								public void run() {
									mActivity.showBrowser(FileInfo.ONLINE_CATALOG_PLUGIN_PREFIX + LitresPlugin.PACKAGE_NAME);
								}
							});
							dlg.show();
						}
						return true;
					}
				});
				view.setOnClickListener(new OnClickListener() {
					@Override
					public void onClick(View v) {
						mActivity.showBrowser(FileInfo.ONLINE_CATALOG_PLUGIN_PREFIX + LitresPlugin.PACKAGE_NAME);
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
						mActivity.showCatalog(item);
					}
				});
				view.setOnLongClickListener(new OnLongClickListener() {
					@Override
					public boolean onLongClick(View v) {
						mActivity.editOPDSCatalog(item);
						return true;
					}
				});
			}
			mOnlineCatalogsScroll.addView(view);
		}
		mOnlineCatalogsScroll.invalidate();
	}

	private void updateFilesystems(List<FileInfo> dirs) {

		LayoutInflater inflater = LayoutInflater.from(mActivity);
		mFilesystemScroll.removeAllViews();
        int idx = 0;
        for (final FileInfo item : dirs) {
            if (item == null)
                continue;
            final View view = inflater.inflate(R.layout.root_item_dir, null);
            ImageView icon = (ImageView) view.findViewById(R.id.item_icon);
            TextView label = (TextView) view.findViewById(R.id.item_name);
            if (item.getType() == FileInfo.TYPE_DOWNLOAD_DIR)
                icon.setImageResource(R.drawable.folder_bookmark);
            else if (item.getType() == FileInfo.TYPE_FS_ROOT)
                icon.setImageResource(R.drawable.media_flash_sd_mmc);
            else
                icon.setImageResource(R.drawable.folder_blue);
            label.setText(item.pathname);
            label.setMaxWidth(coverWidth * 25 / 10);
            view.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    mActivity.showDirectory(item);
                }
            });
            view.setOnLongClickListener(new OnLongClickListener() {
                @Override
                public boolean onLongClick(View view) {
                    registerFoldersContextMenu(item);
                    return false;
                }
            });
            mFilesystemScroll.addView(view);
            ++idx;
        }
		mFilesystemScroll.invalidate();
	}

    private void registerFoldersContextMenu(final FileInfo folder) {
        mActivity.registerForContextMenu(mFilesystemScroll);
        mFilesystemScroll.setOnCreateContextMenuListener(new OnCreateContextMenuListener() {
            @Override
            public void onCreateContextMenu(ContextMenu contextMenu, View view, ContextMenuInfo contextMenuInfo) {
                MenuInflater inflater = mActivity.getMenuInflater();
                inflater.inflate(R.menu.cr3_favorite_folder_context_menu,contextMenu);
                boolean isFavorite = folder.getType() == FileInfo.TYPE_NOT_SET;
                final FileSystemFolders service = Services.getFileSystemFolders();
                for(int idx = 0 ; idx< contextMenu.size(); ++idx){
                    MenuItem item = contextMenu.getItem(idx);
                    boolean enabled = isFavorite;
                    if(item.getItemId() == R.id.folder_left) {
                        enabled = enabled && service.canMove(folder, true);
                        if(enabled)
                            item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
                                @Override
                                public boolean onMenuItemClick(MenuItem menuItem) {
                                    service.moveFavoriteFolder(mActivity.getDB(), folder, true);
                                    return true;
                                }
                            });
                    } else if(item.getItemId() == R.id.folder_right) {
                        enabled = enabled && service.canMove(folder, false);
                        if(enabled)
                            item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
                                @Override
                                public boolean onMenuItemClick(MenuItem menuItem) {
                                    service.moveFavoriteFolder(mActivity.getDB(), folder, false);
                                    return true;
                                }
                            });
                    } else if(item.getItemId() == R.id.folder_remove) {
                        if(enabled)
                            item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
                                @Override
                                public boolean onMenuItemClick(MenuItem menuItem) {
                                    service.removeFavoriteFolder(mActivity.getDB(), folder);
                                    return true;
                                }
                            });
                    }
                    item.setEnabled(enabled);
                }
            }
        });
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
					mActivity.showDirectory(item);
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
		
//		((ImageButton)mView.findViewById(R.id.btn_settings)).setOnClickListener(new OnClickListener() {
//			@Override
//			public void onClick(View v) {
//				showSettings();
//			}
//		});

		((ImageButton)mView.findViewById(R.id.btn_menu)).setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				showMenu();
			}
		});

		mView.findViewById(R.id.current_book).setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				if (currentBook != null) {
					mActivity.loadDocument(currentBook.getFileInfo());
				}
				
			}
		});
		mView.findViewById(R.id.current_book).setOnLongClickListener(new OnLongClickListener() {
			@Override
			public boolean onLongClick(View v) {
				if (currentBook != null)
					mActivity.editBookInfo(Services.getScanner().createRecentRoot(), currentBook.getFileInfo());
				return true;
			}
		});

		refreshRecentBooks();

        Services.getFileSystemFolders().addListener(new FileInfoChangeListener() {
            @Override
            public void onChange(FileInfo object, boolean onlyProperties) {
                BackgroundThread.instance().postGUI(new Runnable() {
              			@Override
              			public void run() {
              				refreshFileSystemFolders();
              			}
              		});
            }
        });

        mActivity.waitForCRDBService(new Runnable() {
            @Override
            public void run() {
                Services.getFileSystemFolders().loadFavoriteFolders(mActivity.getDB());
            }
        });

		BackgroundThread.instance().postGUI(new Runnable() {
			@Override
			public void run() {
				refreshOnlineCatalogs();
			}
		});

		BackgroundThread.instance().postGUI(new Runnable() {
			@Override
			public void run() {
				if (Services.getScanner() != null)
					updateLibraryItems(Services.getScanner().getLibraryItems());
			}
		});

		removeAllViews();
		addView(mView);
		//setFocusable(false);
		//setFocusableInTouchMode(false);
//		requestFocus();
//		setOnTouchListener(new OnTouchListener() {
//			@Override
//			public boolean onTouch(View v, MotionEvent event) {
//				return true;
//			}
//		});
	}



	@Override
	public boolean onTouchEvent(MotionEvent event) {
		log.d("CRRootView.onTouchEvent(" + event.getAction() + ")");
		return false;
	}
	
	

	@Override
	public void onWindowFocusChanged(boolean hasWindowFocus) {
		log.d("CRRootView.onWindowFocusChanged(" + hasWindowFocus + ")");
		super.onWindowFocusChanged(hasWindowFocus);
	}

	public void onCoverpagesReady(ArrayList<CoverpageManager.ImageItem> files) {
		//invalidate();
		log.d("CRRootView.onCoverpagesReady(" + files + ")");
		CoverpageManager.invalidateChildImages(mView, files);
//		for (int i=0; i<mRecentBooksScroll.getChildCount(); i++) {
//			mRecentBooksScroll.getChildAt(i).invalidate();
//		}
//		//mRecentBooksScroll.invalidate();
		//ImageView cover = (ImageView)mView.findViewById(R.id.book_cover);
		//cover.invalidate();
//		//mView.invalidate();
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

	public void showMenu() {
		ReaderAction[] actions = {
			ReaderAction.ABOUT,
			ReaderAction.CURRENT_BOOK,
			ReaderAction.RECENT_BOOKS,
			ReaderAction.USER_MANUAL,
			ReaderAction.OPTIONS,
			ReaderAction.EXIT,	
		};
		mActivity.showActionsPopupMenu(actions, new OnActionHandler() {
			@Override
			public boolean onActionSelected(ReaderAction item) {
				if (item == ReaderAction.EXIT) {
					mActivity.finish();
					return true;
				} else if (item == ReaderAction.ABOUT) {
					mActivity.showAboutDialog();
					return true;
				} else if (item == ReaderAction.RECENT_BOOKS) {
					mActivity.showRecentBooks();
					return true;
				} else if (item == ReaderAction.CURRENT_BOOK) {
					mActivity.showCurrentBook();
					return true;
				} else if (item == ReaderAction.USER_MANUAL) {
					mActivity.showManual();
					return true;
				} else if (item == ReaderAction.OPTIONS) {
					mActivity.showBrowserOptionsDialog();
					return true;
				}
				return false;
			}
		});
	}

	
	
}
