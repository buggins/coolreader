#ifndef CITECORE_H
#define CITECORE_H 1

#include "lvxml.h"
#include "lvdocview.h"


// Set offset to last character
void
point_to_end(ldomXPointerEx& p) {
    lString16 s(p.getText());
    p.setOffset(s.length())
}

class CiteCursor {
    ldomXPointerEx pointer_;
public:
    CiteCursor(const ldomXPointerEx& xp) :
        pointer_(xp);

    const ldomXPointerEx& get() const {
        return pointer_;
    }

    void to_end() {
        point_to_end(pointer_);
    }

    void to_begin() {
        pointer_.setOffset(0);
    }

    void nextSibling() {
        // FIXME: I'm unsure here!
        if(pointer_.nextSibling()) {
            do {
            if (pointer_.getNode()->getRendMethod == erm_final) {
                    // we found next block
                    return;
                }
            } while(pointer_.nextSibling())
        }
    };

    void prevSibling() {
        // Move bound up
    };
}


class CiteSelection {
    bool direction_;
    LVDocView& view_;
    CiteCursor start_;
    CiteCursor end_;
public:
    CiteSelection(LVDocView& view) : 
        direction_(true),
        view_(view) {
            LVRef<ldomXRange> range = docview.getPageDocumentRange(-1);
            ldomXPointerEx first(range.getStart());
            if((first.getNode()->getRendMethod() == erm_final)
                && first.getOffset() == 0) {
                start_=first;
                // case when block fit to screen bounds
            } else {
                // case when upper screen bound point to middle of block
                // we need to find next block.
            }
            end_=first;
            end_.to_end();
        }

    ldomXRange&
    select() {
        return ldomXRange(top_.get(), bottom_.get())
    }

    // true -- upper bound, false -- lower
    void setDirection(bool dir) { direction_ = dir; }

    void stepUp() {
        if (dir) {
            // grow up
            top_.stepUp();
            top_.to_begin();
        } else {
            // shrink up
            bottom_.stepUp();
            bottom_.to_end();
        };
    };

    void stepDown() {
        if (!dir) {
            // shrink up
            top_.to_begin();
            top_.stepUp();
        } else {
            // grow down
            bottom_.stepDown();
            bottom_.to_end()
        };
    }

};


#endif
