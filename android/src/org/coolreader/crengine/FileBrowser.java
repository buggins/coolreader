package org.coolreader.crengine;

import java.io.File;
import java.net.MalformedURLException;
import java.net.URL;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collection;
import java.util.Locale;
import java.util.TimeZone;

import org.coolreader.CoolReader;
import org.coolreader.R;
import org.coolreader.crengine.OPDSUtil.DocInfo;
import org.coolreader.crengine.OPDSUtil.DownloadCallback;
import org.coolreader.crengine.OPDSUtil.EntryInfo;

import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.graphics.drawable.Drawable;
import android.view.ContextMenu;
import android.view.GestureDetector;
import android.view.GestureDetector.SimpleOnGestureListener;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Adapter;
import android.widget.AdapterView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

public class FileBrowser extends LinearLayout {

	public static final Logger log = L.create("fb");
	
	Engine mEngine;
	Scanner mScanner;
	CoolReader mActivity;
	LayoutInflater mInflater;
	History mHistory;
	ListView mListView;

	public static final int MAX_SUBDIR_LEN = 32;
	
	private class FileBrowserListView extends ListView {

		public FileBrowserListView(Context context) {
			super(context);
	        setFocusable(true);
	        setFocusableInTouchMode(true);
	        setLongClickable(true);
	        //registerForContextMenu(this);
	        //final FileBrowser _this = this;
	        setOnItemLongClickListener(new OnItemLongClickListener() {

				@Override
				public boolean onItemLongClick(AdapterView<?> arg0, View arg1,
						int position, long id) {
					log.d("onItemLongClick("+position+")");
					//return super.performItemClick(view, position, id);
					if (position == 0 && currDirectory.parent != null) {
						showParentDirectory();
						return true;
					}
					FileInfo item = (FileInfo) getAdapter().getItem(position);
					if ( item==null )
						return false;
					if (item.isDirectory && !item.isOPDSDir()) {
						showDirectory(item, null);
						return true;
					}
					//openContextMenu(_this);
					//mActivity.loadDocument(item);
					selectedItem = item;
					showContextMenu();
					return true;
				}
			});
			setChoiceMode(CHOICE_MODE_SINGLE);
		}
		
		@Override
		public void createContextMenu(ContextMenu menu) {
			log.d("createContextMenu()");
			menu.clear();
		    MenuInflater inflater = mActivity.getMenuInflater();
		    if ( isRecentDir() ) {
			    inflater.inflate(R.menu.cr3_file_browser_recent_context_menu, menu);
			    menu.setHeaderTitle(mActivity.getString(R.string.context_menu_title_recent_book));
		    } else if (selectedItem!=null && selectedItem.isOPDSDir()) {
			    inflater.inflate(R.menu.cr3_file_browser_opds_context_menu, menu);
			    menu.setHeaderTitle(mActivity.getString(R.string.menu_title_catalog));
		    } else if (selectedItem!=null && selectedItem.isDirectory) {
			    inflater.inflate(R.menu.cr3_file_browser_folder_context_menu, menu);
			    menu.setHeaderTitle(mActivity.getString(R.string.context_menu_title_book));
		    } else {
			    inflater.inflate(R.menu.cr3_file_browser_context_menu, menu);
			    menu.setHeaderTitle(mActivity.getString(R.string.context_menu_title_book));
		    }
		    for ( int i=0; i<menu.size(); i++ ) {
		    	menu.getItem(i).setOnMenuItemClickListener(new OnMenuItemClickListener() {
					public boolean onMenuItemClick(MenuItem item) {
						onContextItemSelected(item);
						return true;
					}
				});
		    }
		    return;
		}



		@Override
		public boolean performItemClick(View view, int position, long id) {
			log.d("performItemClick("+position+")");
			//return super.performItemClick(view, position, id);
			if (position == 0 && currDirectory.parent != null) {
				showParentDirectory();
				return true;
			}
			FileInfo item = (FileInfo) getAdapter().getItem(position);
			if ( item==null )
				return false;
			if ( item.isDirectory ) {
				showDirectory(item, null);
				return true;
			}
			if ( item.isOPDSDir() )
				showOPDSDir(item, null);
			else
				mActivity.loadDocument(item);
			return true;
		}

		@Override
		public boolean onKeyDown(int keyCode, KeyEvent event) {
			if ( keyCode==KeyEvent.KEYCODE_BACK && mActivity.isBookOpened() ) {
				if ( isRootDir() ) {
					if ( mActivity.isBookOpened() ) {
						mActivity.showReader();
						return true;
					} else
						return super.onKeyDown(keyCode, event);
				}
				showParentDirectory();
				return true;
			}
			if (keyCode==KeyEvent.KEYCODE_SEARCH) {
				showFindBookDialog();
				return true;
			}
			return super.onKeyDown(keyCode, event);
		}

