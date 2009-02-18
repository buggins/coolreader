#ifndef CITECORE_H
#define CITECORE_H 1

#include "lvstyles.h"
#include "lvtinydom.h"
#include "lvdocview.h"
#include "crtrace.h"

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
            if(!is_final()){
                nextSibling();
            };
            CRLog::info("CiteCursor::CiteCursor()\n");
            crtrace trace("CiteCursor::CiteCursor() ");
            trace << pointer_.toString();
        };

    CiteCursor() : pointer_()  {};

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
        point_to_end(pointer_);
    }

    void to_begin() {
        pointer_.setOffset(0);
    }

    ldomElement *
    node() const {
        ldomNode * n = pointer_.getNode();
        assert (n->getNodeType() == LXML_ELEMENT_NODE);
        ldomElement * e = static_cast<ldomElement*>(n);
        return e;
    }

    bool
    is_final() {
        ldomNode * n = pointer_.getNode();
        if(n->getNodeType() == LXML_ELEMENT_NODE) {
            ldomElement * e = static_cast<ldomElement*>(n);
            if(e->getRendMethod() == erm_final) {
                return true;
            };
        };
        return false;
    }

    void nextSibling() {
        // FIXME: I'm unsure here!
        while(pointer_.nextSibling()) {
            if(is_final()){
                crtrace trace("CiteCursor::nextSibling() ");
                trace << pointer_.toString();
                return;
            }
        }
        pointer_.parent();
        nextSibling();
    };

    void prevSibling() {
        // Move bound up
        // FIXME: I'm unsure here!
        while(pointer_.prevSibling()) {
            if(is_final()){
                crtrace trace("CiteCursor::prevSibling");
                trace << pointer_.toString();
                break;
            }
        }
    };
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
            LVRef<ldomXRange> range = view_.getPageDocumentRange(-1);
            CiteCursor first(range->getStart());
            if (!range->isInside(first.get())) {
                first.nextSibling();
            };
            start_.assign(first);
            start_.to_begin();
            end_.assign(start_);
            end_.nextSibling();
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
        crtrace trace("highlight: ");
        LVRef<ldomXRange> range = view_.getPageDocumentRange(-1);
        trace << "[ " << range->getStart().toString() <<
            " : " << range->getEnd().toString() << " ] / [ " <<
            ldomXPointer(start_.get()).toString() << " : " << 
            ldomXPointer(end_.get()).toString() << " ]";
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
        if (!direction_) {
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
