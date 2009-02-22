//#include <cstdlib>
#include "dictdlg.h"
#include <lvstring.h>
#include <lvref.h>
//#include "selector.h"
#include <crgui.h>
#include <crtrace.h>


#include <lvarray.h>
#include <lvstring.h>
#include <lvtinydom.h>
#include <lvdocview.h>
#include <stdexcept>
#include "mainwnd.h"
#include "viewdlg.h"
#include "bgfit.h"
#include "t9encoding.h"



//TODO: place TinyDictionary to separate file
CRTinyDict::CRTinyDict( const lString16& config )
{
    lString16 path = config;
    LVAppendPathDelimiter( path );
    LVContainerRef dir = LVOpenDirectory( config.c_str() );
    if ( !dir )
        dir = LVOpenDirectory( LVExtractPath(config).c_str() );
    if ( !dir.isNull() ) {
        int count = dir->GetSize();
        lString16 indexExt(L".index");
        for ( int i=0; i<count; i++ ) {
            const LVContainerItemInfo * item = dir->GetObjectInfo( i );
            if ( !item->IsContainer() ) {
                lString16 name = item->GetName();
                if ( name.endsWith( indexExt ) ) {
                    lString16 nameBase = name.substr( 0, name.length() - indexExt.length() );
                    lString16 name1 = nameBase + L".dict";
                    lString16 name2 = nameBase + L".dict.dz";
                    lString16 dataName;
                    int index = -1;
                    for ( int n=0; n<count; n++ ) {
                        const LVContainerItemInfo * item2 = dir->GetObjectInfo( n );
                        if ( !item2->IsContainer() ) {
                            if ( item2->GetName() == name1 || item2->GetName() == name2 ) {
                                index = n;
                                dataName = item2->GetName();
                                break;
                            }
                        }
                    }
                    if ( index>=0 ) {
                        // found pair
                        dicts.add(UnicodeToUtf8(path + name).c_str(), UnicodeToUtf8(path + dataName).c_str());
                    }
                }
            }
        }
    }
    CRLog::info( "%d dictionaries opened", dicts.length() );
}


lString8 CRTinyDict::translate(const lString8 & w)
{
    lString16 s16 = Utf8ToUnicode( w );
    s16.lowercase();
    lString8 word = UnicodeToUtf8( s16 );
    lString8 body;
    TinyDictResultList results;
    if ( dicts.length() == 0 ) {
        body << "<title><p>No dictionaries found</p></title>";
        body << "<p>Place dictionaries to directory 'dict' of SD card.</p>";
        body << "<p>Dictionaries in standard unix .dict format are supported.</p>";
        body << "<p>For each dictionary, pair of files should be provided: data file (with .dict or .dict.dz extension, and index file with .index extension</p>";
    } else if ( dicts.find(results, word.c_str(), TINY_DICT_OPTION_STARTS_WITH ) ) {
        for ( int d = 0; d<results.length(); d++ ) {
            TinyDictWordList * words = results.get(d);
            if ( words->length()>0 )
                body << "<title><p>From dictionary " << words->getDictionaryName() << ":</p></title>";
            // for each found word
            for ( int i=0; i<words->length(); i++ ) {
                //TinyDictWord * word = words->get(i);
                const char * article = words->getArticle( i );
                body << "<code style=\"text-align: left; text-indent: 0; font-size: 22\">";
                if ( article ) {
                    body << article;
                } else {
                    body << "[cannot read article]";
                }
                body << "</code>";
                if ( i<words->length()-1 )
                    body << "<hr/>";
            }
        }
    } else {
        body << "<title><p>Article for word " << word << " not found</p></title>";
    }

    return CRViewDialog::makeFb2Xml( body );
}



#define DICT_MIN_WORD_LENGTH 3

class WordWithRanges
{
    lString16 word;
    lString16 wordLower;
    lString8  encoded;
    LVArray<ldomWord> ranges;
public:
    const lString16 & getWord() { return word; }
    const lString8 & getEncoded() { return encoded; }
    LVArray<ldomWord> & getRanges() { return ranges; }
    bool matchEncoded( const lString8 & prefix )
    {
        if ( prefix.empty() )
            return false;
        return encoded.startsWith( prefix );
    }
    bool matchWord( const lString16 & prefix )
    {
        if ( prefix.empty() )
            return false;
        return word.startsWithNoCase( prefix );
    }
    bool equals( const lString16 & w )
    {
        lString16 w1( w );
        w1.lowercase();
        return w1 == wordLower;
    }
    void add( const ldomWord & range )
    {
        ranges.add( range );
    }
    WordWithRanges( const lString16 & w, const lString8 & enc, const ldomWord & range )
    : word( w ), encoded( enc )
    {
        wordLower = w;
        wordLower.lowercase();
        ranges.add( range );
    }
};

