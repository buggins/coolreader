#include <cstdlib>
#include "dictdlg.h"
//#include "libdictd.h"
#include "lvstring.h"
#include "lvref.h"
//#include "selector.h"
#include "crgui.h"
#include "crtrace.h"



#include "lvarray.h"
#include "lvstring.h"
#include "lvtinydom.h"
#include "lvdocview.h"
#include "crtrace.h"
#include <stdexcept>
#include "mainwnd.h"

//#define USE_LIBDICTD

template <typename T>
class Dictionary {
    typedef T string_type;
    T config_;
//    static Dictionary<T> * self_;
public:
    Dictionary(const T& config,const T& progname) : config_(config) {
#ifdef USE_LIBDICTD
        libdict_init(progname.c_str(), config_.c_str());
#endif
    }
    virtual ~Dictionary() { 
#ifdef USE_LIBDICTD
        libdict_uninit();
#endif
    }

    T translate(const T& word) {
#ifdef USE_LIBDICTD
        char * ptr;
        ptr = libdict_define(word.c_str(),"*");
        if(!ptr)
            return T();
        T tmp(ptr);
        libdict_free(ptr);
        return tmp;
#else
		return T("Temporary stub for dictionary.\nFake article for ") + word;

#endif
    }
};


class wordlist {
    LVDocView& docview_;
    LVArray<ldomWord> words_;
public:
    wordlist(LVDocView& docview) : docview_(docview) {
        ldomDocument * doc = docview.getDocument();
        int pageIndex = -1; //docview.getCurPage();
        LVRef<ldomXRange> range = docview.getPageDocumentRange( pageIndex );
        if(!range.isNull()) {
            range->getRangeWords(words_);
        };
    }
    
    LVArray<int> match(const lString16& prefix) {
        LVArray<int> result;
        crtrace dumpstr;
        dumpstr << "match with " << prefix;
        for(int i=0;i<words_.length();i++) {
            lString16 wordtext(words_[i].getText());
            // FIXME: startsWirh must be const
            if (wordtext.startsWithNoCase(prefix)) {
                result.add(i);
                dumpstr << " " << i << " " << wordtext;
            }
        }
        return result;
    }

    // for "foo" "bar" "baz" in words_ nth_chars(2) return
    //    "o" "a" "a"
    LVArray<lChar16> nth_chars(int pos) {
        LVArray<lChar16> result;
        for (int i=0; i < words_.length(); i++) {
            lString16 wordtext(words_[i].getText()); 
            // FIXME: startsWirh must be const
            if ((int)wordtext.length() >= pos) {
                result.add(wordtext[pos]);
            }
        }
        return result;
    }

    void highlight(int i) {
        LVArray<ldomWord> selected;




        crtrace trace;
        trace << "Select word #" << i << " ";
        assert(i < words_.length());
        trace << words_[i].getText();

        selected.add(words_[i]);
        docview_.selectWords(selected);
    }
    void highlight(LVArray<int> list) {
        LVArray<ldomWord> selected;
        for (int i=0; i < list.length(); i++) {
            selected.add(words_[list[i]]);
        };
        docview_.selectWords(selected);
    }

    lString16 get(int pos) {
        return words_[pos].getText();
    }
};

void
init_keytable(LVArray<lString16>& keytable){
#define DEFKEY(x)  keytable.add(lString16(x))
DEFKEY(L" "); // 0 and 9 are STUBs
DEFKEY(L"abc");
DEFKEY(L"def");
DEFKEY(L"ghi");
DEFKEY(L"jkl");
DEFKEY(L"mno");
DEFKEY(L"pqrs");
DEFKEY(L"tuv");
DEFKEY(L"wxyz");
DEFKEY(L".,"); // 9 STUB
#undef DEFKEY
};

class selector {
    wordlist words_;
    int current_;
    int level_;
    LVArray<int> candidates_;
    lString16 prefix_;
    LVArray<lString16> keytable_;
    int repeat_;
    int last_;
public:
    selector(LVDocView& docview) : 
        words_(docview), 
        current_(0), 
        level_(0),
        candidates_(),
        repeat_(0),
        prefix_(),
        keytable_()
        {
            init_keytable(keytable_);
            update_candidates();
        };

    void moveBy( int delta )
    {
        current_ += delta;
        if ( current_ < 0 )
            current_ = candidates_.length()-1;
        if ( current_ >= candidates_.length() )
            current_ = 0;
        if ( current_ < 0 )
            current_ = 0;
        if ( candidates_.length() )
            words_.highlight(candidates_[current_]);
    }

    void up() { moveBy( -1 ); }

    void down() { moveBy( 1 ); }

    void update_candidates() {
        CRLog::info("update_candidates() enter\n");
        candidates_ = words_.match(prefix_);
        CRLog::info("update_candidates() mid\n");
        current_=0;
        CRLog::info("update_candidates() mid2\n");
        if(candidates_.length() == 0) {
            CRLog::info("nothig to highlight");
            return;
        };
        words_.highlight(candidates_[current_]);
        CRLog::info("update_candidates() leave\n");
    }

