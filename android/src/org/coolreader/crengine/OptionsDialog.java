package org.coolreader.crengine;

import java.io.File;
import java.io.InputStream;
import java.util.ArrayList;

import org.coolreader.CoolReader;
import org.coolreader.R;
import org.coolreader.crengine.ColorPickerDialog.OnColorChangedListener;
import org.coolreader.plugins.OnlineStorePluginManager;

import android.content.Context;
import android.content.DialogInterface;
import android.database.DataSetObserver;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.RadioButton;
import android.widget.TabHost;
import android.widget.TabHost.TabContentFactory;
import android.widget.TabWidget;
import android.widget.TextView;

public class OptionsDialog extends BaseDialog implements TabContentFactory, OptionOwner, Settings {

	ReaderView mReaderView;
	BaseActivity mActivity;
	String[] mFontFaces;
	int[] mFontSizes = new int[] {
		12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
		31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 42, 44, 48, 52, 56, 60, 64, 68, 72
	};
	int[] mStatusFontSizes = new int[] {
			10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 22, 24, 25, 26, 27, 28, 29, 30,
			32
		};
	public static int findBacklightSettingIndex( int value ) {
		int bestIndex = 0;
		int bestDiff = -1;
		for ( int i=0; i<mBacklightLevels.length; i++ ) {
			int diff = mBacklightLevels[i] - value;
			if (diff<0)
				diff = -diff;
			if ( bestDiff==-1 || diff < bestDiff ) {
				bestDiff = diff;
				bestIndex = i;
			}
		}
		return bestIndex;
	}
	public static final int[] mBacklightLevels = new int[] {
		-1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100
	};
	public static final String[] mBacklightLevelsTitles = new String[] {
			"Default", "1%", "2%", "3%", "4%", "5%", "6%", "7%", "8%", "9%", 
			"10%", "12%", "15%", "20%", "25%", "30%", "35%", "40%", "45%", "50%", "55%", "60%", "65%", "70%", "75%", "80%", "85%", "90%", "95%", "100%",
	};
	int[] mInterlineSpaces = new int[] {
			80, 85, 90, 95, 100, 105, 110, 115, 120, 130, 140, 150, 160, 180, 200
		};
	int[] mMinSpaceWidths = new int[] {
			50, 60, 70, 80, 90, 100
		};
	int[] mMargins = new int[] {
			0, 1, 2, 3, 4, 5, 8, 10, 12, 15, 20, 25, 30, 40, 50, 60, 80, 100, 200, 300
		};
	double[] mGammas = new double[] {
			0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.5, 1.9
		};
	int[] mScreenFullUpdateInterval = new int[] {
			0, 2, 3, 4, 5, 7, 10, 15, 20
		};
	int[] mScreenUpdateModes = new int[] {
			0, 1, 2//, 2, 3
		};
	int[] mScreenUpdateModesTitles = new int[] {
			R.string.options_screen_update_mode_quality, R.string.options_screen_update_mode_fast, R.string.options_screen_update_mode_fast2
		};
	int[] mCoverPageSizes = new int[] {
			0, 1, 2//, 2, 3
		};
	int[] mCoverPageSizeTitles = new int[] {
			R.string.options_app_cover_page_size_small, R.string.options_app_cover_page_size_medium, R.string.options_app_cover_page_size_big
		};
	int[] mHinting = new int[] {
			0, 1, 2
		};
	int[] mHintingTitles = new int[] {
			R.string.options_font_hinting_disabled, R.string.options_font_hinting_bytecode, 
			R.string.options_font_hinting_auto
		};
	int[] mOrientations = new int[] {
			0, 1, 4
		};
	int[] mOrientationsTitles = new int[] {
			R.string.options_page_orientation_0, R.string.options_page_orientation_90, 
			R.string.options_page_orientation_sensor
		};
	int[] mOrientations_API9 = new int[] {
			0, 1, 2, 3, 4
		};
	int[] mOrientationsTitles_API9 = new int[] {
			R.string.options_page_orientation_0, R.string.options_page_orientation_90, R.string.options_page_orientation_180, R.string.options_page_orientation_270
			,R.string.options_page_orientation_sensor
		};

	int[] mToolbarPositions = new int[] {
			Settings.VIEWER_TOOLBAR_NONE, Settings.VIEWER_TOOLBAR_TOP, Settings.VIEWER_TOOLBAR_BOTTOM, Settings.VIEWER_TOOLBAR_LEFT, Settings.VIEWER_TOOLBAR_RIGHT, Settings.VIEWER_TOOLBAR_SHORT_SIDE, Settings.VIEWER_TOOLBAR_LONG_SIDE
		};
	int[] mToolbarPositionsTitles = new int[] {
			R.string.options_view_toolbar_position_none, R.string.options_view_toolbar_position_top, R.string.options_view_toolbar_position_bottom, R.string.options_view_toolbar_position_left, R.string.options_view_toolbar_position_right, R.string.options_view_toolbar_position_short_side, R.string.options_view_toolbar_position_long_side
		};
	
	int[] mStatusPositions = new int[] {
			Settings.VIEWER_STATUS_NONE, 
			//Settings.VIEWER_STATUS_TOP, Settings.VIEWER_STATUS_BOTTOM, 
			Settings.VIEWER_STATUS_PAGE
		};
	int[] mStatusPositionsTitles = new int[] {
			R.string.options_page_show_titlebar_hidden, 
			//R.string.options_page_show_titlebar_top, R.string.options_page_show_titlebar_bottom, 
			R.string.options_page_show_titlebar_page_header
		};
	
