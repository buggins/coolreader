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
import android.widget.AdapterView;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TabHost;
import android.widget.TextView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.TabHost.TabContentFactory;

public class OptionsDialog extends AlertDialog implements TabContentFactory {

	ReaderView mReaderView;
	String[] mFontFaces;
	int[] mFontSizes = new int[] {
		16, 18, 20, 22, 24, 26, 28, 30,
		32, 34, 36, 38, 40, 42, 48, 56
	};
	int[] mInterlineSpaces = new int[] {
			80, 90, 100, 110, 120, 130, 140, 150
		};
	int[] mMargins = new int[] {
			0, 3, 5, 10, 15, 20, 25
		};
	TabHost mTabs;
	LayoutInflater mInflater;
	Properties mProperties;
	OptionsListView mOptionsStyles;
	OptionsListView mOptionsPage;
	OptionsListView mOptionsApplication;
	OptionsListView mOptionsControls;
	
	class OptionBase {
		public String label;
		public String property;
		public String defaultValue;
		public OptionsListView optionsListView;
		public OptionBase( String label, String property ) {
			this.label = label;
			this.property = property;
		}
		public OptionBase setDefaultValue(String value) {
			this.defaultValue = value;
			if ( mProperties.getProperty(property)==null )
				mProperties.setProperty(property, value);
			return this;
		}
		public String getValueLabel() { return mProperties.getProperty(property); }
		public void onSelect() { }
	}
	
	class BoolOption extends OptionBase {
		public BoolOption( String label, String property ) {
			super(label, property);
		}
		public String getValueLabel() { return "1".equals(mProperties.getProperty(property)) ? "on" : "off"; }
		public void onSelect() { 
			mProperties.setProperty(property, "1".equals(mProperties.getProperty(property)) ? "0" : "1");
			optionsListView.refresh();
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
		public ListOption add(String[]values) {
			for ( String item : values ) {
				add(item, item);
			}
			return this;
		}
		public ListOption add(int[]values) {
			for ( int item : values ) {
				String s = String.valueOf(item); 
				add(s, s);
			}
			return this;
		}
		public ListOption addPercents(int[]values) {
			for ( int item : values ) {
				String s = String.valueOf(item); 
				add(s, s + "%");
			}
			return this;
		}
		public String findValueLabel( String value ) {
			for ( Pair pair : list ) {
				if ( value.equals(pair.value) )
					return pair.label;
			}
			return null;
		}
		public int findValue( String value ) {
			if ( value==null )
				return -1;
			for ( int i=0; i<list.size(); i++ ) {
				if ( value.equals(list.get(i).value) )
					return i;
			}
			return -1;
		}
		
		public int getSelectedItemIndex() {
			return findValue(mProperties.getProperty(property));
		}
		
		public String getValueLabel() { return findValueLabel(mProperties.getProperty(property)); }
		
		public void onSelect() {
			AlertDialog.Builder dlg = new AlertDialog.Builder(getContext());
			dlg.setTitle(label);
			ListAdapter listAdapter = new ListAdapter() {

				public boolean areAllItemsEnabled() {
					return true;
				}

				public boolean isEnabled(int position) {
					return true;
				}

				public int getCount() {
					return list.size();
				}

				public Object getItem(int position) {
					return list.get(position);
				}

				public long getItemId(int position) {
					return position;
				}

				public int getItemViewType(int position) {
					return 0;
				}

				public View getView(int position, View convertView,
						ViewGroup parent) {
					TextView view;
					if ( convertView==null ) {
						view = (TextView)mInflater.inflate(R.layout.option_value, null);
						//view = new TextView(getContext());
					} else {
						view = (TextView)convertView;
					}
					Pair item = list.get(position);
					view.setText(item.label);
					return view;
				}

				public int getViewTypeCount() {
					return 1;
				}

				public boolean hasStableIds() {
					return true;
				}

				public boolean isEmpty() {
					return list.size()==0;
				}

				private ArrayList<DataSetObserver> observers = new ArrayList<DataSetObserver>();
				
				public void registerDataSetObserver(DataSetObserver observer) {
					observers.add(observer);
				}

				public void unregisterDataSetObserver(DataSetObserver observer) {
					observers.remove(observer);
				}
				
			};
			int selItem = getSelectedItemIndex();
			if ( selItem<0 )
				selItem = 0;
			ListView listView = new ListView(getContext()) {

//				@Override
//				public boolean performItemClick(View view, int position, long id) {
//					Pair item = list.get(position);
//					mProperties.setProperty(property, item.value);
//					dismiss();
//					mTabs.invalidate();
//					return true;
//				}
				
			};
			listView.setAdapter(listAdapter);
			listView.setSelection(selItem);
			dlg.setView(listView);
			final AlertDialog d = dlg.create();
			listView.setOnItemClickListener(new OnItemClickListener() {

				public void onItemClick(AdapterView<?> adapter, View listview,
						int position, long id) {
					Pair item = list.get(position);
					mProperties.setProperty(property, item.value);
					d.dismiss();
					optionsListView.refresh();
				}
			});

			
			d.show();
		}
	}
	
