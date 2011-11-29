package org.coolreader.crengine;

import org.coolreader.R;

public class InterfaceTheme {

	public final static InterfaceTheme BLACK = new InterfaceTheme("BLACK", R.style.Theme_Black, R.style.Theme_Black_Dialog_Normal, R.style.Theme_Black_Dialog_Fullscreen, R.string.options_app_ui_theme_black);
	
	public final static InterfaceTheme WHITE = new InterfaceTheme("WHITE", R.style.Theme_White, R.style.Theme_White_Dialog_Normal, R.style.Theme_White_Dialog_Fullscreen, R.string.options_app_ui_theme_white);

	public final static InterfaceTheme LIGHT = new InterfaceTheme("LIGHT", R.style.Theme_Light, R.style.Theme_Light_Dialog_Normal, R.style.Theme_Light_Dialog_Fullscreen, R.string.options_app_ui_theme_light);
	
	public final static InterfaceTheme DARK = new InterfaceTheme("DARK", R.style.Theme_Dark, R.style.Theme_Dark_Dialog_Normal, R.style.Theme_Dark_Dialog_Fullscreen, R.string.options_app_ui_theme_dark);
	
	public String getCode() {
		return code;
	}
	public int getThemeId() {
		return themeId;
	}
	public int getDialogThemeId() {
		return dialogThemeId;
	}
	public int getFullscreenDialogThemeId() {
		return fsDialogThemeId;
	}
	public int getDisplayNameResourceId() {
		return displayNameResourceId;
	}
	
	
	public final static InterfaceTheme[] allThemes = {
		BLACK, WHITE, DARK, LIGHT,
	};
	
	public static InterfaceTheme findByCode(String code) {
		if (code == null)
			return null;
		for (InterfaceTheme t : allThemes)
			if (t.getCode().equals(code))
				return t;
		return null;
	}
	
	private final String code;
	private final int themeId;
	private final int dialogThemeId;
	private final int fsDialogThemeId;
	private final int displayNameResourceId;
	private InterfaceTheme(String code, int themeId, int dialogThemeId, int fsDialogThemeId, int displayNameResourceId) {
		this.code = code;
		this.themeId = themeId;
		this.dialogThemeId = dialogThemeId;
		this.fsDialogThemeId = fsDialogThemeId;
		this.displayNameResourceId = displayNameResourceId;
	}
}
