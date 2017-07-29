/*******************************************************

   CoolReader Engine

   cri18n.cpp: internationalization support

   (c) Vadim Lopatin, 2000-2008
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/


#include "../include/cri18n.h"
#include "../include/lvstream.h"

CRI18NTranslator * CRI18NTranslator::_translator = NULL;
CRI18NTranslator * CRI18NTranslator::_defTranslator = NULL;

void CRI18NTranslator::setDefTranslator( CRI18NTranslator * translator )
{
	if ( _defTranslator != NULL )
		delete _defTranslator;
	_defTranslator = translator;
}

void CRI18NTranslator::setTranslator( CRI18NTranslator * translator )
{
	if ( _translator != NULL )
		delete _translator;
	_translator = translator;
}

const char * CRI18NTranslator::translate( const char * src )
{
	if (_translator) {
		const char * s = _translator->getText( src );
		if (s && s[0])
			return s;
		if (_defTranslator) {
			s = _defTranslator->getText( src );
			if (s && s[0])
				return s;
		}
		return src; // return untranslated
	}
	const char * res = src;
#if CR_EMULATE_GETTEXT==1
	res = src;
	CRLog::trace("translation is not supported. returning source string: %s", src);
	return src;
#else
	res = gettext(src);
	CRLog::trace("gettext(%s) is %s", src, res);
	return res;
#endif
}

const lString8 CRI18NTranslator::translate8( const char * src )
{
	return lString8( translate( src ) );
}

const lString16 CRI18NTranslator::translate16( const char * src )
{
	return Utf8ToUnicode( translate8( src ) );
}

void CRMoFileTranslator::add( lString8 src, lString8 dst )
{
	_list.add( new Item( src, dst ) );
}

static int compareItems( const void * p1, const void * p2 )
{
	CRMoFileTranslator::Item * s1 = *((CRMoFileTranslator::Item**)p1);
	CRMoFileTranslator::Item * s2 = *((CRMoFileTranslator::Item**)p2);
	return s1->src.compare( s2->src );
}

void CRMoFileTranslator::sort()
{
	if ( _list.length()>0 )
		qsort( _list.get(), _list.length(), sizeof(Item*), compareItems );
}

const char * CRMoFileTranslator::getText( const char * src )
{
    int len = _list.length();
	if ( len == 0 )
		return src;
	int a = 0;
	int b = len;
	// binary search
	for ( ; ; ) {
		if ( b <= a+1 ) {
			if ( _list[a]->src == src )
				return _list[a]->dst.c_str();
			return src;
		}
		int c = (a + b) / 2;
		int res = _list[c]->src.compare( src );
		if ( res==0 )
			return _list[c]->dst.c_str();
		if ( res<0 )
			a = c+1;
		else
			b = c;
	}
}

static void reverse( lUInt32 & n )
{
	n = ((n & 0xFF) << 24) | ((n & 0xFF00) << 8) | ((n & 0xFF0000) >> 8) | ((n & 0xFF000000) >> 24);
}

#define MO_MAGIC_NUMBER 0x950412de
#define MO_MAGIC_NUMBER_REV 0xde120495
bool CRMoFileTranslator::openMoFile( lString16 fileName )
{
	LVStreamRef stream = LVOpenFileStream( fileName.c_str(), LVOM_READ );
	if ( stream.isNull() ) {
		CRLog::error("CRMoFileTranslator::openMoFile() - Cannot open .mo file");
		return false;
	}
	bool rev = false;
	lUInt32 magic, revision, count, srcOffset, dstOffset;
	if ( !stream->Read( &magic ) )
		return false;
	lvsize_t sz = stream->GetSize();
	if ( magic == MO_MAGIC_NUMBER_REV )
		rev = true;
	else if ( magic != MO_MAGIC_NUMBER ) {
		CRLog::error( "Magic number doesn't match for MO file" );
		return false;
	}
	if ( !stream->Read( &revision ) || !stream->Read( &count )
		|| !stream->Read( &srcOffset ) || !stream->Read( &dstOffset ) ) {
		CRLog::error( "Error reading MO file" );
		return false;
	}
	if ( rev ) {
		reverse( revision );
		reverse( count );
		reverse( srcOffset );
		reverse( dstOffset );
	}
	if ( count<=0 || count>10000 )
		return false;
	lvsize_t bytesRead;
	LVArray<lUInt32> srcTable( count*2, 0 );
	LVArray<lUInt32> dstTable( count*2, 0 );
	lString8Collection src;
	if ( stream->SetPos( srcOffset )!=srcOffset )
		return false;
	if ( stream->Read( srcTable.get(), count*2 * sizeof(lUInt32), &bytesRead )!=LVERR_OK || bytesRead!=count*2 * sizeof(lUInt32) )
		return false;
	if ( stream->SetPos( dstOffset )!=dstOffset )
		return false;
	if ( stream->Read( dstTable.get(), count*2 * sizeof(lUInt32), &bytesRead )!=LVERR_OK || bytesRead!=count*2 * sizeof(lUInt32) )
		return false;
    lUInt32 i;
	if ( rev ) {
		for ( i=0; i<count*2; i++ ) {
			reverse( srcTable[i] );
			reverse( dstTable[i] );
		}
	}
	for ( i=0; i<count; i++ ) {
		lUInt32 len = srcTable[ i*2 ];
		lUInt32 offset = srcTable[ i*2 + 1 ];
		if ( len>=16384 || offset<=0 || offset>sz - len -1 )
			return false;
		lString8 s;
		if ( len ) {
			if ( stream->SetPos( offset )!=offset )
				return false;
			s.append( len, ' ' );
			if ( stream->Read( s.modify(), len, &bytesRead )!=LVERR_OK || bytesRead!=len )
				return false;
		}
		src.add( s );
	}
	for ( i=0; i<count; i++ ) {
		lUInt32 len = dstTable[ i*2 ];
		lUInt32 offset = dstTable[ i*2 + 1 ];
		if ( len>=16384 || offset<=0 || offset>sz - len -1 )
			return false;
		lString8 s;
		if ( len ) {
			if ( stream->SetPos( offset )!=offset )
				return false;
			s.append( len, ' ' );
			if ( stream->Read( s.modify(), len, &bytesRead )!=LVERR_OK || bytesRead!=len )
				return false;
		}
		add( src[i], s );
	}
	sort();
	return true;
}

CRMoFileTranslator::CRMoFileTranslator() { }
CRMoFileTranslator::~CRMoFileTranslator() { }


const char * CRIniFileTranslator::getText( const char * src )
{
	lString8 key(src);
	lString8 res;
	if (_map.get(key, res))
		return _map.get(key).c_str();
	return NULL;
}

CRIniFileTranslator * CRIniFileTranslator::create(const char * fileName)
{
	CRIniFileTranslator * res = new CRIniFileTranslator();
	if (res->open(fileName))
		return res;
	CRLog::error("Cannot load language resources from %s", fileName);
	delete res;
	return NULL;
}

bool CRIniFileTranslator::open(const char * fileName)
{
	LVStreamRef stream = LVOpenFileStream(fileName, LVOM_READ);
	if (stream.isNull())
		return false;
    if (stream->GetMode()!=LVOM_READ )
        return false;
    lvsize_t sz = stream->GetSize() - stream->GetPos();
    if ( sz<=0 )
        return false;
    char * buf = new char[sz + 3];
    lvsize_t bytesRead = 0;
    if ( stream->Read( buf, sz, &bytesRead )!=LVERR_OK ) {
        delete[] buf;
        return false;
    }
    buf[sz] = 0;
    char * p = buf;
    if( (unsigned char)buf[0] == 0xEF && (unsigned char)buf[1]==0xBB && (unsigned char)buf[2]==0xBF )
        p += 3;
    // read lines from buffer
    while (*p) {
        char * elp = p;
        char * eqpos = NULL;
        while ( *elp && !(elp[0]=='\r' && elp[1]=='\n')  && !(elp[0]=='\n') ) {
            if ( *elp == '=' && eqpos==NULL )
                eqpos = elp;
            elp++;
        }
        if ( eqpos!=NULL && eqpos>p && *elp!='#' ) {
            lString8 name( p, (lvsize_t)(eqpos-p) );
            lString8 value( eqpos+1, (lvsize_t)(elp - eqpos - 1));
            _map.set(name, value);
        }
        for ( p=elp; *elp && *elp!='\r' && *elp!='\n'; elp++)
            ;
        p = elp;
		while ( *p=='\r' || *p=='\n' )
			p++;
    }
    // cleanup
    delete[] buf;
    return _map.length() > 0;
}
