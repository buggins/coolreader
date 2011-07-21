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

//const lString16 phrase_bounds(L".?!");


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
        xp.lastChild();
        if (!xp.isText())
            xp.nextVisibleText();
        if (xp.isText()) {
            ldomNode * p = xp.getNode();
            lString16 text = p->getText();
            xp.setOffset(text.length());
        }
    }
};

void
point_to_begin(ldomXPointerEx& xp) {
    ldomNode * p = xp.getNode();
    if(p->isText())
        xp.setOffset(0);
}

bool
contains(const lString16& s, const lString16::value_type c) {
    for(unsigned int i = 0; i < s.length(); ++i) {
        if(s[i]==c) {
            return true;
        }
    }
    return false;
}

ldomXPointerEx
upper_phrase_bound(ldomXPointer& p, const lString16 bounds, bool skip = false) {
    ldomXPointerEx xp(p);
    bool moved = false;
    while(1) {
        if (moved || !xp.isText()) {
            if (!xp.prevVisibleText()) {
                xp.setOffset(0);
                return xp;
            }
            moved = true;
        }
        ldomNode *n = xp.getNode();
        lString16 text = n -> getText();
        int offset = !moved ? xp.getOffset() : text.length();
        while (offset > 0) {
            --offset;
            if ((skip && !contains(bounds, text[offset])) || (!skip && contains(bounds, text[offset]))) {
                xp.setOffset(offset);
                return xp;
            }
        }
        moved = true;
    };
}

ldomXPointerEx
lower_phrase_bound(ldomXPointer& p, const lString16 bounds, bool skip = false) {
    ldomXPointerEx xp(p);
    bool moved = false;
    while(1) {
        if (moved || !xp.isText()) {
            if (!xp.nextVisibleText())
                return p;
        }
        ldomNode *n = xp.getNode();
        lString16 text = n -> getText();
        int offset = xp.getOffset();
        while(offset < text.length()) {
            ++offset;  
            if ((skip && !contains(bounds, text[offset])) || (!skip && contains(bounds, text[offset]))) {
                xp.setOffset(offset);
                return xp;
            }
        };
        moved = true;
    };
}


bool is_at_bound(ldomXPointer& p, const lString16 bounds) {
    ldomXPointerEx xp(p);
    if (xp.isText()) {
        lString16 text = xp.getText();
        int offset = xp.getOffset();
        if (offset < text.length())
            return contains(bounds, text[offset]);
    }
    return false;
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

    void getRange( ldomXRange & range )
    {
            range.setStart( start_ );
            range.setEnd( end_ );
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
        ldomXPointerEx ptr(start_);
        if(prevPara(ptr)){
            start_ = ptr;
            point_to_begin(start_);
            move_to_upper_bound();
        };
    }

    void shrinkDown() {
        CRLog::info("shrink down\n");
        ldomXPointerEx ptr(start_);
        if (nextPara(ptr)) {
            if (end_.compare(ptr) >= 0)
                start_ = ptr;
            point_to_begin(start_);
            move_to_upper_bound();
        }
    }

    void shrinkUp() {
        CRLog::info("shrink up\n");
        ldomXPointerEx ptr(end_);
        if (prevPara(ptr)) {
            if (start_.compare(ptr) <= 0)
                end_ = ptr;
            point_to_end(end_);
            move_to_lower_bound();
        };
    }

    void growDown() {
        CRLog::info("grow down\n");
        ldomXPointerEx ptr(end_);
        if (nextPara(ptr)) {
            end_ = ptr;
            point_to_end(end_);
            move_to_lower_bound();
        }
    }

    void growUpPhrase() {
        lString16 bounds(L".?!");
        ldomXPointerEx pptr(start_);
        bool pp = prevPara(pptr);
        ldomXPointerEx xp = upper_phrase_bound(start_, bounds);        
        xp = upper_phrase_bound(xp, bounds, true);
        xp = upper_phrase_bound(xp, bounds);
        ldomXPointerEx ptr(xp);
        if (ptr.nextVisibleWordStart())
            xp = ptr;
        if (pp && xp.compare(pptr) < 0)
            start_ = pptr;
        else
            start_ = xp;
        move_to_upper_bound();
    }

    void shrinkDownPhrase() {
        lString16 bounds(L".?!");
        ldomXPointerEx pptr(start_);
        bool np = nextPara(pptr);
        ldomXPointerEx xp = lower_phrase_bound(start_, bounds);
        ldomXPointerEx ptr(xp);
        if (ptr.nextVisibleWordStart())
            xp = ptr;
        if (np && xp.compare(pptr) > 0)
            start_ = pptr;
        else
            start_ = xp;
        if (end_.compare(xp) >= 0) {
            start_ = xp;
        };
        move_to_upper_bound();
    }


    void shrinkUpPhrase() {
        lString16 bounds(L".?!");
        ldomXPointerEx xp(end_);
        if (is_at_bound(xp, bounds))
            xp = upper_phrase_bound(xp, bounds, true);
        else
            xp = upper_phrase_bound(xp, bounds);
        xp = upper_phrase_bound(xp, bounds);
        ldomXPointerEx ptr(xp);
        if (ptr.nextVisibleWordStart())
            xp = ptr;
        if (start_.compare(xp) <= 0) {
            end_ = xp;
        };
        move_to_lower_bound();
    }

    void growDownPhrase() {
        lString16 bounds(L".?!");
        ldomXPointerEx pptr(end_);
        bool np = nextPara(pptr);
        ldomXPointerEx xp(end_);
        if (is_at_bound(xp, bounds))
            lower_phrase_bound(xp, bounds, true);
        xp = lower_phrase_bound(xp, bounds);
        xp = lower_phrase_bound(xp, bounds, true);
        if (np && xp.compare(pptr) > 0)
            end_ = pptr;
        else
            end_ = xp;
        move_to_lower_bound();
    }


    bool prevPara(ldomXPointerEx &ptr) {
        if (ptr.isVisibleFinal())
            return ptr.prevVisibleFinal();
        return (ptr.ensureFinal() && ptr.prevVisibleFinal());
    }

    bool nextPara(ldomXPointerEx &ptr) {
        if (ptr.isVisibleFinal())
            return ptr.nextVisibleFinal();
        return (ptr.ensureFinal() && ptr.nextVisibleFinal());
    }

    void moveUp() {
        ldomXPointerEx ptr(start_);
        if (prevPara(ptr)) {
            start_ = ptr;
            end_ = start_;
            point_to_begin(start_);
            point_to_end(end_);
            move_to_upper_bound();
        };
    };

    void moveDown() {
        ldomXPointerEx ptr(start_);
        if (nextPara(ptr)) {
            start_ = ptr;
            end_ = start_;
            point_to_begin(start_);
            point_to_end(end_);
            move_to_lower_bound();
        }
    }

};


#endif
