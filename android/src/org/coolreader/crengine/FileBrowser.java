package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.CoolReader;

import android.content.Context;
import android.database.DataSetObserver;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Adapter;
import android.widget.LinearLayout;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class FileBrowser extends ListView {

	Engine engine;
	Scanner scanner;
	CoolReader activity;
	public FileBrowser(CoolReader activity, Engine engine, Scanner scanner) {
		super(activity);
		this.activity = activity;
		this.engine = engine;
		this.scanner = scanner;
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
		activity.loadDocument(item);
		return true;
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		// TODO Auto-generated method stub
		if ( keyCode==KeyEvent.KEYCODE_BACK && activity.isBookOpened() ) {
			activity.showReader();
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}

	private boolean started = false;
	public void start()
	{
		if ( started ) {
			//showDirectory( scanner.root );
			return;
		}
		engine.showProgress(20, "Scanning directories...");
		execute( new Task() {
			public void work() {
				scanner.scan();
			}
			public void done() {
				Log.e("cr3", "Directory scan is finished. " + scanner.fileList.size() + " files found" + ", root item count is " + scanner.root.size());
				started = true;
				engine.hideProgress();
				showDirectory( scanner.root );
				setSelection(0);
			}
			public void fail(Exception e )
			{
				engine.showProgress(9000, "Scan is failed");
				Log.e("cr3", "Exception while scanning directories", e);
			}
		});
	}

	@Override
	public void setSelection(int position) {
		super.setSelection(position);
	}

	private FileInfo currDirectory;
	public void showDirectory( final FileInfo dir )
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

			public View getView(int position, View convertView, ViewGroup parent) {
				if ( dir==null )
					return null;
				SimpleItemView view;
				if ( convertView==null ) {
					view = new SimpleItemView(getContext(), (FileInfo)getItem(position));					
				} else {
					view = (SimpleItemView)convertView;
					view.setItem((FileInfo)getItem(position));
				}
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
				return scanner.fileList.size()==0;
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
    	engine.execute(task);
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
    
    private class SimpleItemView extends LinearLayout {
    	public SimpleItemView( Context context, FileInfo item )
    	{
    		super(context);
    		this.setOrientation(VERTICAL);
    		mTitle = new TextView(context);
    		setItem( item );
    		addView( mTitle, new LinearLayout.LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
    	}
    	public void setItem( FileInfo item )
    	{
    		if ( item==null ) {
        		mTitle.setText("..");
    		} else {
        		mTitle.setText(item.filename);
    		}
    	}
    	private TextView mTitle;
    }
}