		@Override
		public void setSelection(int position) {
			super.setSelection(position);
		}
		
	}
	
	public FileBrowser(CoolReader activity, Engine engine, Scanner scanner, History history) {
		super(activity);
		this.mActivity = activity;
		this.mEngine = engine;
		this.mScanner = scanner;
		this.mInflater = LayoutInflater.from(activity);// activity.getLayoutInflater();
		this.mHistory = history;
		setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
		createListView(true);
		showDirectory( null, null );
	}
	
	private void createListView(boolean recreateAdapter) {
		mListView = new FileBrowserListView(mActivity);
		final GestureDetector detector = new GestureDetector(new MyGestureListener());
		mListView.setOnTouchListener(new ListView.OnTouchListener() {
			@Override
			public boolean onTouch(View v, MotionEvent event) {
				return detector.onTouchEvent(event);
			}
		});
		if (currentListAdapter == null || recreateAdapter) {
			currentListAdapter = new FileListAdapter();
			mListView.setAdapter(currentListAdapter);
		} else {
			currentListAdapter.notifyDataSetChanged();
		}
		mListView.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
		removeAllViews();
		addView(mListView);
		mListView.setVisibility(VISIBLE);
	}
	
	public void onThemeChanged() {
		createListView(true);
		currentListAdapter.notifyDataSetChanged();
	}
	
	FileInfo selectedItem = null;
	
	public boolean onContextItemSelected(MenuItem item) {
		
		if ( selectedItem==null || (selectedItem.isDirectory && !selectedItem.isOPDSDir()) )
			return false;
			
		switch (item.getItemId()) {
		case R.id.book_open:
			log.d("book_open menu item selected");
			if ( selectedItem.isOPDSDir() )
				showOPDSDir(selectedItem, null);
			else
				mActivity.loadDocument(selectedItem);
			return true;
		case R.id.book_sort_order:
			mActivity.showToast("Sorry, sort order selection is not yet implemented");
			return true;
		case R.id.book_recent_books:
			showRecentBooks();
			return true;
		case R.id.book_opds_root:
			showOPDSRootDirectory();
			return true;
		case R.id.book_root:
			showRootDirectory();
			return true;
		case R.id.book_back_to_reading:
			if ( mActivity.isBookOpened() )
				mActivity.showReader();
			else
				mActivity.showToast("No book opened");
			return true;
		case R.id.book_delete:
			log.d("book_delete menu item selected");
			mActivity.getReaderView().closeIfOpened(selectedItem);
			if ( selectedItem.deleteFile() ) {
				mHistory.removeBookInfo(selectedItem, true, true);
			}
			showDirectory(currDirectory, null);
			return true;
		case R.id.book_recent_goto:
			log.d("book_recent_goto menu item selected");
			showDirectory(selectedItem, selectedItem);
			return true;
		case R.id.book_recent_remove:
			log.d("book_recent_remove menu item selected");
			mActivity.getHistory().removeBookInfo(selectedItem, true, false);
			showRecentBooks();
			return true;
		case R.id.catalog_add:
			log.d("catalog_add menu item selected");
			editOPDSCatalog(null);
			return true;
		case R.id.catalog_delete:
			log.d("catalog_delete menu item selected");
			if (selectedItem!=null && selectedItem.isOPDSDir()) {
				mActivity.getDB().removeOPDSCatalog(selectedItem.id);
				refreshOPDSRootDirectory();
			}
			return true;
		case R.id.catalog_edit:
			log.d("catalog_edit menu item selected");
			editOPDSCatalog(selectedItem);
			return true;
		case R.id.catalog_open:
			log.d("catalog_open menu item selected");
			showOPDSDir(selectedItem, null);
			return true;
		}
		return false;
	}
	
	private void editOPDSCatalog(FileInfo opds) {
		if (opds==null) {
			opds = new FileInfo();
			opds.isDirectory = true;
			opds.pathname = FileInfo.OPDS_DIR_PREFIX + "http://";
			opds.filename = "New Catalog";
			opds.isListed = true;
			opds.isScanned = true;
			opds.parent = mScanner.getOPDSRoot();
		}
		OPDSCatalogEditDialog dlg = new OPDSCatalogEditDialog(mActivity, opds, new Runnable() {
			@Override
			public void run() {
				refreshOPDSRootDirectory();
			}
		});
		dlg.show();
	}
	
	private void refreshOPDSRootDirectory() {
		FileInfo opdsRoot = mScanner.getOPDSRoot();
		if ( opdsRoot!=null ) {
			mActivity.getDB().loadOPDSCatalogs(opdsRoot);
			showDirectory(opdsRoot, null);
		}
	}
	

	protected void showParentDirectory()
	{
		if (currDirectory != null && currDirectory.parent != null) {
			showDirectory(currDirectory.parent, currDirectory);
		}
	}
	
