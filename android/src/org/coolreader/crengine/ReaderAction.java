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

	public final static ReaderAction PAGE_DOWN = new ReaderAction("PAGE_DOWN", R.string.action_pagedown, ReaderCommand.DCMD_PAGEDOWN, 1 );
	public final static ReaderAction PAGE_UP = new ReaderAction("PAGE_UP", R.string.action_pageup, ReaderCommand.DCMD_PAGEUP, 1 );
	
	public final static ReaderAction[] AVAILABLE_ACTIONS = {
		PAGE_DOWN,
		PAGE_UP,
	};
}
