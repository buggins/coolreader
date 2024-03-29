/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007-2013 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2018,2020,2021 Aleksey Chernov <valexlin@gmail.com>     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

#ifndef LVDOCVIEWCMD_H
#define LVDOCVIEWCMD_H

#define LVDOCVIEW_COMMANDS_START 100
/// LVDocView commands
enum LVDocCmd
{
    // 100 -
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
    // 110 -
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
    // 120 -
    DCMD_TOGGLE_PAGE_SCROLL_VIEW,  // toggle paged/scroll view mode
    DCMD_LINK_FIRST, // select first link on page
    DCMD_ROTATE_BY, // rotate view, param =  +1 - clockwise, -1 - counter-clockwise
    DCMD_ROTATE_SET, // rotate viewm param = 0..3 (0=normal, 1=90`, ...)
    DCMD_SAVE_HISTORY, // save history and bookmarks
    DCMD_SAVE_TO_CACHE, // save document to cache for fast opening
    DCMD_SET_BASE_FONT_WEIGHT, // set base font weight globally, replaces DCMD_TOGGLE_BOLD
    DCMD_SCROLL_BY, // scroll by N pixels, for Scroll view mode only
    DCMD_REQUEST_RENDER, // invalidate rendered data
    DCMD_GO_PAGE_DONT_SAVE_HISTORY,
    // 130 -
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

    // 138
    DCMD_SET_REQUESTED_DOM_VERSION,     // set requested dom version for document parsing
    DCMD_RENDER_BLOCK_RENDERING_FLAGS,  // set requested dom version for document parsing

    // 140
    DCMD_SET_ROTATION_INFO_FOR_AA,     // set screen rotation info (but don't rotate screen) to remap font AA subpixel modes

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
#define LVDOCVIEW_COMMANDS_END DCMD_SET_ROTATION_INFO_FOR_AA


#endif // LVDOCVIEWCMD_H