	boolean mInitStarted = false;
//	boolean mInitialized = false;
	public void init()
	{
		if ( mInitStarted )
			return;
		log.e("FileBrowser.init() called");
		mInitStarted = true;
		//mEngine.showProgress(1000, R.string.progress_scanning);
		execute( new Task() {
			public void work() {
				mHistory.loadFromDB(mScanner, 100);
			}
			public void done() {
				log.e("Directory scan is finished. " + mScanner.mFileList.size() + " files found" + ", root item count is " + mScanner.mRoot.itemCount());
				//mInitialized = true;
				//mEngine.hideProgress();
				//mEngine.hideProgress();
				showDirectory( mScanner.mRoot, null );
				mListView.setSelection(0);
			}
			public void fail(Exception e )
			{
				//mEngine.showProgress(9000, "Scan is failed");
				//mEngine.hideProgress();
				mActivity.showToast("Scan is failed");
				log.e("Exception while scanning directories", e);
			}
		});
	}
	
	public static String formatAuthors( String authors ) {
		if ( authors==null || authors.length()==0 )
			return null;
		String[] list = authors.split("\\|");
		StringBuilder buf = new StringBuilder(authors.length());
		for ( String a : list ) {
			if ( buf.length()>0 )
				buf.append(", ");
			String[] items = a.split(" ");
			if ( items.length==3 && items[1]!=null && items[1].length()>=1 )
				buf.append(items[0] + " " + items[1].charAt(0) + ". " + items[2]);
			else
				buf.append(a);
		}
		return buf.toString();
	}
	
	public static String formatSize( int size )
	{
		if ( size==0 )
			return "";
		if ( size<10000 )
			return String.valueOf(size);
		else if ( size<1000000 )
			return String.valueOf(size/1000) + "K";
		else if ( size<10000000 )
			return String.valueOf(size/1000000) + "." + String.valueOf(size%1000000/100000) + "M";
		else
			return String.valueOf(size/1000000) + "M";
	}

	public static String formatSeries( String name, int number )
	{
		if ( name==null || name.length()==0 )
			return null;
		if ( number>0 )
			return "#" + number + " " + name;
		else
			return name;
	}
	
	static private ThreadLocal<SimpleDateFormat> dateFormatThreadLocal = new ThreadLocal<SimpleDateFormat>(); 
	static private ThreadLocal<SimpleDateFormat> timeFormatThreadLocal = new ThreadLocal<SimpleDateFormat>();
	static private SimpleDateFormat dateFormat() {
		if (dateFormatThreadLocal.get() == null)
			dateFormatThreadLocal.set(new SimpleDateFormat("dd.MM.yy", Locale.getDefault()));
		return dateFormatThreadLocal.get();
	}
	
	static private SimpleDateFormat timeFormat() {
		if (timeFormatThreadLocal.get() == null)
			timeFormatThreadLocal.set(new SimpleDateFormat("HH:mm", Locale.getDefault()));
		return timeFormatThreadLocal.get();
	}
	
	public static String formatDate( long timeStamp )
	{
		if ( timeStamp<5000*60*60*24*1000 )
			return "";
		TimeZone tz = java.util.TimeZone.getDefault();
		Calendar now = Calendar.getInstance(tz);
		Calendar c = Calendar.getInstance(tz);
		c.setTimeInMillis(timeStamp);
		if ( c.get(Calendar.YEAR)<1980 )
			return "";
		if ( c.get(Calendar.YEAR)==now.get(Calendar.YEAR)
				&& c.get(Calendar.MONTH)==now.get(Calendar.MONTH)
				&& c.get(Calendar.DAY_OF_MONTH)==now.get(Calendar.DAY_OF_MONTH)) {
			timeFormat().setTimeZone(tz);
			return timeFormat().format(c.getTime());
		} else {
			dateFormat().setTimeZone(tz);
			return dateFormat().format(c.getTime());
		}
	}

	public static String formatPercent( int percent )
	{
		if ( percent<=0 )
			return null;
		return String.valueOf(percent/100) + "." + String.valueOf(percent/10%10) + "%";
	}

	private FileInfo currDirectory;

	public boolean isRootDir()
	{
		return currDirectory == mScanner.getRoot();
	}

	public boolean isRecentDir()
	{
		return currDirectory!=null && currDirectory.isRecentDir();
	}

	public void showRecentBooks()
	{
		showDirectory(null, null);
	}

	public boolean isBookShownInRecentList(FileInfo book) {
		if (currDirectory==null || !currDirectory.isRecentDir())
			return false;
		return currDirectory.findItemByPathName(book.getPathName())!=null;
	}
	
	public void showLastDirectory()
	{
		if ( currDirectory==null || currDirectory==mScanner.getRoot() )
			showRecentBooks();
		else
			showDirectory(currDirectory, null);
	}

