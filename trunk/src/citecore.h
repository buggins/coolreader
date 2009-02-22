#ifndef CITECORE_H
#define CITECORE_H 1

#include "lvstyles.h"
#include "lvtinydom.h"
#include "lvdocview.h"
#include "crtrace.h"


enum granularity {
    grn_char,
    grn_word,
    grn_block,
};


enum last_move_type {
    not_moved,
    moved_up,
    moved_down,
};

void
point_to_end(ldomXPointerEx& xp) {
    ldomNode * p = xp.getNode();
    if(p->isText()) {
        lString16 text = p->getText();
        xp.setOffset(text.length());
    };
    if(p->isElement()) {
        xp.setOffset(p->getChildCount());
    }
};

void
point_to_begin(ldomXPointerEx& xp) {
    ldomNode * p = xp.getNode();
    if(p->isText())
        xp.setOffset(0);
}

class CiteSelection {
    last_move_type last_move_;
    LVDocView& view_;
    ldomXPointerEx start_;
    ldomXPointerEx end_;
public:
    CiteSelection(LVDocView& view) : 
        last_move_(not_moved),
        view_(view) {
            ldomXPointerEx middle = view_.getCurrentPageMiddleParagraph();
            middle.prevVisibleText();
            start_ = middle;
            point_to_begin(start_);
            end_ = middle;
            point_to_end(end_);
            highlight();
        };

    ldomXRange
    select() const {
        return ldomXRange(start_, end_);
    }

    void
    highlight() {
        // show what we try to highlight
        crtrace trace("highlight: \n");
        LVRef<ldomXRange> range = view_.getPageDocumentRange(-1);
        trace << "\tscreen: \n\t\t [ " << range->getStart().toString() <<
            " :\n\t\t" << range->getEnd().toString() << " ] / \n\tselection: [ " <<
            start_.toString() << " :\n\t\t" << 
            end_.toString() << " ]";
        view_.clearSelection();
        ldomXRange selected(start_, end_, 1);
        view_.selectRange(selected); 
    }

    void
    move_to_upper_bound() {
        LVRef<ldomXRange> range = view_.getPageDocumentRange(-1);
        if(!range->isInside(start_)) {
                CRLog::info("move_to_upper_bound(): Scroll Up\n");
                view_.goToBookmark(start_);
        }
        highlight();
    };

    void
    move_to_lower_bound() {
        LVRef<ldomXRange> range = view_.getPageDocumentRange(-1);
        if(!range->isInside(end_)) {
                CRLog::info("move_to_lower_bound(): Scroll Down\n");
                view_.goToBookmark(end_);
        };
        highlight();
    };

    void growUp() {
        CRLog::info("grow up\n");
        start_.prevVisibleText();
        move_to_upper_bound();
    }

    void shrinkDown() {
        CRLog::info("shrink down\n");
        ldomXPointerEx tmpptr(start_);
        tmpptr.nextVisibleText();
        if (start_.compare(tmpptr) < 0) {
            start_ = tmpptr;
        };
        move_to_upper_bound();
    }

    void shrinkUp() {
        CRLog::info("shrink up\n");
        ldomXPointerEx tmpptr(end_);
        tmpptr.prevVisibleText();
        if(end_.compare(tmpptr) > 0) {
            end_ = tmpptr;
        };
        move_to_lower_bound();
    }

    void growDown() {
        CRLog::info("grow down\n");
        end_.nextVisibleText();
        move_to_lower_bound();
    }

};


#endif
