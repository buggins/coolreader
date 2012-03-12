package org.coolreader.crengine;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.content.Context;
import android.util.Log;

public class HelpFileGenerator {

	private static final int MANUAL_VERSION = 4;
	
	private final Context context;
	private final Engine engine;
	private final String langCode;
	private final Properties settings;
	private final String version;
	public HelpFileGenerator(CoolReader context, Engine engine, Properties props, String langCode) {
		this.context = context;
		this.engine = engine;
		this.langCode = langCode;
		this.settings = props;
		this.version = context.getVersion();
	}
	
	private static final String[] settingsUsedInManual = {
		"app.tapzone.action.tap.long.1",
		"app.tapzone.action.tap.long.2",
		"app.tapzone.action.tap.long.3",
		"app.tapzone.action.tap.long.4",
		"app.tapzone.action.tap.long.5",
		"app.tapzone.action.tap.long.6",
		"app.tapzone.action.tap.long.7",
		"app.tapzone.action.tap.long.8",
		"app.tapzone.action.tap.long.9",
	    Settings.PROP_APP_DOUBLE_TAP_SELECTION,
	    Settings.PROP_APP_TAP_ZONE_ACTIONS_TAP,
	    Settings.PROP_APP_KEY_ACTIONS_PRESS,
	    Settings.PROP_APP_TRACKBALL_DISABLED,
	    Settings.PROP_APP_FLICK_BACKLIGHT_CONTROL,
	    Settings.PROP_APP_SELECTION_ACTION,
	    Settings.PROP_APP_MULTI_SELECTION_ACTION,
	    Settings.PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS,
	    Settings.PROP_APP_FILE_BROWSER_SIMPLE_MODE,
	    Settings.PROP_APP_SECONDARY_TAP_ACTION_TYPE,
	    Settings.PROP_APP_GESTURE_PAGE_FLIPPING,
	    Settings.PROP_APP_HIGHLIGHT_BOOKMARKS,
	};
	
	private int getSettingHash(String name) {
		String value = settings.getProperty(name);
		return value == null ? 0 : value.hashCode();
	}
	
	/**
	 * Calculate hash for current values of settings which may be affect help file content generation.
	 * @return hash value
	 */
	public int getSettingsHash() {
		int res = MANUAL_VERSION;
		for (String setting : settingsUsedInManual)
			res = res * 31 + getSettingHash(setting);
		return res;
	}
	
	public static File getHelpFileName(File dir, String lang) {
		File fn = new File(dir, "cr3_manual_" + lang + ".fb2");
		return fn;
	}
	
	public File getHelpFileName(File dir) {
		return getHelpFileName(dir, langCode);
	}
	
	public File generateHelpFile(File dir) {
		String template = readTemplate();
		if (template == null || template.length() == 0)
			return null;
		String content = filterTemplate(template);
		byte[] data;
		try {
			data = content.getBytes("UTF8");
		} catch (UnsupportedEncodingException e1) {
			return null;
		}
		File fn = getHelpFileName(dir);
		try {
			FileOutputStream os = new FileOutputStream(fn);
			os.write(data);
			os.close();
		} catch (IOException e) {
			return null;
		}
		return fn;
	}
	
	private String readTemplate() {
		Settings.Lang lang = Settings.Lang.byCode(langCode);
		int resId = lang.helpFileResId;
		if (resId == 0)
			resId = Settings.Lang.DEFAULT.helpFileResId;
		return engine.loadResourceUtf8(resId);
	}

	private enum MacroType {
		IF(1),
		ELSE(0),
		ENDIF(0),
		IMAGE(1),
		SETTING(1),
		;
		public final int paramCount;
		private MacroType(int paramCount) {
			this.paramCount = paramCount;
		}
		static MacroType byName(String name) {
			for (MacroType item : values()) {
				if (item.name().equalsIgnoreCase(name))
					return item;
			}
			return null;
		}
	}
	
	private static class MacroInfo {
		public MacroType type; 
		public String param1; 
		public int len; 
	}
	
	private static MacroInfo err(String msg) {
		Log.e("cr3help", msg);
		return null;
	}
	
	private static MacroInfo detectMacro(String s, int start) {
		if (start + 11 > s.length())
			return null;
		if (s.charAt(start + 0) != '<' 
			|| s.charAt(start + 1) != '!'
			)
			return null;
		if (s.charAt(start + 2) != '-'
			|| s.charAt(start + 3) != '-'
			)
			return null;
		if (s.charAt(start + 4) != 'c'
     		|| s.charAt(start + 5) != 'r'
			|| s.charAt(start + 6) != '3'
			|| s.charAt(start + 7) != ':'
			)
			return null;
		start += 8;
		int end = s.indexOf("-->", start);
		if (end < 0)
			return err("unfinished macro");
		int len = (end - start) + 8 + 3; 
		String content = s.substring(start, end);
		if (content.length() < 1)
			return err("too short content: " + content);
		String[] items = content.split(" ");
		if (items.length < 1)
			return err("invalid content: " + content);
		MacroInfo res = new MacroInfo();
		res.type = MacroType.byName(items[0]);
		if (res.type == null)
			return err("unknown macro type: " + content);
		if (res.type.paramCount > 0) {
			if (items.length < 1)
				return err("macro param missing: " + content);
			res.param1 = items[1].trim();
			if (res.param1 == null || res.param1.length() == 0)
				return err("macro param missing: " + content);
		}
		res.len = len;
		return res;
	}
	
