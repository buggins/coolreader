/*
 * CoolReader for Android
 * Copyright (C) 2010-2015 Vadim Lopatin <coolreader.org@gmail.com>
 * Copyright (C) 2011 a_lone
 * Copyright (C) 2012 Michael Berganovsky <mike0berg@gmail.com>
 * Copyright (C) 2012 Jeff Doozan <jeff@doozan.com>
 * Copyright (C) 2012 Daniel Savard <daniels@xsoli.com>
 * Copyright (C) 2012 Peter Vagner <pvdeejay@gmail.com>
 * Copyright (C) 2018 Yuri Plotnikov <plotnikovya@gmail.com>
 * Copyright (C) 2018 S-trace <S-trace@list.ru>
 * Copyright (C) 2021 ourairquality <info@ourairquality.org>
 * Copyright (C) 2018,2020-2022 Aleksey Chernov <valexlin@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.coolreader.crengine;

import android.annotation.TargetApi;
import android.content.Context;
import android.database.DataSetObserver;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Bundle;
import android.speech.tts.TextToSpeech;
import android.speech.tts.Voice;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.CompoundButton;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.RadioButton;
import android.widget.TabHost;
import android.widget.TabHost.TabContentFactory;
import android.widget.TextView;

import org.coolreader.CoolReader;
import org.coolreader.Dictionaries;
import org.coolreader.Dictionaries.DictInfo;
import org.coolreader.R;
import org.coolreader.plugins.OnlineStorePluginManager;
import org.coolreader.tts.OnTTSCreatedListener;
import org.coolreader.tts.TTSControlBinder;

import java.io.File;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;
import java.util.Locale;

public class OptionsDialog extends BaseDialog implements TabContentFactory, OptionOwner, Settings {

	ReaderView mReaderView;
	BaseActivity mActivity;
	String[] mFontFaces;

	TTSControlBinder mTTSBinder;

	/*
	int[] mFontSizes = new int[] {
		9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
		31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 42, 44, 48, 52, 56, 60, 64, 68, 72, 78, 84, 90, 110, 130, 150, 170, 200, 230, 260, 300, 340
	};
	int[] mStatusFontSizes = new int[] {
			9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 22, 24, 25, 26, 27, 28, 29, 30,
			31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 42, 44, 48, 52, 56, 60, 64, 68, 72, 78, 84, 90, 110, 130, 150, 170, 200, 230, 260, 300, 340
		};
		
	int[] filterFontSizes(int[] fontSizes) {
	    ArrayList<Integer> list = new ArrayList<>();
		for (int sz : fontSizes) {
			if (sz >= mActivity.getMinFontSize() && sz <= mActivity.getMaxFontSize())
				list.add(sz);
		}
	    int[] res = new int[list.size()];
	    for (int i = 0; i < list.size(); i++)
	        res[i] = list.get(i);
	    return res;
	}
	*/
	int[] mSynthWeights;
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
			-1,  1,  2,  3,  4,  5,  6,  7,  8,  9,
			10, 12, 15, 20, 25, 30, 35, 40, 45, 50,
			55, 60, 65, 70, 75, 80, 85, 90, 95, 100
	};
	public static final String[] mBacklightLevelsTitles = new String[] {
			"Default", "1%", "2%", "3%", "4%", "5%", "6%", "7%", "8%", "9%", 
			"10%", "12%", "15%", "20%", "25%", "30%", "35%", "40%", "45%", "50%",
			"55%", "60%", "65%", "70%", "75%", "80%", "85%", "90%", "95%", "100%",
	};
	public static int[] mMotionTimeouts;
	public static String[] mMotionTimeoutsTitles;
	public static int[] mPagesPerFullSwipe;
	public static String[] mPagesPerFullSwipeTitles;
	int[] mInterlineSpaces = new int[] {
			 80,  81,  82,  83,  84,  85, 86,   87,  88,  89,
			 90,  91,  92,  93,  94,  95, 96,   97,  98,  99,
			100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
			110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
			120, 125, 130, 135, 140, 145, 150, 155, 160, 165,
			170, 175, 180, 185, 190, 195, 200
		};
	int[] mMinSpaceWidths = new int[] {
			25, 30, 40, 50, 60, 70, 80, 90, 100
		};
	int[] mMargins = new int[] {
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 20, 25, 30, 40, 50, 60, 80, 100, 130, 150, 200, 300
		};
	int[] mRoundedCornersMargins = new int[] {
			0, 5, 10, 15, 20, 30, 40, 50, 60, 70,80, 90, 100, 120, 140, 160
	};
	double[] mGammas = new double[] {
			0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.5, 1.9
		};
	int[] mScreenFullUpdateInterval = new int[] {
			1, 2, 3, 4, 5, 7, 10, 15, 20
		};
	int[] mScreenUpdateModes = new int[] {
			EinkScreen.EinkUpdateMode.Clear.code, EinkScreen.EinkUpdateMode.Fast.code, EinkScreen.EinkUpdateMode.Active.code
		};
	int[] mScreenUpdateModesTitles = new int[] {
			R.string.options_screen_update_mode_quality, R.string.options_screen_update_mode_fast, R.string.options_screen_update_mode_fast2
		};
	int[] mOnyxScreenUpdateModes = new int[] {
			EinkScreen.EinkUpdateMode.Regal.code, EinkScreen.EinkUpdateMode.Clear.code, EinkScreen.EinkUpdateMode.Fast.code, EinkScreen.EinkUpdateMode.A2.code
	};
	int[] mOnyxScreenUpdateModesTitles = new int[] {
			R.string.options_screen_update_mode_onyx_regal, R.string.options_screen_update_mode_quality, R.string.options_screen_update_mode_fast, R.string.options_screen_update_mode_onyx_a2,
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
	int[] mShaping = new int[] {
			0, 1, 2
	};
	int[] mShapingTitles = new int[] {
			R.string.options_text_shaping_simple, R.string.options_text_shaping_light,
			R.string.options_text_shaping_full
	};
	int[] mOrientations = new int[] {
			0, 1, 4, 5
		};
	int[] mOrientationsTitles = new int[] {
			R.string.options_page_orientation_0, R.string.options_page_orientation_90, 
			R.string.options_page_orientation_sensor, R.string.options_page_orientation_system
		};
	int[] mOrientations_API9 = new int[] {
			0, 1, 2, 3, 4, 5
		};
	int[] mOrientationsTitles_API9 = new int[] {
			R.string.options_page_orientation_0, R.string.options_page_orientation_90, R.string.options_page_orientation_180, R.string.options_page_orientation_270
			,R.string.options_page_orientation_sensor,R.string.options_page_orientation_system
		};

	int[] mToolbarPositions = new int[] {
			Settings.VIEWER_TOOLBAR_NONE, Settings.VIEWER_TOOLBAR_TOP, Settings.VIEWER_TOOLBAR_BOTTOM, Settings.VIEWER_TOOLBAR_LEFT, Settings.VIEWER_TOOLBAR_RIGHT, Settings.VIEWER_TOOLBAR_SHORT_SIDE, Settings.VIEWER_TOOLBAR_LONG_SIDE
		};
	int[] mToolbarPositionsTitles = new int[] {
			R.string.options_view_toolbar_position_none, R.string.options_view_toolbar_position_top, R.string.options_view_toolbar_position_bottom, R.string.options_view_toolbar_position_left, R.string.options_view_toolbar_position_right, R.string.options_view_toolbar_position_short_side, R.string.options_view_toolbar_position_long_side
		};
	int[] mToolbarApperance = new int[] {
			Settings.VIEWER_TOOLBAR_100, Settings.VIEWER_TOOLBAR_100_gray,
			Settings.VIEWER_TOOLBAR_75, Settings.VIEWER_TOOLBAR_75_gray,
			Settings.VIEWER_TOOLBAR_50, Settings.VIEWER_TOOLBAR_50_gray
	};
	int[] mToolbarApperanceTitles = new int[] {
			R.string.options_view_toolbar_appear_100, R.string.options_view_toolbar_appear_100_gray,
			R.string.options_view_toolbar_appear_75, R.string.options_view_toolbar_appear_75_gray,
			R.string.options_view_toolbar_appear_50, R.string.options_view_toolbar_appear_50_gray
	};
	
	int[] mStatusPositions = new int[] {
			Settings.VIEWER_STATUS_NONE, 
			//Settings.VIEWER_STATUS_TOP, Settings.VIEWER_STATUS_BOTTOM, 
			Settings.VIEWER_STATUS_PAGE_HEADER,
			Settings.VIEWER_STATUS_PAGE_FOOTER
		};
	int[] mStatusPositionsTitles = new int[] {
			R.string.options_page_show_titlebar_hidden, 
			//R.string.options_page_show_titlebar_top, R.string.options_page_show_titlebar_bottom, 
			R.string.options_page_show_titlebar_page_header,
			R.string.options_page_show_titlebar_page_footer
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
	int[] mBounceProtectionValues = new int[] {
			-1,
			100,
			143,
			200,
			333,
	};
	int[] mBounceProtectionTitles = new int[] {
			R.string.options_controls_bonce_protection_disabled,
			R.string.options_controls_bonce_protection_100,
			R.string.options_controls_bonce_protection_143,
			R.string.options_controls_bonce_protection_200,
			R.string.options_controls_bonce_protection_333,
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
	// possible values see in crengine/include/lvfont.h: enum font_antialiasing_t
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
	int[] mRenderingPresets = new int[] {
			Engine.BLOCK_RENDERING_FLAGS_LEGACY, Engine.BLOCK_RENDERING_FLAGS_FLAT,
			Engine.BLOCK_RENDERING_FLAGS_BOOK, Engine.BLOCK_RENDERING_FLAGS_WEB
	};
	int[] mRenderingPresetsTitles = new int[] {
			R.string.options_rendering_preset_legacy, R.string.options_rendering_preset_flat, R.string.options_rendering_preset_book, R.string.options_rendering_preset_web
	};
	int[] mDOMVersionPresets = new int[] {
			0, Engine.DOM_VERSION_CURRENT
	};
	int[] mDOMVersionPresetTitles = new int[] {
			R.string.options_requested_dom_level_legacy, R.string.options_requested_dom_level_newest
	};
	int [] mGoogleDriveAutoSavePeriod = new int[] {
			0, 1, 2, 3, 4, 5, 10, 15, 20, 30
	};
	int [] mGoogleDriveAutoSavePeriodTitles = new int[] {
			R.string.autosave_period_off,
			R.string.autosave_period_1min,
			R.string.autosave_period_2min,
			R.string.autosave_period_3min,
			R.string.autosave_period_4min,
			R.string.autosave_period_5min,
			R.string.autosave_period_10min,
			R.string.autosave_period_15min,
			R.string.autosave_period_20min,
			R.string.autosave_period_30min
	};
	int [] mCloudBookmarksKeepAlive = new int [] {
			0, 1, 2, 3, 4, 5, 6,
			7, 14, 30, 91, 182, 365
	};
	int [] mCloudBookmarksKeepAliveTitles = new int [] {
			R.string.bookmarks_keepalive_off,
			R.string.bookmarks_keepalive_1day,
			R.string.bookmarks_keepalive_2days,
			R.string.bookmarks_keepalive_3days,
			R.string.bookmarks_keepalive_4days,
			R.string.bookmarks_keepalive_5days,
			R.string.bookmarks_keepalive_6days,
			R.string.bookmarks_keepalive_1week,
			R.string.bookmarks_keepalive_2weeks,
			R.string.bookmarks_keepalive_1month,
			R.string.bookmarks_keepalive_1quarter,
			R.string.bookmarks_keepalive_half_a_year,
			R.string.bookmarks_keepalive_1year
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
	OptionsListView mOptionsCloudSync = null;
	OptionsListView mOptionsTTS;
	// Mutable options
	OptionBase mHyphDictOption;
	OptionBase mEmbedFontsOptions;
	OptionBase mIgnoreDocMargins;
	OptionBase mFootNotesOption;
	OptionBase mEnableMultiLangOption;
	OptionBase mEnableHyphOption;
	OptionBase mGoogleDriveEnableSettingsOption;
	OptionBase mGoogleDriveEnableBookmarksOption;
	OptionBase mGoogleDriveEnableCurrentBookInfoOption;
	OptionBase mGoogleDriveEnableCurrentBookBodyOption;
	OptionBase mCloudSyncAskConfirmationsOption;
	OptionBase mGoogleDriveAutoSavePeriodOption;
	OptionBase mCloudSyncDataKeepAliveOptions;
	ListOption mTTSEngineOption;
	OptionBase mTTSUseDocLangOption;
	ListOption mTTSLanguageOption;
	ListOption mTTSVoiceOption;
	ListOption mFontWeightOption;
	OptionBase mFontHintingOption;
	OptionBase mBounceProtectionOption;
	ListOption mFlickBacklightControlOption;
	BoolOption mFlickBacklightTogetherOption;

	public final static int OPTION_VIEW_TYPE_NORMAL = 0;
	public final static int OPTION_VIEW_TYPE_BOOLEAN = 1;
	public final static int OPTION_VIEW_TYPE_COLOR = 2;
	public final static int OPTION_VIEW_TYPE_SUBMENU = 3;
	public final static int OPTION_VIEW_TYPE_NUMBER = 4;
	//public final static int OPTION_VIEW_TYPE_COUNT = 4;

	// This is an engine limitation, see lvfreetypefontman.cpp, lvfreetypeface.cpp
	private static final int MAX_FALLBACK_FONTS_COUNT = 32;

	public BaseActivity getActivity() { return mActivity; }
	public Properties getProperties() { return mProperties; }
	public LayoutInflater getInflater() { return mInflater; }

	public abstract static class OptionBase {
		protected View myView;
		Properties mProperties;
		BaseActivity mActivity;
		OptionOwner mOwner;
		LayoutInflater mInflater;
		public boolean enabled = true;
		public String label;
		public String property;
		public String defaultValue;
		public String disabledNote;
		public int drawableAttrId = R.attr.cr3_option_other_drawable;
		public int fallbackIconId = R.drawable.cr3_option_other;
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
			drawableAttrId = 0;
			fallbackIconId = id;
			return this;
		}
		public OptionBase setIconIdByAttr(int drawableAttrId, int fallbackIconId) {
			this.drawableAttrId = drawableAttrId;
			this.fallbackIconId = fallbackIconId;
			return this;
		}
		public OptionBase noIcon() {
			drawableAttrId = 0;
			fallbackIconId = 0;
			return this;
		}
		public OptionBase setDefaultValue(String value) {
			this.defaultValue = value;
			if ( mProperties.getProperty(property)==null )
				mProperties.setProperty(property, value);
			return this;
		}
		public OptionBase setDisabledNote(String note) {
			disabledNote = note;
			return this;
		}
		public OptionBase setOnChangeHandler( Runnable handler ) {
			onChangeHandler = handler;
			return this;
		}
		public void setEnabled(boolean enabled) {
			this.enabled = enabled;
			refreshItem();
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

		protected void setupIconView(ImageView icon) {
			if (null == icon)
				return;
			int resId = 0;
			if (showIcons) {
				if (drawableAttrId != 0) {
					resId = Utils.resolveResourceIdByAttr(mActivity, drawableAttrId, fallbackIconId);
				} else if (fallbackIconId != 0) {
					resId = fallbackIconId;
				}
			}
			if (resId != 0) {
				icon.setImageResource(resId);
				icon.setVisibility(View.VISIBLE);
			} else {
				icon.setImageResource(0);
				icon.setVisibility(View.INVISIBLE);
			}
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
			labelView.setEnabled(enabled);
			if (valueView != null) {
				String valueLabel = getValueLabel();
				if (!enabled && null != disabledNote && disabledNote.length() > 0) {
					if (null != valueLabel && valueLabel.length() > 0)
						valueLabel = valueLabel + " (" + disabledNote + ")";
					else
						valueLabel = disabledNote;
				}
				if (null != valueLabel && valueLabel.length() > 0) {
					valueView.setText(valueLabel);
					valueView.setVisibility(View.VISIBLE);
				} else {
					valueView.setText("");
					valueView.setVisibility(View.INVISIBLE);
				}
				valueView.setEnabled(enabled);
			}
			setupIconView((ImageView)view.findViewById(R.id.option_icon));
			return view;
		}

		public String getValueLabel() { return mProperties.getProperty(property); }
		public void onSelect() {
			if (!enabled)
				return;
			refreshList();
		}
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
			if (!enabled)
				return;
			ColorPickerDialog dlg = new ColorPickerDialog(mActivity, color -> {
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
				view = convertView;
			}
			myView = view;
			TextView labelView = view.findViewById(R.id.option_label);
			ImageView valueView = view.findViewById(R.id.option_value_color);
			labelView.setText(label);
			labelView.setEnabled(enabled);
			int cl = mProperties.getColor(property, defColor);
			valueView.setBackgroundColor(cl);
			setupIconView(view.findViewById(R.id.option_icon));
			view.setEnabled(enabled);
			return view;
		}
	}
	
	private static boolean showIcons = true;
	private static boolean isTextFormat = false;
	private static boolean isEpubFormat = false;
	private static boolean isFormatWithEmbeddedStyle = false;
	private static boolean isHtmlFormat = false;
	private Mode mode;
	
	class IconsBoolOption extends BoolOption {
		public IconsBoolOption( OptionOwner owner, String label, String property ) {
			super(owner, label, property);
		}
		public void onSelect() {
			if (!enabled)
				return;
			mProperties.setProperty(property, "1".equals(mProperties.getProperty(property)) ? "0" : "1");
			showIcons = mProperties.getBool(property, true);
			mOptionsStyles.refresh();
			mOptionsCSS.refresh();
			mOptionsPage.refresh();
			mOptionsApplication.refresh();
			mOptionsControls.refresh();
			if (null != mOptionsCloudSync) {
				mOptionsCloudSync.refresh();
			}
		}
	}
	class StyleBoolOption extends OptionBase {
		private String onValue;
		private String offValue;
		public StyleBoolOption( OptionOwner owner, String label, String property, String onValue, String offValue) {
			super(owner, label, property);
			this.onValue = onValue;
			this.offValue = offValue;
		}
		private boolean getValueBoolean() {
			return onValue.equals(mProperties.getProperty(property));
		}
		public OptionBase setDefaultValueBoolean(boolean value) {
			defaultValue = value ? onValue : offValue;
			if ( mProperties.getProperty(property)==null )
				mProperties.setProperty(property, defaultValue);
			return this;
		}
		public void onSelect() {
			if (!enabled)
				return;
			// Toggle the state
			mProperties.setProperty(property, getValueBoolean() ? offValue : onValue);
			refreshList();
		}
		public int getItemViewType() {
			return OPTION_VIEW_TYPE_BOOLEAN;
		}
		public View getView(View convertView, ViewGroup parent) {
			View view;
			convertView = myView;
			if ( convertView==null ) {
				view = mInflater.inflate(R.layout.option_item_boolean, null);
			} else {
				view = convertView;
			}
			myView = view;
			TextView labelView = view.findViewById(R.id.option_label);
			TextView commentView = view.findViewById(R.id.option_comment);
			CompoundButton valueView = view.findViewById(R.id.option_value_cb);
			labelView.setText(label);
			labelView.setEnabled(enabled);
			String commentLabel = null;
			if (!enabled && null != disabledNote && disabledNote.length() > 0) {
				commentLabel = disabledNote;
			}
			if (null != commentLabel) {
				commentView.setText(commentLabel);
				commentView.setEnabled(enabled);
				commentView.setVisibility(View.VISIBLE);
			} else {
				commentView.setVisibility(View.GONE);
			}
			valueView.setChecked(getValueBoolean());
			setupIconView((ImageView)view.findViewById(R.id.option_icon));
			valueView.setEnabled(enabled);
			return view;
		}
	}
	class BoolOption extends OptionBase {
		private boolean inverse = false;
		private String comment;
		public BoolOption( OptionOwner owner, String label, String property ) {
			super(owner, label, property);
		}
		private boolean getValueBoolean() { return "1".equals(mProperties.getProperty(property)) ^ inverse; }
		public String getValueLabel() { return getValueBoolean()  ? getString(R.string.options_value_on) : getString(R.string.options_value_off); }
		public void onSelect() {
			if (!enabled)
				return;
			mProperties.setProperty(property, "1".equals(mProperties.getProperty(property)) ? "0" : "1");
			if (null != onChangeHandler)
				onChangeHandler.run();
			refreshList();
		}
		public BoolOption setInverse() { inverse = true; return this; }
		public BoolOption setComment(String comment) { this.comment = comment; return this; }
		public int getItemViewType() {
			return OPTION_VIEW_TYPE_BOOLEAN;
		}
		public View getView(View convertView, ViewGroup parent) {
			View view;
			convertView = myView;
			if ( convertView==null ) {
				view = mInflater.inflate(R.layout.option_item_boolean, null);
			} else {
				view = convertView;
			}
			myView = view;
			TextView labelView = view.findViewById(R.id.option_label);
			TextView commentView = view.findViewById(R.id.option_comment);
			CompoundButton valueView = view.findViewById(R.id.option_value_cb);
			labelView.setText(label);
			labelView.setEnabled(enabled);
			String commentLabel = comment;
			if (!enabled && null != disabledNote && disabledNote.length() > 0) {
				if (null != commentLabel && commentLabel.length() > 0)
					commentLabel = commentLabel + " (" + disabledNote + ")";
				else
					commentLabel = disabledNote;
			}
			if (null != commentLabel) {
				commentView.setText(commentLabel);
				commentView.setEnabled(enabled);
				commentView.setVisibility(View.VISIBLE);
			} else {
				commentView.setVisibility(View.GONE);
			}
			valueView.setChecked(getValueBoolean());
			// For this view, the "focusable" and "clickable" properties are
			// disabled in the layout, so there is no need to set a change listener.
			//valueView.setOnCheckedChangeListener((arg0, checked) -> {
			//			mProperties.setBool(property, checked);
			//			refreshList();
			//});
			setupIconView((ImageView)view.findViewById(R.id.option_icon));
			valueView.setEnabled(enabled);
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
			if (!enabled)
				return;
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

	public enum KeyActionFlag {
		KEY_ACTION_FLAG_NORMAL,
		KEY_ACTION_FLAG_LONG,
		KEY_ACTION_FLAG_DOUBLE
	}
	class KeyMapOption extends SubmenuOption {
		public KeyMapOption( OptionOwner owner, String label ) {
			super(owner, label, PROP_APP_KEY_ACTIONS_PRESS);
		}
		private void addKey( OptionsListView list, int keyCode, String keyName) {
			addKey(list, keyCode, keyName, EnumSet.allOf(KeyActionFlag.class));
		}
		private void addKey( OptionsListView list, int keyCode, String keyName, EnumSet<KeyActionFlag> keyFlags ) {
			if (keyFlags.contains(KeyActionFlag.KEY_ACTION_FLAG_NORMAL)) {
				final String propName = ReaderAction.getKeyProp(keyCode, ReaderAction.NORMAL);
				list.add(new ActionOption(mOwner, keyName, propName, false, false));
			}
			if (keyFlags.contains(KeyActionFlag.KEY_ACTION_FLAG_LONG)) {
				final String longPropName = ReaderAction.getKeyProp(keyCode, ReaderAction.LONG);
				list.add(new ActionOption(mOwner, keyName + " " + getContext().getString(R.string.options_app_key_long_press), longPropName, false, true));
			}
			if (keyFlags.contains(KeyActionFlag.KEY_ACTION_FLAG_DOUBLE)) {
				final String dblPropName = ReaderAction.getKeyProp(keyCode, ReaderAction.DOUBLE);
				list.add(new ActionOption(mOwner, keyName + " " + getContext().getString(R.string.options_app_key_double_press), dblPropName, false, false));
			}
		}
		public void onSelect() {
			if (!enabled)
				return;
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
				EnumSet<KeyActionFlag> keyFlags;
				if (DeviceInfo.EINK_ONYX && DeviceInfo.ONYX_BUTTONS_LONG_PRESS_NOT_AVAILABLE) {
				    keyFlags = EnumSet.of(
				    		KeyActionFlag.KEY_ACTION_FLAG_NORMAL,
							KeyActionFlag.KEY_ACTION_FLAG_DOUBLE
					);
				} else {
					keyFlags = EnumSet.allOf(KeyActionFlag.class);
				}

				if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_MENU))
					addKey(listView, KeyEvent.KEYCODE_MENU, "Menu", keyFlags);
				if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_BACK))
					addKey(listView, KeyEvent.KEYCODE_BACK, "Back", keyFlags);
				if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_DPAD_LEFT))
					addKey(listView, KeyEvent.KEYCODE_DPAD_LEFT, "Left", keyFlags);
				if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_DPAD_RIGHT))
					addKey(listView, KeyEvent.KEYCODE_DPAD_RIGHT, "Right", keyFlags);
				if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_DPAD_UP))
					addKey(listView, KeyEvent.KEYCODE_DPAD_UP, "Up", keyFlags);
				if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_DPAD_DOWN))
					addKey(listView, KeyEvent.KEYCODE_DPAD_DOWN, "Down", keyFlags);
				if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_DPAD_CENTER))
					addKey(listView, KeyEvent.KEYCODE_DPAD_CENTER, "Center", keyFlags);
				if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_SEARCH))
					addKey(listView, KeyEvent.KEYCODE_SEARCH, "Search", keyFlags);
				if (DeviceInfo.EINK_ONYX) {
					if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_VOLUME_UP))
						addKey(listView, KeyEvent.KEYCODE_VOLUME_UP, "Left Side Button (Volume Up)", keyFlags);
					if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_VOLUME_DOWN))
						addKey(listView, KeyEvent.KEYCODE_VOLUME_DOWN, "Right Side Button (Volume Down)", keyFlags);
					if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_PAGE_UP))
						addKey(listView, KeyEvent.KEYCODE_PAGE_UP, "Left Side Button", keyFlags);
					if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_PAGE_DOWN))
						addKey(listView, KeyEvent.KEYCODE_PAGE_DOWN, "Right Side Button", keyFlags);
				}
				else {
					if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_VOLUME_UP))
						addKey(listView, KeyEvent.KEYCODE_VOLUME_UP, "Volume Up");
					if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_VOLUME_DOWN))
						addKey(listView, KeyEvent.KEYCODE_VOLUME_DOWN, "Volume Down");
					if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_PAGE_UP))
						addKey(listView, KeyEvent.KEYCODE_PAGE_UP, "Page Up");
					if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_PAGE_DOWN))
						addKey(listView, KeyEvent.KEYCODE_PAGE_DOWN, "Page Down");
				}
				if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_CAMERA))
					addKey(listView, KeyEvent.KEYCODE_CAMERA, "Camera", keyFlags);
				if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_ESCAPE))
					addKey(listView, ReaderView.KEYCODE_ESCAPE, "Escape", keyFlags);
				addKey(listView, KeyEvent.KEYCODE_HEADSETHOOK, "Headset Hook");
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
			if (!enabled)
				return;
			BaseDialog dlg = new BaseDialog(mActivity, label, false, false);
			OptionsListView listView = new OptionsListView(getContext());
			listView.add(new ListOption(mOwner, getString(R.string.options_page_show_titlebar), PROP_STATUS_LOCATION).add(mStatusPositions, mStatusPositionsTitles).setDefaultValue("1"));
			listView.add(new ListOption(mOwner, getString(R.string.options_rounded_corners_margin), PROP_ROUNDED_CORNERS_MARGIN).add(mRoundedCornersMargins).setDefaultValue("0"));
			listView.add(new ListOption(mOwner, getString(R.string.options_page_titlebar_font_face), PROP_STATUS_FONT_FACE).add(mFontFaces).setDefaultValue(mFontFaces[0]).setIconIdByAttr(R.attr.cr3_option_font_face_drawable, R.drawable.cr3_option_font_face));
			listView.add(new NumberPickerOption(mOwner, getString(R.string.options_page_titlebar_font_size), PROP_STATUS_FONT_SIZE).setMinValue(mActivity.getMinFontSize()).setMaxValue(mActivity.getMaxFontSize()).setDefaultValue("18").setIconIdByAttr(R.attr.cr3_option_font_size_drawable, R.drawable.cr3_option_font_size));

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
			if (!enabled)
				return;
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
			if (!enabled)
				return;
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
			view.setOnClickListener(v -> {
				// TODO: i18n
				ActionOption option = new ActionOption(mOwner, getString(R.string.options_app_tap_action_short), propName, true, false);
				option.setOnChangeHandler(() -> {
					ReaderAction action1 = ReaderAction.findById( mProperties.getProperty(propName) );
					text.setText(getString(action1.nameId));
				});
				option.onSelect();
			});
			view.setOnLongClickListener(v -> {
				// TODO: i18n
				ActionOption option = new ActionOption(mOwner, getString(R.string.options_app_tap_action_long), longPropName, true, true);
				option.setOnChangeHandler(() -> {
					ReaderAction longAction1 = ReaderAction.findById( mProperties.getProperty(longPropName) );
					longtext.setText(getString(longAction1.nameId));
				});
				option.onSelect();
				return true;
			});
		}

		public String getValueLabel() { return ">"; }
		public void onSelect() {
			if (!enabled)
				return;
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
				view = convertView;
			}
			myView = view;
			TextView labelView = view.findViewById(R.id.option_label);
			labelView.setText(label);
			labelView.setEnabled(enabled);
			setupIconView(view.findViewById(R.id.option_icon));
			return view;
		}
	}
	
	public static class ListOption extends OptionBase {
		protected ArrayList<Pair> list = new ArrayList<>();

		class ListOptionAdapter extends BaseAdapter {

			private final ListView mListView;
			private final List<Pair> mList;

			ListOptionAdapter(ListView listView, List<Pair> list) {
				super();
				mListView = listView;
				mList = list;
			}

			public boolean areAllItemsEnabled() {
				return true;
			}

			public boolean isEnabled(int position) {
				return true;
			}

			public int getCount() {
				return mList.size();
			}

			public Object getItem(int position) {
				return mList.get(position);
			}

			public long getItemId(int position) {
				return position;
			}

			public int getItemViewType(int position) {
				return 0;
			}

			public View getView(final int position, View convertView, ViewGroup parent) {
				ViewGroup layout;
				if ( convertView==null ) {
					layout = (ViewGroup)mInflater.inflate(getItemLayoutId(), null);
				} else {
					layout = (ViewGroup)convertView;
				}
				final Pair item = mList.get(position);
				updateItemContents( layout, item, mListView, position );
				return layout;
			}

			public int getViewTypeCount() {
				return 1;
			}

			public boolean hasStableIds() {
				return true;
			}

			public boolean isEmpty() {
				return mList.size()==0;
			}

			private ArrayList<DataSetObserver> observers = new ArrayList<>();

			public void registerDataSetObserver(DataSetObserver observer) {
				observers.add(observer);
			}

			public void unregisterDataSetObserver(DataSetObserver observer) {
				observers.remove(observer);
			}

		};

		public ListOption( OptionOwner owner, String label, String property ) {
			super(owner, label, property);
		}
		public ListOption add(String value, String label) {
			list.add( new Pair(value, label) );
			return this;
		}
		public ListOption add(int value, int labelID) {
			String str_value = String.valueOf(value);
			String label = mActivity.getString(labelID);
			list.add( new Pair(str_value, label) );
			return this;
		}
		public ListOption add(int value, String label) {
			String str_value = String.valueOf(value);
			list.add( new Pair(str_value, label) );
			return this;
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
		public ListOption add(List<?> values, List<String> labels) {
			for ( int i=0; i < values.size(); i++ ) {
				String value = String.valueOf(values.get(i));
				String label = labels.get(i);
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
		public void clear() {
			list.clear();
			refreshList();
		}
		public String findValueLabel( String value ) {
			for ( Pair pair : list ) {
				if (pair.value.equals(value))
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
			view = layout.findViewById(R.id.option_value_text);
			cb = layout.findViewById(R.id.option_value_check);
			view.setText(item.label);
			String currValue = mProperties.getProperty(property);
			boolean isSelected = item.value != null && item.value.equals(currValue);//getSelectedItemIndex()==position;
			cb.setChecked(isSelected);
			cb.setOnClickListener(v -> {
				AdapterView.OnItemClickListener listener = listView.getOnItemClickListener();
				if (null != listener)
					listener.onItemClick(listView, listView, position, 0);
			});
		}
		
		public String getValueLabel() { return findValueLabel(mProperties.getProperty(property)); }
		
		public void onSelect() {
			if (!enabled)
				return;
			final BaseDialog dlg = new BaseDialog(mActivity, label, false, false);
			final ListView listView = new BaseListView(mActivity, false);
			ListOptionAdapter listAdapter = new ListOptionAdapter(listView, list);
			int selItem = getSelectedItemIndex();
			if ( selItem<0 )
				selItem = 0;
			listView.setAdapter(listAdapter);
			listView.setSelection(selItem);
			dlg.setView(listView);
			//final AlertDialog d = dlg.create();
			listView.setOnItemClickListener((adapter, listview, position, id) -> {
				Pair item = list.get(position);
				onClick(item);
				dlg.dismiss();
				closed();
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

	protected interface FontScanCompleted {
		void onComplete(ArrayList<Pair> list, boolean canceled);
	}

	protected class FontSelectOption extends ListOption {
		protected ArrayList<Pair> sourceList;
		private String langTag;
		private String langDescr;
		private ListOptionAdapter listAdapter;

		public FontSelectOption(OptionOwner owner, String label, String property ) {
			super(owner, label, property);
			langTag = null;
			langDescr = null;
			BookInfo bookInfo = mReaderView.getBookInfo();
			if (null != bookInfo) {
				FileInfo fileInfo = bookInfo.getFileInfo();
				if (null != fileInfo) {
					langTag = fileInfo.language;
					langDescr = Engine.getHumanReadableLocaleName(langTag);
				}
			}
		}

		private void asyncFilterFontsByLanguage(String langTag, FontScanCompleted onComplete) {
			BackgroundThread.ensureGUI();
			final Scanner.ScanControl control = new Scanner.ScanControl();
			final Engine.ProgressControl progress = Services.getEngine().createProgress(R.string.scanning_font_files, control);
			final ArrayList<Pair> filtered = new ArrayList<Pair>();
			BackgroundThread.instance().postBackground(() -> {
				int i = 0;
				for (Pair pair : list) {
					if (control.isStopped())
						break;
					String faceName = pair.value;
					Engine.font_lang_compat status = Engine.checkFontLanguageCompatibility(faceName, langTag);
					switch (status) {
						case font_lang_compat_full:
						case font_lang_compat_partial:
							filtered.add(new Pair(faceName, faceName));
							break;
						default:
							break;
					}
					i++;
					progress.setProgress(10000*i/list.size());
				}
				onComplete.onComplete(filtered, control.isStopped());
				progress.hide();
			});
		}

		public void onSelect() {
			if (!enabled)
				return;
			final BaseDialog dlg = new BaseDialog(mActivity, label, false, false);

			LinearLayout layout = new LinearLayout(mActivity);
			layout.setOrientation(LinearLayout.VERTICAL);

			View panel = mInflater.inflate(R.layout.option_lang_filter, null);
			layout.addView(panel);
			CompoundButton filter_by_lang = panel.findViewById(R.id.filter_by_lang);
			if (null != langDescr && langDescr.length() > 0) {
				filter_by_lang.setText(mActivity.getString(R.string.filter_by_book_language_s, langDescr));
			} else {
				filter_by_lang.setText(mActivity.getString(R.string.filter_by_book_language_s, mActivity.getString(R.string.undetermined)));
				filter_by_lang.setEnabled(false);
			}
			final ListView listView = new BaseListView(mActivity, false);
			listAdapter = new ListOptionAdapter(listView, list);
			int selItem = getSelectedItemIndex();
			if ( selItem<0 )
				selItem = 0;
			listView.setAdapter(listAdapter);
			listView.setSelection(selItem);
			layout.addView(listView);

			listView.setOnItemClickListener((adapter, listview, position, id) -> {
				Pair item = (Pair) listAdapter.getItem(position);
				onClick(item);
				dlg.dismiss();
				closed();
			});

			filter_by_lang.setOnCheckedChangeListener((buttonView, isChecked) -> {
				if (isChecked) {
					asyncFilterFontsByLanguage(langTag, (list, canceled) -> {
						if (!canceled) {
							BackgroundThread.instance().executeGUI(() -> {
								FontSelectOption.this.sourceList = FontSelectOption.this.list;
								FontSelectOption.this.list = list;
								listAdapter = new ListOptionAdapter(listView, list);
								int selindex = getSelectedItemIndex();
								if ( selindex<0 )
									selindex = 0;
								listView.setAdapter(listAdapter);
								listView.setSelection(selindex);
							});
						} else {
							BackgroundThread.instance().executeGUI(() -> {
								filter_by_lang.setChecked(false);
							});
						}
					});
				} else {
					if (null != sourceList) {
						list = sourceList;
						listAdapter = new ListOptionAdapter(listView, list);
						int selindex = getSelectedItemIndex();
						if (selindex < 0)
							selindex = 0;
						listView.setAdapter(listAdapter);
						listView.setSelection(selindex);
					}
				}
			});

			dlg.setOnDismissListener(dialog -> closed());

			// TODO: set checked for for filter_by_lang (save in settings)

			dlg.setView(layout);
			dlg.show();
		}

		protected void closed() {
			if (null != sourceList)
				list = sourceList;
		}
	}

	class FallbackFontItemOptions extends ListOption {

		private final FallbackFontsOptions mParentOptions;	// parent option item
		private final int mPosition;						// position in parent list

		public FallbackFontItemOptions(OptionOwner owner, String label, String property, FallbackFontsOptions parentOptions, int pos) {
			super(owner, label, property);
			mParentOptions = parentOptions;
			mPosition = pos;
		}

		public void onClick( Pair item ) {
			super.onClick(item);
			if (item.value.length() > 0) {
				if (mPosition == mParentOptions.size() - 1) {
					if (mPosition < MAX_FALLBACK_FONTS_COUNT - 1) {
						// last item in parent list not empty => add new empty item in parent list
						mParentOptions.addFallbackFontItem(mParentOptions.size(), "");
					}
				}
			} else {
				if (mPosition == mParentOptions.size() - 2) {
					// penultimate item in parent list set to empty => remove last empty item
					mParentOptions.removeFallbackFontItem(mParentOptions.size() - 1);
				}
			}
		}
	}

	class FallbackFontsOptions extends SubmenuOption {

		OptionsListView mListView;
		private final Properties mFallbackProps = new Properties();
		private final OptionOwner mFallbackOptionItemOwner = new OptionOwner() {
			@Override
			public BaseActivity getActivity() {
				return  mOwner.getActivity();
			}
			@Override
			public Properties getProperties() {
				return mFallbackProps;
			}
			@Override
			public LayoutInflater getInflater() {
				return mOwner.getInflater();
			}
		};

		public FallbackFontsOptions( OptionOwner owner, String label ) {
			super( owner, label, PROP_FALLBACK_FONT_FACES );
			mListView = new OptionsListView(getContext());
		}

		protected void addFallbackFontItem( int pos, String fontFace) {
			String propName = Integer.toString(pos);
			String label = getString(R.string.options_font_fallback_face_num, pos + 1);
			mFallbackProps.setProperty(propName, fontFace);
			FallbackFontItemOptions option = new FallbackFontItemOptions(mFallbackOptionItemOwner, label, propName, this, pos);
			option.add("", "(empty)");
			option.add(mFontFaces);
			option.setDefaultValue("");
			option.noIcon();
			mListView.add(option);
		}

		protected boolean removeFallbackFontItem( int pos ) {
			boolean res = mListView.remove(pos);
			if (res)
				mListView.refresh();
			return res;
		}

		public void onSelect() {
			if (!enabled)
				return;
			BaseDialog dlg = new BaseDialog(mActivity, label, false, false) {
				protected void onClose()
				{
					updateMainPropertyValue();
					this.setView(null);
					super.onClose();
				}
			};
			mListView.clear();
			String fallbackFaces = getProperties().getProperty(property);
			if (null == fallbackFaces)
				fallbackFaces = "";
			String[] list = fallbackFaces.split(";");
			int pos = 0;
			for (String face : list) {
				face = face.trim();
				if (face.length() > 0) {
					addFallbackFontItem(pos, face);
					pos++;
				}
			}
			addFallbackFontItem(pos, "");
			dlg.setView(mListView);
			dlg.show();
		}

		public int size() {
			return mListView.size();
		}

		public String getValueLabel() { return ">"; }

		private void updateMainPropertyValue() {
			StringBuilder fallbackFacesBuilder = new StringBuilder();
			for (int i = 0; i < MAX_FALLBACK_FONTS_COUNT; i++) {
				String propName = Integer.toString(i);
				String fallback = mFallbackProps.getProperty(propName);
				if (null != fallback && fallback.length() > 0) {
					fallbackFacesBuilder.append(fallback);
					fallbackFacesBuilder.append("; ");
				}
			}
			String fallbackFaces = fallbackFacesBuilder.toString();
			// Remove trailing "; " part
			if (fallbackFaces.length() >= 2)
				fallbackFaces = fallbackFaces.substring(0, fallbackFaces.length() - 2);
			getProperties().setProperty(property, fallbackFaces);
		}
	}

	class DictOptions extends ListOption
	{
		public DictOptions( OptionOwner owner, String label )
		{
			super( owner, label, PROP_APP_DICTIONARY );
			DictInfo[] dicts = Dictionaries.getDictList();
			setDefaultValue(dicts[0].id);
			for (DictInfo dict : dicts) {
				boolean installed = mActivity.isPackageInstalled(dict.packageName);
				String sAdd = mActivity.getString(R.string.options_app_dictionary_not_installed);
				if ((dict.internal==1) && (dict.packageName.equals("com.socialnmobile.colordict")) && (!installed)) {
					installed = mActivity.isPackageInstalled("mobi.goldendict.android");
					add(dict.id, (installed ? "GoldenDict" : dict.name + " " + sAdd));
				} else {
					add(dict.id, dict.name + (installed ? "" : " " + sAdd));
				}
			}
		}
	}

    class DictOptions2 extends ListOption
    {
        public DictOptions2( OptionOwner owner, String label )
        {
            super( owner, label, PROP_APP_DICTIONARY_2 );
            DictInfo[] dicts = Dictionaries.getDictList();
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
			setDefaultValue(Engine.HyphDict.HYPH_RU_RU_EN_US.code);
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
			setDefaultValue(DeviceInfo.FORCE_HC_THEME ? "HICONTRAST1" : "LIGHT");
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
		ArrayList<Item> list = new ArrayList<>();
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
				if (path.equals(list.get(i).path)) {
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
			ImageView img = layout.findViewById(R.id.option_value_image);
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

	class NumberPickerOption extends OptionBase {
		private int minValue = 9;
		private int maxValue = 340;
		public NumberPickerOption(OptionOwner owner, String label, String property ) {
			super(owner, label, property);
		}
		public int getItemViewType() {
			return OPTION_VIEW_TYPE_NUMBER;
		}
		private int getValueInt() {
			int res = 0;
			try {
				res = Integer.parseInt(mProperties.getProperty(property));
			} catch (NumberFormatException ignored) {}
			return res;
		}
		NumberPickerOption setMinValue(int minValue) {
			this.minValue = minValue;
			return this;
		}
		NumberPickerOption setMaxValue(int maxValue) {
			this.maxValue = maxValue;
			return this;
		}
		public void onSelect() {
			if (!enabled)
				return;
			InputDialog dlg = new InputDialog(mActivity, label, false, "", true, minValue, maxValue, getValueInt(), new InputDialog.InputHandler() {
				@Override
				public boolean validate(String s) throws Exception {
					int value = Integer.parseInt(s);
					return value >= minValue && value <= maxValue;
				}

				@Override
				public void onOk(String s) throws Exception {
					getProperties().setProperty(property, s);
					refreshItem();
				}

				@Override
				public void onCancel() {
				}
			});
			dlg.show();
		}
	}

	//byte[] fakeLongArrayForDebug;
	
	public enum Mode {
		READER,
		BROWSER,
		TTS,
	}
	public OptionsDialog(BaseActivity activity, Mode mode, ReaderView readerView, String[] fontFaces, TTSControlBinder ttsbinder)
	{
		super(activity, null, false, false);
		
		mActivity = activity;
		mReaderView = readerView;
		mFontFaces = fontFaces;
		mTTSBinder = ttsbinder;
		mProperties = new Properties(mActivity.settings()); //  readerView.getSettings();
		mOldProperties = new Properties(mProperties);
		if (mode == Mode.READER) {
			mProperties.setBool(PROP_TXT_OPTION_PREFORMATTED, mReaderView.isTextAutoformatEnabled());
			mProperties.setBool(PROP_EMBEDDED_STYLES, mReaderView.getDocumentStylesEnabled());
			mProperties.setBool(PROP_EMBEDDED_FONTS, mReaderView.getDocumentFontsEnabled());
			mProperties.setInt(PROP_REQUESTED_DOM_VERSION, mReaderView.getDOMVersion());
			mProperties.setInt(PROP_RENDER_BLOCK_RENDERING_FLAGS, mReaderView.getBlockRenderingFlags());
			isTextFormat = readerView.isTextFormat();
			isEpubFormat = readerView.isFormatWithEmbeddedFonts();
			isFormatWithEmbeddedStyle = readerView.isFormatWithEmbeddedStyles();
			isHtmlFormat = readerView.isHtmlFormat();
		}
		showIcons = mProperties.getBool(PROP_APP_SETTINGS_SHOW_ICONS, true);
		mSynthWeights = Engine.getAvailableSynthFontWeight();
		if (null == mSynthWeights)
			mSynthWeights = new int[] {};
		this.mode = mode;
	}
	
	class OptionsListView extends BaseListView {
		private ArrayList<OptionBase> mOptions = new ArrayList<>();
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
		public boolean remove(int index) {
			try {
				mOptions.remove(index);
				return true;
			} catch (Exception ignored) {
			}
			return false;
		}
		public boolean remove( OptionBase option ) {
			return mOptions.remove(option);
		}
		public void clear() {
			mOptions.clear();
		}
		public OptionsListView( Context context )
		{
			super(context, false);
			setFocusable(true);
			setFocusableInTouchMode(true);
			mAdapter = new BaseAdapter() {
				public boolean areAllItemsEnabled() {
					return true;
				}

				public boolean isEnabled(int position) {
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

				private ArrayList<DataSetObserver> observers = new ArrayList<>();
				
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
			try {
				OptionBase option = mOptions.get(position);
				if (option.enabled) {
					option.onSelect();
					return true;
				}
			} catch (Exception ignored) {}
			return false;
		}
		public int size() {
			return mOptions.size();
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
		if ( "Clouds".equals(tag) )
			if (null != mOptionsCloudSync) {
				return mOptionsCloudSync;
			}

		return null;
	}

	private String getString( int resourceId )
	{
		return getContext().getResources().getString(resourceId); 
	}

	public String getString( int resourceId, Object... formatArgs ) {
		return getContext().getResources().getString(resourceId, formatArgs);
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
			listView.add(new ListOption(mOwner, getString(R.string.options_css_text_align), prefix + ".align").add(firstLineOptions, firstLineOptionNames).setIconIdByAttr(R.attr.cr3_option_text_align_drawable, R.drawable.cr3_option_text_align));
			
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
			listView.add(new ListOption(mOwner, getString(R.string.options_css_text_indent), prefix + ".text-indent").add(identOptions, identOptionNames).setIconIdByAttr(R.attr.cr3_option_text_indent_drawable, R.drawable.cr3_option_text_indent));

			ArrayList<String> faces = new ArrayList<>();
			ArrayList<String> faceValues = new ArrayList<>();
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
			listView.add(new ListOption(mOwner, getString(R.string.options_css_font_face), prefix + ".font-face").add(faceValues.toArray(new String[]{}), faces.toArray(new String[]{})).setIconIdByAttr(R.attr.cr3_option_font_face_drawable, R.drawable.cr3_option_font_face));
			
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
			listView.add(new ListOption(mOwner, getString(R.string.options_css_font_size), prefix + ".font-size").add(fontSizeStyles, fontSizeStyleNames).setIconIdByAttr(R.attr.cr3_option_font_size_drawable, R.drawable.cr3_option_font_size));

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
			listView.add(new ListOption(mOwner, getString(R.string.options_css_font_weight), prefix + ".font-weight").add(fontWeightStyles, fontWeightStyleNames).setIconIdByAttr(R.attr.cr3_option_text_bold_drawable, R.drawable.cr3_option_text_bold));

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
			listView.add(new ListOption(mOwner, getString(R.string.options_css_font_style), prefix + ".font-style").add(fontStyleStyles, fontStyleStyleNames).setIconIdByAttr(R.attr.cr3_option_text_italic_drawable, R.drawable.cr3_option_text_italic));

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
			listView.add(new ListOption(mOwner, getString(R.string.options_css_interline_space), prefix + ".line-height").add(lineHeightStyles, lineHeightStyleNames).setIconIdByAttr(R.attr.cr3_option_line_spacing_drawable, R.drawable.cr3_option_line_spacing));

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
			listView.add(new ListOption(mOwner, getString(R.string.options_css_font_decoration), prefix + ".text-decoration").add(textDecorationStyles, textDecorationStyleNames).setIconIdByAttr(R.attr.cr3_option_text_underline_drawable, R.drawable.cr3_option_text_underline));

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
			listView.add(new ListOption(mOwner, getString(R.string.options_css_text_valign), prefix + ".vertical-align").add(verticalAlignStyles, verticalAlignStyleNames).setIconIdByAttr(R.attr.cr3_option_text_superscript_drawable, R.drawable.cr3_option_text_superscript));

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
			listView.add(new ListOption(mOwner, getString(R.string.options_css_margin_top), prefix + ".margin-top").add(marginTopOptions, marginTopBottomOptionNames).setIconIdByAttr(R.attr.cr3_option_text_margin_top_drawable, R.drawable.cr3_option_text_margin_top));
			listView.add(new ListOption(mOwner, getString(R.string.options_css_margin_bottom), prefix + ".margin-bottom").add(marginBottomOptions, marginTopBottomOptionNames).setIconIdByAttr(R.attr.cr3_option_text_margin_bottom_drawable, R.drawable.cr3_option_text_margin_bottom));
			listView.add(new ListOption(mOwner, getString(R.string.options_css_margin_left), prefix + ".margin-left").add(marginLeftOptions, marginLeftRightOptionNames).setIconIdByAttr(R.attr.cr3_option_text_margin_left_drawable, R.drawable.cr3_option_text_margin_left));
			listView.add(new ListOption(mOwner, getString(R.string.options_css_margin_right), prefix + ".margin-right").add(marginRightOptions, marginLeftRightOptionNames).setIconIdByAttr(R.attr.cr3_option_text_margin_right_drawable, R.drawable.cr3_option_text_margin_right));

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
		mOptionsCSS.add(new BoolOption(this, getString(R.string.mi_book_styles_enable), PROP_EMBEDDED_STYLES).setDefaultValue("1").noIcon()
				.setOnChangeHandler(() -> {
					boolean value = mProperties.getBool(PROP_EMBEDDED_STYLES, false);
					mEmbedFontsOptions.setEnabled(isEpubFormat && value);
					mIgnoreDocMargins.setEnabled(isFormatWithEmbeddedStyle && value);
				})
		);
		mEmbedFontsOptions = new BoolOption(this, getString(R.string.options_font_embedded_document_font_enabled), PROP_EMBEDDED_FONTS).setDefaultValue("1").noIcon();
		boolean value = mProperties.getBool(PROP_EMBEDDED_STYLES, false);
		mEmbedFontsOptions.setEnabled(isEpubFormat && value);
		mEmbedFontsOptions.setDisabledNote(getString(R.string.options_disabled_document_styles));
		mOptionsCSS.add(mEmbedFontsOptions);
		mIgnoreDocMargins = new StyleBoolOption(this, getString(R.string.options_ignore_document_margins), "styles.body.margin", "margin: 0em !important", "").setDefaultValueBoolean(false).noIcon();
		mIgnoreDocMargins.setEnabled(isFormatWithEmbeddedStyle && value);
		mIgnoreDocMargins.setDisabledNote(getString(R.string.options_disabled_document_styles));
		mOptionsCSS.add(mIgnoreDocMargins);
		if (isTextFormat) {
			mOptionsCSS.add(new BoolOption(this, getString(R.string.mi_text_autoformat_enable), PROP_TXT_OPTION_PREFORMATTED).setDefaultValue("1").noIcon());
		}
		if (/*isHtmlFormat*/ isFormatWithEmbeddedStyle) {
			Runnable renderindChangeListsner = () -> {
				boolean legacyRender = mProperties.getInt(PROP_RENDER_BLOCK_RENDERING_FLAGS, 0) == 0 ||
						mProperties.getInt(PROP_REQUESTED_DOM_VERSION, 0) < 20180524;
				mEnableMultiLangOption.setEnabled(!legacyRender);
				if (legacyRender) {
					mHyphDictOption.setEnabled(true);
					mEnableHyphOption.setEnabled(false);
				} else {
					boolean embeddedLang = mProperties.getBool(PROP_TEXTLANG_EMBEDDED_LANGS_ENABLED, false);
					mHyphDictOption.setEnabled(!embeddedLang);
					mEnableHyphOption.setEnabled(embeddedLang);
				}
			};
			mOptionsCSS.add(new ListOption(this, getString(R.string.options_rendering_preset), PROP_RENDER_BLOCK_RENDERING_FLAGS).add(mRenderingPresets, mRenderingPresetsTitles).setDefaultValue(Integer.valueOf(Engine.BLOCK_RENDERING_FLAGS_WEB).toString())
					.noIcon()
					.setOnChangeHandler(renderindChangeListsner)
			);
			mOptionsCSS.add(new ListOption(this, getString(R.string.options_requested_dom_level), PROP_REQUESTED_DOM_VERSION).add(mDOMVersionPresets, mDOMVersionPresetTitles).setDefaultValue(Integer.valueOf(Engine.DOM_VERSION_CURRENT).toString())
					.noIcon()
					.setOnChangeHandler(renderindChangeListsner)
			);
		}
		for (int i=0; i<styleCodes.length; i++)
			mOptionsCSS.add(createStyleEditor(styleCodes[i], styleTitles[i]));
	}
	
	private void setupBrowserOptions()
	{
        mInflater = LayoutInflater.from(getContext());
        ViewGroup view = (ViewGroup)mInflater.inflate(R.layout.options_browser, null);
        ViewGroup body = view.findViewById(R.id.body);
        
        
        mOptionsBrowser = new OptionsListView(getContext());

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
		mOptionsBrowser.add(new BoolOption(this, getString(R.string.options_app_browser_hide_empty_dirs), PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS).setComment(getString(R.string.options_hide_empty_dirs_slowdown)).setDefaultValue("0").noIcon());
		mOptionsBrowser.add(new BoolOption(this, getString(R.string.options_app_browser_hide_empty_genres), PROP_APP_FILE_BROWSER_HIDE_EMPTY_GENRES).setDefaultValue("0").noIcon());
		mOptionsBrowser.add(new ListOption(this, getString(R.string.options_app_backlight_screen), PROP_APP_SCREEN_BACKLIGHT).add(mBacklightLevels, mBacklightLevelsTitles).setDefaultValue("-1").noIcon());
		mOptionsBrowser.add(new LangOption(this).noIcon());
		mOptionsBrowser.add(new PluginsOption(this, getString(R.string.options_app_plugins)).noIcon());
		mOptionsBrowser.add(new BoolOption(this, getString(R.string.options_app_fullscreen), PROP_APP_FULLSCREEN).setIconIdByAttr(R.attr.cr3_option_fullscreen_drawable, R.drawable.cr3_option_fullscreen));
		if ( !DeviceInfo.EINK_SCREEN ) {
			mOptionsBrowser.add(new NightModeOption(this, getString(R.string.options_inverse_view), PROP_NIGHT_MODE).setIconIdByAttr(R.attr.cr3_option_night_drawable, R.drawable.cr3_option_night));
		}
		if ( !DeviceInfo.FORCE_HC_THEME) {
			mOptionsBrowser.add(new ThemeOptions(this, getString(R.string.options_app_ui_theme)).noIcon());
		}
		mOptionsBrowser.refresh();
		
		body.addView(mOptionsBrowser);
		setView(view);
	}

	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	private void fillTTSLanguages(ListOption listOption) {
		listOption.clear();
		if (null != mTTSBinder) {
			mTTSBinder.retrieveAvailableLocales(list -> {
				BackgroundThread.instance().executeGUI(() -> {
					for (Locale locale : list) {
						String language = locale.getDisplayLanguage();
						String country = locale.getDisplayCountry();
						if (country.length() > 0)
							language += " (" + country + ")";
						listOption.add(locale.toString(), language);
					}
					listOption.noIcon();
					listOption.refreshList();
				});
			});
		}
	}

	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	private void fillTTSVoices(ListOption listOption, String language) {
		listOption.clear();
		if (null != mTTSBinder) {
			mTTSBinder.retrieveAvailableVoices(new Locale(language), list -> {
				BackgroundThread.instance().executeGUI(() -> {
					for (Voice voice : list) {
						String quality;
						int qualityInt = voice.getQuality();
						if (qualityInt >= Voice.QUALITY_VERY_HIGH)
							quality = getString(R.string.options_tts_voice_quality_very_high);
						else if (qualityInt >= Voice.QUALITY_HIGH)
							quality = getString(R.string.options_tts_voice_quality_high);
						else if (qualityInt >= Voice.QUALITY_NORMAL)
							quality = getString(R.string.options_tts_voice_quality_normal);
						else if (qualityInt >= Voice.QUALITY_LOW)
							quality = getString(R.string.options_tts_voice_quality_low);
						else
							quality = getString(R.string.options_tts_voice_quality_very_low);
						listOption.add(voice.getName(), voice.getName() + " (" + quality + ")");
					}
					listOption.noIcon();
					listOption.refreshList();
				});
			});
		}
	}

	private void setupTTSOptions() {
		mInflater = LayoutInflater.from(getContext());
		ViewGroup view = (ViewGroup)mInflater.inflate(R.layout.options_tts, null);
		ViewGroup body = view.findViewById(R.id.body);

		mOptionsTTS = new OptionsListView(getContext());
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			mTTSEngineOption = new ListOption(this, getString(R.string.options_tts_engine), PROP_APP_TTS_ENGINE);
			mTTSBinder.retrieveAvailableEngines(list -> {
				BackgroundThread.instance().executeGUI(() -> {
					for (TextToSpeech.EngineInfo info : list) {
						mTTSEngineOption.add(info.name, info.label);
					}
					String tts_package = mProperties.getProperty(PROP_APP_TTS_ENGINE, "");
					mTTSEngineOption.setDefaultValue(tts_package);
					mTTSEngineOption.refreshList();
				});
			});
			mOptionsTTS.add(mTTSEngineOption.noIcon());
			mTTSEngineOption.setOnChangeHandler(() -> {
				String tts_package = mProperties.getProperty(PROP_APP_TTS_ENGINE, "");
				mTTSBinder.initTTS(tts_package, new OnTTSCreatedListener() {
					@Override
					public void onCreated() {
						if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP) {
							BackgroundThread.instance().executeGUI(() -> {
								if (null != mTTSLanguageOption)
									fillTTSLanguages(mTTSLanguageOption);
								if (null != mTTSVoiceOption)
									mTTSVoiceOption.clear();
							});
						}
					}
					@Override
					public void onFailed() {
						if (null != mTTSLanguageOption)
							mTTSLanguageOption.clear();
					}
					@Override
					public void onTimedOut() {
						if (null != mTTSLanguageOption)
							mTTSLanguageOption.clear();
					}
				});
			});
		}
		mTTSUseDocLangOption = new BoolOption(this, getString(R.string.options_tts_use_doc_lang), PROP_APP_TTS_USE_DOC_LANG).setDefaultValue("1").noIcon();
		mOptionsTTS.add(mTTSUseDocLangOption);
		if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP) {
			boolean useDocLang = mProperties.getBool(PROP_APP_TTS_USE_DOC_LANG, true);
			mTTSLanguageOption = new ListOption(this, getString(R.string.options_tts_language), PROP_APP_TTS_FORCE_LANGUAGE);
			fillTTSLanguages(mTTSLanguageOption);
			mTTSLanguageOption.setEnabled(!useDocLang);
			mOptionsTTS.add(mTTSLanguageOption);
			// onchange handler
			String lang = mProperties.getProperty (PROP_APP_TTS_FORCE_LANGUAGE, "");
			mTTSUseDocLangOption.setOnChangeHandler(() -> {
				boolean value = mProperties.getBool(PROP_APP_TTS_USE_DOC_LANG, true);
				mTTSLanguageOption.setEnabled(!value);
				mTTSVoiceOption.setEnabled(!value);
			});
			mTTSLanguageOption.setOnChangeHandler(() -> {
				String value = mProperties.getProperty(PROP_APP_TTS_FORCE_LANGUAGE, "");
				fillTTSVoices(mTTSVoiceOption, value);
			});

			mTTSVoiceOption = new ListOption(this, getString(R.string.options_tts_voice), PROP_APP_TTS_VOICE);
			fillTTSVoices(mTTSVoiceOption, lang);
			mTTSVoiceOption.setEnabled(!useDocLang);
			mOptionsTTS.add(mTTSVoiceOption);
		}
		mOptionsTTS.add(new BoolOption(this, getString(R.string.options_tts_google_abbr_workaround), PROP_APP_TTS_GOOGLE_END_OF_SENTENCE_ABBR).setComment(getString(R.string.options_tts_google_abbr_workaround_comment)).setDefaultValue("1").noIcon());
		mOptionsTTS.add(
			new BoolOption(
				this,
				getString(R.string.options_tts_use_audiobook),
				PROP_APP_TTS_USE_AUDIOBOOK
			).setDefaultValue("1").noIcon());
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ECLAIR)
			mOptionsTTS.add(new ListOption(this, getString(R.string.options_app_tts_stop_motion_timeout), PROP_APP_MOTION_TIMEOUT).add(mMotionTimeouts, mMotionTimeoutsTitles).setDefaultValue(Integer.toString(mMotionTimeouts[0])).noIcon());
		mOptionsTTS.refresh();
		body.addView(mOptionsTTS);
		setView(view);
	}

	private String getWeightName(int weight) {
		String name = "";
		switch (weight) {
			case 100:
				name = getString(R.string.font_weight_thin);
				break;
			case 200:
				name = getString(R.string.font_weight_extralight);
				break;
			case 300:
				name = getString(R.string.font_weight_light);
				break;
			case 350:
				name = getString(R.string.font_weight_book);
				break;
			case 400:
				name = getString(R.string.font_weight_regular);
				break;
			case 500:
				name = getString(R.string.font_weight_medium);
				break;
			case 600:
				name = getString(R.string.font_weight_semibold);
				break;
			case 700:
				name = getString(R.string.font_weight_bold);
				break;
			case 800:
				name = getString(R.string.font_weight_extrabold);
				break;
			case 900:
				name = getString(R.string.font_weight_black);
				break;
			case 950:
				name = getString(R.string.font_weight_extrablack);
				break;
		}
		return name;
	}

	private void updateFontWeightValues(ListOption option, String faceName) {
		// get available weight for font faceName
		int[] nativeWeights = Engine.getAvailableFontWeight(faceName);
		if (null == nativeWeights || 0 == nativeWeights.length) {
			// invalid font
			option.clear();
			return;
		}
		ArrayList<Integer> nativeWeightsArray = new ArrayList<>();	// for search
		for (int w : nativeWeights)
			nativeWeightsArray.add(w);
		// combine with synthetic weights
		ArrayList<Integer> weights = new ArrayList<>();
		int synth_idx = 0;
		int i, j;
		int weight = 0, prev_weight = 0;
		for (i = 0; i < nativeWeights.length; i++) {
			weight = nativeWeights[i];
			for (j = synth_idx; j < mSynthWeights.length; j++) {
				int synth_weight = mSynthWeights[j];
				if (synth_weight < weight) {
					if (synth_weight > prev_weight)
						weights.add(synth_weight);
				}
				else
					break;
			}
			synth_idx = j;
			weights.add(weight);
			prev_weight = weight;
		}
		for (j = synth_idx; j < mSynthWeights.length; j++) {
			if (mSynthWeights[j] > weight)
				weights.add(mSynthWeights[j]);
		}
		// fill items
		option.clear();
		for (i = 0; i < weights.size(); i++) {
			weight = weights.get(i);
			String label = String.valueOf(weight);
			String descr = getWeightName(weight);
			if (!nativeWeightsArray.contains(weight)) {
				if (descr.length() > 0)
					descr += ", " + getString(R.string.font_weight_fake);
				else
					descr = getString(R.string.font_weight_fake);
			}
			if (descr.length() > 0)
				label += " (" + descr + ")";
			option.add(weight, label);
		}
		// enable/disable font hinting option
		//int base_weight = mProperties.getInt(PROP_FONT_BASE_WEIGHT, 400);
		//mFontHintingOption.setEnabled(nativeWeightsArray.contains(base_weight));
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
		//tabWidget.
		//new TabHost(getContext());

		boolean legacyRender = mProperties.getInt(PROP_RENDER_BLOCK_RENDERING_FLAGS, 0) == 0 ||
				mProperties.getInt(PROP_REQUESTED_DOM_VERSION, 0) < 20180524;

		mOptionsStyles = new OptionsListView(getContext());
		mFontHintingOption = new ListOption(this, getString(R.string.options_font_hinting), PROP_FONT_HINTING).add(mHinting, mHintingTitles).setDefaultValue("2").setIconIdByAttr(R.attr.cr3_option_text_hinting_drawable, R.drawable.cr3_option_text_hinting);
		OptionBase fontOption = new FontSelectOption(this, getString(R.string.options_font_face), PROP_FONT_FACE).add(mFontFaces).setDefaultValue(mFontFaces[0]).setIconIdByAttr(R.attr.cr3_option_font_face_drawable, R.drawable.cr3_option_font_face);
		mOptionsStyles.add(fontOption);
		mOptionsStyles.add(new NumberPickerOption(this, getString(R.string.options_font_size), PROP_FONT_SIZE).setMinValue(mActivity.getMinFontSize()).setMaxValue(mActivity.getMaxFontSize()).setDefaultValue("24").setIconIdByAttr(R.attr.cr3_option_font_size_drawable, R.drawable.cr3_option_font_size));
		mFontWeightOption = (ListOption) new ListOption(this, getString(R.string.options_font_weight), PROP_FONT_BASE_WEIGHT).setIconIdByAttr(R.attr.cr3_option_text_bold_drawable, R.drawable.cr3_option_text_bold);
		updateFontWeightValues(mFontWeightOption, mProperties.getProperty(PROP_FONT_FACE, ""));
		mOptionsStyles.add(mFontWeightOption);
		fontOption.setOnChangeHandler(() -> {
			String faceName = mProperties.getProperty(PROP_FONT_FACE, "");
			updateFontWeightValues(mFontWeightOption, faceName);
		});
		/*
		mFontWeightOption.setOnChangeHandler(() -> {
			// enable/disable font hinting option
			String faceName = mProperties.getProperty(PROP_FONT_FACE, "");
			int[] nativeWeights = Engine.getAvailableFontWeight(faceName);
			if (null != nativeWeights && 0 != nativeWeights.length) {
				ArrayList<Integer> nativeWeightsArray = new ArrayList<>();    // for search
				for (int w : nativeWeights)
					nativeWeightsArray.add(w);
				int base_weight = mProperties.getInt(PROP_FONT_BASE_WEIGHT, 400);
				mFontHintingOption.setEnabled(nativeWeightsArray.contains(base_weight));
			}
		});
		 */
		mOptionsStyles.add(new ListOption(this, getString(R.string.options_font_antialias), PROP_FONT_ANTIALIASING).add(mAntialias, mAntialiasTitles).setDefaultValue("2").setIconIdByAttr(R.attr.cr3_option_text_antialias_drawable, R.drawable.cr3_option_text_antialias));
		mOptionsStyles.add(new ListOption(this, getString(R.string.options_interline_space), PROP_INTERLINE_SPACE).addPercents(mInterlineSpaces).setDefaultValue("100").setIconIdByAttr(R.attr.cr3_option_line_spacing_drawable, R.drawable.cr3_option_line_spacing));
		//
		mEnableMultiLangOption = new BoolOption(this, getString(R.string.options_style_multilang), PROP_TEXTLANG_EMBEDDED_LANGS_ENABLED).setDefaultValue("0").setIconIdByAttr(R.attr.cr3_option_text_multilang_drawable, R.drawable.cr3_option_text_multilang)
				.setOnChangeHandler(() -> {
					boolean value = mProperties.getBool(PROP_TEXTLANG_EMBEDDED_LANGS_ENABLED, false);
					mHyphDictOption.setEnabled(!value);
					mEnableHyphOption.setEnabled(value);
				});
		mEnableMultiLangOption.enabled = !legacyRender;
		mEnableMultiLangOption.setDisabledNote(getString(R.string.options_legacy_rendering_enabled));
		mOptionsStyles.add(mEnableMultiLangOption);
		mEnableHyphOption = new BoolOption(this, getString(R.string.options_style_enable_hyphenation), PROP_TEXTLANG_HYPHENATION_ENABLED).setDefaultValue("0").setIconIdByAttr(R.attr.cr3_option_text_hyphenation_drawable, R.drawable.cr3_option_text_hyphenation);
		mEnableHyphOption.enabled = !legacyRender && mProperties.getBool(PROP_TEXTLANG_EMBEDDED_LANGS_ENABLED, false);
		mEnableHyphOption.setDisabledNote(getString(R.string.options_multilingual_disabled));
		mOptionsStyles.add(mEnableHyphOption);
		mHyphDictOption = new HyphenationOptions(this, getString(R.string.options_hyphenation_dictionary)).setIconIdByAttr(R.attr.cr3_option_text_hyphenation_drawable, R.drawable.cr3_option_text_hyphenation);
		mHyphDictOption.enabled = legacyRender || !mProperties.getBool(PROP_TEXTLANG_EMBEDDED_LANGS_ENABLED, false);
		mHyphDictOption.setDisabledNote(getString(R.string.options_multilingual_enabled));
		mOptionsStyles.add(mHyphDictOption);
		mOptionsStyles.add(new BoolOption(this, getString(R.string.options_style_floating_punctuation), PROP_FLOATING_PUNCTUATION).setDefaultValue("1").setIconIdByAttr(R.attr.cr3_option_text_floating_punct_drawable, R.drawable.cr3_option_text_other));
		mOptionsStyles.add(new ListOption(this, getString(R.string.options_text_shaping), PROP_FONT_SHAPING).add(mShaping, mShapingTitles).setDefaultValue("1").setIconIdByAttr(R.attr.cr3_option_text_ligatures_drawable, R.drawable.cr3_option_text_ligatures));
		mOptionsStyles.add(new BoolOption(this, getString(R.string.options_font_kerning), PROP_FONT_KERNING_ENABLED).setDefaultValue("0").setIconIdByAttr(R.attr.cr3_option_text_kerning_drawable, R.drawable.cr3_option_text_kerning));

		mOptionsStyles.add(new ImageScalingOption(this, getString(R.string.options_format_image_scaling)).setIconIdByAttr(R.attr.cr3_option_images_drawable, R.drawable.cr3_option_images));
		mOptionsStyles.add(new ListOption(this, getString(R.string.options_render_font_gamma), PROP_FONT_GAMMA).add(mGammas).setDefaultValue("1.0").setIconIdByAttr(R.attr.cr3_option_font_gamma_drawable, R.drawable.cr3_option_font_gamma));
		mOptionsStyles.add(new ListOption(this, getString(R.string.options_format_min_space_width_percent), PROP_FORMAT_MIN_SPACE_CONDENSING_PERCENT).addPercents(mMinSpaceWidths).setDefaultValue("50").setIconIdByAttr(R.attr.cr3_option_text_width_drawable, R.drawable.cr3_option_text_width));
		mOptionsStyles.add(mFontHintingOption);
		mOptionsStyles.add(new FallbackFontsOptions(this, getString(R.string.options_font_fallback_faces)).setIconIdByAttr(R.attr.cr3_option_font_face_drawable, R.drawable.cr3_option_font_face));
		
		//
		mOptionsPage = new OptionsListView(getContext());
		mOptionsPage.add(new BoolOption(this, getString(R.string.options_app_fullscreen), PROP_APP_FULLSCREEN).setIconIdByAttr(R.attr.cr3_option_fullscreen_drawable, R.drawable.cr3_option_fullscreen));
		mOptionsPage.add(new ListOption(this, getString(R.string.options_view_toolbar_position), PROP_TOOLBAR_LOCATION).add(mToolbarPositions, mToolbarPositionsTitles).setDefaultValue("1"));
		mOptionsPage.add(new BoolOption(this, getString(R.string.options_view_toolbar_hide_in_fullscreen), PROP_TOOLBAR_HIDE_IN_FULLSCREEN).setDefaultValue("0"));
		mOptionsPage.add(new ListOption(this, getString(R.string.options_view_toolbar_appearance), PROP_TOOLBAR_APPEARANCE).
				add(mToolbarApperance, mToolbarApperanceTitles).setDefaultValue("0"));
		mOptionsPage.add(new ListOption(this, getString(R.string.options_view_mode), PROP_PAGE_VIEW_MODE).add(mViewModes, mViewModeTitles).setDefaultValue("1").setIconIdByAttr(R.attr.cr3_option_view_mode_scroll_drawable, R.drawable.cr3_option_view_mode_scroll)
				.setOnChangeHandler(() -> {
					int value = mProperties.getInt(PROP_PAGE_VIEW_MODE, 1);
					mFootNotesOption.setEnabled(value == 1);
				})
		);
		//mOptionsPage.add(new ListOption(getString(R.string.options_page_orientation), PROP_ROTATE_ANGLE).add(mOrientations, mOrientationsTitles).setDefaultValue("0"));
		if (DeviceInfo.getSDKLevel() >= 9)
			mOptionsPage.add(new ListOption(this, getString(R.string.options_page_orientation), PROP_APP_SCREEN_ORIENTATION).add(mOrientations_API9, mOrientationsTitles_API9).setDefaultValue("0").setIconIdByAttr(R.attr.cr3_option_page_orientation_landscape_drawable, R.drawable.cr3_option_page_orientation_landscape));
		else
			mOptionsPage.add(new ListOption(this, getString(R.string.options_page_orientation), PROP_APP_SCREEN_ORIENTATION).add(mOrientations, mOrientationsTitles).setDefaultValue("0").setIconIdByAttr(R.attr.cr3_option_page_orientation_landscape_drawable, R.drawable.cr3_option_page_orientation_landscape));
		mOptionsPage.add(new ListOption(this, getString(R.string.options_page_landscape_pages), PROP_LANDSCAPE_PAGES).add(mLandscapePages, mLandscapePagesTitles).setDefaultValue("1").setIconIdByAttr(R.attr.cr3_option_pages_two_drawable, R.drawable.cr3_option_pages_two));
		mOptionsPage.add(new NightModeOption(this, getString(R.string.options_inverse_view), PROP_NIGHT_MODE).setIconIdByAttr(R.attr.cr3_option_night_drawable, R.drawable.cr3_option_night));
		mOptionsPage.add(new ColorOption(this, getString(R.string.options_color_text), PROP_FONT_COLOR, 0x000000).setIconId(R.drawable.cr3_option_font_color));
		mOptionsPage.add(new ColorOption(this, getString(R.string.options_color_background), PROP_BACKGROUND_COLOR, 0xFFFFFF).setIconId(R.drawable.cr3_option_background_color));
		if ( !DeviceInfo.EINK_SCREEN )
			mOptionsPage.add(new TextureOptions(this, getString(R.string.options_background_texture)).setIconId(R.drawable.cr3_option_background_image));
		if ( DeviceInfo.EINK_SCREEN_UPDATE_MODES_SUPPORTED ) {
			ListOption optionMode;
			if ( DeviceInfo.EINK_ONYX ) {
				optionMode = new ListOption(this, getString(R.string.options_screen_update_mode), PROP_APP_SCREEN_UPDATE_MODE);
				if (DeviceInfo.EINK_SCREEN_REGAL)
					optionMode = optionMode.add(mOnyxScreenUpdateModes[0], mOnyxScreenUpdateModesTitles[0]);
				optionMode = optionMode.add(mOnyxScreenUpdateModes[1], mOnyxScreenUpdateModesTitles[1]);
				optionMode = optionMode.add(mOnyxScreenUpdateModes[2], mOnyxScreenUpdateModesTitles[2]);
				optionMode = optionMode.add(mOnyxScreenUpdateModes[3], mOnyxScreenUpdateModesTitles[3]);
				mOptionsPage.add(optionMode.setDefaultValue(String.valueOf(DeviceInfo.EINK_SCREEN_REGAL ? EinkScreen.EinkUpdateMode.Regal.code : EinkScreen.EinkUpdateMode.Clear.code)).setDisabledNote(getString(R.string.options_eink_app_optimization_enabled)));
			} else {
				optionMode = new ListOption(this, getString(R.string.options_screen_update_mode), PROP_APP_SCREEN_UPDATE_MODE).add(mScreenUpdateModes, mScreenUpdateModesTitles);
				mOptionsPage.add(optionMode.setDefaultValue(String.valueOf(EinkScreen.EinkUpdateMode.Clear.code)).setDisabledNote(getString(R.string.options_eink_app_optimization_enabled)));
			}
			ListOption optionInterval = new ListOption(this, getString(R.string.options_screen_update_interval), PROP_APP_SCREEN_UPDATE_INTERVAL).add("0", getString(R.string.options_screen_update_interval_none)).add(mScreenFullUpdateInterval);
			mOptionsPage.add(optionInterval.setDefaultValue("10").setDisabledNote(getString(R.string.options_eink_app_optimization_enabled)));
			optionMode.setEnabled(!activity.getEinkScreen().isAppOptimizationEnabled());
			optionInterval.setEnabled(!activity.getEinkScreen().isAppOptimizationEnabled());
		}

		mOptionsPage.add(new StatusBarOption(this, getString(R.string.options_page_titlebar)));
		mFootNotesOption = new BoolOption(this, getString(R.string.options_page_footnotes), PROP_FOOTNOTES).setDefaultValue("1");
		int value = mProperties.getInt(PROP_PAGE_VIEW_MODE, 1);
		mFootNotesOption.enabled = value == 1;
		mOptionsPage.add(mFootNotesOption);
		if ( !DeviceInfo.EINK_SCREEN )
			mOptionsPage.add(new ListOption(this, getString(R.string.options_page_animation), PROP_PAGE_ANIMATION).add(mAnimation, mAnimationTitles).setDefaultValue("1").noIcon());
		mOptionsPage.add(new ListOption(this, getString(R.string.options_view_bookmarks_highlight), PROP_APP_HIGHLIGHT_BOOKMARKS).add(mHighlightMode, mHighlightModeTitles).setDefaultValue("1").noIcon());
		if ( !DeviceInfo.EINK_SCREEN ) {
			mOptionsPage.add(new ColorOption(this, getString(R.string.options_view_color_selection), PROP_HIGHLIGHT_SELECTION_COLOR, 0xCCCCCC).noIcon());
			mOptionsPage.add(new ColorOption(this, getString(R.string.options_view_color_bookmark_comment), PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT, 0xFFFF40).noIcon());
			mOptionsPage.add(new ColorOption(this, getString(R.string.options_view_color_bookmark_correction), PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION, 0xFF8000).noIcon());
		}

		mOptionsPage.add(new ListOption(this, getString(R.string.options_page_margin_left), PROP_PAGE_MARGIN_LEFT).add(mMargins).setDefaultValue("5").setIconIdByAttr(R.attr.cr3_option_text_margin_left_drawable, R.drawable.cr3_option_text_margin_left));
		mOptionsPage.add(new ListOption(this, getString(R.string.options_page_margin_right), PROP_PAGE_MARGIN_RIGHT).add(mMargins).setDefaultValue("5").setIconIdByAttr(R.attr.cr3_option_text_margin_right_drawable, R.drawable.cr3_option_text_margin_right));
		mOptionsPage.add(new ListOption(this, getString(R.string.options_page_margin_top), PROP_PAGE_MARGIN_TOP).add(mMargins).setDefaultValue("5").setIconIdByAttr(R.attr.cr3_option_text_margin_top_drawable, R.drawable.cr3_option_text_margin_top));
		mOptionsPage.add(new ListOption(this, getString(R.string.options_page_margin_bottom), PROP_PAGE_MARGIN_BOTTOM).add(mMargins).setDefaultValue("5").setIconIdByAttr(R.attr.cr3_option_text_margin_bottom_drawable, R.drawable.cr3_option_text_margin_bottom));

		mOptionsControls = new OptionsListView(getContext());
		mOptionsControls.add(new KeyMapOption(this, getString(R.string.options_app_key_actions)).setIconIdByAttr(R.attr.cr3_option_controls_keys_drawable, R.drawable.cr3_option_controls_keys));
		mOptionsControls.add(new TapZoneOption(this, getString(R.string.options_app_tapzones_normal), PROP_APP_TAP_ZONE_ACTIONS_TAP).setIconIdByAttr(R.attr.cr3_option_controls_tapzones_drawable, R.drawable.cr3_option_controls_tapzones));
		Runnable doubleTapOnChange = () -> {
			int type = mProperties.getInt(PROP_APP_SECONDARY_TAP_ACTION_TYPE, TAP_ACTION_TYPE_LONGPRESS);
			boolean dblText = mProperties.getBool(PROP_APP_DOUBLE_TAP_SELECTION, false);
			mBounceProtectionOption.setEnabled(type == TAP_ACTION_TYPE_LONGPRESS && !dblText);
		};
		mOptionsControls.add(new ListOption(this, getString(R.string.options_controls_tap_secondary_action_type), PROP_APP_SECONDARY_TAP_ACTION_TYPE).add(mTapSecondaryActionType, mTapSecondaryActionTypeTitles).setDefaultValue(String.valueOf(TAP_ACTION_TYPE_LONGPRESS)).setOnChangeHandler(doubleTapOnChange));
		mOptionsControls.add(new BoolOption(this, getString(R.string.options_app_double_tap_selection), PROP_APP_DOUBLE_TAP_SELECTION).setComment(getString(R.string.options_app_double_tap_selection_slowdown)).setDefaultValue("0").setIconIdByAttr(R.attr.cr3_option_touch_drawable, R.drawable.cr3_option_touch).setOnChangeHandler(doubleTapOnChange));
		mBounceProtectionOption = new ListOption(this, getString(R.string.options_controls_bonce_protection), PROP_APP_BOUNCE_TAP_INTERVAL).add(mBounceProtectionValues, mBounceProtectionTitles).setDefaultValue(String.valueOf(150));
		mOptionsControls.add(mBounceProtectionOption);
		doubleTapOnChange.run();
		if ( !DeviceInfo.EINK_SCREEN )
			mOptionsControls.add(new BoolOption(this, getString(R.string.options_controls_enable_volume_keys), PROP_CONTROLS_ENABLE_VOLUME_KEYS).setDefaultValue("1"));
		mOptionsControls.add(new BoolOption(this, getString(R.string.options_app_tapzone_hilite), PROP_APP_TAP_ZONE_HILIGHT).setDefaultValue("0").setIconIdByAttr(R.attr.cr3_option_touch_drawable, R.drawable.cr3_option_touch));
		if ( !DeviceInfo.EINK_SCREEN )
			mOptionsControls.add(new BoolOption(this, getString(R.string.options_app_trackball_disable), PROP_APP_TRACKBALL_DISABLED).setDefaultValue("0"));
		if ( !DeviceInfo.EINK_SCREEN || DeviceInfo.EINK_HAVE_FRONTLIGHT ) {
			mFlickBacklightControlOption = (ListOption) new ListOption(this, getString(R.string.options_controls_flick_brightness), PROP_APP_FLICK_BACKLIGHT_CONTROL).add(mFlickBrightness, mFlickBrightnessTitles).setDefaultValue("1");
			mOptionsControls.add(mFlickBacklightControlOption);
		}
		if (DeviceInfo.EINK_HAVE_NATURAL_BACKLIGHT) {
			Runnable onFlickChanged = () -> {
				int flick = mProperties.getInt(PROP_APP_FLICK_BACKLIGHT_CONTROL, 0);
				mFlickBacklightTogetherOption.setEnabled(flick != 0);
			};
			mFlickBacklightControlOption.setOnChangeHandler(onFlickChanged);
			mFlickBacklightTogetherOption = (BoolOption) new BoolOption(this, getString(R.string.options_controls_flick_cold_and_warm_together), PROP_APP_FLICK_BACKLIGHT_CONTROL_TOGETHER).setDefaultValue("0");
			mOptionsControls.add(mFlickBacklightTogetherOption);
			mOptionsControls.add(new ListOption(this, getString(R.string.options_controls_flick_warm), PROP_APP_FLICK_WARMLIGHT_CONTROL).add(mFlickBrightness, mFlickBrightnessTitles).setDefaultValue("2"));
			onFlickChanged.run();
		}
		mOptionsControls.add(new ListOption(this, getString(R.string.option_controls_gesture_page_flipping_enabled), PROP_APP_GESTURE_PAGE_FLIPPING).add(mPagesPerFullSwipe, mPagesPerFullSwipeTitles).setDefaultValue("1"));
		mOptionsControls.add(new ListOption(this, getString(R.string.options_selection_action), PROP_APP_SELECTION_ACTION).add(mSelectionAction, mSelectionActionTitles).setDefaultValue("0"));
		mOptionsControls.add(new ListOption(this, getString(R.string.options_multi_selection_action), PROP_APP_MULTI_SELECTION_ACTION).add(mMultiSelectionAction, mMultiSelectionActionTitles).setDefaultValue("0"));
		mOptionsControls.add(new BoolOption(this, getString(R.string.options_selection_keep_selection_after_dictionary), PROP_APP_SELECTION_PERSIST).setDefaultValue("0"));
		
		mOptionsApplication = new OptionsListView(getContext());
		mOptionsApplication.add(new LangOption(this).noIcon());
		if ( !DeviceInfo.FORCE_HC_THEME) {
			mOptionsApplication.add(new ThemeOptions(this, getString(R.string.options_app_ui_theme)).noIcon());
		}
		if ( !DeviceInfo.EINK_SCREEN ) {
			mOptionsApplication.add(new ListOption(this, getString(R.string.options_app_backlight_timeout), PROP_APP_SCREEN_BACKLIGHT_LOCK).add(mBacklightTimeout, mBacklightTimeoutTitles).setDefaultValue("3").noIcon());
			mBacklightLevelsTitles[0] = getString(R.string.options_app_backlight_screen_default);
			mOptionsApplication.add(new ListOption(this, getString(R.string.options_app_backlight_screen), PROP_APP_SCREEN_BACKLIGHT).add(mBacklightLevels, mBacklightLevelsTitles).setDefaultValue("-1").noIcon());
		}
		mOptionsApplication.add(new ListOption(this, getString(R.string.options_app_tts_stop_motion_timeout), PROP_APP_MOTION_TIMEOUT).add(mMotionTimeouts, mMotionTimeoutsTitles).setDefaultValue(Integer.toString(mMotionTimeouts[0])).noIcon());
		mOptionsApplication.add(new BoolOption(this, getString(R.string.options_app_key_backlight_off), PROP_APP_KEY_BACKLIGHT_OFF).setDefaultValue("1").noIcon());
		mOptionsApplication.add(new IconsBoolOption(this, getString(R.string.options_app_settings_icons), PROP_APP_SETTINGS_SHOW_ICONS).setDefaultValue("1").noIcon());
		mOptionsApplication.add(new DictOptions(this, getString(R.string.options_app_dictionary)).noIcon());
		mOptionsApplication.add(new DictOptions2(this, getString(R.string.options_app_dictionary2)).noIcon());
		mOptionsApplication.add(new BoolOption(this, getString(R.string.options_app_show_cover_pages), PROP_APP_SHOW_COVERPAGES).noIcon());
		mOptionsApplication.add(new ListOption(this, getString(R.string.options_app_cover_page_size), PROP_APP_COVERPAGE_SIZE).add(mCoverPageSizes, mCoverPageSizeTitles).setDefaultValue("1").noIcon());
		mOptionsApplication.add(new BoolOption(this, getString(R.string.options_app_scan_book_props), PROP_APP_BOOK_PROPERTY_SCAN_ENABLED).setDefaultValue("1").noIcon());
		mOptionsApplication.add(new BoolOption(this, getString(R.string.options_app_browser_hide_empty_dirs), PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS).setComment(getString(R.string.options_hide_empty_dirs_slowdown)).setDefaultValue("0").noIcon());
		mOptionsApplication.add(new BoolOption(this, getString(R.string.options_app_browser_hide_empty_genres), PROP_APP_FILE_BROWSER_HIDE_EMPTY_GENRES).setDefaultValue("0").noIcon());
		mOptionsApplication.add(new BoolOption(this, getString(R.string.mi_book_browser_simple_mode), PROP_APP_FILE_BROWSER_SIMPLE_MODE).noIcon());
		/*
		  Commented until the appearance of free implementation of the binding to the Google Drive (R)
		if (BuildConfig.GSUITE_AVAILABLE && DeviceInfo.getSDKLevel() >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			mOptionsCloudSync = new OptionsListView(getContext());
			Runnable onGoogleDriveEnable = () -> {
				boolean syncEnabled = mProperties.getBool(PROP_APP_CLOUDSYNC_GOOGLEDRIVE_ENABLED, false);
				boolean syncBookInfoEnabled = mProperties.getBool(PROP_APP_CLOUDSYNC_GOOGLEDRIVE_CURRENTBOOK_INFO, false);
				mCloudSyncAskConfirmationsOption.setEnabled(syncEnabled);
				mGoogleDriveEnableSettingsOption.setEnabled(syncEnabled);
				mGoogleDriveEnableBookmarksOption.setEnabled(syncEnabled);
				mGoogleDriveEnableCurrentBookInfoOption.setEnabled(syncEnabled);
				mGoogleDriveEnableCurrentBookBodyOption.setEnabled(syncEnabled && syncBookInfoEnabled);
				mGoogleDriveAutoSavePeriodOption.setEnabled(syncEnabled);
				// mCloudSyncBookmarksKeepAliveOptions should be enabled regardless of PROP_APP_CLOUDSYNC_GOOGLEDRIVE_ENABLED
			};
			mOptionsCloudSync.add(new BoolOption(this, getString(R.string.options_app_googledrive_sync_auto), PROP_APP_CLOUDSYNC_GOOGLEDRIVE_ENABLED).setDefaultValue("0").noIcon()
				.setOnChangeHandler(onGoogleDriveEnable));
			mCloudSyncAskConfirmationsOption = new BoolOption(this, getString(R.string.options_app_cloudsync_confirmations), PROP_APP_CLOUDSYNC_CONFIRMATIONS).setDefaultValue("1").noIcon();
			mGoogleDriveEnableSettingsOption = new BoolOption(this, getString(R.string.options_app_googledrive_sync_settings), PROP_APP_CLOUDSYNC_GOOGLEDRIVE_SETTINGS).setDefaultValue("0").noIcon();
			mGoogleDriveEnableBookmarksOption = new BoolOption(this, getString(R.string.options_app_googledrive_sync_bookmarks), PROP_APP_CLOUDSYNC_GOOGLEDRIVE_BOOKMARKS).setDefaultValue("0").noIcon();
			mGoogleDriveEnableCurrentBookInfoOption = new BoolOption(this, getString(R.string.options_app_googledrive_sync_currentbook_info), PROP_APP_CLOUDSYNC_GOOGLEDRIVE_CURRENTBOOK_INFO).setDefaultValue("0").noIcon();
			mGoogleDriveEnableCurrentBookInfoOption.setOnChangeHandler(() -> {
				boolean syncBookInfoEnabled = mProperties.getBool(PROP_APP_CLOUDSYNC_GOOGLEDRIVE_CURRENTBOOK_INFO, false);
				mGoogleDriveEnableCurrentBookBodyOption.setEnabled(syncBookInfoEnabled);
			});
			mGoogleDriveEnableCurrentBookBodyOption = new BoolOption(this, getString(R.string.options_app_googledrive_sync_currentbook_body), PROP_APP_CLOUDSYNC_GOOGLEDRIVE_CURRENTBOOK_BODY).setDefaultValue("0").noIcon();
			mGoogleDriveEnableCurrentBookInfoOption.onChangeHandler.run();
			mGoogleDriveAutoSavePeriodOption = new ListOption(this, getString(R.string.autosave_period), PROP_APP_CLOUDSYNC_GOOGLEDRIVE_AUTOSAVEPERIOD).add(mGoogleDriveAutoSavePeriod, mGoogleDriveAutoSavePeriodTitles).setDefaultValue(Integer.valueOf(5).toString()).noIcon();
			mCloudSyncDataKeepAliveOptions = new ListOption(this, getString(R.string.sync_data_keepalive_), PROP_APP_CLOUDSYNC_DATA_KEEPALIVE).add(mCloudBookmarksKeepAlive, mCloudBookmarksKeepAliveTitles).setDefaultValue(Integer.valueOf(14).toString()).noIcon();
			onGoogleDriveEnable.run();
			mOptionsCloudSync.add(mCloudSyncAskConfirmationsOption);
			mOptionsCloudSync.add(mGoogleDriveEnableSettingsOption);
			mOptionsCloudSync.add(mGoogleDriveEnableBookmarksOption);
			mOptionsCloudSync.add(mGoogleDriveEnableCurrentBookInfoOption);
			mOptionsCloudSync.add(mGoogleDriveEnableCurrentBookBodyOption);
			mOptionsCloudSync.add(mGoogleDriveAutoSavePeriodOption);
			mOptionsCloudSync.add(mCloudSyncDataKeepAliveOptions);
		}
		 */

		fillStyleEditorOptions();

		mOptionsStyles.refresh();
		mOptionsCSS.refresh();
		mOptionsPage.refresh();
		mOptionsApplication.refresh();
		if (null != mOptionsCloudSync) {
			mOptionsCloudSync.refresh();
		}

		addTab("Styles", R.drawable.cr3_tab_style);
		addTab("CSS", R.drawable.cr3_tab_css);
		addTab("Page", R.drawable.cr3_tab_page);
		addTab("Controls", R.drawable.cr3_tab_controls);
		addTab("App", R.drawable.cr3_tab_application);
		if (null != mOptionsCloudSync) {
			addTab("Clouds", R.drawable.cr3_tab_clouds);
		}

		setView(mTabs);
	}

	private void addTab(String name, int imageDrawable) {
		TabHost.TabSpec ts = mTabs.newTabSpec(name);
		Drawable icon = getContext().getResources().getDrawable(imageDrawable);
		if (Build.VERSION.SDK_INT > Build.VERSION_CODES.HONEYCOMB) {
			// replace too small icons in tabs in Theme.Holo
			View tabIndicator = mInflater.inflate(R.layout.tab_indicator, null);
			ImageView imageView = tabIndicator.findViewById(R.id.tab_icon);
			imageView.setImageDrawable(icon);
			ts.setIndicator(tabIndicator);
		} else {
			ts.setIndicator("", icon);
		}
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

		mMotionTimeoutsTitles = activity.getResources().getStringArray(R.array.motion_timeout_titles);
        mMotionTimeouts = activity.getResources().getIntArray(R.array.motion_timeout_values);

		mPagesPerFullSwipeTitles = activity.getResources().getStringArray(R.array.pages_per_full_swipe_titles);
		mPagesPerFullSwipe = activity.getResources().getIntArray(R.array.pages_per_full_swipe_values);

        switch (mode) {
			case READER:
				setupReaderOptions();
				break;
			case BROWSER:
				setupBrowserOptions();
				break;
			case TTS:
				setupTTSOptions();
				break;
		}
        
		setOnCancelListener(dialog -> onPositiveButtonClick());

		ImageButton positiveButton = view.findViewById(R.id.options_btn_back);
		positiveButton.setOnClickListener(v -> onPositiveButtonClick());
		
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
			int domVersion = mProperties.getInt(PROP_REQUESTED_DOM_VERSION, Engine.DOM_VERSION_CURRENT);
			if (domVersion != mReaderView.getDOMVersion()) {
				mReaderView.setDOMVersion(domVersion);
			}
			int rendFlags = mProperties.getInt(PROP_RENDER_BLOCK_RENDERING_FLAGS, Engine.BLOCK_RENDERING_FLAGS_WEB);
			if (rendFlags != mReaderView.getBlockRenderingFlags()) {
				mReaderView.setBlockRenderingFlags(rendFlags);
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
		if (KeyEvent.KEYCODE_BACK == keyCode) {
			return true;
		}
		if (mode == Mode.READER) {
	        if (mTabs.getCurrentView().onKeyDown(keyCode, event))
	        	return true;
		} else {
	        if (view.onKeyDown(keyCode, event))
	        	return true;
		}
        return super.onKeyDown(keyCode, event);
    }

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if (KeyEvent.KEYCODE_BACK == keyCode) {
			onPositiveButtonClick();
			return true;
		}
		return super.onKeyUp(keyCode, event);
	}

}