	public OptionsDialog( Context context, ReaderView readerView, String[] fontFaces )
	{
		super(context);
		mReaderView = readerView;
		mFontFaces = fontFaces;
		mProperties = readerView.getSettings();
	}
	
	class OptionsListView extends ListView {
		private ArrayList<OptionBase> mOptions = new ArrayList<OptionBase>();
		private ListAdapter mAdapter;
		public void refresh()
		{
			setAdapter(mAdapter);
			invalidate();
		}
		public OptionsListView add( OptionBase option ) {
			mOptions.add(option);
			option.optionsListView = this;
			return this;
		}
		public OptionsListView( Context context )
		{
			
			super(context);
			mAdapter = new ListAdapter() {
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
						//view = new TextView(getContext());
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
			};
			setAdapter(mAdapter);
		}
		@Override
		public boolean performItemClick(View view, int position, long id) {
			mOptions.get(position).onSelect();
			return true;
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
		else if ( "Page".equals(tag))
			return mOptionsPage;
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
		mOptionsStyles.add(new ListOption("Font face", ReaderView.PROP_FONT_FACE).add(mFontFaces).setDefaultValue(mFontFaces[0]));
		mOptionsStyles.add(new ListOption("Font size", ReaderView.PROP_FONT_SIZE).add(mFontSizes).setDefaultValue("24"));
		mOptionsStyles.add(new BoolOption("Embolden font", ReaderView.PROP_FONT_WEIGHT_EMBOLDEN).setDefaultValue("0"));
		mOptionsStyles.add(new BoolOption("Inverse view", ReaderView.PROP_DISPLAY_INVERSE).setDefaultValue("0"));
		mOptionsStyles.add(new ListOption("Interline space", ReaderView.PROP_INTERLINE_SPACE).addPercents(mInterlineSpaces).setDefaultValue("100"));
		mOptionsPage = new OptionsListView(getContext());
		mOptionsPage.add(new BoolOption("Show title bar", ReaderView.PROP_STATUS_LINE).setDefaultValue("1"));
		mOptionsPage.add(new BoolOption("Footnotes", ReaderView.PROP_FOOTNOTES).setDefaultValue("1"));
		mOptionsPage.add(new ListOption("Left margin", ReaderView.PROP_PAGE_MARGIN_LEFT).add(mMargins).setDefaultValue("5"));
		mOptionsPage.add(new ListOption("Right margin", ReaderView.PROP_PAGE_MARGIN_RIGHT).add(mMargins).setDefaultValue("5"));
		mOptionsPage.add(new ListOption("Top margin", ReaderView.PROP_PAGE_MARGIN_TOP).add(mMargins).setDefaultValue("5"));
		mOptionsPage.add(new ListOption("Bottom margin", ReaderView.PROP_PAGE_MARGIN_BOTTOM).add(mMargins).setDefaultValue("5"));
		mOptionsApplication = new OptionsListView(getContext());
		mOptionsApplication.add(new BoolOption("Full screen", "app.fullscreen"));
		mOptionsControls = new OptionsListView(getContext());
		mOptionsControls.add(new BoolOption("Sample option", "controls.sample"));
		TabHost.TabSpec tsStyles = mTabs.newTabSpec("Styles");
		tsStyles.setIndicator("Styles"); // , R.drawable.cr3_option_style
		tsStyles.setContent(this);
		mTabs.addTab(tsStyles);
		TabHost.TabSpec tsPage = mTabs.newTabSpec("Page");
		tsPage.setIndicator("Page"); //, R.drawable.cr3_option_page
		tsPage.setContent(this);
		mTabs.addTab(tsPage);
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
                //cancel();
                dialog.cancel();
                mReaderView.setSettings(mProperties);
            }
        });
 
        setButton(AlertDialog.BUTTON_NEGATIVE, "Cancel",
            new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int whichButton) {
                    dialog.cancel();
                }
            });
		
		super.onCreate(savedInstanceState);
		Log.v("cr3", "OptionsDialog is created");
	}

}