	public void showSearchResult( FileInfo[] books ) {
		FileInfo dir = mScanner.setSearchResults( books );
		showDirectory(dir, null);
	}
	
	public void showFindBookDialog()
	{
		BookSearchDialog dlg = new BookSearchDialog( mActivity, new BookSearchDialog.SearchCallback() {
			@Override
			public void done(FileInfo[] results) {
				if ( results!=null ) {
					if ( results.length==0 ) {
						mActivity.showToast(R.string.dlg_book_search_not_found);
					} else {
						showSearchResult( results );
					}
				}
			}
		});
		dlg.show();
	}

	public void showRootDirectory()
	{
		log.v("showRootDirectory()");
		showDirectory(mScanner.getRoot(), null);
	}

	public void showOPDSRootDirectory()
	{
		log.v("showOPDSRootDirectory()");
		FileInfo opdsRoot = mScanner.getOPDSRoot();
		if ( opdsRoot!=null )
			showDirectory(opdsRoot, null);
	}

	private FileInfo.SortOrder mSortOrder = FileInfo.DEF_SORT_ORDER; 
	public void setSortOrder(FileInfo.SortOrder order) {
		if ( mSortOrder == order )
			return;
		mSortOrder = order!=null ? order : FileInfo.DEF_SORT_ORDER;
		if ( currDirectory!=null && currDirectory.allowSorting() ) {
			currDirectory.sort(mSortOrder);
			showDirectory(currDirectory, null);
			mActivity.saveSetting(ReaderView.PROP_APP_BOOK_SORT_ORDER, mSortOrder.name());
		}
	}
	public void setSortOrder(String orderName) {
		setSortOrder(FileInfo.SortOrder.fromName(orderName));
	}
	public void showSortOrderMenu() {
		final Properties properties = new Properties();
		properties.setProperty(ReaderView.PROP_APP_BOOK_SORT_ORDER, mActivity.getSetting(ReaderView.PROP_APP_BOOK_SORT_ORDER));
		final String oldValue = properties.getProperty(ReaderView.PROP_APP_BOOK_SORT_ORDER);
		int[] optionLabels = {
			FileInfo.SortOrder.FILENAME.resourceId,	
			FileInfo.SortOrder.FILENAME_DESC.resourceId,	
			FileInfo.SortOrder.AUTHOR_TITLE.resourceId,	
			FileInfo.SortOrder.AUTHOR_TITLE_DESC.resourceId,	
			FileInfo.SortOrder.TITLE_AUTHOR.resourceId,	
			FileInfo.SortOrder.TITLE_AUTHOR_DESC.resourceId,	
			FileInfo.SortOrder.TIMESTAMP.resourceId,	
			FileInfo.SortOrder.TIMESTAMP_DESC.resourceId,	
		};
		String[] optionValues = {
			FileInfo.SortOrder.FILENAME.name(),	
			FileInfo.SortOrder.FILENAME_DESC.name(),	
			FileInfo.SortOrder.AUTHOR_TITLE.name(),	
			FileInfo.SortOrder.AUTHOR_TITLE_DESC.name(),	
			FileInfo.SortOrder.TITLE_AUTHOR.name(),	
			FileInfo.SortOrder.TITLE_AUTHOR_DESC.name(),	
			FileInfo.SortOrder.TIMESTAMP.name(),	
			FileInfo.SortOrder.TIMESTAMP_DESC.name(),	
		};
		OptionsDialog.ListOption dlg = new OptionsDialog.ListOption(
			new OptionOwner() {
				public CoolReader getActivity() { return mActivity; }
				public Properties getProperties() { return properties; }
				public LayoutInflater getInflater() { return mInflater; }
			}, 
			mActivity.getString(R.string.mi_book_sort_order), 
			ReaderView.PROP_APP_BOOK_SORT_ORDER).add(optionValues, optionLabels); 
		dlg.setOnChangeHandler(new Runnable() {
			public void run() {
				final String newValue = properties.getProperty(ReaderView.PROP_APP_BOOK_SORT_ORDER);
				if ( newValue!=null && oldValue!=null && !newValue.equals(oldValue) ) {
					log.d("New sort order: " + newValue);
					setSortOrder(newValue);
				}
			}
		});
		dlg.onSelect();
	}
	
