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

	ReaderView mReaderView;
	String[] mFontFaces;
	TabHost mTabs;
	LayoutInflater mInflater;
	OptionsListView mOptionsView;
	Properties mProperties = new Properties();
	OptionsListView mOptionsStyles;
	OptionsListView mOptionsApplication;
	OptionsListView mOptionsControls;
	
	class OptionBase {
		public String label;
		public String property;
		public String defaultValue;
		public OptionBase( String label, String property ) {
			this.label = label;
			this.property = property;
		}
		public OptionBase setDefaultValue(String value) {
			this.defaultValue = value;
			return this;
		}
		public String getValueLabel() { return mProperties.getProperty(property); }
		public void onSelect() { }
	}
	
	class BoolOption extends OptionBase {
		public BoolOption( String label, String property ) {
			super(label, property);
		}
		public String getValueLabel() { return "".equals(mProperties.getProperty(property)) ? "on" : "off"; }
		public void onSelect() { 
			mProperties.setProperty(property, "".equals(mProperties.getProperty(property)) ? "0" : "1");
		}
	}
	
	class Pair {
		public String value;
		public String label;
		public Pair(String value, String label) {
			this.value = value;
			this.label = label;
		}
	}
	class ListOption extends OptionBase {
		private ArrayList<Pair> list = new ArrayList<Pair>();
		public ListOption( String label, String property ) {
			super(label, property);
		}
		public void add(String value, String label) {
			list.add( new Pair(value, label) );
		}
		public void add(String[]values) {
			for ( String item : values ) {
				add(item, item);
			}
		}
		public String findValueLabel( String value ) {
			for ( Pair pair : list ) {
				if ( value.equals(pair.value) )
					return pair.label;
			}
			return null;
		}
		public int findValue( String value ) {
			for ( int i=0; i<list.size(); i++ ) {
				if ( value.equals(list.get(i).value) )
					return i;
			}
			return -1;
		}
		public String getValueLabel() { return findValueLabel(mProperties.getProperty(property)); }
		public void onSelect() {
			AlertDialog.Builder dlg = new AlertDialog.Builder(getContext());
			dlg.setTitle(label);
			dlg.setPositiveButton("Ok", new OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog, int which) {
					// TODO: select item
					dismiss();
				}
			});
			mProperties.setProperty(property, "".equals(mProperties.getProperty(property)) ? "0" : "1");
		}
	}
	
	public OptionsDialog( Context context, ReaderView readerView, String[] fontFaces )
	{
		super(context);
		mReaderView = readerView;
		mFontFaces = fontFaces;
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
					View view;
					if ( convertView==null ) {
						view = new TextView(getContext());
						view = mInflater.inflate(R.layout.option_item, null);
					} else {
						view = (View)convertView;
					}
					TextView labelView = (TextView)view.findViewById(R.id.option_label);
					TextView valueView = (TextView)view.findViewById(R.id.option_value);
					labelView.setText(mOptions.get(position).label);
					valueView.setText(mOptions.get(position).getValueLabel());
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
		
		//setTitle("Options");
        setCancelable(true);
		
        mInflater = LayoutInflater.from(getContext());
        mTabs = (TabHost)mInflater.inflate(R.layout.options, null);
		// setup tabs
		//setView(R.layout.options);
		//setContentView(R.layout.options);
		//mTabs = (TabHost)findViewById(android.R.id.tabhost); 
		mTabs.setup();
		//new TabHost(getContext());
		mOptionsStyles = new OptionsListView(getContext());
		mOptionsStyles.add(new OptionBase("Font face", ReaderView.PROP_FONT_FACE));
		mOptionsStyles.add(new OptionBase("Font size", ReaderView.PROP_FONT_SIZE));
		mOptionsStyles.add(new BoolOption("Embolden font", ReaderView.PROP_FONT_WEIGHT_EMBOLDEN));
		mOptionsStyles.add(new BoolOption("Inverse view", ReaderView.PROP_DISPLAY_INVERSE));
		mOptionsApplication = new OptionsListView(getContext());
		mOptionsApplication.add(new BoolOption("Full screen", "app.fullscreen"));
		mOptionsControls = new OptionsListView(getContext());
		mOptionsControls.add(new BoolOption("Sample option", "controls.sample"));
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
		
		super.onCreate(savedInstanceState);
		Log.v("cr3", "OptionsDialog is created");
	}

}
