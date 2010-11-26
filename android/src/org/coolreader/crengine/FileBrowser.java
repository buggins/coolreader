package org.coolreader.crengine;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.database.DataSetObserver;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Adapter;
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
		setChoiceMode(CHOICE_MODE_SINGLE);
		showDirectory( null );
	}
	
	@Override
	public boolean performItemClick(View view, int position, long id) {
		Log.d("cr3", "performItemClick("+position+")");
		//return super.performItemClick(view, position, id);
		if ( position==0 && currDirectory.parent!=null ) {
			showDirectory(currDirectory.parent);
			return true;
		}
		FileInfo item = (FileInfo) getAdapter().getItem(position);
		if ( item==null )
			return false;
		if ( item.isDirectory ) {
			showDirectory(item);
			return true;
		}
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
			if ( currDirectory.parent!=null ) {
				showDirectory(currDirectory.parent);
				return true;
			}
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
		mEngine.showProgress(1000, R.string.progress_scanning);
		execute( new Task() {
			public void work() {
				mHistory.loadFromDB(mScanner, 100);
			}
			public void done() {
				Log.e("cr3", "Directory scan is finished. " + mScanner.mFileList.size() + " files found" + ", root item count is " + mScanner.mRoot.size());
				mInitialized = true;
				//mEngine.hideProgress();
				showDirectory( mScanner.mRoot );
				setSelection(0);
			}
			public void fail(Exception e )
			{
				//mEngine.showProgress(9000, "Scan is failed");
				mEngine.hideProgress();
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
			return "(#" + number + " " + name +  ")";
		else
			return "(" + name + ")";
	}
	
	public static String formatDate( long timeStamp )
	{
		SimpleDateFormat format = new SimpleDateFormat("dd.MM.yy", Locale.getDefault());
		format.setTimeZone(java.util.TimeZone.getDefault());
		return format.format(new Date(timeStamp));
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
	public void showRecentBooks()
	{
		if ( mScanner.getRoot().getDir(0).fileCount()>0 ) {
			showDirectory(mScanner.getRoot().getDir(0));
		} else {
			showDirectory(mScanner.getRoot());
		}
	}
	public void showLastDirectory()
	{
		if ( currDirectory==null || currDirectory==mScanner.getRoot() )
			showRecentBooks();
		else
			showDirectory(currDirectory);
	}
	public void showDirectory( final FileInfo dir )
	{
		if ( dir!=null )
			mScanner.scanDirectory(dir, new Runnable() {
				public void run() {
					showDirectoryInternal(dir);
				}
			});
		else
			showDirectoryInternal(dir);
	}
	private void showDirectoryInternal( final FileInfo dir )
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
					if ( text!=null && text.length()>0 ) {
						view.setText(text);
						view.setVisibility(VISIBLE);
					} else {
						view.setText(null);
						view.setVisibility(INVISIBLE);
					}
				}
				void setItem(FileInfo item)
				{
					if ( item==null ) {
						image.setImageResource(R.drawable.cr3_browser_back);
						name.setText("..");
						author.setVisibility(INVISIBLE);
						series.setVisibility(INVISIBLE);
						field1.setVisibility(INVISIBLE);
						field2.setVisibility(INVISIBLE);
						field3.setVisibility(INVISIBLE);
						return;
					}
					if ( item.isDirectory ) {
						image.setImageResource(R.drawable.cr3_browser_folder);
						setText(name, item.filename);
						setText(author, null);
						setText(series, null);

						setText(field1, "books: " + String.valueOf(item.fileCount()));
						setText(field2, "folders: " + String.valueOf(item.dirCount()));
						setText(field3, null);
					} else {
						image.setImageResource(item.format.getIconResourceId());
						setText( author, formatAuthors(item.authors) );
						setText( series, formatSeries(item.series, item.seriesNumber) );
						String title = item.title;
						if ( title==null || title.length()==0 )
							title = item.filename;
						setText( name, title );

						field1.setVisibility(VISIBLE);
						field2.setVisibility(VISIBLE);
						field3.setVisibility(VISIBLE);
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
				if ( type == VIEW_TYPE_LEVEL_UP )
					item = null;
				holder.setItem(item);
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
		setSelection(0);
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