	private void showOPDSDir( final FileInfo fileOrDir, final FileInfo itemToSelect ) {
		
		if ( fileOrDir.fileCount()>0 || fileOrDir.dirCount()>0 ) {
			// already downloaded
			BackgroundThread.instance().executeGUI(new Runnable() {
				@Override
				public void run() {
					showDirectoryInternal(fileOrDir, itemToSelect);					
				}
			});
			return;
		}
		
		String url = fileOrDir.getOPDSUrl();
		final FileInfo myCurrDirectory = currDirectory;
		if ( url!=null ) {
			try {
				final URL uri = new URL(url);
				DownloadCallback callback = new DownloadCallback() {

					@Override
					public void onEntries(DocInfo doc,
							Collection<EntryInfo> entries) {
						// TODO Auto-generated method stub
					}

					@Override
					public void onFinish(DocInfo doc,
							Collection<EntryInfo> entries) {
						if ( myCurrDirectory != currDirectory ) {
							log.w("current directory has been changed: ignore downloaded items");
							return;
						}
						ArrayList<FileInfo> items = new ArrayList<FileInfo>();
						for ( EntryInfo entry : entries ) {
							OPDSUtil.LinkInfo acquisition = entry.getBestAcquisitionLink();
							if ( acquisition!=null ) {
								FileInfo file = new FileInfo();
								file.isDirectory = false;
								file.pathname = FileInfo.OPDS_DIR_PREFIX + acquisition.href;
								file.filename = entry.content;
								file.title = entry.title;
								file.format = DocumentFormat.byMimeType(acquisition.type);
								file.authors = entry.getAuthors();
								file.isListed = true;
								file.isScanned = true;
								file.parent = fileOrDir;
								file.tag = entry;
								items.add(file);
							} else if ( entry.link.type!=null && entry.link.type.startsWith("application/atom+xml") ) {
								FileInfo file = new FileInfo();
								file.isDirectory = true;
								file.pathname = FileInfo.OPDS_DIR_PREFIX + entry.link.href;
								file.filename = entry.title;
								file.isListed = true;
								file.isScanned = true;
								file.tag = entry;
								file.parent = fileOrDir;
								items.add(file);
							}
						}
						if ( items.size()>0 ) {
							fileOrDir.replaceItems(items);
							showDirectoryInternal(fileOrDir, null);
						} else {
							mActivity.showToast("No OPDS entries found");
						}
					}

					@Override
					public void onError(String message) {
						mEngine.hideProgress();
						mActivity.showToast(message);
					}

					FileInfo downloadDir;
					@Override
					public File onDownloadStart(String type, String url) {
						//mEngine.showProgress(0, "Downloading " + url);
						//mActivity.showToast("Starting download of " + type + " from " + url);
						log.d("onDownloadStart: called for " + type + " " + url );
						downloadDir = mActivity.getScanner().getDownloadDirectory();
						log.d("onDownloadStart: after getDownloadDirectory()" );
						String subdir = null;
						if ( fileOrDir.authors!=null ) {
							subdir = OPDSUtil.transcribeFileName(fileOrDir.authors);
							if ( subdir.length()>MAX_SUBDIR_LEN )
								subdir = subdir.substring(0, MAX_SUBDIR_LEN);
						} else {
							subdir = "NoAuthor";
						}
						if ( downloadDir==null )
							return null;
						File result = new File(downloadDir.getPathName());
						result = new File(result, subdir);
						result.mkdirs();
						downloadDir.findItemByPathName(result.getAbsolutePath());
						log.d("onDownloadStart: returning " + result.getAbsolutePath() );
						return result;
					}

					@Override
					public void onDownloadEnd(String type, String url, File file) {
						mEngine.hideProgress();
						//mActivity.showToast("Download is finished");
						FileInfo fi = new FileInfo(file);
						FileInfo dir = mScanner.findParent(fi, downloadDir);
						if ( dir==null )
							dir = downloadDir;
						mScanner.listDirectory(dir);
						FileInfo item = dir.findItemByPathName(file.getAbsolutePath());
						if ( item!=null )
							mActivity.loadDocument(item);
						else
							mActivity.loadDocument(fi);
					}

					@Override
					public void onDownloadProgress(String type, String url,
							int percent) {
						mEngine.showProgress(percent * 100, "Downloading");
					}
					
				};
				String fileMimeType = fileOrDir.format!=null ? fileOrDir.format.getMimeFormat() : null;
				String defFileName = OPDSUtil.transcribeFileName( fileOrDir.title!=null ? fileOrDir.title : fileOrDir.filename );
				if ( fileOrDir.format!=null )
					defFileName = defFileName + fileOrDir.format.getExtensions()[0];
				final OPDSUtil.DownloadTask downloadTask = OPDSUtil.create(mActivity, uri, defFileName, fileOrDir.isDirectory?"application/atom+xml":fileMimeType, 
						myCurrDirectory.getOPDSUrl(), callback);
				downloadTask.run();
			} catch (MalformedURLException e) {
				log.e("MalformedURLException: " + url);
				mActivity.showToast("Wrong URI: " + url);
			}
		}
	}
	
