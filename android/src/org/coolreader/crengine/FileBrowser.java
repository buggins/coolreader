package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.content.Context;
import android.database.DataSetObserver;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Adapter;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class FileBrowser extends ListView {

	Engine engine;
	Scanner scanner;
	CoolReader activity;
	LayoutInflater inflater;
	public FileBrowser(CoolReader activity, Engine engine, Scanner scanner) {
		super(activity);
		this.activity = activity;
		this.engine = engine;
		this.scanner = scanner;
		this.inflater = LayoutInflater.from(activity);// activity.getLayoutInflater();
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
		final ListView thisView = this;
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
						author.setVisibility(INVISIBLE);
						series.setVisibility(INVISIBLE);
						name.setText(item.filename);

						field1.setVisibility(VISIBLE);
						field2.setVisibility(VISIBLE);
						field3.setVisibility(INVISIBLE);
						field1.setText("books: " + String.valueOf(item.fileCount()));
						field2.setText("folders: " + String.valueOf(item.dirCount()));
					} else {
						image.setImageResource(item.format.getIconResourceId());
						setText( author, item.authors );
						setText( series, item.series );
						String title = item.title;
						if ( title==null || title.length()==0 )
							title = item.filename;
						setText( name, title );

						field1.setVisibility(VISIBLE);
						field2.setVisibility(VISIBLE);
						field3.setVisibility(VISIBLE);
						field1.setText(String.valueOf(item.size/1024) + "K");
						field2.setText("01/02/2010");
						field3.setText("25%");
						
					}
				}
			}
			
			public View getView(int position, View convertView, ViewGroup parent) {
				if ( dir==null )
					return null;
				View view;
				ViewHolder holder;
				if ( convertView==null ) {
					view = inflater.inflate(R.layout.browser_item_book, null);
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
    
//    private class SimpleItemView extends View {
//    	public SimpleItemView( Context context, FileInfo item )
//    	{
//    		super(context);
//    		mTitle = new TextView(context);
//    		setItem( item );
//    		addView( mTitle, new LinearLayout.LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
//    	}
//    	
//    	public void setItem( FileInfo item )
//    	{
//    		if ( item==null ) {
//        		mTitle.setText("..");
//    		} else {
//        		mTitle.setText(item.filename);
//    		}
//    	}
//    	private TextView mTitle;
//    }
}