	private boolean getConditionValue(String conditionName) {
		int eqpos = conditionName.indexOf("==");
		int neqpos = conditionName.indexOf("!=");
		String param = conditionName;
		String value = null;
		if (eqpos > 0) {
			param = conditionName.substring(0, eqpos);
			value = conditionName.substring(eqpos + 2);
		} else if (neqpos > 0) {
			param = conditionName.substring(0, neqpos);
			value = conditionName.substring(neqpos + 2);
		}
		String prop = settings.getProperty(param);
		if (prop == null)
			return false;
		if (eqpos > 0)
			return prop.equals(value);
		if (neqpos > 0)
			return !prop.equals(value);
		return "1".equals(prop);
	}

	private static class ImageRes {
		public String name;
		public int resourceId;
		ImageRes(String name, int resourceId) {
			this.name = name;
			this.resourceId = resourceId;
		}
	}
	private static final ImageRes[] images = {
		new ImageRes("cr3_logo", R.drawable.cr3_logo),
		new ImageRes("open_file", R.drawable.ic_menu_archive),
		new ImageRes("goto", R.drawable.ic_menu_goto),
		new ImageRes("bookmarks", R.drawable.ic_menu_mark),
		new ImageRes("select", android.R.drawable.ic_menu_edit),
		new ImageRes("options", android.R.drawable.ic_menu_preferences),
		new ImageRes("search", android.R.drawable.ic_menu_search),
	};
	
	private static int findImageResIdByName(String name) {
		for (ImageRes image : images) {
			if (image.name.equalsIgnoreCase(name))
				return image.resourceId;
		}
		return 0;
	}
	
	private static final char ENCODE[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/',
    };	

	private void encodeImage(StringBuilder buf, byte[] data) {
		int i = 0;
		for (; i <= data.length-3; i += 3) {
			int v = ((data[i] & 0xFF) << 16) | ((data[i+1] & 0xFF) << 8) | (data[i+2] & 0xFF);
            buf.append(ENCODE[(v >> 18) & 0x3f]);
            buf.append(ENCODE[(v >> 12) & 0x3f]);
            buf.append(ENCODE[(v >> 6) & 0x3f]);
            buf.append(ENCODE[v & 0x3f]);
            if (i / 3 % 16 == 15)
            	buf.append("\n");
		}
		int tail = data.length - i;
		if (tail == 1) {
            int v = (data[i] & 0xff) << 4;
            buf.append(ENCODE[(v >> 6) & 0x3f]);
            buf.append(ENCODE[v & 0x3f]);
            buf.append('=');
            buf.append('=');
		} else if (tail == 2) {
            int v = ((data[i] & 0xff) << 10) | ((data[i] & 0xff) << 2);
            buf.append(ENCODE[(v >> 12) & 0x3f]);
            buf.append(ENCODE[(v >> 6) & 0x3f]);
            buf.append(ENCODE[v & 0x3f]);
            buf.append('=');
		}
	}
	
	private boolean appendImage(String name, StringBuilder mainBuf, StringBuilder binBuf) {
		int res = findImageResIdByName(name);
		if (res == 0)
			return false;
		byte[] data = engine.loadResourceBytes(res);
		if (data == null || data.length == 0)
			return false;
		mainBuf.append("<image l:href=\"#" + name + ".png" + "\"/>");
		binBuf.append("\n<binary content-type=\"image/png\" id=\"" + name + ".png\">");
		encodeImage(binBuf, data);
		binBuf.append("</binary>");
		return true;
	}
	
	private String getActionName(String actionId) {
		ReaderAction a = ReaderAction.findById(actionId);
		return context.getString(a.nameId);
	}
	
	private String getSettingValueName(String name) {
		if ("version".equals(name))
			return version;
		String v = settings.getProperty(name);
		if (v == null || v.length() == 0)
			return null;
		if (name.startsWith(Settings.PROP_APP_TAP_ZONE_ACTIONS_TAP))
			return getActionName(v);
		if (name.startsWith(Settings.PROP_APP_KEY_ACTIONS_PRESS))
			return getActionName(v);
		return v;
	}
	
	private String filterTemplate(String template) {
		StringBuilder buf = new StringBuilder(template.length());
		StringBuilder binary = new StringBuilder();
		ArrayList<Boolean> ifStack = new ArrayList<Boolean>();
		boolean ifState = true;
		for (int i=0; i<template.length(); i++) {
			// <!--cr3:if condition-->    (condition may be setting==value or setting!=value or setting (=="1", for bool values)
			// <!--cr3:else -->      else branch of if
			// <!--cr3:endif -->     end if
			// <!--cr3:image name-->     place image here
			// <!--cr3:setting name-->
			MacroInfo macro = null;
			char ch = template.charAt(i);
			if (ch == '<')
				macro = detectMacro(template, i);
			if (macro != null) {
				boolean condValue = false;
				switch (macro.type) {
				case IF:
					ifStack.add(ifState);
					ifState = getConditionValue(macro.param1);
					break;
				case ELSE:
					ifState = !ifState;
					break;
				case ENDIF:
					if (ifStack.size() > 0)
						ifState = ifStack.remove(ifStack.size() - 1);
					break;
				case IMAGE:
					if (ifState) {
						appendImage(macro.param1, buf, binary);
					}
					break;
				case SETTING:
					if (ifState) {
						// TODO: show param value name here
						String value = getSettingValueName(macro.param1);
						if (value != null)
							buf.append(getSettingValueName(macro.param1));
					}
					break;
				}
				i += macro.len - 1;
			} else {
				if (ch == '<' && i >= template.length() - 20 && "</FictionBook>".equals(template.substring(i, i+14))) {
					// before closing </FictionBook> tag put all binary (image) data
					buf.append(binary);
				}
				if (ifState)
					buf.append(ch);
			}
		}
		return buf.toString();
	}
}