class wordlist {
    LVDocView& docview_;
	const TEncoding& encoding_;
    LVPtrVector<WordWithRanges> _words;
public:
    void init()
	{
		_words.clear();
        //ldomDocument * doc = docview.getDocument();
        int pageIndex = -1; //docview.getCurPage();
        LVRef<ldomXRange> range = docview_.getPageDocumentRange( pageIndex );
        crtrace trace;
        if( !range.isNull() ) {
            LVArray<ldomWord> words;
            range->getRangeWords(words);
            for ( int i=0; i<words.length(); i++ ) {
                lString16 w = words[i].getText();
                lString8 encoded = encoding_.encode_string( w );
                if ( w.length() < DICT_MIN_WORD_LENGTH )
                    continue;
                /*
                trace << "string " << w <<
                    " encoded as " << encoded << "\n";
                */
                int index = -1;
                for ( int j=0; j<_words.length(); j++ )
                    if ( _words[j]->equals(w) ) {
                        index = j;
                        break;
                    }
                if ( index>=0 )
                    _words[index]->add( words[i] );  // add range to existing word
                else
                    _words.add( new WordWithRanges( w, encoded, words[i] ) ); // add new word
            }
        }
	}

    wordlist(LVDocView& docview, const TEncoding& encoding) :
        docview_(docview), encoding_( encoding )
    {
		init();
    }

    void match( const lString8& prefix, LVArray<WordWithRanges *> & result )
    {
        crtrace dumpstr;
        dumpstr << "match with " << prefix;
        for( int i=0; i<_words.length(); i++ ) {
            if( _words[i]->matchEncoded( prefix ) ) {
                result.add( _words[i] );
                //dumpstr << " " << i << " " << encoded_words_[i];
            };
        };
    }

    void highlight( WordWithRanges * p )
    {
        crtrace trace;
        trace << "Select word " << p->getWord() << " (" << p->getRanges().length() << " occurences) : ";
        for ( int i=0; i<p->getRanges().length(); i++ ) {
            trace << "[" << p->getRanges()[i].getStart() << "," << p->getRanges()[i].getEnd() << "] ";
        }
        docview_.selectWords( p->getRanges() );
    }
};

class selector {
    wordlist words_;
    int current_;
    int level_;
    LVArray<WordWithRanges *> candidates_;
    LVArray<lString16> encoded_;
    lString8 prefix_;
//    LVArray<lString16> keytable_;
    int repeat_;
    int last_;
public:
	void reinit() { words_.init(); candidates_.clear(); }
    lString8 getPrefix() { return prefix_; }
    selector(LVDocView& docview, const TEncoding& encoding) : 
        words_(docview, encoding), 
        current_(0), 
        level_(0),
        candidates_(),
        prefix_(),
        repeat_(0)
        {
//            init_keytable(keytable_);
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

    bool update_candidates()
    {
        CRLog::info("update_candidates() enter\n");
        LVArray<WordWithRanges *> new_candidates;
        words_.match( prefix_, new_candidates );
        CRLog::info("update_candidates() mid\n");
        current_=0;
        CRLog::info("update_candidates() mid2\n");
        if( new_candidates.length() == 0 ) {
            CRLog::info("nothig to highlight");
            return false;
        };
        candidates_ = new_candidates;
        words_.highlight( candidates_[current_] );
        CRLog::info("update_candidates() leave\n");
        return true;
    }

    bool push_button(int key) {
        crtrace trace("selector::push(): ");
        lString8 old_prefix = prefix_;
        prefix_.append(1,static_cast<char>(key));
        if(update_candidates()){
            level_++;
            return true;
        }
        prefix_ = old_prefix;
        return false;
    }


    bool pop()
    {
        if (level_==0) {
            return true;
        }
        prefix_ = prefix_.substr(0,prefix_.length()-1);
        level_--;
        update_candidates();
        return false;
    }

    const lString16 get()
    {
        if ( current_ >= 0 && current_ < candidates_.length() )
            return candidates_[current_]->getWord();
        return lString16();
    }
};


class CRT9Keyboard : public BackgroundFitWindow
{
	int _command;
	lString16 & _buf;
    TEncoding encoding_;
    selector selector_;
protected:

    virtual void draw();

public:
	void setDefaultLayout();
	void setLayout( CRKeyboardLayoutRef layout );

	CRT9Keyboard(CRGUIWindowManager * wm, CRDocViewWindow * mainwin, int id, lString16 & buffer );

