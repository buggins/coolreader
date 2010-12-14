package org.coolreader.crengine;

import java.util.ArrayList;

import org.coolreader.R;
import org.coolreader.crengine.ColorPickerDialog.OnColorChangedListener;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.database.DataSetObserver;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ImageView;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.RadioButton;
import android.widget.TabHost;
import android.widget.TabHost.TabContentFactory;
import android.widget.TextView;

public class OptionsDialog extends BaseDialog implements TabContentFactory {

	ReaderView mReaderView;
	String[] mFontFaces;
	int[] mFontSizes = new int[] {
		14, 16, 18, 20, 22, 24, 26, 28, 30,
		32, 34, 36, 38, 40, 42, 44, 48, 52, 56, 60
	};
	int[] mBacklightLevels = new int[] {
		-1, 5, 10, 15, 20, 30, 40, 50, 60, 70, 80, 100
	};
	String[] mBacklightLevelsTitles = new String[] {
			"Default", "5%", "10%", "15%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "100%",
	};
	int[] mInterlineSpaces = new int[] {
			80, 90, 100, 110, 120, 130, 140, 150
		};
	int[] mMargins = new int[] {
			0, 1, 2, 3, 4, 5, 10, 15, 20, 25
		};
	int[] mOrientations = new int[] {
			0, 1//, 2, 3
			,4
		};
	int[] mOrientationsTitles = new int[] {
			R.string.options_page_orientation_0, R.string.options_page_orientation_90 //, R.string.options_page_orientation_180, R.string.options_page_orientation_270
			,R.string.options_page_orientation_sensor
		};
	int[] mAntialias = new int[] {
			0, 1, 2
		};
	int[] mAntialiasTitles = new int[] {
			R.string.options_font_antialias_off, R.string.options_font_antialias_on_for_big, R.string.options_font_antialias_on_for_all
		};
	int[] mLandscapePages = new int[] {
			1, 2
		};
	int[] mLandscapePagesTitles = new int[] {
			R.string.options_page_landscape_pages_one, R.string.options_page_landscape_pages_two
		};
	int[] mViewModes = new int[] {
			1, 0
		};
	int[] mViewModeTitles = new int[] {
			R.string.options_view_mode_pages, R.string.options_view_mode_scroll
		};
	TabHost mTabs;
	LayoutInflater mInflater;
	Properties mProperties;
	Properties mOldProperties;
	OptionsListView mOptionsStyles;
	OptionsListView mOptionsPage;
	OptionsListView mOptionsApplication;
	OptionsListView mOptionsControls;
	
	class OptionBase {
		public String label;
		public String property;
		public String defaultValue;
		public int iconId = R.drawable.cr3_option_other;
		public OptionsListView optionsListView;
		protected Runnable onChangeHandler;
		public OptionBase( String label, String property ) {
			this.label = label;
			this.property = property;
		}
		public OptionBase setIconId(int id) {
			this.iconId = id;
			return this;
		}
		public OptionBase setDefaultValue(String value) {
			this.defaultValue = value;
			if ( mProperties.getProperty(property)==null )
				mProperties.setProperty(property, value);
			return this;
		}
		public void setOnChangeHandler( Runnable handler ) {
			onChangeHandler = handler;
		}
		public String getValueLabel() { return mProperties.getProperty(property); }
		public void onSelect() { }
	}
	
	class ColorOption extends OptionBase {
		final int defColor;
		public ColorOption( String label, String property, int defColor ) {
			super(label, property);
			this.defColor = defColor;
		}
		public String getValueLabel() { return mProperties.getProperty(property); }
		public void onSelect()
		{ 
			ColorPickerDialog dlg = new ColorPickerDialog(getOwnerActivity(), new OnColorChangedListener() {
				public void colorChanged(int color) {
					mProperties.setColor(property, color);
					optionsListView.refresh();
				}
			}, mProperties.getColor(property, defColor), label);
			dlg.show();
		}
	}
	
