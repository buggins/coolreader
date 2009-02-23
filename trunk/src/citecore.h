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

const lString16 phrase_bounds(L".?!");

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
upper_phrase_bound(ldomXPointer& p, const lString16 bounds) {
    ldomXPointerEx xp(p);
    ldomNode * n = xp.getNode();
    if(!n->isText())
        return p;
    int offset = p.getOffset();
    lString16 text(n->getText());
    while(1) {
        while(offset > 0) {
            --offset;
            if(contains(bounds, text[offset])){
                xp.setOffset(offset);
                return xp;
            }
        }
        if(!xp.prevVisibleText())
            return p;
        n = xp.getNode();
        text = n -> getText();
        offset = text.length();
    };
}

ldomXPointerEx
lower_phrase_bound(ldomXPointer& p, const lString16 bounds) {
    ldomXPointerEx xp(p);
    ldomNode * n = xp.getNode();
    if(!n->isText())
        return p;
    unsigned int offset = p.getOffset();
    lString16 text(n->getText());
    while(1) {
        while(offset < text.length()) {
            ++offset;  
            if(contains(bounds, text[offset])){
                xp.setOffset(offset);
                return xp;
            }
        };
        if(!xp.nextVisibleText()) 
            return p;
        n = xp.getNode();
        text = n->getText();
        offset = 0;
    };
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
        start_.prevVisibleText();
        point_to_begin(start_);
        move_to_upper_bound();
    }

    void shrinkDown() {
        CRLog::info("shrink down\n");
        ldomXPointerEx tmpptr(start_);
        tmpptr.nextVisibleText();
        if (end_.compare(tmpptr) >= 0) {
            start_ = tmpptr;
        };
        point_to_begin(start_);
        move_to_upper_bound();
    }

    void shrinkUp() {
        CRLog::info("shrink up\n");
        ldomXPointerEx tmpptr(end_);
        tmpptr.prevVisibleText();
        if(start_.compare(tmpptr) <= 0) {
            end_ = tmpptr;
        };
        point_to_end(end_);
        move_to_lower_bound();
    }

    void growDown() {
        CRLog::info("grow down\n");
        end_.nextVisibleText();
        point_to_end(end_);
        move_to_lower_bound();
    }

    void growUpPhrase() {
        start_ = upper_phrase_bound(start_, phrase_bounds);
        move_to_upper_bound();
    }

    void shrinkDownPhrase() {
        ldomXPointerEx xp = lower_phrase_bound(start_, phrase_bounds);
        if (end_.compare(xp) >= 0) {
            start_ = xp;
        };
        move_to_upper_bound();
    }

    void shrinkUpPhrase() {
        ldomXPointerEx xp = upper_phrase_bound(end_, phrase_bounds);
        if(start_.compare(xp) <= 0) {
            end_ = xp;
        };
        move_to_lower_bound();
    }

    void growDownPhrase() {
        end_ = lower_phrase_bound(end_, phrase_bounds);
        move_to_lower_bound();
    }


    void moveUp() {
        if(start_.prevVisibleText()){
            end_.prevVisibleText();
            point_to_begin(start_);
            point_to_end(end_);
            move_to_upper_bound();
        };
    };

    void moveDown() {
        if(end_.nextVisibleText()){
            start_.nextVisibleText();
            point_to_begin(start_);
            point_to_end(end_);
            move_to_lower_bound();
        }
    }

};


#endif
