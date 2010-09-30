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
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TabHost;
import android.widget.TextView;
import android.widget.TabHost.TabContentFactory;

public class OptionsDialog  extends AlertDialog implements TabContentFactory {

	TabHost mTabs;
	OptionsListView mOptionsView;
	Properties mProperties;
	OptionsListView mOptionsStyles;
	OptionsListView mOptionsApplication;
	OptionsListView mOptionsControls;
	
	class OptionBase {
		public String label;
		public String property;
		public String getValueLabel() { return mProperties.getProperty(property); }
		public void onSelect() { }
	}
	
	class BoolOption {
		public String label;
		public String property;
		public String getValueLabel() { return "".equals(mProperties.getProperty(property)) ? "on" : "off"; }
		public void onSelect() { 
			mProperties.setProperty(property, "".equals(mProperties.getProperty(property)) ? "0" : "1");
		}
	}
	
	public OptionsDialog( Context context )
	{
		super(context);
	}
	
	class OptionsListView extends ListView {
		private ArrayList<OptionBase> mOptions = new ArrayList<OptionBase>();
		public OptionsListView add( OptionBase option ) {
			mOptions.add(option);
			return this;
		}
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
					return mOptions.size();
				}

				public Object getItem(int position) {
					return mOptions.get(position);
				}

				public long getItemId(int position) {
					return position;
				}

				public int getItemViewType(int position) {
					return 0;
				}

				
				public View getView(int position, View convertView, ViewGroup parent) {
					TextView view;
					if ( convertView==null ) {
						view = new TextView(getContext());
					} else {
						view = (TextView)convertView;
					}
					view.setText(mOptions.get(position).label);
					return view;
				}

				public int getViewTypeCount() {
					return 1;
				}

				public boolean hasStableIds() {
					return true;
				}

				public boolean isEmpty() {
					return mOptions.size()==0;
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

	public View createTabContent(String tag) {
		if ( "App".equals(tag) )
			return mOptionsApplication;
		else if ( "Styles".equals(tag) )
			return mOptionsStyles;
		else if ( "Controls".equals(tag) )
			return mOptionsControls;
		return null;
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.v("cr3", "creating OptionsDialog");
		super.onCreate(savedInstanceState);
		
		setTitle("Options");
        setCancelable(true);
		
        LayoutInflater inflater = LayoutInflater.from(getContext());
        mTabs = (TabHost)inflater.inflate(R.layout.options, null);
		// setup tabs
		//setView(R.layout.options);
		//setContentView(R.layout.options);
		//mTabs = (TabHost)findViewById(android.R.id.tabhost); 
		mTabs.setup();
		//new TabHost(getContext());
		mOptionsStyles = new OptionsListView(getContext());
		mOptionsApplication = new OptionsListView(getContext());
		mOptionsControls = new OptionsListView(getContext());
		TabHost.TabSpec tsStyles = mTabs.newTabSpec("Styles");
		tsStyles.setIndicator("Styles");
		tsStyles.setContent(this);
		mTabs.addTab(tsStyles);
		TabHost.TabSpec tsApp = mTabs.newTabSpec("App");
		tsApp.setIndicator("App");
		tsApp.setContent(this);
		mTabs.addTab(tsApp);
		TabHost.TabSpec tsControls = mTabs.newTabSpec("Controls");
		tsControls.setIndicator("Controls");
		tsControls.setContent(this);
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
		
		Log.v("cr3", "OptionsDialog is created");
	}

}
