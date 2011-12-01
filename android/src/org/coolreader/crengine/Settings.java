package org.coolreader.crengine;

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
    public static final String PROP_FONT_GAMMA              ="font.gamma";
    public static final String PROP_FONT_GAMMA_DAY          ="font.gamma.day";
    public static final String PROP_FONT_GAMMA_NIGHT        ="font.gamma.night";
    public static final String PROP_FONT_WEIGHT_EMBOLDEN    ="font.face.weight.embolden";
    public static final String PROP_TXT_OPTION_PREFORMATTED ="crengine.file.txt.preformatted";
    public static final String PROP_LOG_FILENAME            ="crengine.log.filename";
    public static final String PROP_LOG_LEVEL               ="crengine.log.level";
    public static final String PROP_LOG_AUTOFLUSH           ="crengine.log.autoflush";
    public static final String PROP_FONT_SIZE               ="crengine.font.size";
    public static final String PROP_FALLBACK_FONT_FACE      ="crengine.font.fallback.face";
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
    public static final String PROP_PAGE_VIEW_MODE          ="crengine.page.view.mode"; // pages/scroll
    public static final String PROP_PAGE_ANIMATION          ="crengine.page.animation";
    public static final String PROP_INTERLINE_SPACE         ="crengine.interline.space";
    public static final String PROP_ROTATE_ANGLE            ="window.rotate.angle";
    public static final String PROP_EMBEDDED_STYLES         ="crengine.doc.embedded.styles.enabled";
    public static final String PROP_DISPLAY_INVERSE         ="crengine.display.inverse";
//    public static final String PROP_DISPLAY_FULL_UPDATE_INTERVAL ="crengine.display.full.update.interval";
//    public static final String PROP_DISPLAY_TURBO_UPDATE_MODE ="crengine.display.turbo.update";
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
    public static final String PROP_FONT_KERNING_ENABLED    ="font.kerning.enabled";
    public static final String PROP_FLOATING_PUNCTUATION    ="crengine.style.floating.punctuation.enabled";
    public static final String PROP_LANDSCAPE_PAGES         ="window.landscape.pages";
    public static final String PROP_HYPHENATION_DICT        ="crengine.hyphenation.dictionary.code"; // non-crengine
    public static final String PROP_AUTOSAVE_BOOKMARKS      ="crengine.autosave.bookmarks";

    public static final String PROP_PROFILE_NUMBER          ="crengine.profile.number"; // current settings profile number
    public static final String PROP_APP_SETTINGS_SHOW_ICONS ="app.settings.show.icons";

    
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
    
    public static final String PROP_FORMAT_MIN_SPACE_CONDENSING_PERCENT = "crengine.style.space.condensing.percent";
    
    public static final String PROP_MIN_FILE_SIZE_TO_CACHE  ="crengine.cache.filesize.min";
    public static final String PROP_FORCED_MIN_FILE_SIZE_TO_CACHE  ="crengine.cache.forced.filesize.min";
    public static final String PROP_PROGRESS_SHOW_FIRST_PAGE="crengine.progress.show.first.page";

    public static final String PROP_CONTROLS_ENABLE_VOLUME_KEYS ="app.controls.volume.keys.enabled";
    
    public static final String PROP_APP_FULLSCREEN          ="app.fullscreen";
    public static final String PROP_APP_BOOK_PROPERTY_SCAN_ENABLED ="app.browser.fileprops.scan.enabled";
    public static final String PROP_APP_SHOW_COVERPAGES     ="app.browser.coverpages";
    public static final String PROP_APP_SCREEN_ORIENTATION  ="app.screen.orientation";
    public static final String PROP_APP_SCREEN_BACKLIGHT    ="app.screen.backlight";
    public static final String PROP_APP_SCREEN_BACKLIGHT_DAY   ="app.screen.backlight.day";
    public static final String PROP_APP_SCREEN_BACKLIGHT_NIGHT ="app.screen.backlight.night";
    public static final String PROP_APP_DOUBLE_TAP_SELECTION     ="app.controls.doubletap.selection";
    public static final String PROP_APP_TAP_ZONE_ACTIONS_TAP     ="app.tapzone.action.tap";
    public static final String PROP_APP_KEY_ACTIONS_PRESS     ="app.key.action.press";
    public static final String PROP_APP_TRACKBALL_DISABLED    ="app.trackball.disabled";
    public static final String PROP_APP_SCREEN_BACKLIGHT_LOCK    ="app.screen.backlight.lock.enabled";
    public static final String PROP_APP_TAP_ZONE_HILIGHT     ="app.tapzone.hilight";
    public static final String PROP_APP_FLICK_BACKLIGHT_CONTROL = "app.screen.backlight.control.flick";
    public static final String PROP_APP_BOOK_SORT_ORDER = "app.browser.sort.order";
    public static final String PROP_APP_DICTIONARY = "app.dictionary.current";
    public static final String PROP_APP_SELECTION_ACTION = "app.selection.action";
    public static final String PROP_APP_SELECTION_PERSIST = "app.selection.persist";
    public static final String PROP_APP_HIGHLIGHT_BOOKMARKS = "crengine.highlight.bookmarks";
    public static final String PROP_APP_FILE_BROWSER_HIDE_EMPTY_FOLDERS = "app.browser.hide.empty.folders";
    public static final String PROP_APP_FILE_BROWSER_SIMPLE_MODE = "app.browser.simple.mode";

    public static final String PROP_APP_SCREEN_UPDATE_MODE  ="app.screen.update.mode";
    public static final String PROP_APP_SCREEN_UPDATE_INTERVAL  ="app.screen.update.interval";
    public static final String PROP_APP_SECONDARY_TAP_ACTION_TYPE  ="app.touch.secondary.action.type";

    public static final String PROP_APP_VIEW_AUTOSCROLL_SPEED  ="app.view.autoscroll.speed";
    public static final String PROP_APP_VIEW_AUTOSCROLL_TYPE  ="app.view.autoscroll.type";

    public static final String PROP_APP_THEME  ="app.ui.theme";
    public static final String PROP_APP_THEME_DAY  ="app.ui.theme.day";
    public static final String PROP_APP_THEME_NIGHT  ="app.ui.theme.night";
    
    // available options for PROP_APP_SELECTION_ACTION setting
    public static final int SELECTION_ACTION_TOOLBAR = 0;
    public static final int SELECTION_ACTION_COPY = 1;
    public static final int SELECTION_ACTION_DICTIONARY = 2;
    public static final int SELECTION_ACTION_BOOKMARK = 3;
    
    // available options for PROP_APP_SECONDARY_TAP_ACTION_TYPE setting
    public static final int TAP_ACTION_TYPE_LONGPRESS = 0;
    public static final int TAP_ACTION_TYPE_DOUBLE = 1;
    public static final int TAP_ACTION_TYPE_SHORT = 2;

    // available options for PROP_APP_FLICK_BACKLIGHT_CONTROL setting
    public static final int BACKLIGHT_CONTROL_FLICK_NONE = 0;
    public static final int BACKLIGHT_CONTROL_FLICK_LEFT = 1;
    public static final int BACKLIGHT_CONTROL_FLICK_RIGHT = 2;

	public final static int MAX_PROFILES = 6;

}