	public void showDirectory( FileInfo fileOrDir, FileInfo itemToSelect )
	{
		BackgroundThread.ensureGUI();
		if ( fileOrDir!=null && fileOrDir.isOPDSDir() ) {
			showOPDSDir(fileOrDir, itemToSelect);
			return;
		}
		if (fileOrDir!=null && fileOrDir.isSearchShortcut()) {
			showFindBookDialog();
			return;
		}
		if (fileOrDir!=null && fileOrDir.isBooksByAuthorRoot()) {
			// refresh authors list
			log.d("Updating authors list");
			mActivity.getDB().loadAuthorsList(fileOrDir);
		}
		if (fileOrDir!=null && fileOrDir.isBooksByAuthorDir()) {
			log.d("Updating author book list");
			mActivity.getDB().loadAuthorBooks(fileOrDir);
		}
		
		if ( fileOrDir==null && mScanner.getRoot()!=null && mScanner.getRoot().dirCount()>0 ) {
			if ( mScanner.getRoot().getDir(0).fileCount()>0 ) {
				fileOrDir = mScanner.getRoot().getDir(0);
				itemToSelect = mScanner.getRoot().getDir(0).getFile(0);
			} else {
				fileOrDir = mScanner.getRoot();
				itemToSelect = mScanner.getRoot().dirCount()>1 ? mScanner.getRoot().getDir(1) : null;
			}
		}
		final FileInfo file = fileOrDir==null || fileOrDir.isDirectory ? itemToSelect : fileOrDir;
		final FileInfo dir = fileOrDir!=null && !fileOrDir.isDirectory ? mScanner.findParent(file, mScanner.getRoot()) : fileOrDir;
		if ( dir!=null ) {
			mScanner.scanDirectory(dir, new Runnable() {
				public void run() {
					if ( dir.allowSorting() )
						dir.sort(mSortOrder);
					showDirectoryInternal(dir, file);
				}
			}, false, new Scanner.ScanControl() );
		} else
			showDirectoryInternal(null, file);
	}
	
	public void scanCurrentDirectoryRecursive() {
		if ( currDirectory==null )
			return;
		log.i("scanCurrentDirectoryRecursive started");
		final Scanner.ScanControl control = new Scanner.ScanControl(); 
		final ProgressDialog dlg = ProgressDialog.show(mActivity, 
				mActivity.getString(R.string.dlg_scan_title), 
				mActivity.getString(R.string.dlg_scan_message),
				true, true, new OnCancelListener() {
					@Override
					public void onCancel(DialogInterface dialog) {
						log.i("scanCurrentDirectoryRecursive : stop handler");
						control.stop();
					}
		});
		mScanner.scanDirectory(currDirectory, new Runnable() {
			@Override
			public void run() {
				log.i("scanCurrentDirectoryRecursive : finish handler");
				if ( dlg.isShowing() )
					dlg.dismiss();
			}
		}, true, control); 
	}


	public boolean isSimpleViewMode() {
		return isSimpleViewMode;
	}

	public void setSimpleViewMode( boolean isSimple ) {
		if ( isSimpleViewMode!=isSimple ) {
			isSimpleViewMode = isSimple;
			mSortOrder = FileInfo.SortOrder.FILENAME;
			mActivity.saveSetting(ReaderView.PROP_APP_BOOK_SORT_ORDER, mSortOrder.name());
			if ( isShown() && currDirectory!=null ) {
				showDirectory(currDirectory, null);
			}
		}
	}
	private boolean isSimpleViewMode = true;

	private FileListAdapter currentListAdapter;
	
	private class FileListAdapter extends BaseListAdapter {
		public boolean areAllItemsEnabled() {
			return true;
		}

		public boolean isEnabled(int arg0) {
			return true;
		}

		public int getCount() {
			if (currDirectory == null)
				return 0;
			return currDirectory.fileCount() + currDirectory.dirCount() + (currDirectory.parent!=null ? 1 : 0);
		}

		public Object getItem(int position) {
			if (currDirectory == null)
				return null;
			if ( position<0 )
				return null;
			int start = (currDirectory.parent!=null ? 1 : 0);
			if ( position<start )
				return currDirectory.parent;
			return currDirectory.getItem(position-start);
		}

		public long getItemId(int position) {
			if (currDirectory == null)
				return 0;
			return position;
		}

		public final int VIEW_TYPE_LEVEL_UP = 0;
		public final int VIEW_TYPE_DIRECTORY = 1;
		public final int VIEW_TYPE_FILE = 2;
		public final int VIEW_TYPE_FILE_SIMPLE = 3;
		public final int VIEW_TYPE_COUNT = 4;
		public int getItemViewType(int position) {
			if (currDirectory == null)
				return 0;
			if (position < 0)
				return Adapter.IGNORE_ITEM_VIEW_TYPE;
			int start = (currDirectory.parent!=null ? 1 : 0);
			if (position<start)
				return VIEW_TYPE_LEVEL_UP;
			if (position<start + currDirectory.dirCount())
				return VIEW_TYPE_DIRECTORY;
			start += currDirectory.dirCount();
			position -= start;
			if (position < currDirectory.fileCount())
				return isSimpleViewMode ? VIEW_TYPE_FILE_SIMPLE : VIEW_TYPE_FILE;
			return Adapter.IGNORE_ITEM_VIEW_TYPE;
		}