	class BoolOption extends OptionBase {
		private boolean inverse = false;
		public BoolOption( String label, String property ) {
			super(label, property);
		}
		public String getValueLabel() { return "1".equals(mProperties.getProperty(property)) ^ inverse  ? getString(R.string.options_value_on) : getString(R.string.options_value_off); }
		public void onSelect() { 
			mProperties.setProperty(property, "1".equals(mProperties.getProperty(property)) ? "0" : "1");
			optionsListView.refresh();
		}
		public BoolOption setInverse() { inverse = true; return this; }
	}

	static public void saveColor( Properties mProperties, boolean night )
	{
		if ( night ) {
			mProperties.setColor(ReaderView.PROP_BACKGROUND_COLOR_NIGHT, mProperties.getColor(ReaderView.PROP_BACKGROUND_COLOR, 0x000000));
			mProperties.setColor(ReaderView.PROP_FONT_COLOR_NIGHT, mProperties.getColor(ReaderView.PROP_FONT_COLOR, 0xFFFFFF));
			mProperties.setInt(ReaderView.PROP_APP_SCREEN_BACKLIGHT_NIGHT, mProperties.getInt(ReaderView.PROP_APP_SCREEN_BACKLIGHT, -1));
		} else {
			mProperties.setColor(ReaderView.PROP_BACKGROUND_COLOR_DAY, mProperties.getColor(ReaderView.PROP_BACKGROUND_COLOR, 0xFFFFFF));
			mProperties.setColor(ReaderView.PROP_FONT_COLOR_DAY, mProperties.getColor(ReaderView.PROP_FONT_COLOR, 0x000000));
			mProperties.setInt(ReaderView.PROP_APP_SCREEN_BACKLIGHT_DAY, mProperties.getInt(ReaderView.PROP_APP_SCREEN_BACKLIGHT, -1));
		}
	}
	static public void restoreColor( Properties mProperties,  boolean night )
	{
		if ( night ) {
			mProperties.setColor(ReaderView.PROP_BACKGROUND_COLOR, mProperties.getColor(ReaderView.PROP_BACKGROUND_COLOR_NIGHT, 0x000000));
			mProperties.setColor(ReaderView.PROP_FONT_COLOR, mProperties.getColor(ReaderView.PROP_FONT_COLOR_NIGHT, 0xFFFFFF));
			mProperties.setInt(ReaderView.PROP_APP_SCREEN_BACKLIGHT, mProperties.getInt(ReaderView.PROP_APP_SCREEN_BACKLIGHT_NIGHT, 70));
		} else {
			mProperties.setColor(ReaderView.PROP_BACKGROUND_COLOR, mProperties.getColor(ReaderView.PROP_BACKGROUND_COLOR_DAY, 0xFFFFFF));
			mProperties.setColor(ReaderView.PROP_FONT_COLOR, mProperties.getColor(ReaderView.PROP_FONT_COLOR_DAY, 0x000000));
			mProperties.setInt(ReaderView.PROP_APP_SCREEN_BACKLIGHT, mProperties.getInt(ReaderView.PROP_APP_SCREEN_BACKLIGHT_DAY, 80));
		}
	}
	static public void toggleDayNightMode( Properties mProperties ) {
		boolean oldMode = mProperties.getBool(ReaderView.PROP_DISPLAY_INVERSE, false);
		saveColor(mProperties, oldMode);
		boolean newMode = !oldMode;
		restoreColor(mProperties, newMode);
		mProperties.setBool(ReaderView.PROP_DISPLAY_INVERSE, newMode);
	}
	class NightModeOption extends OptionBase {
		public NightModeOption( String label, String property ) {
			super(label, property);
		}
		public String getValueLabel() { return "1".equals(mProperties.getProperty(property)) ? getString(R.string.options_value_on) : getString(R.string.options_value_off); }
		public void onSelect() { 
			boolean oldMode = mProperties.getBool(property, false);
			saveColor(mProperties, oldMode);
			boolean newMode = !oldMode;
			restoreColor(mProperties, newMode);
			mProperties.setBool(property, newMode);
			optionsListView.refresh();
		}
	}
	
	class ActionOption extends ListOption {
		public ActionOption( String label, String property ) {
			super(label, property);
			ReaderAction[] actions = ReaderAction.AVAILABLE_ACTIONS;
			for ( ReaderAction a : actions )
				add(a.id, getString(a.nameId));
		}
	}

