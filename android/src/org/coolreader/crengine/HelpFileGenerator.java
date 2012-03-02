package org.coolreader.crengine;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;

import org.coolreader.R;

import android.content.Context;
import android.util.Log;

public class HelpFileGenerator {
	private final Context context;
	private final Engine engine;
	private final String langCode;
	private final Properties settings;
	public HelpFileGenerator(Context context, Engine engine, Properties props, String langCode) {
		this.context = context;
		this.engine = engine;
		this.langCode = langCode;
		this.settings = props;
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
		File fn = new File(dir, "cr3_manual_" + langCode + ".fb2");
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
		if (start + 11 < s.length())
			return null;
		if (s.charAt(start + 0) != '<' 
			|| s.charAt(start + 1) != '!'
			|| s.charAt(start + 2) != '-'
			|| s.charAt(start + 3) != '-'
			|| s.charAt(start + 4) != 'c'
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

	private static int findImageResIdByName(String name) {
		if ("cr3_logo".equals(name))
			return R.drawable.cr3_logo;
		// TODO: more image resources to support
		return 0;
	}
	
	private void encodeImage(StringBuilder buf, byte[] data) {
		// TODO:
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
						buf.append(macro.param1 + "=" + settings.getProperty(macro.param1));
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