		class ViewHolder {
			int viewType;
			ImageView image;
			TextView name;
			TextView author;
			TextView series;
			TextView filename;
			TextView field1;
			TextView field2;
			//TextView field3;
			void setText( TextView view, String text )
			{
				if ( view==null )
					return;
				if ( text!=null && text.length()>0 ) {
					view.setText(text);
					view.setVisibility(ViewGroup.VISIBLE);
				} else {
					view.setText(null);
					view.setVisibility(ViewGroup.INVISIBLE);
				}
			}
			void setItem(FileInfo item, FileInfo parentItem)
			{
				if ( item==null ) {
					image.setImageResource(R.drawable.cr3_browser_back);
					String thisDir = "";
					if ( parentItem!=null ) {
						if ( parentItem.pathname.startsWith("@") )
							thisDir = "/" + parentItem.filename;
//						else if ( parentItem.isArchive )
//							thisDir = parentItem.arcname;
						else
							thisDir = parentItem.pathname;
						//parentDir = parentItem.path;
					}
					name.setText(thisDir);
					return;
				}
				if ( item.isDirectory ) {
					if (item.isBooksByAuthorRoot())
						image.setImageResource(R.drawable.cr3_browser_folder_authors);
					else if (item.isOPDSRoot() || item.isOPDSDir())
						image.setImageResource(R.drawable.cr3_browser_folder_opds);
					else if (item.isSearchShortcut())
						image.setImageResource(R.drawable.cr3_browser_find);
					else if ( item.isRecentDir() )
						image.setImageResource(R.drawable.cr3_browser_folder_recent);
					else if ( item.isArchive )
						image.setImageResource(R.drawable.cr3_browser_folder_zip);
					else
						image.setImageResource(R.drawable.cr3_browser_folder);
					setText(name, item.filename);

					if ( item.isBooksByAuthorDir() ) {
						int bookCount = 0;
						if (item.fileCount() > 0)
							bookCount = item.fileCount();
						else if (item.tag != null && item.tag instanceof Integer)
							bookCount = (Integer)item.tag;
						setText(field1, "books: " + String.valueOf(bookCount));
						setText(field2, "folders: 0");
					} else  if ( !item.isOPDSDir() && !item.isSearchShortcut() && (!item.isBooksByAuthorRoot() || item.dirCount()>0)) {
						setText(field1, "books: " + String.valueOf(item.fileCount()));
						setText(field2, "folders: " + String.valueOf(item.dirCount()));
					} else {
						setText(field1, "");
						setText(field2, "");
					}
				} else {
					boolean isSimple = (viewType == VIEW_TYPE_FILE_SIMPLE);
					if ( image!=null ) {
						if ( isSimple ) {
							image.setImageResource(item.format.getIconResourceId());
						} else {
							Drawable drawable = null;
							if ( item.id!=null )
								drawable = mHistory.getBookCoverpageImage(null, item.id);
							if ( drawable!=null ) {
								image.setImageDrawable(drawable);
							} else {
								int resId = item.format!=null ? item.format.getIconResourceId() : 0;
								if ( resId!=0 )
									image.setImageResource(item.format.getIconResourceId());
							}
						}
					}
					if ( isSimple ) {
						String fn = item.getFileNameToDisplay();
						setText( filename, fn );
					} else {
						setText( author, formatAuthors(item.authors) );
						String seriesName = formatSeries(item.series, item.seriesNumber);
						String title = item.title;
						String filename1 = item.filename;
						String filename2 = item.isArchive /*&& !item.isDirectory */
								? new File(item.arcname).getName() : null;
						if ( title==null || title.length()==0 ) {
							title = filename1;
							if (seriesName==null) 
								seriesName = filename2;
						} else if (seriesName==null) 
							seriesName = filename1;
						setText( name, title );
						setText( series, seriesName );

//						field1.setVisibility(VISIBLE);
//						field2.setVisibility(VISIBLE);
//						field3.setVisibility(VISIBLE);
						if (field1 != null)
							field1.setText(formatSize(item.size) + " " + (item.format!=null ? item.format.name().toLowerCase() : "") + " " + formatDate(item.createTime) + "  ");
						//field2.setText(formatDate(pos!=null ? pos.getTimeStamp() : item.createTime));
						if (field2 != null) {
							Bookmark pos = mHistory.getLastPos(item);
							if ( pos!=null ) {
								field2.setText(formatPercent(pos.getPercent()) + " " + formatDate(pos.getTimeStamp())) ;
							} else {
								field2.setText("");
							}
						}
						//field3.setText(pos!=null ? formatPercent(pos.getPercent()) : null);
					} 
					
				}
			}
		}
		
