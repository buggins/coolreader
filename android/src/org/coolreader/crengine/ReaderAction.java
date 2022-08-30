/*
 * CoolReader for Android
 * Copyright (C) 2010-2014 Vadim Lopatin <coolreader.org@gmail.com>
 * Copyright (C) 2012 Jasper Poppe <jpoppe@ebay.com>
 * Copyright (C) 2018 Yuri Plotnikov <plotnikovya@gmail.com>
 * Copyright (C) 2018,2020,2021 Aleksey Chernov <valexlin@gmail.com>
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

import java.util.ArrayList;

import org.coolreader.R;

public class ReaderAction {
	final public String id;
	final public int nameId;
	public int    iconId;
	final public ReaderCommand cmd;
	final public int param;
	final public int menuItemId;
	private boolean canRepeat = false;
	private boolean mayAssignOnKey = true;
	private boolean mayAssignOnTap = true;
	private boolean activateWithLongMenuKey = false;
	private ReaderAction setActivateWithLongMenuKey() { this.activateWithLongMenuKey = true; return this; }
	public ReaderAction setIconId(int iconId) { this.iconId = iconId; return this; }
	private ReaderAction setCanRepeat() { canRepeat = true; return this; }
	//private ReaderAction dontAssignOnKey() { mayAssignOnKey=false; return this; }
	private ReaderAction dontAssignOnTap() { mayAssignOnTap = false; return this; }
	public boolean canRepeat() { return canRepeat; }
	public boolean mayAssignOnKey() { return mayAssignOnKey; }
	public boolean mayAssignOnTap() { return mayAssignOnTap; }
	public boolean activateWithLongMenuKey() { return activateWithLongMenuKey; }

	public ReaderAction(String id, int nameId, ReaderCommand cmd, int param) {
		super();
		this.id = id;
		this.nameId = nameId;
		this.cmd = cmd;
		this.param = param;
		this.menuItemId = 0;
		this.iconId = 0;
	}

	public ReaderAction(String id, int nameId, ReaderCommand cmd, int param, int menuItemId) {
		super();
		this.id = id;
		this.nameId = nameId;
		this.cmd = cmd;
		this.param = param;
		this.menuItemId = menuItemId;
		this.iconId = 0;
	}

	public String toString() {
		return id;
	}
	
	public final static ReaderAction NONE = new ReaderAction("NONE", R.string.action_none, ReaderCommand.DCMD_NONE, 0 );
	public final static ReaderAction REPEAT = new ReaderAction("REPEAT", R.string.action_repeat, ReaderCommand.DCMD_REPEAT, 0 );
	public final static ReaderAction PAGE_DOWN = new ReaderAction("PAGE_DOWN", R.string.action_pagedown, ReaderCommand.DCMD_PAGEDOWN, 1 ).setCanRepeat();
	public final static ReaderAction PAGE_DOWN_10 = new ReaderAction("PAGE_DOWN_10", R.string.action_pagedown_10, ReaderCommand.DCMD_PAGEDOWN, 10 ).setCanRepeat();
	public final static ReaderAction PAGE_UP = new ReaderAction("PAGE_UP", R.string.action_pageup, ReaderCommand.DCMD_PAGEUP, 1 ).setCanRepeat();
	public final static ReaderAction PAGE_UP_10 = new ReaderAction("PAGE_UP_10", R.string.action_pageup_10, ReaderCommand.DCMD_PAGEUP, 10 ).setCanRepeat();
	public final static ReaderAction ZOOM_IN = new ReaderAction("ZOOM_IN", R.string.mi_font_size_increase, ReaderCommand.DCMD_ZOOM_IN, 1); //,  R.id.cr3_mi_font_size_increase 
	public final static ReaderAction ZOOM_OUT = new ReaderAction("ZOOM_OUT", R.string.mi_font_size_decrease, ReaderCommand.DCMD_ZOOM_OUT, 1); //,  R.id.cr3_mi_font_size_decrease 
	public final static ReaderAction DOCUMENT_STYLES = new ReaderAction("DOCUMENT_STYLES", R.string.action_toggle_document_styles, ReaderCommand.DCMD_TOGGLE_DOCUMENT_STYLES, 0, R.id.cr3_mi_toggle_document_styles );
	public final static ReaderAction TEXT_AUTOFORMAT = new ReaderAction("TEXT_AUTOFORMAT", R.string.action_toggle_text_autoformat, ReaderCommand.DCMD_TOGGLE_TEXT_AUTOFORMAT, 0, R.id.cr3_mi_toggle_text_autoformat );
	public final static ReaderAction BOOKMARKS = new ReaderAction("BOOKMARKS", R.string.action_bookmarks, ReaderCommand.DCMD_BOOKMARKS, 0, R.id.cr3_mi_bookmarks ).setIconId(R.drawable.cr3_button_bookmarks);
	public final static ReaderAction NEW_BOOKMARK_PAGE = new ReaderAction("NEW_BOOKMARKS_PAGE", R.string.add_bookmark_to_page, ReaderCommand.DCMD_NEW_BOOKMARK, 0);
	public final static ReaderAction ABOUT = new ReaderAction("ABOUT", R.string.dlg_about, ReaderCommand.DCMD_ABOUT, 0, R.id.cr3_mi_about ).setIconId(R.drawable.cr3_logo_button);
	public final static ReaderAction USER_MANUAL = new ReaderAction("USER_MANUAL", R.string.mi_goto_manual, ReaderCommand.DCMD_USER_MANUAL, 0, R.id.cr3_mi_user_manual );
	public final static ReaderAction BOOK_INFO = new ReaderAction("BOOK_INFO", R.string.dlg_book_info, ReaderCommand.DCMD_BOOK_INFO, 0, R.id.cr3_mi_book_info );
	public final static ReaderAction TOC = new ReaderAction("TOC", R.string.action_toc, ReaderCommand.DCMD_TOC_DIALOG, 0, R.id.cr3_go_toc ).setIconId(R.drawable.cr3_viewer_toc);
	public final static ReaderAction SEARCH = new ReaderAction("SEARCH", R.string.action_search, ReaderCommand.DCMD_SEARCH, 0, R.id.cr3_mi_search ).setIconId(R.drawable.cr3_viewer_find);
	public final static ReaderAction GO_PAGE = new ReaderAction("GO_PAGE", R.string.action_go_page, ReaderCommand.DCMD_GO_PAGE_DIALOG, 0, R.id.cr3_mi_go_page ).setIconId(R.drawable.cr3_button_go_page);
	public final static ReaderAction GO_PERCENT = new ReaderAction("GO_PERCENT", R.string.action_go_percent, ReaderCommand.DCMD_GO_PERCENT_DIALOG, 0, R.id.cr3_mi_go_percent ).setIconId(R.drawable.cr3_button_go_percent);
	public final static ReaderAction FIRST_PAGE = new ReaderAction("FIRST_PAGE", R.string.action_go_first_page, ReaderCommand.DCMD_BEGIN, 0 );
	public final static ReaderAction LAST_PAGE = new ReaderAction("LAST_PAGE", R.string.action_go_last_page, ReaderCommand.DCMD_END, 0 );
	public final static ReaderAction OPTIONS = new ReaderAction("OPTIONS", R.string.action_options, ReaderCommand.DCMD_OPTIONS_DIALOG, 0, R.id.cr3_mi_options ).setIconId(R.drawable.cr3_viewer_settings);
	public final static ReaderAction READER_MENU = new ReaderAction("READER_MENU", R.string.action_reader_menu, ReaderCommand.DCMD_READER_MENU, 0 );
	public final static ReaderAction TOGGLE_DAY_NIGHT = new ReaderAction("TOGGLE_DAY_NIGHT", R.string.action_toggle_day_night, ReaderCommand.DCMD_TOGGLE_DAY_NIGHT_MODE, 0, R.id.cr3_mi_toggle_day_night ).setIconId(R.drawable.cr3_option_night);
	public final static ReaderAction RECENT_BOOKS = new ReaderAction("RECENT_BOOKS", R.string.action_recent_books_list, ReaderCommand.DCMD_RECENT_BOOKS_LIST, R.id.book_recent_books ).setIconId(R.drawable.cr3_browser_folder_recent);
	public final static ReaderAction OPDS_CATALOGS = new ReaderAction("OPDS_CATALOGS", R.string.mi_book_opds_root, ReaderCommand.DCMD_OPDS_CATALOGS, 0).setIconId(R.drawable.cr3_browser_folder_opds);
	public final static ReaderAction FILE_BROWSER_ROOT = new ReaderAction("FILE_BROWSER_ROOT", R.string.mi_book_root, ReaderCommand.DCMD_FILE_BROWSER_ROOT, 0).setIconId(R.drawable.cr3_browser_folder_root);
	public final static ReaderAction FILE_BROWSER = new ReaderAction("FILE_BROWSER", R.string.action_file_browser, ReaderCommand.DCMD_FILE_BROWSER, 0, R.id.cr3_mi_open_file ).setIconId(R.drawable.cr3_browser_folder);
	public final static ReaderAction FILE_BROWSER_UP = new ReaderAction("FILE_BROWSER_UP", R.string.action_go_back, ReaderCommand.DCMD_FILE_BROWSER_UP, 0).setIconId(R.drawable.cr3_button_prev);
	public final static ReaderAction CURRENT_BOOK_DIRECTORY = new ReaderAction("DCMD_CURRENT_BOOK_DIRECTORY", R.string.mi_book_recent_goto, ReaderCommand.DCMD_CURRENT_BOOK_DIRECTORY, 0).setIconId(R.drawable.cr3_browser_folder_current_book);
	public final static ReaderAction CURRENT_BOOK = new ReaderAction("DCMD_CURRENT_BOOK", R.string.mi_book_back_to_reading, ReaderCommand.DCMD_CURRENT_BOOK, 0).setIconId(R.drawable.cr3_button_book_open);
	public final static ReaderAction FILE_BROWSER_SORT_ORDER = new ReaderAction("FILE_BROWSER_SORT_ORDER", R.string.mi_book_sort_order, ReaderCommand.DCMD_FILE_BROWSER_SORT_ORDER, 0);
	public final static ReaderAction TOGGLE_DICT_ONCE = new ReaderAction("TOGGLE_DICT_ONCE", R.string.toggle_dict_once, ReaderCommand.DCMD_TOGGLE_DICT_ONCE, 0);
	public final static ReaderAction TOGGLE_DICT = new ReaderAction("TOGGLE_DICT", R.string.toggle_dict, ReaderCommand.DCMD_TOGGLE_DICT, 0);

	public final static ReaderAction FONT_PREVIOUS = new ReaderAction("FONT_PREVIOUS", R.string.mi_font_previous, ReaderCommand.DCMD_FONT_PREVIOUS, 0); //, R.id.cr3_mi_font_previous 
	public final static ReaderAction FONT_NEXT = new ReaderAction("FONT_NEXT", R.string.mi_font_next, ReaderCommand.DCMD_FONT_NEXT, 0); //, R.id.cr3_mi_font_next 	
	public final static ReaderAction TOGGLE_TOUCH_SCREEN_LOCK = new ReaderAction("TOGGLE_TOUCH_SCREEN_LOCK", R.string.action_touch_screen_toggle_lock, ReaderCommand.DCMD_TOGGLE_TOUCH_SCREEN_LOCK, 0 ).dontAssignOnTap();
	public final static ReaderAction TOGGLE_ORIENTATION = new ReaderAction("TOGGLE_ORIENTATION", R.string.action_toggle_screen_orientation, ReaderCommand.DCMD_TOGGLE_ORIENTATION, 0 );
	public final static ReaderAction TOGGLE_FULLSCREEN = new ReaderAction("TOGGLE_FULLSCREEN", R.string.action_toggle_fullscreen, ReaderCommand.DCMD_TOGGLE_FULLSCREEN, 0 );
	public final static ReaderAction TOGGLE_SELECTION_MODE = new ReaderAction("TOGGLE_SELECTION_MODE", R.string.action_toggle_selection_mode, ReaderCommand.DCMD_TOGGLE_SELECTION_MODE, 0, R.id.cr3_mi_select_text).setIconId(R.drawable.cr3_option_touch);
	public final static ReaderAction HOME_SCREEN = new ReaderAction("HOME_SCREEN", R.string.action_exit_home_screen, ReaderCommand.DCMD_SHOW_HOME_SCREEN, 0 );
	public final static ReaderAction GO_BACK = new ReaderAction("GO_BACK", R.string.action_go_back, ReaderCommand.DCMD_LINK_BACK, 0, R.id.cr3_go_back ).setIconId(R.drawable.cr3_button_prev);
	public final static ReaderAction GO_FORWARD = new ReaderAction("GO_FORWARD", R.string.action_go_forward, ReaderCommand.DCMD_LINK_FORWARD, 0, R.id.cr3_go_forward).setIconId(R.drawable.cr3_button_next);
	public final static ReaderAction TTS_PLAY = new ReaderAction("TTS_PLAY", R.string.mi_tts_play, ReaderCommand.DCMD_TTS_PLAY, 0, R.id.cr3_mi_tts_play ).setIconId(R.drawable.cr3_button_tts); //.setActivateWithLongMenuKey()
	public final static ReaderAction TTS_STOP = new ReaderAction("TTS_STOP", R.string.mi_tts_stop, ReaderCommand.DCMD_TTS_STOP, 0, R.id.cr3_mi_tts_stop ).setIconId(R.drawable.cr3_button_tts); //.setActivateWithLongMenuKey()
	public final static ReaderAction TOGGLE_TITLEBAR = new ReaderAction("TOGGLE_TITLEBAR", R.string.action_toggle_titlebar, ReaderCommand.DCMD_TOGGLE_TITLEBAR, 0 );
	public final static ReaderAction SHOW_POSITION_INFO_POPUP = new ReaderAction("SHOW_POSITION_INFO_POPUP", R.string.action_show_position_info, ReaderCommand.DCMD_SHOW_POSITION_INFO_POPUP, 0 );
	public final static ReaderAction SHOW_DICTIONARY = new ReaderAction("SHOW_DICTIONARY", R.string.action_show_dictionary, ReaderCommand.DCMD_SHOW_DICTIONARY, 0);
	public final static ReaderAction OPEN_PREVIOUS_BOOK = new ReaderAction("OPEN_PREVIOUS_BOOK", R.string.action_open_last_book, ReaderCommand.DCMD_OPEN_PREVIOUS_BOOK, 0, R.id.cr3_go_previous_book).setIconId(R.drawable.cr3_btn_books_swap);
	public final static ReaderAction TOGGLE_AUTOSCROLL = new ReaderAction("TOGGLE_AUTOSCROLL", R.string.action_toggle_autoscroll, ReaderCommand.DCMD_TOGGLE_AUTOSCROLL, 0, R.id.cr3_mi_toggle_autoscroll).setIconId(R.drawable.cr3_button_scroll_go);
	public final static ReaderAction AUTOSCROLL_SPEED_INCREASE = new ReaderAction("AUTOSCROLL_SPEED_INCREASE", R.string.action_autoscroll_speed_increase, ReaderCommand.DCMD_AUTOSCROLL_SPEED_INCREASE, 0);
	public final static ReaderAction AUTOSCROLL_SPEED_DECREASE = new ReaderAction("AUTOSCROLL_SPEED_DECREASE", R.string.action_autoscroll_speed_decrease, ReaderCommand.DCMD_AUTOSCROLL_SPEED_DECREASE, 0);
	public final static ReaderAction START_SELECTION = new ReaderAction("START_SELECTION", R.string.action_toggle_selection_mode, ReaderCommand.DCMD_START_SELECTION, 0);
	public final static ReaderAction SWITCH_PROFILE = new ReaderAction("SWITCH_PROFILE", R.string.action_switch_settings_profile, ReaderCommand.DCMD_SWITCH_PROFILE, 0);
	public final static ReaderAction SCAN_DIRECTORY_RECURSIVE = new ReaderAction("SCAN_DIRECTORY_RECURSIVE", R.string.mi_book_scan_recursive, ReaderCommand.DCMD_SCAN_DIRECTORY_RECURSIVE, 0);
	public final static ReaderAction NEXT_CHAPTER = new ReaderAction("NEXT_CHAPTER", R.string.action_chapter_next, ReaderCommand.DCMD_MOVE_BY_CHAPTER, 1);
	public final static ReaderAction PREV_CHAPTER = new ReaderAction("PREV_CHAPTER", R.string.action_chapter_prev, ReaderCommand.DCMD_MOVE_BY_CHAPTER, -1);
	public final static ReaderAction SAVE_LOGCAT = new ReaderAction("SAVE_LOGCAT", R.string.action_logcat, ReaderCommand.DCMD_SAVE_LOGCAT, 0).setIconId(R.drawable.cr3_button_log);
	public final static ReaderAction EXIT = new ReaderAction("EXIT", R.string.action_exit, ReaderCommand.DCMD_EXIT, 0, R.id.cr3_mi_exit ).setIconId(R.drawable.cr3_viewer_exit);

	public final static ReaderAction BACKLIGHT_SET_DEFAULT = new ReaderAction("BACKLIGHT_SET_DEFAULT", R.string.action_backlight_set_default, ReaderCommand.DCMD_BACKLIGHT_SET_DEFAULT, -1);
	public final static ReaderAction SHOW_SYSTEM_BACKLIGHT_DIALOG = new ReaderAction("SHOW_SYSTEM_BACKLIGHT_DIALOG", R.string.action_show_onyx_backlight_system_dialog, ReaderCommand.DCMD_SHOW_SYSTEM_BACKLIGHT_DIALOG, -1);

	/*
	  Commented until the appearance of free implementation of the binding to the Google Drive (R)
	public final static ReaderAction GDRIVE_SYNCTO = new ReaderAction("GDRIVE_SYNCTO", R.string.googledrive_sync_to, ReaderCommand.DCMD_GOOGLEDRIVE_SYNC, 0).setIconId(R.drawable.google_drive);
	public final static ReaderAction GDRIVE_SYNCFROM = new ReaderAction("GDRIVE_SYNCFROM", R.string.googledrive_sync_from, ReaderCommand.DCMD_GOOGLEDRIVE_SYNC, 1).setIconId(R.drawable.google_drive);
	 */

	public final static ReaderAction[] AVAILABLE_ACTIONS;

	public boolean isNone() {
		return cmd == NONE.cmd;
	}
	
	public boolean isRepeat() {
		return cmd == REPEAT.cmd;
	}
	
	public static ReaderAction findById( String id ) {
		if ( id==null )
			return NONE;
		for ( ReaderAction a : AVAILABLE_ACTIONS ) {
			if ( id.equals(a.id) )
				return a;
		}
		if ( id.equals(REPEAT.id) )
			return REPEAT;
		return NONE;
	}
	public static ReaderAction findByMenuId( int id ) {
		if ( id==0 )
			return NONE;
		for ( ReaderAction a : AVAILABLE_ACTIONS ) {
			if ( id == a.menuItemId )
				return a;
		}
		return NONE;
	}
	public final static String NORMAL_PROP = ".";
	public final static String LONG_PROP = ".long.";
	public final static String DOUBLECLICK_PROP = ".dbl.";
	
	public final static int NORMAL = 0;
	public final static int LONG = 1;
	public final static int DOUBLE = 2;
	public final static String[] TYPE_PROP_SUBPATH = new String[] {NORMAL_PROP, LONG_PROP, DOUBLECLICK_PROP};

	public static String getTypeString( int type ) {
		return TYPE_PROP_SUBPATH[type];
	}
	
	public static String getTapZoneProp( int tapZoneNumber, int type ) {
		return ReaderView.PROP_APP_TAP_ZONE_ACTIONS_TAP + getTypeString(type) + tapZoneNumber;
	}
	public static String getKeyProp( int keyCode, int type ) {
		return ReaderView.PROP_APP_KEY_ACTIONS_PRESS + getTypeString(type) + keyCode;
	}
	public static ReaderAction findForTap( int tapZoneNumber, Properties settings ) {
		String id = settings.getProperty( getTapZoneProp( tapZoneNumber, NORMAL ) );
		return findById(id);
	}
	public static ReaderAction findForLongTap( int tapZoneNumber, Properties settings ) {
		String id = settings.getProperty( getTapZoneProp( tapZoneNumber, LONG ) );
		return findById(id);
	}
	public static ReaderAction findForDoubleTap( int tapZoneNumber, Properties settings ) {
		String id = settings.getProperty( getTapZoneProp( tapZoneNumber, DOUBLE ) );
		return findById(id);
	}
	public static ReaderAction findForKey( int keyCode, Properties settings ) {
		String id = settings.getProperty( getKeyProp( keyCode, NORMAL ) );
		return findById(id);
	}
	public static ReaderAction findForLongKey( int keyCode, Properties settings ) {
		String id = settings.getProperty( getKeyProp( keyCode, LONG ) );
		return findById(id);
	}
	public static ReaderAction findForDoubleKey( int keyCode, Properties settings ) {
		String id = settings.getProperty( getKeyProp( keyCode, DOUBLE ) );
		return findById(id);
	}
	
	public static ArrayList<ReaderAction> createList(ReaderAction ... actions) {
		ArrayList<ReaderAction> list = new ArrayList<ReaderAction>(actions.length);
		for (ReaderAction item : actions)
			list.add(item);
		return list;
	}

	static {
		ReaderAction[] BASE_ACTIONS = new ReaderAction[]{
				NONE,
				PAGE_DOWN,
				PAGE_UP,
				PAGE_DOWN_10,
				PAGE_UP_10,
				FIRST_PAGE,
				LAST_PAGE,
				NEXT_CHAPTER,
				PREV_CHAPTER,
				TOC,
				GO_PAGE,
				GO_PERCENT,
				BOOKMARKS,
				NEW_BOOKMARK_PAGE,
				SEARCH,
				OPTIONS,
				EXIT,
				TOGGLE_DAY_NIGHT,
				RECENT_BOOKS,
				FILE_BROWSER,
				FILE_BROWSER_ROOT,
				CURRENT_BOOK_DIRECTORY,
				READER_MENU,
				TOGGLE_TOUCH_SCREEN_LOCK,
				TOGGLE_SELECTION_MODE,
				TOGGLE_ORIENTATION,
				TOGGLE_FULLSCREEN,
				GO_BACK,
				GO_FORWARD,
				HOME_SCREEN,
				ZOOM_IN,
				ZOOM_OUT,
				FONT_PREVIOUS,
				FONT_NEXT,
				DOCUMENT_STYLES,
				ABOUT,
				BOOK_INFO,
				TTS_PLAY,
				TTS_STOP,
				TOGGLE_TITLEBAR,
				SHOW_POSITION_INFO_POPUP,
				SHOW_DICTIONARY,
				OPEN_PREVIOUS_BOOK,
				TOGGLE_AUTOSCROLL,
				SWITCH_PROFILE,
				TEXT_AUTOFORMAT,
				USER_MANUAL,
//				AUTOSCROLL_SPEED_INCREASE,
//				AUTOSCROLL_SPEED_DECREASE,
				TOGGLE_DICT_ONCE,
				TOGGLE_DICT,
				BACKLIGHT_SET_DEFAULT,
				SAVE_LOGCAT
		};
		/*
		  Commented until the appearance of free implementation of the binding to the Google Drive (R)
		if (DeviceInfo.getSDKLevel() >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			int count = BASE_ACTIONS.length;
			ReaderAction[] new_array = new ReaderAction[count + 2];
			System.arraycopy(BASE_ACTIONS, 0, new_array, 0, count);
			new_array[count] = GDRIVE_SYNCTO;
			new_array[count + 1] = GDRIVE_SYNCFROM;
			BASE_ACTIONS = new_array;
		}
		 */
		if (DeviceInfo.EINK_HAVE_FRONTLIGHT) {
			// TODO: and may be other eink devices with frontlight...
			if (DeviceInfo.EINK_ONYX && DeviceInfo.ONYX_HAVE_BRIGHTNESS_SYSTEM_DIALOG) {
				int count = BASE_ACTIONS.length;
				ReaderAction[] new_array = new ReaderAction[count + 1];
				System.arraycopy(BASE_ACTIONS, 0, new_array, 0, count);
				new_array[count] = SHOW_SYSTEM_BACKLIGHT_DIALOG;
				BASE_ACTIONS = new_array;
			}
		}
		AVAILABLE_ACTIONS = BASE_ACTIONS;
	}
}