	class KeyMapOption extends ListOption {
		public KeyMapOption( String label, String property ) {
			super(label, property);
			addKey(KeyEvent.KEYCODE_DPAD_CENTER, "Center");
			addKey(KeyEvent.KEYCODE_DPAD_LEFT, "Left");
			addKey(KeyEvent.KEYCODE_DPAD_RIGHT, "Right");
			addKey(KeyEvent.KEYCODE_DPAD_UP, "Up");
			addKey(KeyEvent.KEYCODE_DPAD_DOWN, "Down");
		}
		View grid;
		private void addKey( int keyCode, String keyName )
		{
			final String propName = property + "." + keyCode;
			add( keyName, propName );
//			ReaderAction action = ReaderAction.findById( mProperties.getProperty(propName) );
//			add( getString(action.nameId), keyName );
//			text.setText(getString(action.nameId));
//			text.setOnClickListener(new View.OnClickListener () {
//				@Override
//				public void onClick(View v) {
//					ActionOption option = new ActionOption("Tap Zone " + tapZoneId + " action", propName);
//					option.setOnChangeHandler(new Runnable() {
//						public void run() {
//							ReaderAction action = ReaderAction.findById( mProperties.getProperty(propName) );
//							text.setText(getString(action.nameId));
//						}
//					});
//					option.onSelect();
//				}
//			});
		}

		public String getValueLabel() { return ">"; }

		public void onClick( final Pair item ) {
			ActionOption option = new ActionOption(item.label + " action", item.value);
			option.setOnChangeHandler(new Runnable() {
				public void run() {
					ReaderAction action = ReaderAction.findById( mProperties.getProperty(item.value) );
					//KeyMapOption.
					// TODO:
				}
			});
			option.onSelect();
		}
		
	}
	
	class TapZoneOption extends OptionBase {
		public TapZoneOption( String label, String property ) {
			super(label, property);
		}
		View grid;
		private void initTapZone( View view, final int tapZoneId )
		{
			if ( view==null )
				return;
			final TextView text = (TextView)view;
			final String propName = property + "." + tapZoneId;
			ReaderAction action = ReaderAction.findById( mProperties.getProperty(propName) );
			text.setText(getString(action.nameId));
			text.setOnClickListener(new View.OnClickListener () {
				@Override
				public void onClick(View v) {
					ActionOption option = new ActionOption("Tap Zone " + tapZoneId + " action", propName);
					option.setOnChangeHandler(new Runnable() {
						public void run() {
							ReaderAction action = ReaderAction.findById( mProperties.getProperty(propName) );
							text.setText(getString(action.nameId));
						}
					});
					option.onSelect();
				}
			});
		}

