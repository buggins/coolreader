#ifndef LVDOCVIEWPROPS_H
#define LVDOCVIEWPROPS_H

// standard properties supported by LVDocView
#define PROP_FONT_GAMMA              "font.gamma" // currently supported: 0.65 .. 1.35, see gammatbl.h
#define PROP_FONT_GAMMA_INDEX        "font.gamma.index" // currently supported: 0..30 ~= 0.65 .. 1.35, see gammatbl.h
#define PROP_FONT_ANTIALIASING       "font.antialiasing.mode"
#define PROP_FONT_HINTING            "font.hinting.mode"
#define PROP_FONT_COLOR              "font.color.default"
#define PROP_FONT_FACE               "font.face.default"
#define PROP_FONT_WEIGHT_EMBOLDEN    "font.face.weight.embolden"
#define PROP_BACKGROUND_COLOR        "background.color.default"
#define PROP_BACKGROUND_IMAGE        "background.image.filename"
#define PROP_TXT_OPTION_PREFORMATTED "crengine.file.txt.preformatted"
#define PROP_LOG_FILENAME            "crengine.log.filename"
#define PROP_LOG_LEVEL               "crengine.log.level"
#define PROP_LOG_AUTOFLUSH           "crengine.log.autoflush"
#define PROP_FONT_SIZE               "crengine.font.size"
#define PROP_FALLBACK_FONT_FACE      "crengine.font.fallback.face"
#define PROP_STATUS_FONT_COLOR       "crengine.page.header.font.color"
#define PROP_STATUS_FONT_FACE        "crengine.page.header.font.face"
#define PROP_STATUS_FONT_SIZE        "crengine.page.header.font.size"
#define PROP_PAGE_MARGIN_TOP         "crengine.page.margin.top"
#define PROP_PAGE_MARGIN_BOTTOM      "crengine.page.margin.bottom"
#define PROP_PAGE_MARGIN_LEFT        "crengine.page.margin.left"
#define PROP_PAGE_MARGIN_RIGHT       "crengine.page.margin.right"
#define PROP_ROUNDED_CORNERS_MARGIN  "crengine.rounded.corners.margin"
#define PROP_PAGE_VIEW_MODE          "crengine.page.view.mode" // pages/scroll
#define PROP_INTERLINE_SPACE         "crengine.interline.space"
#if CR_INTERNAL_PAGE_ORIENTATION==1
#define PROP_ROTATE_ANGLE            "window.rotate.angle"
#endif
#define PROP_EMBEDDED_STYLES         "crengine.doc.embedded.styles.enabled"
#define PROP_EMBEDDED_FONTS          "crengine.doc.embedded.fonts.enabled"
#define PROP_DISPLAY_INVERSE         "crengine.display.inverse"
#define PROP_DISPLAY_FULL_UPDATE_INTERVAL "crengine.display.full.update.interval"
#define PROP_DISPLAY_TURBO_UPDATE_MODE "crengine.display.turbo.update"
#define PROP_STATUS_LINE             "window.status.line"
#define PROP_BOOKMARK_ICONS          "crengine.bookmarks.icons"
#define PROP_FOOTNOTES               "crengine.footnotes"
#define PROP_SHOW_TIME               "window.status.clock"
#define PROP_SHOW_TITLE              "window.status.title"
#define PROP_STATUS_CHAPTER_MARKS    "crengine.page.header.chapter.marks"
#define PROP_SHOW_BATTERY            "window.status.battery"
#define PROP_SHOW_POS_PERCENT        "window.status.pos.percent"
#define PROP_SHOW_PAGE_COUNT         "window.status.pos.page.count"
#define PROP_SHOW_PAGE_NUMBER        "window.status.pos.page.number"
#define PROP_SHOW_BATTERY_PERCENT    "window.status.battery.percent"
#define PROP_FONT_KERNING_ENABLED    "font.kerning.enabled"
#define PROP_FONT_LIGATURES_ENABLED  "font.ligatures.enabled"
#define PROP_LANDSCAPE_PAGES         "window.landscape.pages"
#define PROP_HYPHENATION_DICT        "crengine.hyphenation.directory"
#define PROP_HYPHENATION_DICT_VALUE_NONE "@none"
#define PROP_HYPHENATION_DICT_VALUE_ALGORITHM "@algorithm"
#define PROP_AUTOSAVE_BOOKMARKS      "crengine.autosave.bookmarks"

#define PROP_FLOATING_PUNCTUATION    "crengine.style.floating.punctuation.enabled"
#define PROP_FORMAT_MIN_SPACE_CONDENSING_PERCENT "crengine.style.space.condensing.percent"

#define PROP_FILE_PROPS_FONT_SIZE    "cr3.file.props.font.size"


#define PROP_CACHE_VALIDATION_ENABLED  "crengine.cache.validation.enabled"
#define PROP_MIN_FILE_SIZE_TO_CACHE  "crengine.cache.filesize.min"
#define PROP_FORCED_MIN_FILE_SIZE_TO_CACHE  "crengine.cache.forced.filesize.min"
#define PROP_PROGRESS_SHOW_FIRST_PAGE  "crengine.progress.show.first.page"
#define PROP_HIGHLIGHT_COMMENT_BOOKMARKS "crengine.highlight.bookmarks"
#define PROP_HIGHLIGHT_SELECTION_COLOR "crengine.highlight.selection.color"
#define PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT "crengine.highlight.bookmarks.color.comment"
#define PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION "crengine.highlight.bookmarks.color.correction"
// image scaling settings
// mode: 0=disabled, 1=integer scaling factors, 2=free scaling
// scale: 0=auto based on font size, 1=no zoom, 2=scale up to *2, 3=scale up to *3
#define PROP_IMG_SCALING_ZOOMIN_INLINE_MODE  "crengine.image.scaling.zoomin.inline.mode"
#define PROP_IMG_SCALING_ZOOMIN_INLINE_SCALE  "crengine.image.scaling.zoomin.inline.scale"
#define PROP_IMG_SCALING_ZOOMOUT_INLINE_MODE "crengine.image.scaling.zoomout.inline.mode"
#define PROP_IMG_SCALING_ZOOMOUT_INLINE_SCALE "crengine.image.scaling.zoomout.inline.scale"
#define PROP_IMG_SCALING_ZOOMIN_BLOCK_MODE  "crengine.image.scaling.zoomin.block.mode"
#define PROP_IMG_SCALING_ZOOMIN_BLOCK_SCALE  "crengine.image.scaling.zoomin.block.scale"
#define PROP_IMG_SCALING_ZOOMOUT_BLOCK_MODE "crengine.image.scaling.zoomout.block.mode"
#define PROP_IMG_SCALING_ZOOMOUT_BLOCK_SCALE "crengine.image.scaling.zoomout.block.scale"

#endif // LVDOCVIEWPROPS_H