		public View getView(int position, View convertView, ViewGroup parent) {
			if (currDirectory == null)
				return null;
			View view;
			ViewHolder holder;
			int vt = getItemViewType(position);
			if (convertView == null) {
				if ( vt==VIEW_TYPE_LEVEL_UP )
					view = mInflater.inflate(R.layout.browser_item_parent_dir, null);
				else if ( vt==VIEW_TYPE_DIRECTORY )
					view = mInflater.inflate(R.layout.browser_item_folder, null);
				else if ( vt==VIEW_TYPE_FILE_SIMPLE )
					view = mInflater.inflate(R.layout.browser_item_book_simple, null);
				else
					view = mInflater.inflate(R.layout.browser_item_book, null);
				holder = new ViewHolder();
				holder.image = (ImageView)view.findViewById(R.id.book_icon);
				holder.name = (TextView)view.findViewById(R.id.book_name);
				holder.author = (TextView)view.findViewById(R.id.book_author);
				holder.series = (TextView)view.findViewById(R.id.book_series);
				holder.filename = (TextView)view.findViewById(R.id.book_filename);
				holder.field1 = (TextView)view.findViewById(R.id.browser_item_field1);
				holder.field2 = (TextView)view.findViewById(R.id.browser_item_field2);
				//holder.field3 = (TextView)view.findViewById(R.id.browser_item_field3);
				view.setTag(holder);
			} else {
				view = convertView;
				holder = (ViewHolder)view.getTag();
			}
			holder.viewType = vt;
			FileInfo item = (FileInfo)getItem(position);
			FileInfo parentItem = null;//item!=null ? item.parent : null;
			if ( vt == VIEW_TYPE_LEVEL_UP ) {
				item = null;
				parentItem = currDirectory;
			}
			holder.setItem(item, parentItem);
//			if ( DeviceInfo.FORCE_LIGHT_THEME ) {
//				view.setBackgroundColor(Color.WHITE);
//			}
			return view;
		}

		public int getViewTypeCount() {
			if (currDirectory == null)
				return 1;
			return VIEW_TYPE_COUNT;
		}

		public boolean hasStableIds() {
			return true;
		}

		public boolean isEmpty() {
			if (currDirectory == null)
				return true;
			return mScanner.mFileList.size()==0;
		}

	}
	
	private void showDirectoryInternal( final FileInfo dir, final FileInfo file )
	{
		BackgroundThread.ensureGUI();
		currDirectory = dir;
		if ( dir!=null )
			log.i("Showing directory " + dir + " " + Thread.currentThread().getName());
		if ( !BackgroundThread.instance().isGUIThread() )
			throw new IllegalStateException("showDirectoryInternal should be called from GUI thread!");
		int index = dir!=null ? dir.getItemIndex(file) : -1;
		if ( dir!=null && !dir.isRootDir() )
			index++;
		mListView.setAdapter(currentListAdapter);
		currentListAdapter.notifyDataSetChanged();
		mListView.setSelection(index);
		mListView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
		mListView.invalidate();
	}

	private class MyGestureListener extends SimpleOnGestureListener {

		@Override
		public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX,
				float velocityY) {
			if (e1 == null || e2 == null)
				return false;
			int thresholdDistance = mActivity.getPalmTipPixels() * 2;
			int thresholdVelocity = mActivity.getPalmTipPixels();
			int x1 = (int)e1.getX();
			int x2 = (int)e2.getX();
			int y1 = (int)e1.getY();
			int y2 = (int)e2.getY();
			int dist = x2 - x1;
			int adist = dist > 0 ? dist : -dist;
			int ydist = y2 - y1;
			int aydist = ydist > 0 ? ydist : -ydist;
			int vel = (int)velocityX;
			if (vel<0)
				vel = -vel;
			if (vel > thresholdVelocity && adist > thresholdDistance && adist > aydist * 2) {
				if (dist > 0) {
					log.d("LTR fling detected: moving to parent");
					showParentDirectory();
					return true;
				} else {
					log.d("RTL fling detected: show menu");
					mActivity.openOptionsMenu();
					return true;
				}
			}
			return false;
		}
		
	}
	
	private void execute( Engine.EngineTask task )
    {
    	mEngine.execute(task);
    }

    private abstract class Task implements Engine.EngineTask {
    	
		public void done() {
			// override to do something useful
		}

		public void fail(Exception e) {
			// do nothing, just log exception
			// override to do custom action
			log.e("Task " + this.getClass().getSimpleName() + " is failed with exception " + e.getMessage(), e);
		}
    }
    
}
