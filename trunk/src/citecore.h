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

// Set offset to last character
void
point_to_end(ldomXPointerEx& p) {
    lString16 s(p.getText());
    p.setOffset(s.length());
};

class CiteCursor {
    ldomXPointerEx pointer_;
public:
    CiteCursor(const ldomXPointerEx& xp) :
        pointer_(xp) {
            crtrace trace("CiteCursor::CiteCursor() ");
            if(!pointer_.isVisibleFinal()){
                trace << pointer_.toString() << " NOT visible ";
                pointer_.nextVisibleFinal();
            };
            trace << pointer_.toString();
        };

    CiteCursor() : pointer_()  {};
    CiteCursor(const CiteCursor& other) :
        pointer_(other.pointer_) {};

    void
    assign(const ldomXPointerEx& xp) { 
        pointer_=xp; 
        crtrace trace("CiteCursor::assign() ");
        trace << pointer_.toString();
    }

    void
    assign(const CiteCursor& c) { assign(c.get()); }

    const ldomXPointerEx& get() const {
        return pointer_;
    }

    void to_end() {
		pointer_.setOffset( pointer_.getNode()->getChildCount() );
    }

    void to_begin() {
        pointer_.setOffset(0);
    }


    void nextSiblingBlock () {
        pointer_.nextVisibleFinal();
    };

    void nextSibling() {
        // switch on granularity
        nextSiblingBlock();
    }

    void prevSiblingBlock() {
        pointer_.prevVisibleFinal();
    };

    void prevSibling() {
        // switch on granularity
        prevSiblingBlock();
    };

	lString16 toString() { return pointer_.toString(); }
};


class CiteSelection {
    bool direction_;
    LVDocView& view_;
    CiteCursor start_;
    CiteCursor end_;
public:
    CiteSelection(LVDocView& view) : 
        direction_(false),
        view_(view) {
//            LVRef<ldomXRange> range = view_.getPageDocumentRange(-1);
            CiteCursor first(view.getCurrentPageMiddleParagraph());
            start_.assign(first);
            start_.to_begin();
            end_.assign(start_);
            end_.to_end();
            highlight();
        };

    ldomXRange
    select() const {
        return ldomXRange(start_.get(), end_.get());
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
        view_.selectRange(select()); 
    }

    // true -- upper bound, false -- lower
    void setDirection(bool dir) { direction_ = dir; }

    void stepUp() {
        if (direction_) {
            CRLog::info("grow up\n");
            start_.prevSibling();
            start_.to_begin();
        } else {
            CRLog::info("shrink up\n");
            end_.prevSibling();
            end_.to_end();
        };
        highlight();
    };

    void stepDown() {
        if (direction_) {
            CRLog::info("shrink down\n");
            start_.nextSibling();
            start_.to_begin();
        } else {
            CRLog::info("grow down\n");
            end_.nextSibling();
            end_.to_end();
        };
        highlight();
    };

};


#endif