    virtual bool onCommand( int command, int params );

};

const lChar16 * defT5encoding[] = {
    L"",     // 0 STUB
    L"abcde",  // 1
    L"fghij",  // 2
    L"klmno",  // 3
    L"pqrst",  // 4
    L"uvwxyz", // 5
    NULL
};

void CRT9Keyboard::setDefaultLayout()
{
	encoding_.init(defT5encoding);
}

void CRT9Keyboard::setLayout( CRKeyboardLayoutRef layout )
{
    while ( !selector_.pop() )
		;
	_mainwin->getDocView()->clearSelection();

	if ( !layout.isNull() ) {
		encoding_.set( layout->tXKeyboard->getItems() );
	}
	if ( encoding_.length()==0 )
		setDefaultLayout();
	selector_.reinit();
	setDirty();
}

CRT9Keyboard::CRT9Keyboard(CRGUIWindowManager * wm, CRDocViewWindow * mainwin, int id, lString16 & buffer )
	: BackgroundFitWindow(wm, mainwin)
	, _command( id )
	, _buf( buffer )
	, encoding_(defT5encoding)
	, selector_(*mainwin->getDocView(), encoding_)
{
    _passKeysToParent = false;
    _passCommandsToParent = false;

    //this->setAccelerators( mainwin->getDialogAccelerators() );
    setAccelerators( _wm->getAccTables().get("txkeyboard") );

    _rect = _wm->getScreen()->getRect();
    //_rect.bottom = _rect.top;
    _rect.top = _rect.bottom - 40;
}

void CRT9Keyboard::draw()
{
    BackgroundFitWindow::draw();
    CRMenuSkinRef skin = _wm->getSkin()->getMenuSkin( L"#t9input" );
    CRRectSkinRef shortcutSkin = skin->getItemShortcutSkin();
    CRRectSkinRef itemSkin = skin->getItemSkin();
    CRRectSkinRef clientSkin = skin->getClientSkin();
    LVDrawBuf * buf = _wm->getScreen()->getCanvas().get();
    skin->draw( *buf, _rect );
    lString16 prompt = Utf8ToUnicode(selector_.getPrefix());
    prompt << L"_";
    skin->draw( *buf, _rect );
    //buf->FillRect( _rect, 0xAAAAAA );
    lvRect rect = _rect;
    lvRect borders = skin->getBorderWidths();
    rect.shrinkBy( borders );
    lvRect keyRect = rect;
    lvPoint minSizeN = shortcutSkin->getMinSize();
    for ( int i=0; i<encoding_.length(); i++ ) {
        lString16 txtN = lString16::itoa(i);
        lString16 txt = encoding_[i];
        if ( txt.empty() )
            continue;
        // label 0..9
        lvPoint sz = shortcutSkin->measureTextItem( txtN );
        keyRect.right = keyRect.left + sz.x; //borders.left + borders.right;
        shortcutSkin->draw( *buf, keyRect );
        shortcutSkin->drawText( *buf, keyRect, txtN );
        keyRect.left = keyRect.right;
        // chars (abc)
        sz = itemSkin->measureTextItem( txt );
        keyRect.right = keyRect.left + sz.x; //borders.left + borders.right;
        itemSkin->draw( *buf, keyRect );
        itemSkin->drawText( *buf, keyRect, txt );
        keyRect.left = keyRect.right; //borders.left;
    }
    keyRect.right = rect.right;
    if ( !clientSkin.isNull() && !keyRect.isEmpty() ) {
        clientSkin->draw( *buf, keyRect );
        clientSkin->drawText( *buf, keyRect, prompt );
    }
}

bool CRT9Keyboard::onCommand( int command, int params )
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
            setDirty();
            break;
        case MCMD_SCROLL_FORWARD:
            selector_.down();
            break;
		case MCMD_KBD_NEXTLAYOUT:
			setLayout( _wm->getKeyboardLayouts().nextLayout() );
			break;
		case MCMD_KBD_PREVLAYOUT:
			setLayout( _wm->getKeyboardLayouts().prevLayout() );
			break;
        case MCMD_SCROLL_BACK:
            selector_.up();
            break;
        case MCMD_OK:
            {
                lString8 translated;
                lString8 output;
                lString16 src = selector_.get();
                if ( src.empty() ) {
					_mainwin->getDocView()->clearSelection();
					_wm->closeWindow(this);
                    return true;
                }
				CRLog::info("Closing dict");
				_buf = src;
				_mainwin->getDocView()->clearSelection();
				_wm->postCommand( _command, 1 );
				_wm->closeWindow(this);
            };
            break;
        case MCMD_CANCEL:
            if ( selector_.pop() ) {
				_mainwin->getDocView()->clearSelection();
				_wm->closeWindow(this);
                return true;
            }
            setDirty();
            break;
    }
    return true;
}

void showT9Keyboard(CRGUIWindowManager * wm, CRDocViewWindow * mainwin, int id, lString16 & buffer)
{
	CRT9Keyboard * dlg = new CRT9Keyboard( wm, mainwin, id, buffer );
    wm->activateWindow( dlg );
}

