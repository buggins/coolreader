package org.coolreader.crengine;

import org.coolreader.R;
import org.coolreader.crengine.ReaderView.ReaderCommand;

public class ReaderAction {
	String id;
	int nameId;
	int    iconId;
	ReaderView.ReaderCommand cmd;
	int param;
	public ReaderAction(String id, int nameId, ReaderCommand cmd, int param) {
		super();
		this.id = id;
		this.nameId = nameId;
		this.cmd = cmd;
		this.param = param;
	}

	public final static ReaderAction NONE = new ReaderAction("NONE", R.string.action_pagedown, ReaderCommand.DCMD_NONE, 0 );
	public final static ReaderAction PAGE_DOWN = new ReaderAction("PAGE_DOWN", R.string.action_pagedown, ReaderCommand.DCMD_PAGEDOWN, 1 );
	public final static ReaderAction PAGE_UP = new ReaderAction("PAGE_UP", R.string.action_pageup, ReaderCommand.DCMD_PAGEUP, 1 );
	public final static ReaderAction SEARCH = new ReaderAction("SEARCH", R.string.action_search, ReaderCommand.DCMD_SEARCH, 0 );
	public final static ReaderAction RECENT_BOOKS = new ReaderAction("RECENT_BOOKS", R.string.action_recent_books_list, ReaderCommand.DCMD_RECENT_BOOKS_LIST, 0 );
	public final static ReaderAction EXIT = new ReaderAction("EXIT", R.string.action_exit, ReaderCommand.DCMD_EXIT, 0 );
	
	public final static ReaderAction[] AVAILABLE_ACTIONS = {
		NONE,
		PAGE_DOWN,
		PAGE_UP,
		SEARCH,
		RECENT_BOOKS,
		EXIT,
	};
	public static ReaderAction findById( String id ) {
		if ( id==null )
			return NONE;
		for ( ReaderAction a : AVAILABLE_ACTIONS ) {
			if ( id.equals(a.id) )
				return a;
		}
		return NONE;
	}
}
