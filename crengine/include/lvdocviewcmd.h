#ifndef LVDOCVIEWCMD_H
#define LVDOCVIEWCMD_H

#define LVDOCVIEW_COMMANDS_START 100
/// LVDocView commands
enum LVDocCmd
{
    DCMD_BEGIN = LVDOCVIEW_COMMANDS_START,
    DCMD_LINEUP,
    DCMD_PAGEUP,
    DCMD_PAGEDOWN,
    DCMD_LINEDOWN,
    DCMD_LINK_FORWARD,
    DCMD_LINK_BACK,
    DCMD_LINK_NEXT,
    DCMD_LINK_PREV,
    DCMD_LINK_GO,
    DCMD_END,
    DCMD_GO_POS,
    DCMD_GO_PAGE,
    DCMD_ZOOM_IN,
    DCMD_ZOOM_OUT,
    DCMD_TOGGLE_TEXT_FORMAT,
    DCMD_BOOKMARK_SAVE_N, // save current page bookmark under spicified number
    DCMD_BOOKMARK_GO_N,  // go to bookmark with specified number
    DCMD_MOVE_BY_CHAPTER, // param=-1 - previous chapter, 1 = next chapter
    DCMD_GO_SCROLL_POS,  // param=position of scroll bar slider
    DCMD_TOGGLE_PAGE_SCROLL_VIEW,  // toggle paged/scroll view mode
    DCMD_LINK_FIRST, // select first link on page
    DCMD_ROTATE_BY, // rotate view, param =  +1 - clockwise, -1 - counter-clockwise
    DCMD_ROTATE_SET, // rotate viewm param = 0..3 (0=normal, 1=90`, ...)
    DCMD_SAVE_HISTORY, // save history and bookmarks
    DCMD_SAVE_TO_CACHE, // save document to cache for fast opening
    DCMD_TOGGLE_BOLD, // togle font bolder attribute
    DCMD_SCROLL_BY, // scroll by N pixels, for Scroll view mode only
    DCMD_REQUEST_RENDER, // invalidate rendered data
    DCMD_GO_PAGE_DONT_SAVE_HISTORY,
    DCMD_SET_INTERNAL_STYLES, // set internal styles option

    // selection by sentences
    DCMD_SELECT_FIRST_SENTENCE, // select first sentence on page
    DCMD_SELECT_NEXT_SENTENCE, // nove selection to next sentence
    DCMD_SELECT_PREV_SENTENCE, // nove selection to next sentence
    DCMD_SELECT_MOVE_LEFT_BOUND_BY_WORDS, // move selection start by words
    DCMD_SELECT_MOVE_RIGHT_BOUND_BY_WORDS, // move selection end by words

    // 136
    DCMD_SET_TEXT_FORMAT, // set text format, param=1 to autoformat, 0 for preformatted
    // 137
    DCMD_SET_DOC_FONTS, // set embedded fonts option (1=enabled, 0=disabled)


    //=======================================
    DCMD_EDIT_CURSOR_LEFT,
    DCMD_EDIT_CURSOR_RIGHT,
    DCMD_EDIT_CURSOR_UP,
    DCMD_EDIT_CURSOR_DOWN,
    DCMD_EDIT_PAGE_UP,
    DCMD_EDIT_PAGE_DOWN,
    DCMD_EDIT_HOME,
    DCMD_EDIT_END,
    DCMD_EDIT_INSERT_CHAR,
    DCMD_EDIT_REPLACE_CHAR
};
#define LVDOCVIEW_COMMANDS_END DCMD_SET_DOC_FONTS


#endif // LVDOCVIEWCMD_H