	int[] mImageScalingModes = new int[] {
			0, 1, 2
		};
	int[] mImageScalingModesTitles = new int[] {
			R.string.options_format_image_scaling_mode_disabled, R.string.options_format_image_scaling_mode_integer_factor, R.string.options_format_image_scaling_mode_arbitrary
		};
	int[] mImageScalingFactors = new int[] {
			0, 1, 2, 3
		};
	int[] mImageScalingFactorsTitles = new int[] {
			R.string.options_format_image_scaling_scale_auto, R.string.options_format_image_scaling_scale_1, R.string.options_format_image_scaling_scale_2, R.string.options_format_image_scaling_scale_3
		};
	int[] mFlickBrightness = new int[] {
			0, 1, 2
		};
	int[] mFlickBrightnessTitles = new int[] {
			R.string.options_controls_flick_brightness_none, R.string.options_controls_flick_brightness_left, R.string.options_controls_flick_brightness_right
		};
	int[] mBacklightTimeout = new int[] {
			0, 2, 3, 4, 5, 6
		};
	int[] mBacklightTimeoutTitles = new int[] {
			R.string.options_app_backlight_timeout_0, R.string.options_app_backlight_timeout_2, R.string.options_app_backlight_timeout_3, R.string.options_app_backlight_timeout_4, R.string.options_app_backlight_timeout_5, R.string.options_app_backlight_timeout_6
		};
	int[] mTapSecondaryActionType = new int[] {
			TAP_ACTION_TYPE_LONGPRESS, TAP_ACTION_TYPE_DOUBLE
		};
	int[] mTapSecondaryActionTypeTitles = new int[] {
			R.string.options_controls_tap_type_long, R.string.options_controls_tap_type_double
		};
	int[] mAnimation = new int[] {
			ReaderView.PAGE_ANIMATION_NONE, ReaderView.PAGE_ANIMATION_SLIDE, ReaderView.PAGE_ANIMATION_SLIDE2, 
			ReaderView.PAGE_ANIMATION_PAPER
		};
	int[] mAnimationTitles = new int[] {
			R.string.options_page_animation_none, R.string.options_page_animation_slide, R.string.options_page_animation_slide_2_pages,
			R.string.options_page_animation_paperbook
		};
	int[] mHighlightMode = new int[] {
			0, 1, 2
		};
	int[] mHighlightModeTitles = new int[] {
			R.string.options_view_bookmarks_highlight_none, R.string.options_view_bookmarks_highlight_solid,
			R.string.options_view_bookmarks_highlight_underline
		};
	int[] mSelectionAction = new int[] {
			ReaderView.SELECTION_ACTION_TOOLBAR,
			ReaderView.SELECTION_ACTION_COPY, 
			ReaderView.SELECTION_ACTION_DICTIONARY,
			ReaderView.SELECTION_ACTION_BOOKMARK,
			ReaderView.SELECTION_ACTION_FIND
		};
	int[] mSelectionActionTitles = new int[] {
			R.string.options_selection_action_toolbar, 
			R.string.options_selection_action_copy, 
			R.string.options_selection_action_dictionary, 
			R.string.options_selection_action_bookmark, 
			R.string.mi_search, 
		};
	int[] mMultiSelectionAction = new int[] {
			ReaderView.SELECTION_ACTION_TOOLBAR,
			ReaderView.SELECTION_ACTION_COPY, 
			ReaderView.SELECTION_ACTION_DICTIONARY,
			ReaderView.SELECTION_ACTION_BOOKMARK
		};
	int[] mMultiSelectionActionTitles = new int[] {
			R.string.options_selection_action_toolbar, 
			R.string.options_selection_action_copy, 
			R.string.options_selection_action_dictionary, 
			R.string.options_selection_action_bookmark, 
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
	ViewGroup mContentView;
	TabHost mTabs;
	LayoutInflater mInflater;
	Properties mProperties;
	Properties mOldProperties;
	OptionsListView mOptionsStyles;
	OptionsListView mOptionsCSS;
	OptionsListView mOptionsPage;
	OptionsListView mOptionsApplication;
	OptionsListView mOptionsControls;
	OptionsListView mOptionsBrowser;

	public final static int OPTION_VIEW_TYPE_NORMAL = 0;
	public final static int OPTION_VIEW_TYPE_BOOLEAN = 1;
	public final static int OPTION_VIEW_TYPE_COLOR = 2;
	public final static int OPTION_VIEW_TYPE_SUBMENU = 3;
	//public final static int OPTION_VIEW_TYPE_COUNT = 3;

	public BaseActivity getActivity() { return mActivity; }
	public Properties getProperties() { return mProperties; }
	public LayoutInflater getInflater() { return mInflater; }
	
	public abstract static class OptionBase {
		protected View myView;
		Properties mProperties;
		BaseActivity mActivity;
		OptionOwner mOwner;
		LayoutInflater mInflater;
		public String label;
		public String property;
		public String defaultValue;
		public int iconId = R.drawable.cr3_option_other;
		public OptionsListView optionsListView;
		protected Runnable onChangeHandler;
		public OptionBase( OptionOwner owner, String label, String property ) {
			this.mOwner = owner;
			this.mActivity = owner.getActivity();
			this.mInflater = owner.getInflater();
			this.mProperties = owner.getProperties();
			this.label = label;
			this.property = property;
		}
		public OptionBase setIconId(int id) {
			this.iconId = id;
			return this;
		}
		public OptionBase noIcon() {
			this.iconId = 0;
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

		public int getItemViewType() {
			return OPTION_VIEW_TYPE_NORMAL;
		}

		protected void refreshItem()
		{
			getView(null, null).invalidate();
			//if ( optionsListView!=null )
			//	optionsListView.refresh();
		}

		protected void refreshList()
		{
			getView(null, null).invalidate();
			if ( optionsListView!=null )
				optionsListView.refresh();
		}

		public View getView(View convertView, ViewGroup parent) {
			View view;
			convertView = myView;
			if ( convertView==null ) {
				//view = new TextView(getContext());
				view = mInflater.inflate(R.layout.option_item, null);
			} else {
				view = (View)convertView;
			}
			myView = view;
			TextView labelView = (TextView)view.findViewById(R.id.option_label);
			TextView valueView = (TextView)view.findViewById(R.id.option_value);
			labelView.setText(label);
			if (valueView != null) {
				String valueLabel = getValueLabel();
				if (valueLabel != null && valueLabel.length() > 0) {
					valueView.setText(valueLabel);
					valueView.setVisibility(View.VISIBLE);
				} else {
					valueView.setText("");
					valueView.setVisibility(View.INVISIBLE);
				}
			}
			ImageView icon = (ImageView)view.findViewById(R.id.option_icon);
			if (icon != null) {
				if (iconId != 0 && showIcons) {
					icon.setVisibility(View.VISIBLE);
					icon.setImageResource(iconId);
				} else {
					icon.setImageResource(0);
					icon.setVisibility(View.INVISIBLE);
				}
			}
			return view;
		}

		public String getValueLabel() { return mProperties.getProperty(property); }
		public void onSelect() { refreshList(); }
	}
	
	class ColorOption extends OptionBase {
		final int defColor;
		public ColorOption( OptionOwner owner, String label, String property, int defColor ) {
			super(owner, label, property);
			this.defColor = defColor;
		}
		public String getValueLabel() { return mProperties.getProperty(property); }
		public void onSelect()
		{ 
			ColorPickerDialog dlg = new ColorPickerDialog(mActivity, new OnColorChangedListener() {
				public void colorChanged(int color) {
					mProperties.setColor(property, color);
					if ( property.equals(PROP_BACKGROUND_COLOR) ) {
						String texture = mProperties.getProperty(PROP_PAGE_BACKGROUND_IMAGE, Engine.NO_TEXTURE.id);
						if ( texture!=null && !texture.equals(Engine.NO_TEXTURE.id) ) {
							// reset background image
							mProperties.setProperty(PROP_PAGE_BACKGROUND_IMAGE, Engine.NO_TEXTURE.id);
							// TODO: show notification?
						}
					}
					refreshList();
				}
			}, mProperties.getColor(property, defColor), label);
			dlg.show();
		}
		public int getItemViewType() {
			return OPTION_VIEW_TYPE_COLOR;
		}
		public View getView(View convertView, ViewGroup parent) {
			View view;
			convertView = myView;
			if ( convertView==null ) {
				//view = new TextView(getContext());
				view = mInflater.inflate(R.layout.option_item_color, null);
			} else {
				view = (View)convertView;
			}
			myView = view;
			TextView labelView = (TextView)view.findViewById(R.id.option_label);
			ImageView valueView = (ImageView)view.findViewById(R.id.option_value_color);
			labelView.setText(label);
			int cl = mProperties.getColor(property, defColor);
			valueView.setBackgroundColor(cl);
			ImageView icon = (ImageView)view.findViewById(R.id.option_icon);
			if (icon != null) {
				if (iconId != 0 && showIcons) {
					icon.setVisibility(View.VISIBLE);
					icon.setImageResource(iconId);
				} else {
					icon.setImageResource(0);
					icon.setVisibility(View.INVISIBLE);
				}
			}
			return view;
		}
	}
	
	private static boolean showIcons = true;
	private static boolean isTextFormat = false;
	private static boolean isEpubFormat = false;
	private Mode mode;
	
	class IconsBoolOption extends BoolOption {
		public IconsBoolOption( OptionOwner owner, String label, String property ) {
			super(owner, label, property);
		}
		public void onSelect() {
			mProperties.setProperty(property, "1".equals(mProperties.getProperty(property)) ? "0" : "1");
			showIcons = mProperties.getBool(property, true);
			mOptionsStyles.refresh();
			mOptionsCSS.refresh();
			mOptionsPage.refresh();
			mOptionsApplication.refresh();
			mOptionsControls.refresh();
		}
	}
	class BoolOption extends OptionBase {
		private boolean inverse = false;
		public BoolOption( OptionOwner owner, String label, String property ) {
			super(owner, label, property);
		}
		private boolean getValueBoolean() { return "1".equals(mProperties.getProperty(property)) ^ inverse; }
		public String getValueLabel() { return getValueBoolean()  ? getString(R.string.options_value_on) : getString(R.string.options_value_off); }
		public void onSelect() { 
			mProperties.setProperty(property, "1".equals(mProperties.getProperty(property)) ? "0" : "1");
			refreshList();
		}
		public BoolOption setInverse() { inverse = true; return this; }
		public int getItemViewType() {
			return OPTION_VIEW_TYPE_BOOLEAN;
		}
		public View getView(View convertView, ViewGroup parent) {
			View view;
			convertView = myView;
			if ( convertView==null ) {
				//view = new TextView(getContext());
				view = mInflater.inflate(R.layout.option_item_boolean, null);
			} else {
				view = (View)convertView;
			}
			myView = view;
			TextView labelView = (TextView)view.findViewById(R.id.option_label);
			CheckBox valueView = (CheckBox)view.findViewById(R.id.option_value_cb);
//			valueView.setFocusable(false);
//			valueView.setClickable(false);
			labelView.setText(label);
			valueView.setChecked(getValueBoolean());
			valueView.setOnCheckedChangeListener(new OnCheckedChangeListener() {
					@Override
					public void onCheckedChanged(CompoundButton arg0,
							boolean checked) {
//						mProperties.setBool(property, checked);
//						refreshList();
					}
				});
			ImageView icon = (ImageView)view.findViewById(R.id.option_icon);
			if (icon != null) {
				if (iconId != 0 && showIcons) {
					icon.setVisibility(View.VISIBLE);
					icon.setImageResource(iconId);
				} else {
					icon.setImageResource(0);
					icon.setVisibility(View.INVISIBLE);
				}
			}
//			view.setClickable(true);
//			view.setFocusable(true);
			return view;
		}
	}

	static public void saveColor( Properties mProperties, boolean night )
	{
		if ( night ) {
			mProperties.setProperty(PROP_PAGE_BACKGROUND_IMAGE_NIGHT, mProperties.getProperty(PROP_PAGE_BACKGROUND_IMAGE, "(NONE)"));
			mProperties.setColor(PROP_BACKGROUND_COLOR_NIGHT, mProperties.getColor(PROP_BACKGROUND_COLOR, 0x000000));
			mProperties.setColor(PROP_FONT_COLOR_NIGHT, mProperties.getColor(PROP_FONT_COLOR, 0xFFFFFF));
			mProperties.setColor(PROP_STATUS_FONT_COLOR_NIGHT, mProperties.getColor(PROP_STATUS_FONT_COLOR, 0xFFFFFF));
			mProperties.setInt(PROP_APP_SCREEN_BACKLIGHT_NIGHT, mProperties.getInt(PROP_APP_SCREEN_BACKLIGHT, -1));
			mProperties.setProperty(PROP_FONT_GAMMA_NIGHT, mProperties.getProperty(PROP_FONT_GAMMA, "1.0"));
			mProperties.setProperty(PROP_APP_THEME_NIGHT, mProperties.getProperty(PROP_APP_THEME, "BLACK"));
			mProperties.setInt(PROP_APP_HIGHLIGHT_BOOKMARKS_NIGHT, mProperties.getInt(PROP_APP_HIGHLIGHT_BOOKMARKS, 1));
			mProperties.setColor(PROP_HIGHLIGHT_SELECTION_COLOR_NIGHT, mProperties.getColor(PROP_HIGHLIGHT_SELECTION_COLOR, 0xCCCCCC));
			mProperties.setColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT_NIGHT, mProperties.getColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT, 0xFFFF40));
			mProperties.setColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION_NIGHT, mProperties.getColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION, 0xFF8000));
		} else {
			mProperties.setProperty(PROP_PAGE_BACKGROUND_IMAGE_DAY, mProperties.getProperty(PROP_PAGE_BACKGROUND_IMAGE, "(NONE)"));
			mProperties.setColor(PROP_BACKGROUND_COLOR_DAY, mProperties.getColor(PROP_BACKGROUND_COLOR, 0xFFFFFF));
			mProperties.setColor(PROP_FONT_COLOR_DAY, mProperties.getColor(PROP_FONT_COLOR, 0x000000));
			mProperties.setColor(PROP_STATUS_FONT_COLOR_DAY, mProperties.getColor(PROP_STATUS_FONT_COLOR, 0x000000));
			mProperties.setInt(PROP_APP_SCREEN_BACKLIGHT_DAY, mProperties.getInt(PROP_APP_SCREEN_BACKLIGHT, -1));
			mProperties.setProperty(PROP_FONT_GAMMA_DAY, mProperties.getProperty(PROP_FONT_GAMMA, "1.0"));
			mProperties.setProperty(PROP_APP_THEME_DAY, mProperties.getProperty(PROP_APP_THEME, "WHITE"));
			mProperties.setInt(PROP_APP_HIGHLIGHT_BOOKMARKS_DAY, mProperties.getInt(PROP_APP_HIGHLIGHT_BOOKMARKS, 1));
			mProperties.setColor(PROP_HIGHLIGHT_SELECTION_COLOR_DAY, mProperties.getColor(PROP_HIGHLIGHT_SELECTION_COLOR, 0xCCCCCC));
			mProperties.setColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT_DAY, mProperties.getColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT, 0xFFFF40));
			mProperties.setColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION_DAY, mProperties.getColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION, 0xFF8000));
		}
		for (String code : styleCodes) {
			String styleName = "styles." + code + ".color";
			String v = mProperties.getProperty(styleName); 
			if (v != null) {
				if ( night )
					mProperties.setProperty(styleName + ".night", v);
				else
					mProperties.setProperty(styleName + ".day", v);
			}
		}
	}

	static public void restoreColor( Properties mProperties,  boolean night )
	{
		if ( night ) {
			mProperties.setProperty(PROP_PAGE_BACKGROUND_IMAGE, mProperties.getProperty(PROP_PAGE_BACKGROUND_IMAGE_NIGHT, "(NONE)"));
			mProperties.setColor(PROP_BACKGROUND_COLOR, mProperties.getColor(PROP_BACKGROUND_COLOR_NIGHT, 0x000000));
			mProperties.setColor(PROP_FONT_COLOR, mProperties.getColor(PROP_FONT_COLOR_NIGHT, 0xFFFFFF));
			mProperties.setColor(PROP_STATUS_FONT_COLOR, mProperties.getColor(PROP_STATUS_FONT_COLOR_NIGHT, 0xFFFFFF));
			mProperties.setInt(PROP_APP_SCREEN_BACKLIGHT, mProperties.getInt(PROP_APP_SCREEN_BACKLIGHT_NIGHT, 70));
			mProperties.setProperty(PROP_FONT_GAMMA, mProperties.getProperty(PROP_FONT_GAMMA_NIGHT, "1.0"));
			mProperties.setProperty(PROP_APP_THEME, mProperties.getProperty(PROP_APP_THEME_NIGHT, "BLACK"));
			mProperties.setInt(PROP_APP_HIGHLIGHT_BOOKMARKS, mProperties.getInt(PROP_APP_HIGHLIGHT_BOOKMARKS_NIGHT, 1));
			mProperties.setColor(PROP_HIGHLIGHT_SELECTION_COLOR, mProperties.getColor(PROP_HIGHLIGHT_SELECTION_COLOR_NIGHT, 0xCCCCCC));
			mProperties.setColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT, mProperties.getColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT_NIGHT, 0xFFFF40));
			mProperties.setColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION, mProperties.getColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION_NIGHT, 0xFF8000));
		} else {
			mProperties.setProperty(PROP_PAGE_BACKGROUND_IMAGE, mProperties.getProperty(PROP_PAGE_BACKGROUND_IMAGE_DAY, "(NONE)"));
			mProperties.setColor(PROP_BACKGROUND_COLOR, mProperties.getColor(PROP_BACKGROUND_COLOR_DAY, 0xFFFFFF));
			mProperties.setColor(PROP_FONT_COLOR, mProperties.getColor(PROP_FONT_COLOR_DAY, 0x000000));
			mProperties.setColor(PROP_STATUS_FONT_COLOR, mProperties.getColor(PROP_STATUS_FONT_COLOR_DAY, 0x000000));
			mProperties.setInt(PROP_APP_SCREEN_BACKLIGHT, mProperties.getInt(PROP_APP_SCREEN_BACKLIGHT_DAY, 80));
			mProperties.setProperty(PROP_FONT_GAMMA, mProperties.getProperty(PROP_FONT_GAMMA_DAY, "1.0"));
			mProperties.setProperty(PROP_APP_THEME, mProperties.getProperty(PROP_APP_THEME_DAY, "WHITE"));
			mProperties.setInt(PROP_APP_HIGHLIGHT_BOOKMARKS, mProperties.getInt(PROP_APP_HIGHLIGHT_BOOKMARKS_DAY, 1));
			mProperties.setColor(PROP_HIGHLIGHT_SELECTION_COLOR, mProperties.getColor(PROP_HIGHLIGHT_SELECTION_COLOR_DAY, 0xCCCCCC));
			mProperties.setColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT, mProperties.getColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT_DAY, 0xFFFF40));
			mProperties.setColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION, mProperties.getColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION_DAY, 0xFF8000));
		}
		for (String code : styleCodes) {
			String styleName = "styles." + code + ".color";
			String pname = night ? styleName + ".night" : styleName + ".day";
			String v = mProperties.getProperty(pname);
			if (v != null)
				mProperties.setProperty(styleName, mProperties.getProperty(pname));
		}
	}

	static public void toggleDayNightMode( Properties mProperties ) {
		boolean oldMode = mProperties.getBool(PROP_NIGHT_MODE, false);
		saveColor(mProperties, oldMode);
		boolean newMode = !oldMode;
		restoreColor(mProperties, newMode);
		mProperties.setBool(PROP_NIGHT_MODE, newMode);
	}

	class NightModeOption extends BoolOption {
		public NightModeOption( OptionOwner owner, String label, String property ) {
			super(owner, label, property);
		}
		public void onSelect() { 
			toggleDayNightMode(mProperties);
			refreshList();
		}
	}
	
	class LangOption extends ListOption {
		public LangOption(OptionOwner owner) {
			super(owner, getString(R.string.options_app_locale), PROP_APP_LOCALE);
			for (Lang lang : Lang.values()) {
				add(lang.code, getString(lang.nameId));
			}
			if ( mProperties.getProperty(property)==null )
				mProperties.setProperty(property, Lang.DEFAULT.code);
		}
	}

	class ActionOption extends ListOption {
		public ActionOption( OptionOwner owner, String label, String property, boolean isTap, boolean allowRepeat ) {
			super(owner, label, property);
			ReaderAction[] actions = ReaderAction.AVAILABLE_ACTIONS;
			for ( ReaderAction a : actions )
				if ( !isTap || a.mayAssignOnTap() )
					add(a.id, getString(a.nameId));
			if ( allowRepeat )
				add(ReaderAction.REPEAT.id, getString(ReaderAction.REPEAT.nameId));
			if ( mProperties.getProperty(property)==null )
				mProperties.setProperty(property, ReaderAction.NONE.id);
		}
	}

	class KeyMapOption extends SubmenuOption {
		public KeyMapOption( OptionOwner owner, String label ) {
			super(owner, label, PROP_APP_KEY_ACTIONS_PRESS);
		}
		private void addKey( OptionsListView list, int keyCode, String keyName ) {
			final String propName = ReaderAction.getKeyProp(keyCode, ReaderAction.NORMAL);
			final String longPropName = ReaderAction.getKeyProp(keyCode, ReaderAction.LONG);
			final String dblPropName = ReaderAction.getKeyProp(keyCode, ReaderAction.DOUBLE);
			list.add(new ActionOption(mOwner, keyName, propName, false, false));
			list.add(new ActionOption(mOwner, keyName + " " + getContext().getString(R.string.options_app_key_long_press), longPropName, false, true));
			list.add(new ActionOption(mOwner, keyName + " " + getContext().getString(R.string.options_app_key_double_press), dblPropName, false, false));
		}
		public void onSelect() {
			BaseDialog dlg = new BaseDialog(mActivity, label, false, false);
			OptionsListView listView = new OptionsListView(getContext());
			if ( DeviceInfo.NOOK_NAVIGATION_KEYS ) {
				addKey(listView, ReaderView.KEYCODE_PAGE_TOPLEFT, "Top left navigation button");
				addKey(listView, ReaderView.KEYCODE_PAGE_BOTTOMLEFT, "Bottom left navigation button");
				addKey(listView, ReaderView.KEYCODE_PAGE_TOPRIGHT, "Top right navigation button");
				addKey(listView, ReaderView.NOOK_12_KEY_NEXT_LEFT, "Bottom right navigation button");
//				addKey(listView, ReaderView.KEYCODE_PAGE_BOTTOMRIGHT, "Bottom right navigation button");

				// on rooted Nook, side navigation keys may be reassigned on some standard android keycode
				addKey(listView, KeyEvent.KEYCODE_MENU, "Menu");
				addKey(listView, KeyEvent.KEYCODE_BACK, "Back");
				addKey(listView, KeyEvent.KEYCODE_SEARCH, "Search");
				
				addKey(listView, KeyEvent.KEYCODE_HOME, "Home");
				
				addKey(listView, KeyEvent.KEYCODE_2, "Up");
				addKey(listView, KeyEvent.KEYCODE_8, "Down");
			} else if ( DeviceInfo.SONY_NAVIGATION_KEYS ) {
//				addKey(listView, KeyEvent.KEYCODE_DPAD_UP, "Prev button");
//				addKey(listView, KeyEvent.KEYCODE_DPAD_DOWN, "Next button");
				addKey(listView, ReaderView.SONY_DPAD_UP_SCANCODE, "Prev button");
				addKey(listView, ReaderView.SONY_DPAD_DOWN_SCANCODE, "Next button");
				addKey(listView, ReaderView.SONY_DPAD_LEFT_SCANCODE, "Left button");
				addKey(listView, ReaderView.SONY_DPAD_RIGHT_SCANCODE, "Right button");
//				addKey(listView, ReaderView.SONY_MENU_SCANCODE, "Menu");
//				addKey(listView, ReaderView.SONY_BACK_SCANCODE, "Back");
//				addKey(listView, ReaderView.SONY_HOME_SCANCODE, "Home");
				addKey(listView, KeyEvent.KEYCODE_MENU, "Menu");
				addKey(listView, KeyEvent.KEYCODE_BACK, "Back");

				addKey(listView, KeyEvent.KEYCODE_HOME, "Home");
			} else {
				addKey(listView, KeyEvent.KEYCODE_MENU, "Menu");
				addKey(listView, KeyEvent.KEYCODE_BACK, "Back");
				addKey(listView, KeyEvent.KEYCODE_DPAD_LEFT, "Left");
				addKey(listView, KeyEvent.KEYCODE_DPAD_RIGHT, "Right");
				addKey(listView, KeyEvent.KEYCODE_DPAD_UP, "Up");
				addKey(listView, KeyEvent.KEYCODE_DPAD_DOWN, "Down");
				addKey(listView, KeyEvent.KEYCODE_DPAD_CENTER, "Center");
				addKey(listView, KeyEvent.KEYCODE_SEARCH, "Search");
				addKey(listView, KeyEvent.KEYCODE_VOLUME_UP, "Volume Up");
				addKey(listView, KeyEvent.KEYCODE_VOLUME_DOWN, "Volume Down");
				addKey(listView, KeyEvent.KEYCODE_CAMERA, "Camera");
				addKey(listView, KeyEvent.KEYCODE_HEADSETHOOK, "Headset Hook");
				addKey(listView, ReaderView.KEYCODE_ESCAPE, "Escape");
			}

			dlg.setView(listView);
			dlg.show();
		}

		public String getValueLabel() { return ">"; }
	}
	
	class StatusBarOption extends SubmenuOption {
		public StatusBarOption( OptionOwner owner, String label ) {
			super(owner, label, PROP_SHOW_TITLE);
		}
		public void onSelect() {
			BaseDialog dlg = new BaseDialog(mActivity, label, false, false);
			OptionsListView listView = new OptionsListView(getContext());
			listView.add(new ListOption(mOwner, getString(R.string.options_page_show_titlebar), PROP_STATUS_LOCATION).add(mStatusPositions, mStatusPositionsTitles).setDefaultValue("1"));
			listView.add(new ListOption(mOwner, getString(R.string.options_page_titlebar_font_face), PROP_STATUS_FONT_FACE).add(mFontFaces).setDefaultValue(mFontFaces[0]).setIconId(R.drawable.cr3_option_font_face));
			listView.add(new ListOption(mOwner, getString(R.string.options_page_titlebar_font_size), PROP_STATUS_FONT_SIZE).add(mStatusFontSizes).setDefaultValue("18").setIconId(R.drawable.cr3_option_font_size));
			listView.add(new ColorOption(mOwner, getString(R.string.options_page_titlebar_font_color), PROP_STATUS_FONT_COLOR, 0x000000));
			listView.add(new BoolOption(mOwner, getString(R.string.options_page_show_titlebar_title), PROP_SHOW_TITLE).setDefaultValue("1"));
			listView.add(new BoolOption(mOwner, getString(R.string.options_page_show_titlebar_page_number), PROP_SHOW_PAGE_NUMBER).setDefaultValue("1"));
			listView.add(new BoolOption(mOwner, getString(R.string.options_page_show_titlebar_page_count), PROP_SHOW_PAGE_COUNT).setDefaultValue("1"));
			listView.add(new BoolOption(mOwner, getString(R.string.options_page_show_titlebar_percent), PROP_SHOW_POS_PERCENT).setDefaultValue("0"));
			listView.add(new BoolOption(mOwner, getString(R.string.options_page_show_titlebar_chapter_marks), PROP_STATUS_CHAPTER_MARKS).setDefaultValue("1"));
			listView.add(new BoolOption(mOwner, getString(R.string.options_page_show_titlebar_battery_percent), PROP_SHOW_BATTERY_PERCENT).setDefaultValue("1"));
			dlg.setView(listView);
			dlg.show();
		}

		public String getValueLabel() { return ">"; }
	}
	
	class PluginsOption extends SubmenuOption {
		public PluginsOption( OptionOwner owner, String label ) {
			super(owner, label, PROP_APP_PLUGIN_ENABLED);
		}
		public void onSelect() {
			BaseDialog dlg = new BaseDialog(mActivity, label, false, false);
			OptionsListView listView = new OptionsListView(getContext());
			boolean defEnableLitres = activity.getCurrentLanguage().toLowerCase().startsWith("ru") && !DeviceInfo.POCKETBOOK;
			listView.add(new BoolOption(mOwner, "LitRes", PROP_APP_PLUGIN_ENABLED + "." + OnlineStorePluginManager.PLUGIN_PKG_LITRES).setDefaultValue(defEnableLitres ? "1" : "0"));
			dlg.setView(listView);
			dlg.show();
		}

		public String getValueLabel() { return ">"; }
	}
	
	class ImageScalingOption extends SubmenuOption {
		public ImageScalingOption( OptionOwner owner, String label ) {
			super(owner, label, PROP_IMG_SCALING_ZOOMIN_BLOCK_MODE);
		}
		public void onSelect() {
			BaseDialog dlg = new BaseDialog(mActivity, label, false, false);
			OptionsListView listView = new OptionsListView(getContext());
			listView.add(new ListOption(mOwner, getString(R.string.options_format_image_scaling_block_mode), PROP_IMG_SCALING_ZOOMIN_BLOCK_MODE).add(mImageScalingModes, mImageScalingModesTitles).setDefaultValue("2"));
			listView.add(new ListOption(mOwner, getString(R.string.options_format_image_scaling_block_scale), PROP_IMG_SCALING_ZOOMIN_BLOCK_SCALE).add(mImageScalingFactors, mImageScalingFactorsTitles).setDefaultValue("2"));
			listView.add(new ListOption(mOwner, getString(R.string.options_format_image_scaling_inline_mode), PROP_IMG_SCALING_ZOOMIN_INLINE_MODE).add(mImageScalingModes, mImageScalingModesTitles).setDefaultValue("2"));
			listView.add(new ListOption(mOwner, getString(R.string.options_format_image_scaling_inline_scale), PROP_IMG_SCALING_ZOOMIN_INLINE_SCALE).add(mImageScalingFactors, mImageScalingFactorsTitles).setDefaultValue("2"));
			dlg.setView(listView);
			dlg.show();
		}

		private void copyProperty( String to, String from ) {
			mProperties.put(to, mProperties.get(from));
		}

		protected void closed() {
			copyProperty(PROP_IMG_SCALING_ZOOMOUT_BLOCK_MODE, PROP_IMG_SCALING_ZOOMIN_BLOCK_MODE);
			copyProperty(PROP_IMG_SCALING_ZOOMOUT_INLINE_MODE, PROP_IMG_SCALING_ZOOMIN_INLINE_MODE);
			copyProperty(PROP_IMG_SCALING_ZOOMOUT_BLOCK_SCALE, PROP_IMG_SCALING_ZOOMIN_BLOCK_SCALE);
			copyProperty(PROP_IMG_SCALING_ZOOMOUT_INLINE_SCALE, PROP_IMG_SCALING_ZOOMIN_INLINE_SCALE);
		}
		
		public String getValueLabel() { return ">"; }
	}
	
	class TapZoneOption extends SubmenuOption {
		public TapZoneOption( OptionOwner owner, String label, String property ) {
			super( owner, label, property);
		}
		View grid;
		private void initTapZone( View view, final int tapZoneId )
		{
			if ( view==null )
				return;
			final TextView text = (TextView)view.findViewById(R.id.tap_zone_action_text_short);
			final TextView longtext = (TextView)view.findViewById(R.id.tap_zone_action_text_long);
			final String propName = property + "." + tapZoneId;
			final String longPropName = property + ".long." + tapZoneId;
			ReaderAction action = ReaderAction.findById( mProperties.getProperty(propName) );
			ReaderAction longAction = ReaderAction.findById( mProperties.getProperty(longPropName) );
			text.setText(getString(action.nameId));
			longtext.setText(getString(longAction.nameId));
			view.setLongClickable(true);
			view.setOnClickListener(new View.OnClickListener () {
				@Override
				public void onClick(View v) {
					// TODO: i18n
					ActionOption option = new ActionOption(mOwner, getString(R.string.options_app_tap_action_short), propName, true, false);
					option.setOnChangeHandler(new Runnable() {
						public void run() {
							ReaderAction action = ReaderAction.findById( mProperties.getProperty(propName) );
							text.setText(getString(action.nameId));
						}
					});
					option.onSelect();
				}
			});
			view.setOnLongClickListener(new View.OnLongClickListener () {
				@Override
				public boolean onLongClick(View v) {
					// TODO: i18n
					ActionOption option = new ActionOption(mOwner, getString(R.string.options_app_tap_action_long), longPropName, true, true);
					option.setOnChangeHandler(new Runnable() {
						public void run() {
							ReaderAction longAction = ReaderAction.findById( mProperties.getProperty(longPropName) );
							longtext.setText(getString(longAction.nameId));
						}
					});
					option.onSelect();
					return true;
				}
			});
		}

		public String getValueLabel() { return ">"; }
		public void onSelect() {
			BaseDialog dlg = new BaseDialog(mActivity, label, false, false);
			grid = (View)mInflater.inflate(R.layout.options_tap_zone_grid, null);
			initTapZone(grid.findViewById(R.id.tap_zone_grid_cell1), 1);
			initTapZone(grid.findViewById(R.id.tap_zone_grid_cell2), 2);
			initTapZone(grid.findViewById(R.id.tap_zone_grid_cell3), 3);
			initTapZone(grid.findViewById(R.id.tap_zone_grid_cell4), 4);
			initTapZone(grid.findViewById(R.id.tap_zone_grid_cell5), 5);
			initTapZone(grid.findViewById(R.id.tap_zone_grid_cell6), 6);
			initTapZone(grid.findViewById(R.id.tap_zone_grid_cell7), 7);
			initTapZone(grid.findViewById(R.id.tap_zone_grid_cell8), 8);
			initTapZone(grid.findViewById(R.id.tap_zone_grid_cell9), 9);
			dlg.setView(grid);
			dlg.show();
		}
	}
	
	public static class Pair {
		public String value;
		public String label;
		public Pair(String value, String label) {
			this.value = value;
			this.label = label;
		}
	}

	public static class SubmenuOption extends ListOption {
		public SubmenuOption( OptionOwner owner, String label, String property ) {
			super(owner, label, property);
		}
		public int getItemViewType() {
			return OPTION_VIEW_TYPE_SUBMENU; 
		}
		public View getView(View convertView, ViewGroup parent) {
			View view;
			convertView = myView;
			if ( convertView==null ) {
				//view = new TextView(getContext());
				view = mInflater.inflate(R.layout.option_item_submenu, null);
			} else {
				view = (View)convertView;
			}
			myView = view;
			TextView labelView = (TextView)view.findViewById(R.id.option_label);
			labelView.setText(label);
			ImageView icon = (ImageView)view.findViewById(R.id.option_icon);
			if (icon != null) {
				if (iconId != 0 && showIcons) {
					icon.setVisibility(View.VISIBLE);
					icon.setImageResource(iconId);
				} else {
					icon.setImageResource(0);
					icon.setVisibility(View.INVISIBLE);
				}
			}
			return view;
		}
	}
	
	public static class ListOption extends OptionBase {
		private ArrayList<Pair> list = new ArrayList<Pair>();
		public ListOption( OptionOwner owner, String label, String property ) {
			super(owner, label, property);
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
		public ListOption add(double[]values) {
			for ( double item : values ) {
				String s = String.valueOf(item); 
				add(s, s);
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
				String label = mActivity.getString(labelIDs[i]); 
				add(value, label);
			}
			return this;
		}
		public ListOption add(String[]values, int[]labelIDs) {
			for ( int i=0; i<values.length; i++ ) {
				String value = values[i]; 
				String label = mActivity.getString(labelIDs[i]); 
				add(value, label);
			}
			return this;
		}
		public ListOption add(String[]values, String[]labels) {
			for ( int i=0; i<values.length; i++ ) {
				String value = values[i]; 
				String label = labels[i]; 
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
				if ( value!=null && pair.value.equals(value) )
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

		protected void closed() {
			
		}
		
		protected int getItemLayoutId() {
			return R.layout.option_value; 
		}
		
		protected void updateItemContents( final View layout, final Pair item, final ListView listView, final int position ) {
			TextView view;
			RadioButton cb;
			view = (TextView)layout.findViewById(R.id.option_value_text);
			cb = (RadioButton)layout.findViewById(R.id.option_value_check);
			view.setText(item.label);
			String currValue = mProperties.getProperty(property);
			boolean isSelected = item.value!=null && currValue!=null && item.value.equals(currValue) ;//getSelectedItemIndex()==position;
			cb.setChecked(isSelected);
			cb.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					listView.getOnItemClickListener().onItemClick(listView, listView, position, 0);
//					mProperties.setProperty(property, item.value);
//					dismiss();
//					optionsListView.refresh();
				}
			});
		}
		
		public String getValueLabel() { return findValueLabel(mProperties.getProperty(property)); }
		
		public void onSelect() {
			final BaseDialog dlg = new BaseDialog(mActivity, label, false, false);

			final ListView listView = new BaseListView(mActivity, false);
			
			
			ListAdapter listAdapter = new BaseAdapter() {

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
					if ( convertView==null ) {
						layout = (ViewGroup)mInflater.inflate(getItemLayoutId(), null);
						//view = new TextView(getContext());
					} else {
						layout = (ViewGroup)convertView;
					}
					final Pair item = list.get(position);
					updateItemContents( layout, item, listView, position );
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
			//final AlertDialog d = dlg.create();
			listView.setOnItemClickListener(new OnItemClickListener() {

				public void onItemClick(AdapterView<?> adapter, View listview,
						int position, long id) {
					Pair item = list.get(position);
					onClick(item);
					dlg.dismiss();
					closed();
				}
			});
			dlg.show();
		}
		
		public void onClick( Pair item ) {
			mProperties.setProperty(property, item.value);
			refreshList();
			if ( onChangeHandler!=null )
				onChangeHandler.run();
			if ( optionsListView!=null )
				optionsListView.refresh();
		}
	}
	
	class DictOptions extends ListOption
	{
		public DictOptions( OptionOwner owner, String label )
		{
			super( owner, label, PROP_APP_DICTIONARY );
			DictInfo[] dicts = BaseActivity.getDictList();
			setDefaultValue(dicts[0].id);
			for (DictInfo dict : dicts) {
				boolean installed = mActivity.isPackageInstalled(dict.packageName);
				add( dict.id, dict.name + (installed ? "" : " " + mActivity.getString(R.string.options_app_dictionary_not_installed)));
			}
		}
	} 
	
	class HyphenationOptions extends ListOption
	{
		public HyphenationOptions( OptionOwner owner, String label )
		{
			super( owner, label, PROP_HYPHENATION_DICT );
			setDefaultValue("RUSSIAN");
			Engine.HyphDict[] dicts = Engine.HyphDict.values();
			for ( Engine.HyphDict dict : dicts )
				add( dict.toString(), dict.getName() );
		}
	}
	
	class ThemeOptions extends ListOption
	{
		public ThemeOptions( OptionOwner owner, String label )
		{
			super( owner, label, PROP_APP_THEME );
			setDefaultValue(DeviceInfo.FORCE_LIGHT_THEME ? "WHITE" : "LIGHT");
			for (InterfaceTheme theme : InterfaceTheme.allThemes)
				add(theme.getCode(), getString(theme.getDisplayNameResourceId()));
		}
	}
	
	class ThumbnailCache {
		final int maxcount;
		final int dx;
		final int dy;
		class Item {
			Drawable drawable;
			Bitmap bmp;
			String path;
			int id;
			public void clear() {
				if ( bmp!=null ) {
					//bmp.recycle();
					bmp = null;
				}
				if ( drawable!=null )
					drawable = null;
			}
		}
		ArrayList<Item> list = new ArrayList<Item>(); 
		public ThumbnailCache( int dx, int dy, int maxcount ) {
			this.dx = dx;
			this.dy = dy;
			this.maxcount = maxcount;
		}
		private void remove( int maxsize ) {
			while ( list.size()>maxsize ) {
				Item item = list.remove(0);
				item.clear();
			}
		}
		private Drawable createDrawable( String path ) {
			File f = new File(path);
			if ( !f.isFile() || !f.exists() )
				return null;
			try { 
				BitmapDrawable drawable = (BitmapDrawable)BitmapDrawable.createFromPath(path);
				if ( drawable==null )
					return null;
				Bitmap src = drawable.getBitmap();
				Bitmap bmp = Bitmap.createScaledBitmap(src, dx, dy, true);
				//Canvas canvas = new Canvas(bmp);
				BitmapDrawable res = new BitmapDrawable(bmp);
				//src.recycle();
				Item item = new Item();
				item.path = path;
				item.drawable = res; //drawable;
				item.bmp = bmp;
				list.add(item);
				remove(maxcount);
				return drawable;
			} catch ( Exception e ) {
				return null;
			}
		}
		private Drawable createDrawable( int resourceId ) {
			try { 
				//Drawable drawable = mReaderView.getActivity().getResources().getDrawable(resourceId);
				InputStream is = getContext().getResources().openRawResource(resourceId);
				if ( is==null )
					return null;
				BitmapDrawable src = new BitmapDrawable(is);
				Item item = new Item();
				item.id = resourceId;
				Bitmap bmp = Bitmap.createScaledBitmap(src.getBitmap(), dx, dy, true);
				BitmapDrawable res = new BitmapDrawable(bmp);
				item.drawable = res;
				item.bmp = bmp;
				list.add(item);
				remove(maxcount);
				return res;
			} catch ( Exception e ) {
				return null;
			}
		}
		public Drawable getImage( String path ) {
			if ( path==null || !path.startsWith("/"))
				return null;
			// find existing
			for ( int i=0; i<list.size(); i++ ) {
				if ( list.get(i).path!=null && path.equals(list.get(i).path) ) {
					Item item = list.remove(i);
					list.add(item);
					return item.drawable;
				}
			}
			return createDrawable( path ); 
		}
		public Drawable getImage( int resourceId ) {
			if ( resourceId==0 )
				return null;
			// find existing
			for ( int i=0; i<list.size(); i++ ) {
				if ( list.get(i).id == resourceId ) {
					Item item = list.remove(i);
					list.add(item);
					return item.drawable;
				}
			}
			return createDrawable( resourceId ); 
		}
		public void clear() {
			remove(0);
		}
	}
	
	ThumbnailCache textureSampleCache = new ThumbnailCache(64, 64, 100);
	
	class TextureOptions extends ListOption
	{
		public TextureOptions( OptionOwner owner, String label )
		{
			super( owner, label, PROP_PAGE_BACKGROUND_IMAGE );
			setDefaultValue("(NONE)");
			BackgroundTextureInfo[] textures = mReaderView.getEngine().getAvailableTextures();
			for ( BackgroundTextureInfo item : textures )
				add( item.id, item.name );
		}

		protected void closed() {
			textureSampleCache.clear();
		}

		protected int getItemLayoutId() {
			return R.layout.option_value_image; 
		}
		
		protected void updateItemContents( final View layout, final Pair item, final ListView listView, final int position ) {
			super.updateItemContents(layout, item, listView, position);
			ImageView img = (ImageView)layout.findViewById(R.id.option_value_image);
			int cl = mProperties.getColor(PROP_BACKGROUND_COLOR, Color.WHITE);
			BackgroundTextureInfo texture = Services.getEngine().getTextureInfoById(item.value);
			img.setBackgroundColor(cl);
			if ( texture.resourceId!=0 ) {
//				img.setImageDrawable(null);
//				img.setImageResource(texture.resourceId);
//				img.setBackgroundColor(Color.TRANSPARENT);
				Drawable drawable = textureSampleCache.getImage(texture.resourceId);
				if ( drawable!=null ) {
					img.setImageResource(0);
					img.setImageDrawable(drawable);
					img.setBackgroundColor(Color.TRANSPARENT);
				} else {
					img.setBackgroundColor(cl);
					img.setImageResource(0);
					img.setImageDrawable(null);
				}
			} else {
				// load image from file
				Drawable drawable = textureSampleCache.getImage(texture.id);
				if ( drawable!=null ) {
					img.setImageResource(0);
					img.setImageDrawable(drawable);
					img.setBackgroundColor(Color.TRANSPARENT);
				} else {
					img.setBackgroundColor(cl);
					img.setImageResource(0);
					img.setImageDrawable(null);
				}
			}
		}
	}
	
	//byte[] fakeLongArrayForDebug;
	
	public enum Mode {
		READER,
		BROWSER,
	}
	public OptionsDialog(BaseActivity activity, ReaderView readerView, String[] fontFaces, Mode mode)
	{
		super(activity, null, false, false);
		
		mActivity = activity;
		mReaderView = readerView;
		mFontFaces = fontFaces;
		mProperties = new Properties(mActivity.settings()); //  readerView.getSettings();
		mOldProperties = new Properties(mProperties);
		if (mode == Mode.READER) {
			mProperties.setBool(PROP_TXT_OPTION_PREFORMATTED, mReaderView.isTextAutoformatEnabled());
			mProperties.setBool(PROP_EMBEDDED_STYLES, mReaderView.getDocumentStylesEnabled());
			mProperties.setBool(PROP_EMBEDDED_FONTS, mReaderView.getDocumentFontsEnabled());
			isTextFormat = readerView.isTextFormat();
			isEpubFormat = readerView.isFormatWithEmbeddedFonts();
		}
		showIcons = mProperties.getBool(PROP_APP_SETTINGS_SHOW_ICONS, true);
		this.mode = mode;
	}
	
	class OptionsListView extends BaseListView {
		private ArrayList<OptionBase> mOptions = new ArrayList<OptionBase>();
		private ListAdapter mAdapter;
		public void refresh()
		{
			//setAdapter(mAdapter);
			for ( OptionBase item : mOptions ) {
				item.refreshItem();
			}
			invalidate();
		}
		public OptionsListView add( OptionBase option ) {
			mOptions.add(option);
			option.optionsListView = this;
			return this;
		}
		public OptionsListView( Context context )
		{
			super(context, false);
			setFocusable(true);
			setFocusableInTouchMode(true);
			mAdapter = new BaseAdapter() {
				public boolean areAllItemsEnabled() {
					return false;
				}

				public boolean isEnabled(int position) {
					boolean isPageMode = mProperties.getBool(PROP_PAGE_VIEW_MODE, true);
					OptionBase option = mOptions.get(position);
					String prop = option.property;
					if ( prop.equals(PROP_STATUS_LINE) || prop.equals(PROP_FOOTNOTES) )
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
//					OptionBase item = mOptions.get(position);
//					return item.getItemViewType();
					return position;
				}

				
				public View getView(int position, View convertView, ViewGroup parent) {
					OptionBase item = mOptions.get(position);
					return item.getView(convertView, parent);
				}

				public int getViewTypeCount() {
					//return OPTION_VIEW_TYPE_COUNT;
					return mOptions.size() > 0 ? mOptions.size() : 1;
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
		else if ( "CSS".equals(tag) )
			return mOptionsCSS;
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

	class StyleEditorOption extends SubmenuOption {
		
		private final String prefix;
		
		public StyleEditorOption( OptionOwner owner, String label, String prefix ) {
			super(owner, label, "dummy.prop");
			this.prefix = prefix;
		}
		public void onSelect() {
			BaseDialog dlg = new BaseDialog(mActivity, label, false, false);
			OptionsListView listView = new OptionsListView(getContext());
			String[] firstLineOptions = {"", "text-align: justify", "text-align: left", "text-align: center", "text-align: right", };
			int[] firstLineOptionNames = {
					R.string.options_css_inherited,
					R.string.options_css_text_align_justify,
					R.string.options_css_text_align_left,
					R.string.options_css_text_align_center,
					R.string.options_css_text_align_right,
			};
			listView.add(new ListOption(mOwner, getString(R.string.options_css_text_align), prefix + ".align").add(firstLineOptions, firstLineOptionNames).setIconId(R.drawable.cr3_option_text_align));
			
			String[] identOptions = {"", // inherited
			        "text-indent: 0em",
			        "text-indent: 1.2em",
			        "text-indent: 2em",
			        "text-indent: -1.2em",
			        "text-indent: -2em"};
			int[] identOptionNames = {
					R.string.options_css_inherited,
					R.string.options_css_text_indent_no_indent,
					R.string.options_css_text_indent_small_indent,
					R.string.options_css_text_indent_big_indent,
					R.string.options_css_text_indent_small_outdent,
					R.string.options_css_text_indent_big_outdent};
			listView.add(new ListOption(mOwner, getString(R.string.options_css_text_indent), prefix + ".text-indent").add(identOptions, identOptionNames).setIconId(R.drawable.cr3_option_text_indent));

			ArrayList<String> faces = new ArrayList<String>(); 
			ArrayList<String> faceValues = new ArrayList<String>(); 
		    faces.add("-");
		    faceValues.add("");
		    faces.add(getString(R.string.options_css_font_face_sans_serif));
		    faceValues.add("font-family: sans-serif");
		    faces.add(getString(R.string.options_css_font_face_serif));
		    faceValues.add("font-family: serif");
		    faces.add(getString(R.string.options_css_font_face_monospace));
		    faceValues.add("font-family: \"Courier New\", \"Courier\", monospace");
		    for (String face : mFontFaces) {
			    faces.add(face);
			    faceValues.add("font-family: " + face);
		    }
			listView.add(new ListOption(mOwner, getString(R.string.options_css_font_face), prefix + ".font-face").add(faceValues.toArray(new String[]{}), faces.toArray(new String[]{})).setIconId(R.drawable.cr3_option_font_face));
			
		    String[] fontSizeStyles = {
		        "", // inherited
		        "font-size: 110%",
		        "font-size: 120%",
		        "font-size: 150%",
		        "font-size: 90%",
		        "font-size: 80%",
		        "font-size: 70%",
		        "font-size: 60%",
		    };
		    int[] fontSizeStyleNames = {
			    R.string.options_css_inherited,
			    R.string.options_css_font_size_110p,
			    R.string.options_css_font_size_120p,
			    R.string.options_css_font_size_150p,
			    R.string.options_css_font_size_90p,
			    R.string.options_css_font_size_80p,
			    R.string.options_css_font_size_70p,
			    R.string.options_css_font_size_60p,
		    };
			listView.add(new ListOption(mOwner, getString(R.string.options_css_font_size), prefix + ".font-size").add(fontSizeStyles, fontSizeStyleNames).setIconId(R.drawable.cr3_option_font_size));

		    String[] fontWeightStyles = {
		        "", // inherited
		        "font-weight: normal",
		        "font-weight: bold",
		        "font-weight: bolder",
		        "font-weight: lighter",
		    };
		    int[] fontWeightStyleNames = {
		        R.string.options_css_inherited,
		        R.string.options_css_font_weight_normal,
		        R.string.options_css_font_weight_bold,
		        R.string.options_css_font_weight_bolder,
		        R.string.options_css_font_weight_lighter,
		    };
			listView.add(new ListOption(mOwner, getString(R.string.options_css_font_weight), prefix + ".font-weight").add(fontWeightStyles, fontWeightStyleNames).setIconId(R.drawable.cr3_option_text_bold));

		    String[] fontStyleStyles = {
		        "", // inherited
		        "font-style: normal",
		        "font-style: italic",
		    };
		    int[] fontStyleStyleNames = {
		    	R.string.options_css_inherited,
		    	R.string.options_css_font_style_normal,
		    	R.string.options_css_font_style_italic,
		    };
			listView.add(new ListOption(mOwner, getString(R.string.options_css_font_style), prefix + ".font-style").add(fontStyleStyles, fontStyleStyleNames).setIconId(R.drawable.cr3_option_text_italic));

		    String[] lineHeightStyles = {
			        "", // inherited
			        "line-height: 75%",
			        "line-height: 80%",
			        "line-height: 85%",
			        "line-height: 90%",
			        "line-height: 95%",
			        "line-height: 100%",
			        "line-height: 110%",
			        "line-height: 120%",
			        "line-height: 130%",
			        "line-height: 140%",
			        "line-height: 150%",
			    };
		    String[] lineHeightStyleNames = {
			        "-",
			        "75%",
			        "80%",
			        "85%",
			        "90%",
			        "95%",
			        "100%",
			        "110%",
			        "120%",
			        "130%",
			        "140%",
			        "150%",
			    };
			listView.add(new ListOption(mOwner, getString(R.string.options_css_interline_space), prefix + ".line-height").add(lineHeightStyles, lineHeightStyleNames).setIconId(R.drawable.cr3_option_line_spacing));

		    String[] textDecorationStyles = {
		    		"", // inherited
		            "text-decoration: none",
		            "text-decoration: underline",
		            "text-decoration: line-through",
		            "text-decoration: overline",
			    };
		    int[] textDecorationStyleNames = {
			    	R.string.options_css_inherited,
			    	R.string.options_css_text_decoration_none,
			    	R.string.options_css_text_decoration_underline,
			    	R.string.options_css_text_decoration_line_through,
			    	R.string.options_css_text_decoration_overlineline,
			    };
			listView.add(new ListOption(mOwner, getString(R.string.options_css_font_decoration), prefix + ".text-decoration").add(textDecorationStyles, textDecorationStyleNames).setIconId(R.drawable.cr3_option_text_underline));

		    String[] verticalAlignStyles = {
		    		"", // inherited
		            "vertical-align: baseline",
		            "vertical-align: sub",
		            "vertical-align: super",
			    };
		    int[] verticalAlignStyleNames = {
			    	R.string.options_css_inherited,
			    	R.string.options_css_text_valign_baseline,
			    	R.string.options_css_text_valign_subscript,
			    	R.string.options_css_text_valign_superscript,
			    };
			listView.add(new ListOption(mOwner, getString(R.string.options_css_text_valign), prefix + ".vertical-align").add(verticalAlignStyles, verticalAlignStyleNames).setIconId(R.drawable.cr3_option_text_superscript));

		    String[] fontColorStyles = {
		        "", // inherited
		        "color: black",
		        "color: green",
		        "color: silver",
		        "color: lime",
		        "color: gray",
		        "color: olive",
		        "color: white",
		        "color: yellow",
		        "color: maroon",
		        "color: navy",
		        "color: red",
		        "color: blue",
		        "color: purple",
		        "color: teal",
		        "color: fuchsia",
		        "color: aqua",
		    };
		    String[] fontColorStyleNames = {
		        "-",
		        "Black",
		        "Green",
		        "Silver",
		        "Lime",
		        "Gray",
		        "Olive",
		        "White",
		        "Yellow",
		        "Maroon",
		        "Navy",
		        "Red",
		        "Blue",
		        "Purple",
		        "Teal",
		        "Fuchsia",
		        "Aqua",
		    };
			listView.add(new ListOption(mOwner, getString(R.string.options_css_text_color), prefix + ".color").add(fontColorStyles, fontColorStyleNames).setIconId(R.drawable.cr3_option_font_color));
			
			String[] marginTopOptions = {"", // inherited
			        "margin-top: 0em",
			        "margin-top: 0.2em",
			        "margin-top: 0.3em",
			        "margin-top: 0.5em",
			        "margin-top: 1em",
			        "margin-top: 2em"};
			String[] marginBottomOptions = {"", // inherited
			        "margin-bottom: 0em",
			        "margin-bottom: 0.2em",
			        "margin-bottom: 0.3em",
			        "margin-bottom: 0.5em",
			        "margin-bottom: 1em",
			        "margin-bottom: 2em"};
			int[] marginTopBottomOptionNames = {
			    	R.string.options_css_inherited,
			    	R.string.options_css_margin_0,
			    	R.string.options_css_margin_02em,
			    	R.string.options_css_margin_03em,
			    	R.string.options_css_margin_05em,
			    	R.string.options_css_margin_1em,
			    	R.string.options_css_margin_15em,
			        };
			String[] marginLeftOptions = {
					"", // inherited
			        "margin-left: 0em",
			        "margin-left: 0.5em",
			        "margin-left: 1em",
			        "margin-left: 1.5em",
			        "margin-left: 2em",
			        "margin-left: 4em",
			        "margin-left: 5%",
			        "margin-left: 10%",
			        "margin-left: 15%",
			        "margin-left: 20%",
			        "margin-left: 30%"};
			String[] marginRightOptions = {
					"", // inherited
			        "margin-right: 0em",
			        "margin-right: 0.5em",
			        "margin-right: 1em",
			        "margin-right: 1.5em",
			        "margin-right: 2em",
			        "margin-right: 4em",
			        "margin-right: 5%",
			        "margin-right: 10%",
			        "margin-right: 15%",
			        "margin-right: 20%",
			        "margin-right: 30%"};
			int[] marginLeftRightOptionNames = {
			    	R.string.options_css_inherited,
			    	R.string.options_css_margin_0,
			    	R.string.options_css_margin_05em,
			    	R.string.options_css_margin_1em,
			    	R.string.options_css_margin_15em,
			    	R.string.options_css_margin_2em,
			    	R.string.options_css_margin_4em,
			    	R.string.options_css_margin_5p,
			    	R.string.options_css_margin_10p,
			    	R.string.options_css_margin_15p,
			    	R.string.options_css_margin_20p,
			    	R.string.options_css_margin_30p,
			};
			listView.add(new ListOption(mOwner, getString(R.string.options_css_margin_top), prefix + ".margin-top").add(marginTopOptions, marginTopBottomOptionNames).setIconId(R.drawable.cr3_option_text_margin_top));
			listView.add(new ListOption(mOwner, getString(R.string.options_css_margin_bottom), prefix + ".margin-bottom").add(marginBottomOptions, marginTopBottomOptionNames).setIconId(R.drawable.cr3_option_text_margin_bottom));
			listView.add(new ListOption(mOwner, getString(R.string.options_css_margin_left), prefix + ".margin-left").add(marginLeftOptions, marginLeftRightOptionNames).setIconId(R.drawable.cr3_option_text_margin_left));
			listView.add(new ListOption(mOwner, getString(R.string.options_css_margin_right), prefix + ".margin-right").add(marginRightOptions, marginLeftRightOptionNames).setIconId(R.drawable.cr3_option_text_margin_right));
			

			dlg.setTitle(label);
			dlg.setView(listView);
			dlg.show();
		}

		public String getValueLabel() { return ">"; }
	}
	
	
	private ListOption createStyleEditor(String styleCode, int titleId) {
		ListOption res = new StyleEditorOption(this, getString(titleId), "styles." + styleCode);
		res.noIcon();
		return res;
	}

	final static private String[] styleCodes = {
		"def",
		"title",
		"subtitle",
		"pre",
		"link",
		"cite",
		"epigraph",
		"poem",
		"text-author",
		"footnote",
		"footnote-link",
		"footnote-title",
		"annotation",
	};
	
	final static private int[] styleTitles = {
		R.string.options_css_def,
		R.string.options_css_title,
		R.string.options_css_subtitle,
		R.string.options_css_pre,
		R.string.options_css_link,
		R.string.options_css_cite,
		R.string.options_css_epigraph,
		R.string.options_css_poem,
		R.string.options_css_textauthor,
		R.string.options_css_footnote,
		R.string.options_css_footnotelink,
		R.string.options_css_footnotetitle,
		R.string.options_css_annotation,
	};
	
	private void fillStyleEditorOptions() {
		mOptionsCSS = new OptionsListView(getContext());
		//mProperties.setBool(PROP_TXT_OPTION_PREFORMATTED, mReaderView.isTextAutoformatEnabled());
		//mProperties.setBool(PROP_EMBEDDED_STYLES, mReaderView.getDocumentStylesEnabled());
		mOptionsCSS.add(new BoolOption(this, getString(R.string.mi_book_styles_enable), PROP_EMBEDDED_STYLES).setDefaultValue("1").noIcon());
		if (isEpubFormat) {
			mOptionsCSS.add(new BoolOption(this, getString(R.string.options_font_embedded_document_font_enabled), PROP_EMBEDDED_FONTS).setDefaultValue("1").noIcon());
		}
		if (isTextFormat) {
			mOptionsCSS.add(new BoolOption(this, getString(R.string.mi_text_autoformat_enable), PROP_TXT_OPTION_PREFORMATTED).setDefaultValue("1").noIcon());
		}
		for (int i=0; i<styleCodes.length; i++)
			mOptionsCSS.add(createStyleEditor(styleCodes[i], styleTitles[i]));
	}
	
	private void setupBrowserOptions()
	{
        mInflater = LayoutInflater.from(getContext());
        ViewGroup view = (ViewGroup)mInflater.inflate(R.layout.options_browser, null);
        ViewGroup body = (ViewGroup)view.findViewById(R.id.body);
        
        
        mOptionsBrowser = new OptionsListView(getContext());

		final Properties properties = new Properties();
		properties.setProperty(ReaderView.PROP_APP_BOOK_SORT_ORDER, mActivity.settings().getProperty(ReaderView.PROP_APP_BOOK_SORT_ORDER));
		int[] sortOrderLabels = {
			FileInfo.SortOrder.FILENAME.resourceId,	
			FileInfo.SortOrder.FILENAME_DESC.resourceId,	
			FileInfo.SortOrder.AUTHOR_TITLE.resourceId,	
			FileInfo.SortOrder.AUTHOR_TITLE_DESC.resourceId,	
			FileInfo.SortOrder.TITLE_AUTHOR.resourceId,	
			FileInfo.SortOrder.TITLE_AUTHOR_DESC.resourceId,	
			FileInfo.SortOrder.TIMESTAMP.resourceId,	
			FileInfo.SortOrder.TIMESTAMP_DESC.resourceId,	
		};
		String[] sortOrderValues = {
			FileInfo.SortOrder.FILENAME.name(),	
			FileInfo.SortOrder.FILENAME_DESC.name(),	
			FileInfo.SortOrder.AUTHOR_TITLE.name(),	
			FileInfo.SortOrder.AUTHOR_TITLE_DESC.name(),	
			FileInfo.SortOrder.TITLE_AUTHOR.name(),	
			FileInfo.SortOrder.TITLE_AUTHOR_DESC.name(),	
			FileInfo.SortOrder.TIMESTAMP.name(),	
			FileInfo.SortOrder.TIMESTAMP_DESC.name(),	
		};
		mOptionsBrowser.add(new ListOption(this, getString(R.string.mi_book_sort_order), PROP_APP_BOOK_SORT_ORDER).add(sortOrderValues, sortOrderLabels).setDefaultValue(FileInfo.SortOrder.TITLE_AUTHOR.name()).noIcon());
		mOptionsBrowser.add(new BoolOption(this, getString(R.string.mi_book_browser_simple_mode), PROP_APP_FILE_BROWSER_SIMPLE_MODE).noIcon());
		mOptionsBrowser.add(new BoolOption(this, getString(R.string.options_app_show_cover_pages), PROP_APP_SHOW_COVERPAGES).noIcon());
		mOptionsBrowser.add(new ListOption(this, getString(R.string.options_app_cover_page_size), PROP_APP_COVERPAGE_SIZE).add(mCoverPageSizes, mCoverPageSizeTitles).setDefaultValue("1").noIcon());
		mOptionsBrowser.add(new BoolOption(this, getString(R.string.options_app_scan_book_props), PROP_APP_BOOK_PROPERTY_SCAN_ENABLED).setDefaultValue("1").noIcon());
		mOptionsBrowser.add(new BoolOption(this, getString(R.string.options_app_browser_hide_empty_dirs), PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS).setDefaultValue("0").noIcon());
		mOptionsBrowser.add(new ListOption(this, getString(R.string.options_app_backlight_screen), PROP_APP_SCREEN_BACKLIGHT).add(mBacklightLevels, mBacklightLevelsTitles).setDefaultValue("-1").noIcon());
		mOptionsBrowser.add(new LangOption(this).noIcon());
		mOptionsBrowser.add(new PluginsOption(this, getString(R.string.options_app_plugins)).noIcon());
		mOptionsBrowser.add(new BoolOption(this, getString(R.string.options_app_fullscreen), PROP_APP_FULLSCREEN).setIconId(R.drawable.cr3_option_fullscreen));
		if ( !DeviceInfo.EINK_SCREEN ) {
			mOptionsBrowser.add(new NightModeOption(this, getString(R.string.options_inverse_view), PROP_NIGHT_MODE).setIconId(R.drawable.cr3_option_night));
		}
		if ( !DeviceInfo.FORCE_LIGHT_THEME ) {
			mOptionsBrowser.add(new ThemeOptions(this, getString(R.string.options_app_ui_theme)).noIcon());
		}
		mOptionsBrowser.refresh();
		
		body.addView(mOptionsBrowser);
		setView(view);
	}
	private void setupReaderOptions()
	{
        mInflater = LayoutInflater.from(getContext());
        mTabs = (TabHost)mInflater.inflate(R.layout.options, null);
		// setup tabs
		//setView(R.layout.options);
		//setContentView(R.layout.options);
		//mTabs = (TabHost)findViewById(android.R.id.tabhost); 
		mTabs.setup();
		
		TabWidget tabWidget = (TabWidget)mTabs.findViewById(android.R.id.tabs);
		//tabWidget.
		//new TabHost(getContext());
		
		mOptionsStyles = new OptionsListView(getContext());
		mOptionsStyles.add(new ListOption(this, getString(R.string.options_font_face), PROP_FONT_FACE).add(mFontFaces).setDefaultValue(mFontFaces[0]).setIconId(R.drawable.cr3_option_font_face));
		mOptionsStyles.add(new ListOption(this, getString(R.string.options_font_size), PROP_FONT_SIZE).add(mFontSizes).setDefaultValue("24").setIconId(R.drawable.cr3_option_font_size));
		mOptionsStyles.add(new BoolOption(this, getString(R.string.options_font_embolden), PROP_FONT_WEIGHT_EMBOLDEN).setDefaultValue("0").setIconId(R.drawable.cr3_option_text_bold));
		//mOptionsStyles.add(new BoolOption(getString(R.string.options_font_antialias), PROP_FONT_ANTIALIASING).setInverse().setDefaultValue("0"));
		mOptionsStyles.add(new ListOption(this, getString(R.string.options_font_antialias), PROP_FONT_ANTIALIASING).add(mAntialias, mAntialiasTitles).setDefaultValue("2").setIconId(R.drawable.cr3_option_text_antialias));
		mOptionsStyles.add(new ListOption(this, getString(R.string.options_interline_space), PROP_INTERLINE_SPACE).addPercents(mInterlineSpaces).setDefaultValue("100").setIconId(R.drawable.cr3_option_line_spacing));
		//
		mOptionsStyles.add(new HyphenationOptions(this, getString(R.string.options_hyphenation_dictionary)).setIconId(R.drawable.cr3_option_text_hyphenation));
		mOptionsStyles.add(new BoolOption(this, getString(R.string.options_style_floating_punctuation), PROP_FLOATING_PUNCTUATION).setDefaultValue("1").setIconId(R.drawable.cr3_option_text_other));
		mOptionsStyles.add(new BoolOption(this, getString(R.string.options_font_kerning), PROP_FONT_KERNING_ENABLED).setDefaultValue("0").setIconId(R.drawable.cr3_option_text_kerning));
		mOptionsStyles.add(new ImageScalingOption(this, getString(R.string.options_format_image_scaling)).setIconId(R.drawable.cr3_option_images));
		mOptionsStyles.add(new ListOption(this, getString(R.string.options_render_font_gamma), PROP_FONT_GAMMA).add(mGammas).setDefaultValue("1.0").setIconId(R.drawable.cr3_option_font_gamma));
		mOptionsStyles.add(new ListOption(this, getString(R.string.options_format_min_space_width_percent), PROP_FORMAT_MIN_SPACE_CONDENSING_PERCENT).addPercents(mMinSpaceWidths).setDefaultValue("50").setIconId(R.drawable.cr3_option_text_width));
		mOptionsStyles.add(new ListOption(this, getString(R.string.options_font_hinting), PROP_FONT_HINTING).add(mHinting, mHintingTitles).setDefaultValue("2").noIcon());
		mOptionsStyles.add(new ListOption(this, getString(R.string.options_font_fallback_face), PROP_FALLBACK_FONT_FACE).add(mFontFaces).setDefaultValue(mFontFaces[0]).setIconId(R.drawable.cr3_option_font_face));
		
		//
		mOptionsPage = new OptionsListView(getContext());
		mOptionsPage.add(new BoolOption(this, getString(R.string.options_app_fullscreen), PROP_APP_FULLSCREEN).setIconId(R.drawable.cr3_option_fullscreen));
		mOptionsPage.add(new ListOption(this, getString(R.string.options_view_toolbar_position), PROP_TOOLBAR_LOCATION).add(mToolbarPositions, mToolbarPositionsTitles).setDefaultValue("1"));
		mOptionsPage.add(new BoolOption(this, getString(R.string.options_view_toolbar_hide_in_fullscreen), PROP_TOOLBAR_HIDE_IN_FULLSCREEN).setDefaultValue("0"));
		mOptionsPage.add(new ListOption(this, getString(R.string.options_view_mode), PROP_PAGE_VIEW_MODE).add(mViewModes, mViewModeTitles).setDefaultValue("1").setIconId(R.drawable.cr3_option_view_mode_scroll));
		//mOptionsPage.add(new ListOption(getString(R.string.options_page_orientation), PROP_ROTATE_ANGLE).add(mOrientations, mOrientationsTitles).setDefaultValue("0"));
		if (DeviceInfo.getSDKLevel() >= 9)
			mOptionsPage.add(new ListOption(this, getString(R.string.options_page_orientation), PROP_APP_SCREEN_ORIENTATION).add(mOrientations_API9, mOrientationsTitles_API9).setDefaultValue("0").setIconId(R.drawable.cr3_option_page_orientation_landscape));
		else
			mOptionsPage.add(new ListOption(this, getString(R.string.options_page_orientation), PROP_APP_SCREEN_ORIENTATION).add(mOrientations, mOrientationsTitles).setDefaultValue("0").setIconId(R.drawable.cr3_option_page_orientation_landscape));
		mOptionsPage.add(new ListOption(this, getString(R.string.options_page_landscape_pages), PROP_LANDSCAPE_PAGES).add(mLandscapePages, mLandscapePagesTitles).setDefaultValue("1").setIconId(R.drawable.cr3_option_pages_two));
		mOptionsPage.add(new NightModeOption(this, getString(R.string.options_inverse_view), PROP_NIGHT_MODE).setIconId(R.drawable.cr3_option_night));
		mOptionsPage.add(new ColorOption(this, getString(R.string.options_color_text), PROP_FONT_COLOR, 0x000000).setIconId(R.drawable.cr3_option_font_color));
		mOptionsPage.add(new ColorOption(this, getString(R.string.options_color_background), PROP_BACKGROUND_COLOR, 0xFFFFFF).setIconId(R.drawable.cr3_option_background_color));
		if ( !DeviceInfo.EINK_SCREEN )
			mOptionsPage.add(new TextureOptions(this, getString(R.string.options_background_texture)).setIconId(R.drawable.cr3_option_background_image));
		if ( DeviceInfo.EINK_SCREEN_UPDATE_MODES_SUPPORTED ) {
			mOptionsPage.add(new ListOption(this, getString(R.string.options_screen_update_mode), PROP_APP_SCREEN_UPDATE_MODE).add(mScreenUpdateModes, mScreenUpdateModesTitles).setDefaultValue("0"));
			mOptionsPage.add(new ListOption(this, getString(R.string.options_screen_update_interval), PROP_APP_SCREEN_UPDATE_INTERVAL).add(mScreenFullUpdateInterval).setDefaultValue("10"));
		}

		mOptionsPage.add(new StatusBarOption(this, getString(R.string.options_page_titlebar)));
		mOptionsPage.add(new BoolOption(this, getString(R.string.options_page_footnotes), PROP_FOOTNOTES).setDefaultValue("1"));
		if ( !DeviceInfo.EINK_SCREEN )
			mOptionsPage.add(new ListOption(this, getString(R.string.options_page_animation), PROP_PAGE_ANIMATION).add(mAnimation, mAnimationTitles).setDefaultValue("1").noIcon());
		mOptionsPage.add(new ListOption(this, getString(R.string.options_view_bookmarks_highlight), PROP_APP_HIGHLIGHT_BOOKMARKS).add(mHighlightMode, mHighlightModeTitles).setDefaultValue("1").noIcon());
		if ( !DeviceInfo.EINK_SCREEN ) {
			mOptionsPage.add(new ColorOption(this, getString(R.string.options_view_color_selection), PROP_HIGHLIGHT_SELECTION_COLOR, 0xCCCCCC).noIcon());
			mOptionsPage.add(new ColorOption(this, getString(R.string.options_view_color_bookmark_comment), PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT, 0xFFFF40).noIcon());
			mOptionsPage.add(new ColorOption(this, getString(R.string.options_view_color_bookmark_correction), PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION, 0xFF8000).noIcon());
		}

		mOptionsPage.add(new ListOption(this, getString(R.string.options_page_margin_left), PROP_PAGE_MARGIN_LEFT).add(mMargins).setDefaultValue("5").setIconId(R.drawable.cr3_option_text_margin_left));
		mOptionsPage.add(new ListOption(this, getString(R.string.options_page_margin_right), PROP_PAGE_MARGIN_RIGHT).add(mMargins).setDefaultValue("5").setIconId(R.drawable.cr3_option_text_margin_right));
		mOptionsPage.add(new ListOption(this, getString(R.string.options_page_margin_top), PROP_PAGE_MARGIN_TOP).add(mMargins).setDefaultValue("5").setIconId(R.drawable.cr3_option_text_margin_top));
		mOptionsPage.add(new ListOption(this, getString(R.string.options_page_margin_bottom), PROP_PAGE_MARGIN_BOTTOM).add(mMargins).setDefaultValue("5").setIconId(R.drawable.cr3_option_text_margin_bottom));
		
		mOptionsControls = new OptionsListView(getContext());
		mOptionsControls.add(new KeyMapOption(this, getString(R.string.options_app_key_actions)).setIconId(R.drawable.cr3_option_controls_keys));
		mOptionsControls.add(new TapZoneOption(this, getString(R.string.options_app_tapzones_normal), PROP_APP_TAP_ZONE_ACTIONS_TAP).setIconId(R.drawable.cr3_option_controls_tapzones));
		mOptionsControls.add(new ListOption(this, getString(R.string.options_controls_tap_secondary_action_type), PROP_APP_SECONDARY_TAP_ACTION_TYPE).add(mTapSecondaryActionType, mTapSecondaryActionTypeTitles).setDefaultValue(String.valueOf(TAP_ACTION_TYPE_LONGPRESS)));
		mOptionsControls.add(new BoolOption(this, getString(R.string.options_app_double_tap_selection), PROP_APP_DOUBLE_TAP_SELECTION).setDefaultValue("0").setIconId(R.drawable.cr3_option_touch));
		if ( !DeviceInfo.EINK_SCREEN )
			mOptionsControls.add(new BoolOption(this, getString(R.string.options_controls_enable_volume_keys), PROP_CONTROLS_ENABLE_VOLUME_KEYS).setDefaultValue("1"));
		mOptionsControls.add(new BoolOption(this, getString(R.string.options_app_tapzone_hilite), PROP_APP_TAP_ZONE_HILIGHT).setDefaultValue("0").setIconId(R.drawable.cr3_option_touch));
		if ( !DeviceInfo.EINK_SCREEN )
			mOptionsControls.add(new BoolOption(this, getString(R.string.options_app_trackball_disable), PROP_APP_TRACKBALL_DISABLED).setDefaultValue("0"));
		if ( !DeviceInfo.EINK_SCREEN )
			mOptionsControls.add(new ListOption(this, getString(R.string.options_controls_flick_brightness), PROP_APP_FLICK_BACKLIGHT_CONTROL).add(mFlickBrightness, mFlickBrightnessTitles).setDefaultValue("1"));
		mOptionsControls.add(new BoolOption(this, getString(R.string.option_controls_gesture_page_flipping_enabled), PROP_APP_GESTURE_PAGE_FLIPPING).setDefaultValue("1"));
		mOptionsControls.add(new ListOption(this, getString(R.string.options_selection_action), PROP_APP_SELECTION_ACTION).add(mSelectionAction, mSelectionActionTitles).setDefaultValue("0"));
		mOptionsControls.add(new ListOption(this, getString(R.string.options_multi_selection_action), PROP_APP_MULTI_SELECTION_ACTION).add(mMultiSelectionAction, mMultiSelectionActionTitles).setDefaultValue("0"));
		mOptionsControls.add(new BoolOption(this, getString(R.string.options_selection_keep_selection_after_dictionary), PROP_APP_SELECTION_PERSIST).setDefaultValue("0"));
		
		mOptionsApplication = new OptionsListView(getContext());
		mOptionsApplication.add(new LangOption(this).noIcon());
		if ( !DeviceInfo.FORCE_LIGHT_THEME ) {
			mOptionsApplication.add(new ThemeOptions(this, getString(R.string.options_app_ui_theme)).noIcon());
		}
		if ( !DeviceInfo.EINK_SCREEN ) {
			mOptionsApplication.add(new ListOption(this, getString(R.string.options_app_backlight_timeout), PROP_APP_SCREEN_BACKLIGHT_LOCK).add(mBacklightTimeout, mBacklightTimeoutTitles).setDefaultValue("3").noIcon());
			mBacklightLevelsTitles[0] = getString(R.string.options_app_backlight_screen_default);
			mOptionsApplication.add(new ListOption(this, getString(R.string.options_app_backlight_screen), PROP_APP_SCREEN_BACKLIGHT).add(mBacklightLevels, mBacklightLevelsTitles).setDefaultValue("-1").noIcon());
		}
		mOptionsApplication.add(new BoolOption(this, getString(R.string.options_app_key_backlight_off), PROP_APP_KEY_BACKLIGHT_OFF).setDefaultValue("1").noIcon());
		mOptionsApplication.add(new IconsBoolOption(this, getString(R.string.options_app_settings_icons), PROP_APP_SETTINGS_SHOW_ICONS).setDefaultValue("1").noIcon());
		mOptionsApplication.add(new DictOptions(this, getString(R.string.options_app_dictionary)).noIcon());
		mOptionsApplication.add(new BoolOption(this, getString(R.string.options_app_show_cover_pages), PROP_APP_SHOW_COVERPAGES).noIcon());
		mOptionsApplication.add(new ListOption(this, getString(R.string.options_app_cover_page_size), PROP_APP_COVERPAGE_SIZE).add(mCoverPageSizes, mCoverPageSizeTitles).setDefaultValue("1").noIcon());
		mOptionsApplication.add(new BoolOption(this, getString(R.string.options_app_scan_book_props), PROP_APP_BOOK_PROPERTY_SCAN_ENABLED).setDefaultValue("1").noIcon());
		mOptionsApplication.add(new BoolOption(this, getString(R.string.options_app_browser_hide_empty_dirs), PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS).setDefaultValue("0").noIcon());
		mOptionsApplication.add(new BoolOption(this, getString(R.string.mi_book_browser_simple_mode), PROP_APP_FILE_BROWSER_SIMPLE_MODE).noIcon());
		
		fillStyleEditorOptions();
		
		mOptionsStyles.refresh();
		mOptionsCSS.refresh();
		mOptionsPage.refresh();
		mOptionsApplication.refresh();
		
		addTab("Styles", R.drawable.cr3_tab_style, R.string.tab_options_styles);
		addTab("CSS", R.drawable.cr3_tab_css, R.string.tab_options_css);
		addTab("Page", R.drawable.cr3_tab_page, R.string.tab_options_page);
		addTab("Controls", R.drawable.cr3_tab_controls, R.string.tab_options_controls);
		addTab("App", R.drawable.cr3_tab_application, R.string.tab_options_app);
		
		setView(mTabs);
	}
	
	private void addTab(String name, int imageDrawable, int contentDescription) {
		TabHost.TabSpec ts = mTabs.newTabSpec(name);
		Drawable icon = getContext().getResources().getDrawable(imageDrawable);
		
		// temporary rollback ImageButton tabs: no highlight for current tab in this implementation
//		if (true) {
			ts.setIndicator("", icon);
//		} else {
//			// ACCESSIBILITY: we need to specify contentDescription
//			ImageButton ib = new ImageButton(getContext());
//			ib.setImageDrawable(icon);
//			ib.setBackgroundResource(R.drawable.cr3_toolbar_button_background);
//			Utils.setContentDescription(ib, getContext().getResources().getString(contentDescription));
//			ts.setIndicator(ib);
//		}
		
		ts.setContent(this);
		mTabs.addTab(ts);
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		L.v("creating OptionsDialog");
		CoolReader.dumpHeapAllocation();
		L.v("calling gc");
		System.gc();
		CoolReader.dumpHeapAllocation();
		L.v("creating options dialog");
        setCancelable(true);
        setCanceledOnTouchOutside(true);
		
        if (mode == Mode.READER)
        	setupReaderOptions();
        else if (mode == Mode.BROWSER)
        	setupBrowserOptions();
        
		setOnCancelListener(new OnCancelListener() {

			public void onCancel(DialogInterface dialog) {
				onPositiveButtonClick();
			}
		});

		ImageButton positiveButton = (ImageButton)view.findViewById(R.id.options_btn_back);
		positiveButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				onPositiveButtonClick();
			}
		});
		
//		ImageButton negativeButton = (ImageButton)mTabs.findViewById(R.id.options_btn_cancel);
//		negativeButton.setOnClickListener(new View.OnClickListener() {
//			public void onClick(View v) {
//				onNegativeButtonClick();
//			}
//		});
		
		super.onCreate(savedInstanceState);
		L.v("OptionsDialog is created");
	}

//	private void askApply()
//	{
//		Properties diff = mProperties.diff(mOldProperties);
//		if ( diff.size()>0 ) {
//			L.d("Some properties were changed, ask user whether to apply");
//			AlertDialog.Builder dlg = new AlertDialog.Builder(getContext());
//			dlg.setTitle(R.string.win_title_options_apply);
//			dlg.setPositiveButton(R.string.dlg_button_ok, new OnClickListener() {
//				public void onClick(DialogInterface arg0, int arg1) {
//					onPositiveButtonClick();
//				}
//			});
//			dlg.setNegativeButton(R.string.dlg_button_cancel, new OnClickListener() {
//				public void onClick(DialogInterface arg0, int arg1) {
//					onNegativeButtonClick();
//				}
//			});
//			dlg.show();
//		}
//	}
	
	protected void apply() {
		if (mode == Mode.READER) {
			if (mProperties.getBool(PROP_TXT_OPTION_PREFORMATTED, true) != mReaderView.isTextAutoformatEnabled()) {
				mReaderView.toggleTextFormat();
			}
			if (mProperties.getBool(PROP_EMBEDDED_STYLES, true) != mReaderView.getDocumentStylesEnabled()) {
				mReaderView.toggleDocumentStyles();
			}
			if (mProperties.getBool(PROP_EMBEDDED_FONTS, true) != mReaderView.getDocumentFontsEnabled()) {
				mReaderView.toggleEmbeddedFonts();
			}
		}
		mActivity.setSettings(mProperties, 0, true);
        //mReaderView.setSettings(mProperties, mOldProperties);
	}
	
	@Override
	protected void onPositiveButtonClick() {
		apply();
        dismiss();
	}

	@Override
	protected void onNegativeButtonClick() {
		onPositiveButtonClick();
	}

	@Override
	protected void onStop() {
		//L.d("OptionsDialog.onStop() : calling gc()");
		//System.gc();
		super.onStop();
	}

	@Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (mode == Mode.READER) {
	        if (((OptionsListView)mTabs.getCurrentView()).onKeyDown(keyCode, event))
	        	return true;
		} else {
	        if (view.onKeyDown(keyCode, event))
	        	return true;
		}
        return super.onKeyDown(keyCode, event);
    }	
}