    bool push(lChar16 key) {
        LVArray<lChar16> keys = words_.nth_chars(level_);
        for(int i=0; i<keys.length();i++){
            if (keys[i==key]) {
                prefix_ << key;
                level_++;
                update_candidates();
                return true;
            }
        }
        return false;
    }

    bool push_button(int key) {
        key = key - '0';
        const lString16& keylist(keytable_[key]);
        for (unsigned i=0; i <keylist.length(); i++) {
            if(push(keylist[0])){
                return true;
            }
        }
        return false;
    }

    bool pop() {
        if (level_==0) {
            return true;
        }
        prefix_ = prefix_.substr(0,prefix_.length()-1);
        level_--;
        update_candidates();
        return false;
    }

    lString16 get() {
        return words_.get(candidates_[current_]);
    }
};



class DictWindow;

class Article : public  CRDocViewWindow {
    LVStreamRef stream_;
    DictWindow * parent_;
public:
    Article(CRGUIWindowManager * wm, lString16 title, lString8 text, DictWindow * parent) :
        CRDocViewWindow(wm),
        parent_(parent) {
            if ( text.empty() )
                text = lString8(L"Test article.\nA little text only.\n");
            stream_ = LVCreateStringStream( text );
            _skin = _wm->getSkin()->getWindowSkin(L"#dialog");
			_title = title;
			lvRect rc = _wm->getScreen()->getRect();
			int dx = rc.width() / 6;
			int dy = rc.height() / 6;
			rc.left += dx;
			rc.top += dy;
			rc.right -= dx;
			rc.bottom -= dy;
			_fullscreen = false;
			setRect( rc );
            getDocView()->setBackgroundColor(0xFFFFFF);
            getDocView()->setTextColor(0x000000);
            getDocView()->setFontSize( 20 );
            getDocView()->setShowCover( false );
            getDocView()->LoadDocument(stream_);

        }

    virtual bool onCommand( int command, int params = 0 );
};

typedef Dictionary<lString8> dict_type;
LVRef<dict_type> dict_;

class DictWindow : public CRGUIWindowBase
{
	V3DocViewWin * mainwin_;
    LVDocView& docview_;
    selector selector_;
    lString8 progname_;
    lString8 dict_conf_;
protected:
	virtual void draw() { }
public:

	DictWindow( CRGUIWindowManager * wm, V3DocViewWin * mainwin ) :
		CRGUIWindowBase(wm),
		mainwin_(mainwin),
		docview_(*mainwin->getDocView()),
		selector_(*mainwin->getDocView()),
		progname_("lbook-fb2-plugin"),
		dict_conf_("/etc/dictd/dictd.conf") {

		this->setAccelerators( mainwin->getDialogAccelerators() );

		if (dict_.isNull()){
			dict_ = LVRef<dict_type>(new dict_type(dict_conf_,progname_));
		};
		_rect.bottom = _rect.top;
	}

	bool onCommand( int command, int params )
	{
		switch ( command ) {
			case MCMD_SELECT_0:
			case MCMD_SELECT_1:
			case MCMD_SELECT_2:
			case MCMD_SELECT_3:
			case MCMD_SELECT_4:
			case MCMD_SELECT_5:
			case MCMD_SELECT_6:
			case MCMD_SELECT_7:
			case MCMD_SELECT_8:
			case MCMD_SELECT_9:
				selector_.push_button( command - MCMD_SELECT_0 + '0' );
				break;
			case MCMD_SCROLL_FORWARD:
				selector_.down();
				break;
			case MCMD_SCROLL_BACK:
				selector_.up();
				break;
			case MCMD_OK:
				{
					lString8 translated;
					lString8 output;
					lString16 src = selector_.get();
					translated = dict_->translate( UnicodeToUtf8(src) );
					if(translated.length() == 0) {
						output = lString8("No article for this word");
					} else {
						output = lString8("<?xml version=\"1.0\" encoding=\"UTF-8\">");
						output << "<FictionBook><body><p>";
						output << translated;
						output << "</p></body></FictionBook>";
					};
					crtrace crt("article: ");
					crt << output;
					Article * article = new Article(_wm, src, output, this);
					article->setAccelerators( mainwin_->getDialogAccelerators() );
					_wm->activateWindow( article );
				};
				break;
			case MCMD_CANCEL:
				if ( selector_.pop() ) {
					close();
					return true ;
				}
				break;
		}
		return true;
	}

    void close() {
        CRLog::info("Closing dict");
        docview_.clearSelection();
        _wm->closeWindow(this);
    };


protected:
    DictWindow(const DictWindow&); //non-copyable
};



bool Article::onCommand( int command, int params )
{
        switch ( command ) {
			case MCMD_CANCEL:
			case MCMD_OK:
                parent_->close();
                _wm->closeWindow(this);
                return true;
			case MCMD_SCROLL_FORWARD:
			case MCMD_SCROLL_BACK:
				_wm->postCommand(command, 0);
                _wm->closeWindow(this);
				return true;
            default:
                return true; //CRDocViewWindow::onKeyPressed(key,flags);
        }
}

void
activate_dict(CRGUIWindowManager *wm, V3DocViewWin * mainwin) {
    CRLog::info("Entering dict mode\n");
    wm->activateWindow(new DictWindow(wm,mainwin));
};