		public String getValueLabel() { return ">"; }
		public void onSelect() {
			BaseDialog dlg = new BaseDialog(getOwnerActivity(), R.string.dlg_button_ok, 0);
			grid = (View)mInflater.inflate(R.layout.options_tap_zone_grid, null);
			initTapZone(grid.findViewById(R.id.tap_zone_action_text1), 1);
			initTapZone(grid.findViewById(R.id.tap_zone_action_text2), 2);
			initTapZone(grid.findViewById(R.id.tap_zone_action_text3), 3);
			initTapZone(grid.findViewById(R.id.tap_zone_action_text4), 4);
			initTapZone(grid.findViewById(R.id.tap_zone_action_text5), 5);
			initTapZone(grid.findViewById(R.id.tap_zone_action_text6), 6);
			initTapZone(grid.findViewById(R.id.tap_zone_action_text7), 7);
			initTapZone(grid.findViewById(R.id.tap_zone_action_text8), 8);
			initTapZone(grid.findViewById(R.id.tap_zone_action_text9), 9);
			dlg.setTitle(label);
			dlg.setView(grid);
			dlg.show();
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
		public ListOption add(int[]values, int[]labelIDs) {
			for ( int i=0; i<values.length; i++ ) {
				String value = String.valueOf(values[i]); 
				String label = getContext().getString(labelIDs[i]); 
				add(value, label);
			}
			return this;
		}
		public ListOption add(int[]values, String[]labels) {
			for ( int i=0; i<values.length; i++ ) {
				String value = String.valueOf(values[i]); 
				String label = labels[i]; 
				add(value, label);
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

			final ListView listView = new ListView(getContext()) {

//				@Override
//				public boolean performItemClick(View view, int position, long id) {
//					Pair item = list.get(position);
//					mProperties.setProperty(property, item.value);
//					dismiss();
//					mTabs.invalidate();
//					return true;
//				}
				
			};
			
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

				public View getView(final int position, View convertView,
						ViewGroup parent) {
					ViewGroup layout;
					TextView view;
					RadioButton cb;
					if ( convertView==null ) {
						layout = (ViewGroup)mInflater.inflate(R.layout.option_value, null);
						//view = new TextView(getContext());
					} else {
						layout = (ViewGroup)convertView;
					}
					view = (TextView)layout.findViewById(R.id.option_value_text);
					cb = (RadioButton)layout.findViewById(R.id.option_value_check);
					final Pair item = list.get(position);
					view.setText(item.label);
					boolean isSelected = getSelectedItemIndex()==position;
					cb.setChecked(isSelected);
					cb.setOnClickListener(new View.OnClickListener() {
						@Override
						public void onClick(View v) {
							listView.getOnItemClickListener().onItemClick(listView, listView, position, 0);
//							mProperties.setProperty(property, item.value);
//							dismiss();
//							optionsListView.refresh();
						}
					});
					//cb.setClickable(false);
//					cb.setOnClickListener(new View.OnClickListener() {
//						@Override
//						public void onClick(View v) {
//							
//						}
//					});
					return layout;
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
			listView.setAdapter(listAdapter);
			listView.setSelection(selItem);
			dlg.setView(listView);
			final AlertDialog d = dlg.create();
			listView.setOnItemClickListener(new OnItemClickListener() {

				public void onItemClick(AdapterView<?> adapter, View listview,
						int position, long id) {
					Pair item = list.get(position);
					onClick(item);
					d.dismiss();
				}
			});

			
			d.show();
		}
		
		public void onClick( Pair item ) {
			mProperties.setProperty(property, item.value);
			if ( onChangeHandler!=null )
				onChangeHandler.run();
			if ( optionsListView!=null )
				optionsListView.refresh();
		}
	}
	
	class HyphenationOptions extends ListOption
	{
		public HyphenationOptions( String label )
		{
			super( label, ReaderView.PROP_HYPHENATION_DICT );
			setDefaultValue("RUSSIAN");
			Engine.HyphDict[] dicts = Engine.HyphDict.values();
			for ( Engine.HyphDict dict : dicts )
				add( dict.toString(), dict.name );
		}
	}
	
	public OptionsDialog( Activity activity, ReaderView readerView, String[] fontFaces )
	{
		super(activity, R.string.dlg_button_ok, R.string.dlg_button_cancel);
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
			setFocusable(true);
			setFocusableInTouchMode(true);
			mAdapter = new ListAdapter() {
				public boolean areAllItemsEnabled() {
					return false;
				}

				public boolean isEnabled(int position) {
					boolean isPageMode = mProperties.getBool(ReaderView.PROP_PAGE_VIEW_MODE, true);
					OptionBase option = mOptions.get(position);
					String prop = option.property;
					if ( prop.equals(ReaderView.PROP_STATUS_LINE) || prop.equals(ReaderView.PROP_FOOTNOTES) )
						return isPageMode;
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
					ImageView icon = (ImageView)view.findViewById(R.id.option_icon);
					icon.setImageResource(mOptions.get(position).iconId);
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

	private String getString( int resourceId )
	{
		return getContext().getResources().getString(resourceId); 
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.v("cr3", "creating OptionsDialog");
		
		setTitle(null);
        setCancelable(true);
        setCanceledOnTouchOutside(true);
        mInflater = LayoutInflater.from(getContext());
        mTabs = (TabHost)mInflater.inflate(R.layout.options, null);
		// setup tabs
		//setView(R.layout.options);
		//setContentView(R.layout.options);
		//mTabs = (TabHost)findViewById(android.R.id.tabhost); 
		mTabs.setup();
		//new TabHost(getContext());
		mOptionsStyles = new OptionsListView(getContext());
		mOptionsStyles.add(new ListOption(getString(R.string.options_font_face), ReaderView.PROP_FONT_FACE).add(mFontFaces).setDefaultValue(mFontFaces[0]).setIconId(R.drawable.cr3_option_font_face));
		mOptionsStyles.add(new ListOption(getString(R.string.options_font_size), ReaderView.PROP_FONT_SIZE).add(mFontSizes).setDefaultValue("24").setIconId(R.drawable.cr3_option_font_size));
		mOptionsStyles.add(new BoolOption(getString(R.string.options_font_embolden), ReaderView.PROP_FONT_WEIGHT_EMBOLDEN).setDefaultValue("0").setIconId(R.drawable.cr3_option_text_bold));
		//mOptionsStyles.add(new BoolOption(getString(R.string.options_font_antialias), ReaderView.PROP_FONT_ANTIALIASING).setInverse().setDefaultValue("0"));
		mOptionsStyles.add(new ListOption(getString(R.string.options_font_antialias), ReaderView.PROP_FONT_ANTIALIASING).add(mAntialias, mAntialiasTitles).setDefaultValue("2"));
		mOptionsStyles.add(new ListOption(getString(R.string.options_interline_space), ReaderView.PROP_INTERLINE_SPACE).addPercents(mInterlineSpaces).setDefaultValue("100"));
		mOptionsStyles.add(new NightModeOption(getString(R.string.options_inverse_view), ReaderView.PROP_NIGHT_MODE));
		mOptionsStyles.add(new ColorOption(getString(R.string.options_color_text), ReaderView.PROP_FONT_COLOR, 0x000000));
		mOptionsStyles.add(new ColorOption(getString(R.string.options_color_background), ReaderView.PROP_BACKGROUND_COLOR, 0xFFFFFF));
		mBacklightLevelsTitles[0] = getString(R.string.options_app_backlight_screen_default);
		mOptionsStyles.add(new ListOption(getString(R.string.options_app_backlight_screen), ReaderView.PROP_APP_SCREEN_BACKLIGHT).add(mBacklightLevels, mBacklightLevelsTitles).setDefaultValue("-1"));
		//
		mOptionsStyles.add(new HyphenationOptions(getString(R.string.options_hyphenation_dictionary)));
		//
		mOptionsPage = new OptionsListView(getContext());
		mOptionsPage.add(new ListOption(getString(R.string.options_view_mode), ReaderView.PROP_PAGE_VIEW_MODE).add(mViewModes, mViewModeTitles).setDefaultValue("1"));
		mOptionsPage.add(new BoolOption(getString(R.string.options_page_show_titlebar), ReaderView.PROP_STATUS_LINE).setInverse().setDefaultValue("0"));
		mOptionsPage.add(new BoolOption(getString(R.string.options_page_footnotes), ReaderView.PROP_FOOTNOTES).setDefaultValue("1"));
		//mOptionsPage.add(new ListOption(getString(R.string.options_page_orientation), ReaderView.PROP_ROTATE_ANGLE).add(mOrientations, mOrientationsTitles).setDefaultValue("0"));
		mOptionsPage.add(new ListOption(getString(R.string.options_page_orientation), ReaderView.PROP_APP_SCREEN_ORIENTATION).add(mOrientations, mOrientationsTitles).setDefaultValue("0"));
		mOptionsPage.add(new ListOption(getString(R.string.options_page_landscape_pages), ReaderView.PROP_LANDSCAPE_PAGES).add(mLandscapePages, mLandscapePagesTitles).setDefaultValue("1"));
		mOptionsPage.add(new BoolOption(getString(R.string.options_page_animation), ReaderView.PROP_PAGE_ANIMATION).setDefaultValue("1"));
		
		
		mOptionsPage.add(new ListOption(getString(R.string.options_page_margin_left), ReaderView.PROP_PAGE_MARGIN_LEFT).add(mMargins).setDefaultValue("5"));
		mOptionsPage.add(new ListOption(getString(R.string.options_page_margin_right), ReaderView.PROP_PAGE_MARGIN_RIGHT).add(mMargins).setDefaultValue("5"));
		mOptionsPage.add(new ListOption(getString(R.string.options_page_margin_top), ReaderView.PROP_PAGE_MARGIN_TOP).add(mMargins).setDefaultValue("5"));
		mOptionsPage.add(new ListOption(getString(R.string.options_page_margin_bottom), ReaderView.PROP_PAGE_MARGIN_BOTTOM).add(mMargins).setDefaultValue("5"));
		mOptionsApplication = new OptionsListView(getContext());
		mOptionsApplication.add(new BoolOption(getString(R.string.options_app_fullscreen), ReaderView.PROP_APP_FULLSCREEN));
		mOptionsApplication.add(new BoolOption(getString(R.string.options_app_show_cover_pages), ReaderView.PROP_APP_SHOW_COVERPAGES));
		mOptionsApplication.add(new BoolOption(getString(R.string.options_controls_enable_volume_keys), ReaderView.PROP_CONTROLS_ENABLE_VOLUME_KEYS).setDefaultValue("1"));
		mOptionsApplication.add(new TapZoneOption(getString(R.string.options_app_tapzones_normal), ReaderView.PROP_APP_TAP_ZONE_ACTIONS_TAP));
		mOptionsApplication.add(new TapZoneOption(getString(R.string.options_app_tapzones_long), ReaderView.PROP_APP_TAP_ZONE_ACTIONS_LONGTAP));
		
		mOptionsControls = new OptionsListView(getContext());
		mOptionsControls.add(new BoolOption("Sample option", "controls.sample"));
		TabHost.TabSpec tsStyles = mTabs.newTabSpec("Styles");
		tsStyles.setIndicator(getContext().getResources().getString(R.string.tab_options_styles), 
				getContext().getResources().getDrawable(R.drawable.cr3_option_style));
		tsStyles.setContent(this);
		mTabs.addTab(tsStyles);
		TabHost.TabSpec tsPage = mTabs.newTabSpec("Page");
		tsPage.setIndicator(getContext().getResources().getString(R.string.tab_options_page), getContext().getResources().getDrawable(R.drawable.cr3_option_page));
		tsPage.setContent(this);
		mTabs.addTab(tsPage);
		TabHost.TabSpec tsApp = mTabs.newTabSpec("App");
		//tsApp.setIndicator(null, getContext().getResources().getDrawable(R.drawable.cr3_option_));
		tsApp.setIndicator(getContext().getResources().getString(R.string.tab_options_app));
		tsApp.setContent(this);
		mTabs.addTab(tsApp);
		
		TabHost.TabSpec tsControls = mTabs.newTabSpec("Controls");
		tsControls.setIndicator(getContext().getResources().getString(R.string.tab_options_controls));
		tsControls.setContent(this);
		//mTabs.addTab(tsControls);

		setView(mTabs);
		
		mOldProperties = new Properties(mProperties);
		
		setOnCancelListener(new OnCancelListener() {

			public void onCancel(DialogInterface dialog) {
				askApply();
			}
		});
		
		super.onCreate(savedInstanceState);
		Log.v("cr3", "OptionsDialog is created");
	}

	private void askApply()
	{
		Properties diff = mProperties.diff(mOldProperties);
		if ( diff.size()>0 ) {
			Log.d("cr3", "Some properties were changed, ask user whether to apply");
			AlertDialog.Builder dlg = new AlertDialog.Builder(getContext());
			dlg.setTitle(R.string.win_title_options_apply);
			dlg.setPositiveButton(R.string.dlg_button_ok, new OnClickListener() {
				public void onClick(DialogInterface arg0, int arg1) {
					onPositiveButtonClick();
				}
			});
			dlg.setNegativeButton(R.string.dlg_button_cancel, new OnClickListener() {
				public void onClick(DialogInterface arg0, int arg1) {
					onNegativeButtonClick();
				}
			});
			dlg.show();
		}
	}
	
	@Override
	protected void onPositiveButtonClick() {
        mReaderView.setSettings(mProperties, mOldProperties);
        dismiss();
        //super.onPositiveButtonClick();
	}

}
