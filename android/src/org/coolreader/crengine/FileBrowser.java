package org.coolreader.crengine;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Locale;
import java.util.TimeZone;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.database.DataSetObserver;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Adapter;
import android.widget.AdapterView;
import android.widget.ImageView;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class FileBrowser extends ListView {

	Engine mEngine;
	Scanner mScanner;
	CoolReader mActivity;
	LayoutInflater mInflater;
	History mHistory;
	
	public FileBrowser(CoolReader activity, Engine engine, Scanner scanner, History history) {
		super(activity);
		this.mActivity = activity;
		this.mEngine = engine;
		this.mScanner = scanner;
		this.mInflater = LayoutInflater.from(activity);// activity.getLayoutInflater();
		this.mHistory = history;
        setFocusable(true);
        setFocusableInTouchMode(true);
        setLongClickable(true);
        //registerForContextMenu(this);
        //final FileBrowser _this = this;
        setOnItemLongClickListener(new OnItemLongClickListener() {

			@Override
			public boolean onItemLongClick(AdapterView<?> arg0, View arg1,
					int position, long id) {
				Log.d("cr3", "onItemLongClick("+position+")");
				//return super.performItemClick(view, position, id);
				if ( position==0 && currDirectory.parent!=null ) {
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
				//openContextMenu(_this);
				//mActivity.loadDocument(item);
				selectedItem = item;
				showContextMenu();
				return true;
			}
		});
		setChoiceMode(CHOICE_MODE_SINGLE);
		showDirectory( null, null );
	}
	
	FileInfo selectedItem = null;
	
	public boolean onContextItemSelected(MenuItem item) {
		
		if ( selectedItem==null || selectedItem.isDirectory )
			return false;
			
		switch (item.getItemId()) {
		case R.id.book_open:
			Log.d("cr3", "book_open menu item selected");
			mActivity.loadDocument(selectedItem);
			return true;
		case R.id.book_sort_order:
			mActivity.showToast("Sorry, sort order selection is not yet implemented");
			return true;
		case R.id.book_recent_books:
			showRecentBooks();
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
			Log.d("cr3", "book_delete menu item selected");
			mActivity.getReaderView().closeIfOpened(selectedItem);
			if ( selectedItem.deleteFile() ) {
				mHistory.removeBookInfo(selectedItem, true, true);
			}
			showDirectory(currDirectory, null);
			return true;
		case R.id.book_recent_goto:
			Log.d("cr3", "book_recent_goto menu item selected");
			showDirectory(selectedItem, selectedItem);
			return true;
		case R.id.book_recent_remove:
			Log.d("cr3", "book_recent_remove menu item selected");
			mActivity.getHistory().removeBookInfo(selectedItem, true, false);
			showRecentBooks();
			return true;
		}
		return false;
	}
	
	@Override
	public void createContextMenu(ContextMenu menu) {
		Log.d("cr3", "createContextMenu()");
		menu.clear();
	    MenuInflater inflater = mActivity.getMenuInflater();
	    if ( isRecentDir() ) {
		    inflater.inflate(R.menu.cr3_file_browser_recent_context_menu, menu);
		    menu.setHeaderTitle(mActivity.getString(R.string.context_menu_title_recent_book));
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
		Log.d("cr3", "performItemClick("+position+")");
		//return super.performItemClick(view, position, id);
		if ( position==0 && currDirectory.parent!=null ) {
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
		mActivity.loadDocument(item);
		return true;
	}

	protected void showParentDirectory()
	{
		if ( currDirectory.parent!=null ) {
			showDirectory(currDirectory.parent, currDirectory);
		}
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
		return super.onKeyDown(keyCode, event);
	}

	boolean mInitStarted = false;
	boolean mInitialized = false;
	public void init()
	{
		if ( mInitStarted )
			return;
		Log.e("cr3", "FileBrowser.init() called");
		mInitStarted = true;
		//mEngine.showProgress(1000, R.string.progress_scanning);
		execute( new Task() {
			public void work() {
				mHistory.loadFromDB(mScanner, 100);
			}
			public void done() {
				Log.e("cr3", "Directory scan is finished. " + mScanner.mFileList.size() + " files found" + ", root item count is " + mScanner.mRoot.itemCount());
				mInitialized = true;
				//mEngine.hideProgress();
				//mEngine.hideProgress();
				showDirectory( mScanner.mRoot, null );
				setSelection(0);
			}
			public void fail(Exception e )
			{
				//mEngine.showProgress(9000, "Scan is failed");
				//mEngine.hideProgress();
				mActivity.showToast("Scan is failed");
				Log.e("cr3", "Exception while scanning directories", e);
			}
		});
	}
	
	@Override
	public void setSelection(int position) {
		super.setSelection(position);
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
	
	static private SimpleDateFormat dateFormat = new SimpleDateFormat("dd.MM.yy", Locale.getDefault());
	static private SimpleDateFormat timeFormat = new SimpleDateFormat("HH:mm", Locale.getDefault());
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
			timeFormat.setTimeZone(tz);
			return timeFormat.format(c.getTime());
		} else {
			dateFormat.setTimeZone(tz);
			return dateFormat.format(c.getTime());
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

	public void showLastDirectory()
	{
		if ( currDirectory==null || currDirectory==mScanner.getRoot() )
			showRecentBooks();
		else
			showDirectory(currDirectory, null);
	}

	public void showRootDirectory()
	{
		showDirectory(mScanner.getRoot(), null);
	}

	public void showDirectory( FileInfo fileOrDir, FileInfo itemToSelect )
	{
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
					if ( !dir.isRootDir() && !dir.isRecentDir() )
						dir.sort(FileInfo.DEF_SORT_ORDER);
					showDirectoryInternal(dir, file);
				}
			});
		} else
			showDirectoryInternal(dir, file);
	}

	private void showDirectoryInternal( final FileInfo dir, final FileInfo file )
	{
		currDirectory = dir;
		if ( dir!=null )
			Log.i("cr3", "Showing directory " + dir);
		this.setAdapter(new ListAdapter() {

			public boolean areAllItemsEnabled() {
				return true;
			}

			public boolean isEnabled(int arg0) {
				return true;
			}

			public int getCount() {
				if ( dir==null )
					return 0;
				return dir.fileCount() + dir.dirCount() + (dir.parent!=null ? 1 : 0);
			}

			public Object getItem(int position) {
				if ( dir==null )
					return null;
				if ( position<0 )
					return null;
				int start = (dir.parent!=null ? 1 : 0);
				if ( position<start )
					return dir.parent;
				return dir.getItem(position-start);
			}

			public long getItemId(int position) {
				if ( dir==null )
					return 0;
				return position;
			}

			public final int VIEW_TYPE_LEVEL_UP = 0;
			public final int VIEW_TYPE_DIRECTORY = 1;
			public final int VIEW_TYPE_FILE = 2;
			public int getItemViewType(int position) {
				if ( dir==null )
					return 0;
				if ( position<0 )
					return Adapter.IGNORE_ITEM_VIEW_TYPE;
				int start = (dir.parent!=null ? 1 : 0);
				if ( position<start )
					return VIEW_TYPE_LEVEL_UP;
				if ( position<start + dir.dirCount() )
					return VIEW_TYPE_DIRECTORY;
				start += dir.dirCount();
				position -= start;
				if ( position<dir.fileCount() )
					return VIEW_TYPE_FILE;
				return Adapter.IGNORE_ITEM_VIEW_TYPE;
			}

			class ViewHolder {
				ImageView image;
				TextView name;
				TextView author;
				TextView series;
				TextView field1;
				TextView field2;
				TextView field3;
				void setText( TextView view, String text )
				{
					if ( view==null )
						return;
					if ( text!=null && text.length()>0 ) {
						view.setText(text);
						view.setVisibility(VISIBLE);
					} else {
						view.setText(null);
						view.setVisibility(INVISIBLE);
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
//							else if ( parentItem.isArchive )
//								thisDir = parentItem.arcname;
							else
								thisDir = parentItem.pathname;
							//parentDir = parentItem.path;
						}
						name.setText(thisDir);
						return;
					}
					if ( item.isDirectory ) {
						if ( item.isRecentDir() )
							image.setImageResource(R.drawable.cr3_browser_folder_recent);
						else if ( item.isArchive )
							image.setImageResource(R.drawable.cr3_browser_folder_zip);
						else
							image.setImageResource(R.drawable.cr3_browser_folder);
						setText(name, item.filename);

						setText(field1, "books: " + String.valueOf(item.fileCount()));
						setText(field2, "folders: " + String.valueOf(item.dirCount()));
					} else {
						if ( image!=null ) {
							Drawable drawable = null;
							if ( item.id!=null )
								drawable = mHistory.getBookCoverpageImage(null, item.id);
							if ( drawable!=null ) {
								image.setImageDrawable(drawable);
							} else {
								image.setImageResource(item.format.getIconResourceId());
							}
						}
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
						field1.setText(formatSize(item.size));
						Bookmark pos = mHistory.getLastPos(item);
						field2.setText(formatDate(pos!=null ? pos.getTimeStamp() : item.createTime));
						field3.setText(pos!=null ? formatPercent(pos.getPercent()) : null);
						
					}
				}
			}
			
			public View getView(int position, View convertView, ViewGroup parent) {
				if ( dir==null )
					return null;
				View view;
				ViewHolder holder;
				if ( convertView==null ) {
					int vt = getItemViewType(position);
					if ( vt==VIEW_TYPE_LEVEL_UP )
						view = mInflater.inflate(R.layout.browser_item_parent_dir, null);
					else if ( vt==VIEW_TYPE_DIRECTORY )
						view = mInflater.inflate(R.layout.browser_item_folder, null);
					else
						view = mInflater.inflate(R.layout.browser_item_book, null);
					holder = new ViewHolder();
					holder.image = (ImageView)view.findViewById(R.id.book_icon);
					holder.name = (TextView)view.findViewById(R.id.book_name);
					holder.author = (TextView)view.findViewById(R.id.book_author);
					holder.series = (TextView)view.findViewById(R.id.book_series);
					holder.field1 = (TextView)view.findViewById(R.id.browser_item_field1);
					holder.field2 = (TextView)view.findViewById(R.id.browser_item_field2);
					holder.field3 = (TextView)view.findViewById(R.id.browser_item_field3);
					view.setTag(holder);
				} else {
					view = convertView;
					holder = (ViewHolder)view.getTag();
				}
				int type = getItemViewType(position);
				FileInfo item = (FileInfo)getItem(position);
				FileInfo parentItem = null;//item!=null ? item.parent : null;
				if ( type == VIEW_TYPE_LEVEL_UP ) {
					item = null;
					parentItem = currDirectory;
				}
				holder.setItem(item, parentItem);
				return view;
			}

			public int getViewTypeCount() {
				if ( dir==null )
					return 1;
				return 3;
			}

			public boolean hasStableIds() {
				return true;
			}

			public boolean isEmpty() {
				if ( dir==null )
					return true;
				return mScanner.mFileList.size()==0;
			}

			private ArrayList<DataSetObserver> observers = new ArrayList<DataSetObserver>();
			
			public void registerDataSetObserver(DataSetObserver observer) {
				observers.add(observer);
			}

			public void unregisterDataSetObserver(DataSetObserver observer) {
				observers.remove(observer);
			}
			
		});
		int index = dir!=null ? dir.getItemIndex(file) : -1;
		if ( dir!=null && !dir.isRootDir() )
			index++;
		setSelection(index);
		setChoiceMode(CHOICE_MODE_SINGLE);
		invalidate();
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
			Log.e("cr3", "Task " + this.getClass().getSimpleName() + " is failed with exception " + e.getMessage(), e);
		}
    }
    
}
