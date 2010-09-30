package org.coolreader.crengine;

import java.util.ArrayList;
import java.util.Properties;

import org.coolreader.R;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.database.DataSetObserver;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TabHost;
import android.widget.TabHost.TabContentFactory;

public class OptionsDialog  extends AlertDialog {

	TabHost mTabs;
	OptionsListView mOptionsView;
	Properties mProperties;
	OptionsListView mOptionsStyles;
	OptionsListView mOptionsApplication;
	OptionsListView mOptionsControls;
	
	public OptionsDialog( Context context )
	{
		super(context);
	}
	
	class OptionsListView extends ListView {
		public OptionsListView( Context context )
		{
			super(context);
			setAdapter( new ListAdapter() {
				public boolean areAllItemsEnabled() {
					return true;
				}

				public boolean isEnabled(int arg0) {
					return true;
				}

				public int getCount() {
					return 0;
				}

				public Object getItem(int position) {
					return null;
				}

				public long getItemId(int position) {
					return position;
				}

				public int getItemViewType(int position) {
					return 0;
				}

				
				public View getView(int position, View convertView, ViewGroup parent) {
					return null;
				}

				public int getViewTypeCount() {
					return 1;
				}

				public boolean hasStableIds() {
					return true;
				}

				public boolean isEmpty() {
					return true;
				}

				private ArrayList<DataSetObserver> observers = new ArrayList<DataSetObserver>();
				
				public void registerDataSetObserver(DataSetObserver observer) {
					observers.add(observer);
				}

				public void unregisterDataSetObserver(DataSetObserver observer) {
					observers.remove(observer);
				}
			});
		}
		
	}
	
	@Override
	public ListView getListView() {
		return super.getListView();
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.v("cr3", "creating OptionsDialog");
		// setup tabs
		setContentView(R.layout.options);
		mTabs = (TabHost)findViewById(android.R.id.tabhost); 
		//new TabHost(getContext());
		mOptionsStyles = new OptionsListView(getContext());
		mOptionsApplication = new OptionsListView(getContext());
		mOptionsControls = new OptionsListView(getContext());
		TabHost.TabSpec tsStyles = mTabs.newTabSpec("Styles");
		tsStyles.setIndicator("Styles");
		tsStyles.setContent(new TabContentFactory() {
			public View createTabContent(String arg0) {
				return mOptionsStyles;
			}
		});
		mTabs.addTab(tsStyles);
		TabHost.TabSpec tsApp = mTabs.newTabSpec("App");
		tsApp.setIndicator("App");
		tsApp.setContent(new TabContentFactory() {
			public View createTabContent(String arg0) {
				return mOptionsApplication;
			}
		});
		mTabs.addTab(tsApp);
		TabHost.TabSpec tsControls = mTabs.newTabSpec("Controls");
		tsControls.setIndicator("Controls");
		tsControls.setContent(new TabContentFactory() {
			public View createTabContent(String arg0) {
				return mOptionsControls;
			}
		});
		mTabs.addTab(tsControls);

		setView(mTabs);
		
		// setup buttons
        setButton(AlertDialog.BUTTON_POSITIVE, "Ok", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                cancel();
                // TODO: add handler here
            }
        });
 
        setButton(AlertDialog.BUTTON_NEGATIVE, "Cancel",
            new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int whichButton) {
                    dialog.cancel();
                    // TODO: add handler here
                }
            });
		
		super.onCreate(savedInstanceState);
		Log.v("cr3", "OptionsDialog is created");
	}

}
