/*
 * CoolReader for Android
 * Copyright (C) 2011-2015 Vadim Lopatin <coolreader.org@gmail.com>
 * Copyright (C) 2012,2013 Jeff Doozan <jeff@doozan.com>
 * Copyright (C) 2012 Daniel Savard <daniels@xsoli.com>
 * Copyright (C) 2013 Sandro Muramoto <sandro.muramoto@3o-tech.com>
 * Copyright (C) 2018 Yuri Plotnikov <plotnikovya@gmail.com>
 * Copyright (C) 2018 S-trace <S-trace@list.ru>
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

import android.util.Log;

import org.coolreader.R;

import java.util.Locale;

public interface Settings {
    public static final String PROP_PAGE_BACKGROUND_IMAGE       ="background.image";
    public static final String PROP_PAGE_BACKGROUND_IMAGE_DAY   ="background.image.day";
    public static final String PROP_PAGE_BACKGROUND_IMAGE_NIGHT ="background.image.night";
    public static final String PROP_NIGHT_MODE              ="crengine.night.mode";
    public static final String PROP_FONT_COLOR_DAY          ="font.color.day";
    public static final String PROP_BACKGROUND_COLOR_DAY    ="background.color.day";
    public static final String PROP_FONT_COLOR_NIGHT        ="font.color.night";
    public static final String PROP_BACKGROUND_COLOR_NIGHT  ="background.color.night";
    public static final String PROP_FONT_COLOR              ="font.color.default";
    public static final String PROP_BACKGROUND_COLOR        ="background.color.default";
    public static final String PROP_FONT_ANTIALIASING       ="font.antialiasing.mode";
    public static final String PROP_FONT_FACE               ="font.face.default";
    public static final String PROP_FONT_HINTING            ="font.hinting.mode";
    public static final String PROP_FONT_GAMMA              ="font.gamma";
    public static final String PROP_FONT_GAMMA_DAY          ="font.gamma.day";
    public static final String PROP_FONT_GAMMA_NIGHT        ="font.gamma.night";
    public static final String PROP_FONT_WEIGHT_EMBOLDEN_OBSOLETED ="font.face.weight.embolden";	// obsoleted
    public static final String PROP_FONT_BASE_WEIGHT        ="font.face.base.weight";        // replaces PROP_FONT_WEIGHT_EMBOLDEN ("font.face.weight.embolden")
    public static final String PROP_TXT_OPTION_PREFORMATTED ="crengine.file.txt.preformatted";
    public static final String PROP_LOG_FILENAME            ="crengine.log.filename";
    public static final String PROP_LOG_LEVEL               ="crengine.log.level";
    public static final String PROP_LOG_AUTOFLUSH           ="crengine.log.autoflush";
    public static final String PROP_FONT_SIZE               ="crengine.font.size";
	public static final String PROP_FALLBACK_FONT_FACES     ="crengine.font.fallback.faces";
    public static final String PROP_STATUS_FONT_COLOR       ="crengine.page.header.font.color";
    public static final String PROP_STATUS_FONT_COLOR_DAY   ="crengine.page.header.font.color.day";
    public static final String PROP_STATUS_FONT_COLOR_NIGHT ="crengine.page.header.font.color.night";
    public static final String PROP_STATUS_FONT_FACE        ="crengine.page.header.font.face";
    public static final String PROP_STATUS_FONT_SIZE        ="crengine.page.header.font.size";
    public static final String PROP_STATUS_CHAPTER_MARKS    ="crengine.page.header.chapter.marks";
    public static final String PROP_PAGE_MARGIN_TOP         ="crengine.page.margin.top";
    public static final String PROP_PAGE_MARGIN_BOTTOM      ="crengine.page.margin.bottom";
    public static final String PROP_PAGE_MARGIN_LEFT        ="crengine.page.margin.left";
    public static final String PROP_PAGE_MARGIN_RIGHT       ="crengine.page.margin.right";
    public static final String PROP_ROUNDED_CORNERS_MARGIN  ="crengine.rounded.corners.margin";
    public static final String PROP_PAGE_VIEW_MODE          ="crengine.page.view.mode"; // pages/scroll
    public static final String PROP_PAGE_ANIMATION          ="crengine.page.animation";
    public static final String PROP_INTERLINE_SPACE         ="crengine.interline.space";
    public static final String PROP_ROTATE_ANGLE            ="window.rotate.angle";
    public static final String PROP_EMBEDDED_STYLES         ="crengine.doc.embedded.styles.enabled";
    public static final String PROP_EMBEDDED_FONTS          ="crengine.doc.embedded.fonts.enabled";
    public static final String PROP_DISPLAY_INVERSE         ="crengine.display.inverse";
//    public static final String PROP_DISPLAY_FULL_UPDATE_INTERVAL ="crengine.display.full.update.interval";
//    public static final String PROP_DISPLAY_TURBO_UPDATE_MODE ="crengine.display.turbo.update";

    public static final String PROP_STATUS_LOCATION         ="viewer.status.location";
    public static final String PROP_TOOLBAR_LOCATION        ="viewer.toolbar.location2";
    public static final String PROP_TOOLBAR_HIDE_IN_FULLSCREEN="viewer.toolbar.fullscreen.hide";
    public static final String PROP_TOOLBAR_APPEARANCE="viewer.toolbar.appearance";
    
    public static final String PROP_STATUS_LINE             ="window.status.line";
    public static final String PROP_BOOKMARK_ICONS          ="crengine.bookmarks.icons";
    public static final String PROP_FOOTNOTES               ="crengine.footnotes";
    public static final String PROP_SHOW_TIME               ="window.status.clock";
    public static final String PROP_SHOW_TITLE              ="window.status.title";
    public static final String PROP_SHOW_BATTERY            ="window.status.battery";
    public static final String PROP_SHOW_BATTERY_PERCENT    ="window.status.battery.percent";
    public static final String PROP_SHOW_POS_PERCENT        ="window.status.pos.percent";
    public static final String PROP_SHOW_PAGE_COUNT         ="window.status.pos.page.count";
    public static final String PROP_SHOW_PAGE_NUMBER        ="window.status.pos.page.number";
    public static final String PROP_FONT_SHAPING            ="font.shaping.mode";
    public static final String PROP_FONT_KERNING_ENABLED    ="font.kerning.enabled";
    public static final String PROP_FLOATING_PUNCTUATION    ="crengine.style.floating.punctuation.enabled";
    public static final String PROP_LANDSCAPE_PAGES         ="window.landscape.pages";
    //public static final String PROP_HYPHENATION_DICT        ="crengine.hyphenation.dictionary.code"; // non-crengine (old)
	public static final String PROP_HYPHENATION_DICT        = "crengine.hyphenation.directory";
	public static final String PROP_AUTOSAVE_BOOKMARKS      ="crengine.autosave.bookmarks";
	// New textlang typography settings:
	public static final String PROP_TEXTLANG_MAIN_LANG      = "crengine.textlang.main.lang";
	public static final String PROP_TEXTLANG_EMBEDDED_LANGS_ENABLED = "crengine.textlang.embedded.langs.enabled";
	public static final String PROP_TEXTLANG_HYPHENATION_ENABLED    = "crengine.textlang.hyphenation.enabled";
	public static final String PROP_TEXTLANG_HYPH_SOFT_HYPHENS_ONLY = "crengine.textlang.hyphenation.soft.hyphens.only";
	public static final String PROP_TEXTLANG_HYPH_FORCE_ALGORITHMIC = "crengine.textlang.hyphenation.force.algorithmic";

    public static final String PROP_PROFILE_NUMBER          ="crengine.profile.number"; // current settings profile number
    public static final String PROP_APP_SETTINGS_SHOW_ICONS ="app.settings.show.icons";
    public static final String PROP_APP_KEY_BACKLIGHT_OFF   ="app.key.backlight.disabled";

	 // image scaling settings
	 // mode: 0=disabled, 1=integer scaling factors, 2=free scaling
	 // scale: 0=auto based on font size, 1=no zoom, 2=scale up to *2, 3=scale up to *3
    public static final String PROP_IMG_SCALING_ZOOMIN_INLINE_MODE = "crengine.image.scaling.zoomin.inline.mode";
    public static final String PROP_IMG_SCALING_ZOOMIN_INLINE_SCALE = "crengine.image.scaling.zoomin.inline.scale";
    public static final String PROP_IMG_SCALING_ZOOMOUT_INLINE_MODE = "crengine.image.scaling.zoomout.inline.mode";
    public static final String PROP_IMG_SCALING_ZOOMOUT_INLINE_SCALE = "crengine.image.scaling.zoomout.inline.scale";
    public static final String PROP_IMG_SCALING_ZOOMIN_BLOCK_MODE = "crengine.image.scaling.zoomin.block.mode";
    public static final String PROP_IMG_SCALING_ZOOMIN_BLOCK_SCALE = "crengine.image.scaling.zoomin.block.scale";
    public static final String PROP_IMG_SCALING_ZOOMOUT_BLOCK_MODE = "crengine.image.scaling.zoomout.block.mode";
    public static final String PROP_IMG_SCALING_ZOOMOUT_BLOCK_SCALE = "crengine.image.scaling.zoomout.block.scale";

    public static final String PROP_FORMAT_SPACE_WIDTH_SCALE_PERCENT = "crengine.style.space.width.scale.percent";
    public static final String PROP_FORMAT_MIN_SPACE_CONDENSING_PERCENT = "crengine.style.space.condensing.percent";
    public static final String PROP_FORMAT_UNUSED_SPACE_THRESHOLD_PERCENT = "crengine.style.unused.space.threshold.percent";
    public static final String PROP_FORMAT_MAX_ADDED_LETTER_SPACING_PERCENT = "crengine.style.max.added.letter.spacing.percent";

	// default is 96 (1 css px = 1 screen px)
	// use 0 for old crengine behaviour (no support for absolute units and 1css px = 1 screen px)
	public static final String PROP_RENDER_DPI              = "crengine.render.dpi";
	//public static final String PROP_RENDER_SCALE_FONT_WITH_DPI = "crengine.render.scale.font.with.dpi";
	public static final String PROP_RENDER_BLOCK_RENDERING_FLAGS = "crengine.render.block.rendering.flags";
	public static final String PROP_REQUESTED_DOM_VERSION   = "crengine.render.requested_dom_version";

	public static final String PROP_MIN_FILE_SIZE_TO_CACHE  ="crengine.cache.filesize.min";
    public static final String PROP_FORCED_MIN_FILE_SIZE_TO_CACHE  ="crengine.cache.forced.filesize.min";
    public static final String PROP_PROGRESS_SHOW_FIRST_PAGE="crengine.progress.show.first.page";

    public static final String PROP_CONTROLS_ENABLE_VOLUME_KEYS ="app.controls.volume.keys.enabled";
    
    public static final String PROP_APP_FULLSCREEN          ="app.fullscreen";
    public static final String PROP_APP_BOOK_PROPERTY_SCAN_ENABLED ="app.browser.fileprops.scan.enabled";
    public static final String PROP_APP_SHOW_COVERPAGES     ="app.browser.coverpages";
    public static final String PROP_APP_COVERPAGE_SIZE     ="app.browser.coverpage.size"; // 0==small, 2==BIG
    public static final String PROP_APP_SCREEN_ORIENTATION  ="app.screen.orientation";
    public static final String PROP_APP_SCREEN_BACKLIGHT    ="app.screen.backlight";
    public static final String PROP_APP_SCREEN_WARM_BACKLIGHT    ="app.screen.warm.backlight";
    public static final String PROP_APP_MOTION_TIMEOUT    ="app.motion.timeout";
    public static final String PROP_APP_SCREEN_BACKLIGHT_DAY   ="app.screen.backlight.day";
    public static final String PROP_APP_SCREEN_BACKLIGHT_NIGHT ="app.screen.backlight.night";
    public static final String PROP_APP_DOUBLE_TAP_SELECTION     ="app.controls.doubletap.selection";
    public static final String PROP_APP_BOUNCE_TAP_INTERVAL   ="app.controls.bounce.interval";
    public static final String PROP_APP_TAP_ZONE_ACTIONS_TAP     ="app.tapzone.action.tap";
    public static final String PROP_APP_KEY_ACTIONS_PRESS     ="app.key.action.press";
    public static final String PROP_APP_TRACKBALL_DISABLED    ="app.trackball.disabled";
    public static final String PROP_APP_SCREEN_BACKLIGHT_LOCK    ="app.screen.backlight.lock.enabled";
    public static final String PROP_APP_TAP_ZONE_HILIGHT     ="app.tapzone.hilight";
    public static final String PROP_APP_FLICK_BACKLIGHT_CONTROL = "app.screen.backlight.control.flick";
    public static final String PROP_APP_FLICK_BACKLIGHT_CONTROL_TOGETHER = "app.screen.backlight.control.flick.together";
    public static final String PROP_APP_FLICK_WARMLIGHT_CONTROL = "app.screen.warmlight.control.flick";
    public static final String PROP_APP_BOOK_SORT_ORDER = "app.browser.sort.order";
    public static final String PROP_APP_DICTIONARY = "app.dictionary.current";
    public static final String PROP_APP_DICTIONARY_2 = "app.dictionary2.current";
    public static final String PROP_APP_SELECTION_ACTION = "app.selection.action";
    public static final String PROP_APP_MULTI_SELECTION_ACTION = "app.multiselection.action";
    public static final String PROP_APP_SELECTION_PERSIST = "app.selection.persist";

    public static final String PROP_APP_HIGHLIGHT_BOOKMARKS = "crengine.highlight.bookmarks";
    public static final String PROP_HIGHLIGHT_SELECTION_COLOR = "crengine.highlight.selection.color";
    public static final String PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT = "crengine.highlight.bookmarks.color.comment";
    public static final String PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION = "crengine.highlight.bookmarks.color.correction";
    public static final String PROP_APP_HIGHLIGHT_BOOKMARKS_DAY = "crengine.highlight.bookmarks.day";
    public static final String PROP_HIGHLIGHT_SELECTION_COLOR_DAY = "crengine.highlight.selection.color.day";
    public static final String PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT_DAY = "crengine.highlight.bookmarks.color.comment.day";
    public static final String PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION_DAY = "crengine.highlight.bookmarks.color.correction.day";
    public static final String PROP_APP_HIGHLIGHT_BOOKMARKS_NIGHT = "crengine.highlight.bookmarks.night";
    public static final String PROP_HIGHLIGHT_SELECTION_COLOR_NIGHT = "crengine.highlight.selection.color.night";
    public static final String PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT_NIGHT = "crengine.highlight.bookmarks.color.comment.night";
    public static final String PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION_NIGHT = "crengine.highlight.bookmarks.color.correction.night";

    public static final String PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS = "app.browser.hide.empty.folders";
    public static final String PROP_APP_FILE_BROWSER_HIDE_EMPTY_GENRES = "app.browser.hide.empty.genres";
    public static final String PROP_APP_FILE_BROWSER_SIMPLE_MODE = "app.browser.simple.mode";

    public static final String PROP_APP_SCREEN_UPDATE_MODE = "app.screen.update.mode";
    public static final String PROP_APP_SCREEN_UPDATE_INTERVAL = "app.screen.update.interval";
    public static final String PROP_APP_SECONDARY_TAP_ACTION_TYPE = "app.touch.secondary.action.type";
    public static final String PROP_APP_GESTURE_PAGE_FLIPPING = "app.touch.gesture.page.flipping";

    public static final String PROP_APP_VIEW_AUTOSCROLL_SPEED  ="app.view.autoscroll.speed";
    public static final String PROP_APP_VIEW_AUTOSCROLL_TYPE  ="app.view.autoscroll.type";

    public static final String PROP_APP_THEME = "app.ui.theme";
    public static final String PROP_APP_THEME_DAY  = "app.ui.theme.day";
    public static final String PROP_APP_THEME_NIGHT = "app.ui.theme.night";

    public static final String PROP_APP_LOCALE = "app.locale.name";
    
    public static final String PROP_APP_STARTUP_ACTION = "app.startup.action";

    public static final String PROP_APP_PLUGIN_ENABLED = "app.plugin.enabled.litres";

    /*
      Commented until the appearance of free implementation of the binding to the Google Drive (R)
    String PROP_APP_CLOUDSYNC_GOOGLEDRIVE_ENABLED = "app.cloudsync.googledrive.enabled";
    String PROP_APP_CLOUDSYNC_GOOGLEDRIVE_SETTINGS = "app.cloudsync.googledrive.settings";
    String PROP_APP_CLOUDSYNC_GOOGLEDRIVE_BOOKMARKS = "app.cloudsync.googledrive.bookmarks";
    String PROP_APP_CLOUDSYNC_GOOGLEDRIVE_CURRENTBOOK_INFO = "app.cloudsync.googledrive.currentbook";
    String PROP_APP_CLOUDSYNC_GOOGLEDRIVE_CURRENTBOOK_BODY = "app.cloudsync.googledrive.currentbook.body";
    String PROP_APP_CLOUDSYNC_GOOGLEDRIVE_AUTOSAVEPERIOD = "app.cloudsync.googledrive.autosaveperiod";
    String PROP_APP_CLOUDSYNC_CONFIRMATIONS = "app.cloudsync.confirmations";
    String PROP_APP_CLOUDSYNC_DATA_KEEPALIVE = "app.cloudsync.bookmarks.keepalive";		// days
     */

    String PROP_APP_TTS_SPEED = "app.tts.speed";
    String PROP_APP_TTS_ENGINE = "app.tts.engine";
    String PROP_APP_TTS_USE_DOC_LANG = "app.tts.use.doc.lang";		// The TTS language is set according to the language of the book.
    String PROP_APP_TTS_FORCE_LANGUAGE = "app.tts.force.lang";		// Force use specified language
    String PROP_APP_TTS_VOICE = "app.tts.voice";
    String PROP_APP_TTS_GOOGLE_END_OF_SENTENCE_ABBR = "app.tts.google.end-of-sentence-abbreviation.workaround";	// Use a workaround to disable processing of abbreviations at the end of a sentence when using "Google Speech Services"
    String PROP_APP_TTS_USE_AUDIOBOOK = "app.tts.use.audiobook"; //if *.wordtiming file exists for ebook

    String PROP_APP_VIEW_ANIM_DURATION ="app.view.anim.duration";

    // available options for PROP_APP_SELECTION_ACTION setting
    public static final int SELECTION_ACTION_TOOLBAR = 0;
    public static final int SELECTION_ACTION_COPY = 1;
    public static final int SELECTION_ACTION_DICTIONARY = 2;
    public static final int SELECTION_ACTION_BOOKMARK = 3;
    public static final int SELECTION_ACTION_FIND = 4;
    
    // available options for PROP_APP_SECONDARY_TAP_ACTION_TYPE setting
    public static final int TAP_ACTION_TYPE_LONGPRESS = 0;
    public static final int TAP_ACTION_TYPE_DOUBLE = 1;
    public static final int TAP_ACTION_TYPE_SHORT = 2;

    // available options for PROP_APP_FLICK_BACKLIGHT_CONTROL setting
    public static final int BACKLIGHT_CONTROL_FLICK_NONE = 0;
    public static final int BACKLIGHT_CONTROL_FLICK_LEFT = 1;
    public static final int BACKLIGHT_CONTROL_FLICK_RIGHT = 2;

    public static final int APP_STARTUP_ACTION_LAST_BOOK = 0;
    public static final int APP_STARTUP_ACTION_ROOT = 1;
    public static final int APP_STARTUP_ACTION_RECENT_BOOKS = 2;
    public static final int APP_STARTUP_ACTION_LAST_BOOK_FOLDER = 3;
    
    public static final int VIEWER_STATUS_NONE = 0;
    public static final int VIEWER_STATUS_TOP = 1;
    public static final int VIEWER_STATUS_BOTTOM = 2;
    public static final int VIEWER_STATUS_PAGE_HEADER = 3;
    public static final int VIEWER_STATUS_PAGE_FOOTER = 4;

    public static final int VIEWER_TOOLBAR_NONE = 0;
    public static final int VIEWER_TOOLBAR_TOP = 1;
    public static final int VIEWER_TOOLBAR_BOTTOM = 2;
    public static final int VIEWER_TOOLBAR_LEFT = 3;
    public static final int VIEWER_TOOLBAR_RIGHT = 4;
    public static final int VIEWER_TOOLBAR_SHORT_SIDE = 5;
    public static final int VIEWER_TOOLBAR_LONG_SIDE = 6;

    public static final int VIEWER_TOOLBAR_100 = 0;
    public static final int VIEWER_TOOLBAR_100_gray = 1;
    public static final int VIEWER_TOOLBAR_75 = 2;
    public static final int VIEWER_TOOLBAR_75_gray = 3;
    public static final int VIEWER_TOOLBAR_50 = 4;
    public static final int VIEWER_TOOLBAR_50_gray = 5;
    
    
    public enum Lang {
    	DEFAULT("system", R.string.options_app_locale_system, R.raw.help_template_en),
    	EN("en", R.string.options_app_locale_en, R.raw.help_template_en),
        DE("de", R.string.options_app_locale_de, 0),
    	ES("es", R.string.options_app_locale_es, 0),
    	FR("fr", R.string.options_app_locale_fr, 0),
    	JA("ja", R.string.options_app_locale_ja, 0),
    	RU("ru", R.string.options_app_locale_ru, R.raw.help_template_ru),
    	UK("uk", R.string.options_app_locale_uk, R.raw.help_template_ru),
    	BG("bg", R.string.options_app_locale_bg, 0),
    	BY("by", R.string.options_app_locale_by, 0),
    	SK("sk", R.string.options_app_locale_sk, 0),
    	TR("tr", R.string.options_app_locale_tr, 0),
    	LT("lt", R.string.options_app_locale_lt, 0),
    	IT("it", R.string.options_app_locale_it, 0),
    	HU("hu", R.string.options_app_locale_hu, R.raw.help_template_hu),
    	NL("nl", R.string.options_app_locale_nl, 0),
    	PL("pl", R.string.options_app_locale_pl, 0),
        PT("pt", R.string.options_app_locale_pt, 0),
        PT_BR("pt_BR", R.string.options_app_locale_pt_rbr, 0),
    	CS("cs", R.string.options_app_locale_cs, 0),
    	ZH_CN("zh_CN", R.string.options_app_locale_zh_cn, R.raw.help_template_zh_cn),
    	;
    	
    	public Locale getLocale() {
   			return getLocale(code);
    	}
    	
    	static public Locale getLocale(String code) {
    		if (code.length() == 2)
    			return new Locale(code);
    		if (code.length() == 5)
    			return new Locale(code.substring(0, 2), code.substring(3, 5));
    		return null;
    	}

    	static public String getCode(Locale locale) {
    		String country = locale.getCountry();
    		if (country == null || country.length()==0)
    			return locale.getLanguage();
			return locale.getLanguage() + "_" + country;
    	}
    	
    	static public Lang byCode(String code) {
    		for (Lang lang : values())
    			if (lang.code.equals(code))
    				return lang;
    		if (code.length() > 2) {
    			code = code.substring(0, 2);
        		for (Lang lang : values())
        			if (lang.code.equals(code))
        				return lang;
    		}
    		Log.w("cr3", "language not found by code " + code);
    		return DEFAULT;
    	}
    	
    	private Lang(String code, int nameResId, int helpFileResId) {
    		this.code = code;
    		this.nameId = nameResId;
    		this.helpFileResId = helpFileResId;
    	}
    	public final String code;
    	public final int nameId;
    	public final int helpFileResId;
    };
    
    
	public final static int MAX_PROFILES = 6;

	// settings which depend on profile
	public final static String[] PROFILE_SETTINGS = {
	    "background.*",
	    PROP_NIGHT_MODE,
	    "font.*",
	    "crengine.page.*",
	    PROP_FONT_SIZE,
	    PROP_FALLBACK_FONT_FACES,
	    PROP_INTERLINE_SPACE,
	    PROP_STATUS_LINE,
	    PROP_FOOTNOTES,
	    "window.status.*",
	    PROP_FLOATING_PUNCTUATION,
	    PROP_LANDSCAPE_PAGES,
	    PROP_HYPHENATION_DICT,
	    "crengine.image.*",
	    PROP_FORMAT_MIN_SPACE_CONDENSING_PERCENT,
	    PROP_APP_FULLSCREEN,
	    "app.screen.*",
	    PROP_APP_DICTIONARY,
	    PROP_APP_SELECTION_ACTION,
	    PROP_APP_SELECTION_PERSIST,
	    PROP_APP_HIGHLIGHT_BOOKMARKS + "*",
	    PROP_HIGHLIGHT_SELECTION_COLOR + "*",
	    PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT + "*",
	    PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION + "*",

      "viewer.*",
	    PROP_APP_VIEW_AUTOSCROLL_SPEED,
	    PROP_APP_VIEW_AUTOSCROLL_TYPE,
	    	    
      "app.key.*",
	    "app.tapzone.*",
	    PROP_APP_DOUBLE_TAP_SELECTION,
	    "app.touch.*",

	    "app.ui.theme*",
	};


}
