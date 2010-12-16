package org.coolreader.crengine;

import org.coolreader.R;
import org.coolreader.crengine.ReaderView.ReaderCommand;

public class ReaderAction {
	final public String id;
	final public int nameId;
	final public int    iconId;
	final public ReaderView.ReaderCommand cmd;
	final public int param;
	final public int menuItemId;
	private boolean canRepeat = false;
	private boolean mayAssignOnKey = false;
	private boolean mayAssignOnTap = false;
	private ReaderAction setCanRepeat() { canRepeat=true; return this; }
	//private ReaderAction dontAssignOnKey() { mayAssignOnKey=false; return this; }
	private ReaderAction dontAssignOnTap() { mayAssignOnTap=false; return this; }
	public boolean canRepeat() { return canRepeat; }
	public boolean mayAssignOnKey() { return mayAssignOnKey; }
	public boolean mayAssignOnTap() { return mayAssignOnTap; }
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

	public final static ReaderAction NONE = new ReaderAction("NONE", R.string.action_none, ReaderCommand.DCMD_NONE, 0 );
	public final static ReaderAction REPEAT = new ReaderAction("REPEAT", R.string.action_repeat, ReaderCommand.DCMD_REPEAT, 0 );
	public final static ReaderAction PAGE_DOWN = new ReaderAction("PAGE_DOWN", R.string.action_pagedown, ReaderCommand.DCMD_PAGEDOWN, 1 ).setCanRepeat();
	public final static ReaderAction PAGE_DOWN_10 = new ReaderAction("PAGE_DOWN_10", R.string.action_pagedown_10, ReaderCommand.DCMD_PAGEDOWN, 10 ).setCanRepeat();
	public final static ReaderAction PAGE_UP = new ReaderAction("PAGE_UP", R.string.action_pageup, ReaderCommand.DCMD_PAGEUP, 1 ).setCanRepeat();
	public final static ReaderAction PAGE_UP_10 = new ReaderAction("PAGE_UP_10", R.string.action_pageup_10, ReaderCommand.DCMD_PAGEUP, 10 ).setCanRepeat();
	public final static ReaderAction BOOKMARKS = new ReaderAction("BOOKMARKS", R.string.action_bookmarks, ReaderCommand.DCMD_BOOKMARKS, 0, R.id.cr3_mi_bookmarks );
	public final static ReaderAction TOC = new ReaderAction("TOC", R.string.action_toc, ReaderCommand.DCMD_TOC_DIALOG, 0, R.id.cr3_go_toc );
	public final static ReaderAction SEARCH = new ReaderAction("SEARCH", R.string.action_search, ReaderCommand.DCMD_SEARCH, 0, R.id.cr3_mi_search );
	public final static ReaderAction GO_PAGE = new ReaderAction("GO_PAGE", R.string.action_go_page, ReaderCommand.DCMD_GO_PAGE_DIALOG, 0, R.id.cr3_mi_go_page );
	public final static ReaderAction GO_PERCENT = new ReaderAction("GO_PERCENT", R.string.action_go_percent, ReaderCommand.DCMD_GO_PERCENT_DIALOG, 0, R.id.cr3_mi_go_percent );
	public final static ReaderAction FIRST_PAGE = new ReaderAction("FIRST_PAGE", R.string.action_go_first_page, ReaderCommand.DCMD_BEGIN, 0 );
	public final static ReaderAction LAST_PAGE = new ReaderAction("LAST_PAGE", R.string.action_go_last_page, ReaderCommand.DCMD_END, 0 );
	public final static ReaderAction OPTIONS = new ReaderAction("OPTIONS", R.string.action_options, ReaderCommand.DCMD_OPTIONS_DIALOG, 0, R.id.cr3_mi_options );
	public final static ReaderAction READER_MENU = new ReaderAction("READER_MENU", R.string.action_reader_menu, ReaderCommand.DCMD_READER_MENU, 0 );
	public final static ReaderAction TOGGLE_DAY_NIGHT = new ReaderAction("TOGGLE_DAY_NIGHT", R.string.action_toggle_day_night, ReaderCommand.DCMD_TOGGLE_DAY_NIGHT_MODE, 0 );
	public final static ReaderAction RECENT_BOOKS = new ReaderAction("RECENT_BOOKS", R.string.action_recent_books_list, ReaderCommand.DCMD_RECENT_BOOKS_LIST, 0 );
	public final static ReaderAction FILE_BROWSER = new ReaderAction("FILE_BROWSER", R.string.action_file_browser, ReaderCommand.DCMD_FILE_BROWSER, 0, R.id.cr3_mi_open_file );
	public final static ReaderAction TOGGLE_TOUCH_SCREEN_LOCK = new ReaderAction("FILE_BROWSER", R.string.action_touch_screen_toggle_lock, ReaderCommand.DCMD_TOGGLE_TOUCH_SCREEN_LOCK, 0 ).dontAssignOnTap();
	public final static ReaderAction EXIT = new ReaderAction("EXIT", R.string.action_exit, ReaderCommand.DCMD_EXIT, 0, R.id.cr3_mi_exit );
	
	
	public final static ReaderAction[] AVAILABLE_ACTIONS = {
		NONE,
		PAGE_DOWN,
		PAGE_UP,
		PAGE_DOWN_10,
		PAGE_UP_10,
		FIRST_PAGE,
		LAST_PAGE,
		TOC,
		GO_PAGE,
		GO_PERCENT,
		BOOKMARKS,
		SEARCH,
		OPTIONS,
		TOGGLE_DAY_NIGHT,
		RECENT_BOOKS,
		FILE_BROWSER,
		READER_MENU,
		TOGGLE_TOUCH_SCREEN_LOCK,
		EXIT,
	};

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
	public static ReaderAction findForTap( int tapZoneNumber, Properties settings ) {
		String id = settings.getProperty( ReaderView.PROP_APP_TAP_ZONE_ACTIONS_TAP + "." + tapZoneNumber );
		return findById(id);
	}
	public static ReaderAction findForLongTap( int tapZoneNumber, Properties settings ) {
		String id = settings.getProperty( ReaderView.PROP_APP_TAP_ZONE_ACTIONS_TAP + ".long." + tapZoneNumber );
		return findById(id);
	}
	public static ReaderAction findForKey( int keyCode, Properties settings ) {
		String id = settings.getProperty( ReaderView.PROP_APP_KEY_ACTIONS_PRESS + "." + keyCode );
		return findById(id);
	}
	public static ReaderAction findForLongKey( int keyCode, Properties settings ) {
		String id = settings.getProperty( ReaderView.PROP_APP_KEY_ACTIONS_PRESS + ".long." + keyCode );
		return findById(id);
	}
}
