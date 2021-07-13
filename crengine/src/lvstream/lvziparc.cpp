/*******************************************************

   CoolReader Engine

   lvziparc.cpp

   (c) Vadim Lopatin, 2000-2009
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvziparc.h"

#if (USE_ZLIB==1)

#include "lvzipdecodestream.h"
#include "ziphdr.h"
#include "crtxtenc.h"
#include "crlog.h"

LVZipArc::LVZipArc(LVStreamRef stream) : LVArcContainerBase(stream)
{
    SetName(stream->GetName());
}

LVZipArc::~LVZipArc()
{
}

LVStreamRef LVZipArc::OpenStream(const char32_t *fname, lvopen_mode_t)
{
    if ( fname[0]=='/' )
        fname++;
    int found_index = -1;
    for (int i=0; i<m_list.length(); i++) {
        if ( m_list[i]->GetName() != NULL && !lStr_cmp( fname, m_list[i]->GetName() ) ) {
            if ( m_list[i]->IsContainer() ) {
                // found directory with same name!!!
                return LVStreamRef();
            }
            found_index = i;
            break;
        }
    }
    if (found_index<0)
        return LVStreamRef(); // not found
    // make filename
    lString32 fn = fname;
    LVStreamRef strm = m_stream; // fix strange arm-linux-g++ bug
    LVStreamRef stream(
                LVZipDecodeStream::Create(
                    strm,
                    m_list[found_index]->GetSrcPos(),
                    fn,
                    m_list[found_index]->GetSrcSize(),
                    m_list[found_index]->GetSize() )
                );
    if (!stream.isNull()) {
        stream->SetName(m_list[found_index]->GetName());
        // Use buffering?
        //return stream;
        return stream;
        //return LVCreateBufferedStream( stream, ZIP_STREAM_BUFFER_SIZE );
    }
    return stream;
}

int LVZipArc::ReadContents()
{
    lvByteOrderConv cnv;
    //bool arcComment = false;
    bool truncated = false;
    
    m_list.clear();
    if (!m_stream || m_stream->Seek(0, LVSEEK_SET, NULL)!=LVERR_OK)
        return 0;
    
    SetName( m_stream->GetName() );
    
    
    lvsize_t sz = 0;
    if (m_stream->GetSize( &sz )!=LVERR_OK)
        return 0;
    lvsize_t m_FileSize = (unsigned)sz;
    
    char ReadBuf[1024];
    lUInt32 NextPosition;
    lvpos_t CurPos;
    lvsize_t ReadSize;
    int Buf;
    bool found = false;
    CurPos=NextPosition=(int)m_FileSize;
    if (CurPos < sizeof(ReadBuf)-18)
        CurPos = 0;
    else
        CurPos -= sizeof(ReadBuf)-18;
    // Find End of central directory record (EOCD)
    for ( Buf=0; Buf<64 && !found; Buf++ )
    {
        //SetFilePointer(ArcHandle,CurPos,NULL,FILE_BEGIN);
        m_stream->Seek( CurPos, LVSEEK_SET, NULL );
        m_stream->Read( ReadBuf, sizeof(ReadBuf), &ReadSize);
        if (ReadSize==0)
            break;
        for (int I=(int)ReadSize-4;I>=0;I--)
        {
            if (ReadBuf[I]==0x50 && ReadBuf[I+1]==0x4b && ReadBuf[I+2]==0x05 &&
                ReadBuf[I+3]==0x06)
            {
                m_stream->Seek( CurPos+I+16, LVSEEK_SET, NULL );
                m_stream->Read( &NextPosition, sizeof(NextPosition), &ReadSize);
                cnv.lsf( &NextPosition );
                found=true;
                break;
            }
        }
        if (CurPos==0)
            break;
        if (CurPos<sizeof(ReadBuf)-4)
            CurPos=0;
        else
            CurPos-=sizeof(ReadBuf)-4;
    }
    
    truncated = !found;
    
    // If the main reading method (using zip header at the end of the
    // archive) failed, we can try using the alternative method used
    // when this zip header is missing ("truncated"), which uses
    // local zip headers met along while scanning the zip.
    if (m_alt_reading_method)
        truncated = true; // do as if truncated
    else if (truncated) // archive detected as truncated
        // flag that, so there's no need to try that alt method,
        // as it was used on first scan
        m_alt_reading_method = true;
    
    if (truncated)
        NextPosition=0;
    
    //================================================================
    // get files
    
    
    ZipLocalFileHdr ZipHd1;
    ZipHd2 ZipHeader = { 0 };
    unsigned ZipHeader_size = 0x2E; //sizeof(ZipHd2); //0x34; //
    unsigned ZipHd1_size = 0x1E; //sizeof(ZipHd1); //sizeof(ZipHd1)
    //lUInt32 ReadSize;
    
    for (;;) {
        
        if (m_stream->Seek( NextPosition, LVSEEK_SET, NULL )!=LVERR_OK)
            return 0;
        
        if (truncated)
        {
            // The offset (that we don't find in a local header, but
            // that we will store in the ZipHeader we're building)
            // happens to be the current position here.
            lUInt32 offset = (lUInt32)m_stream->GetPos();
            
            m_stream->Read( &ZipHd1, ZipHd1_size, &ReadSize);
            ZipHd1.byteOrderConv();
            
            //ReadSize = fread(&ZipHd1, 1, sizeof(ZipHd1), f);
            if (ReadSize != ZipHd1_size) {
                //fclose(f);
                if (ReadSize==0 && NextPosition==m_FileSize)
                    return m_list.length();
                if ( ReadSize==0 )
                    return m_list.length();
                return 0;
            }
            
            ZipHeader.UnpVer=ZipHd1.UnpVer;
            ZipHeader.UnpOS=ZipHd1.UnpOS;
            ZipHeader.Flags=ZipHd1.Flags;
            ZipHeader.ftime=ZipHd1.getftime();
            ZipHeader.PackSize=ZipHd1.getPackSize();
            ZipHeader.UnpSize=ZipHd1.getUnpSize();
            ZipHeader.NameLen=ZipHd1.getNameLen();
            ZipHeader.AddLen=ZipHd1.getAddLen();
            ZipHeader.Method=ZipHd1.getMethod();
            ZipHeader.setOffset(offset);
            // We may get a last invalid record with NameLen=0, which shouldn't hurt.
            // If it does, use:
            // if (ZipHeader.NameLen == 0) break;
        } else {
            
            m_stream->Read( &ZipHeader, ZipHeader_size, &ReadSize);
            
            ZipHeader.byteOrderConv();
            //ReadSize = fread(&ZipHeader, 1, sizeof(ZipHeader), f);
            if (ReadSize!=ZipHeader_size) {
                if (ReadSize>16 && ZipHeader.Mark==0x06054B50 ) {
                    break;
                }
                //fclose(f);
                return 0;
            }
        }
        
        if (ReadSize==0 || ZipHeader.Mark==0x06054b50 ||
            (truncated && ZipHeader.Mark==0x02014b50) )
        {
            //                if (!truncated && *(lUInt16 *)((char *)&ZipHeader+20)!=0)
            //                    arcComment=true;
            break; //(GETARC_EOF);
        }
        
        //const int NM = 513;
        const int max_NM = 4096;
        if ( ZipHeader.NameLen>max_NM ) {
            CRLog::error("ZIP entry name length is too big: %d", (int)ZipHeader.NameLen);
            return 0;
        }
        lUInt32 SizeToRead=(ZipHeader.NameLen<max_NM) ? ZipHeader.NameLen : max_NM;
        char fnbuf[max_NM+1];
        m_stream->Read( fnbuf, SizeToRead, &ReadSize);
        
        if (ReadSize!=SizeToRead) {
            CRLog::error("error while reading zip entry name");
            return 0;
        }
        
        fnbuf[SizeToRead]=0;
        
        long SeekLen=ZipHeader.AddLen+ZipHeader.CommLen;
        
        LVCommonContainerItemInfo * item = new LVCommonContainerItemInfo();
        
        if (truncated)
            SeekLen+=ZipHeader.PackSize;
        
        NextPosition = (lUInt32)m_stream->GetPos();
        NextPosition += SeekLen;
        m_stream->Seek(NextPosition, LVSEEK_SET, NULL);
        
        lString32 fName;
        if (ZipHeader.PackVer >= 63 && (ZipHeader.Flags & 0x0800) == 0x0800) {
            // Language encoding flag (EFS).  If this bit is set,
            // the filename and comment fields for this file
            // MUST be encoded using UTF-8. (InfoZip APPNOTE-6.3.0)
            //CRLog::trace("ZIP 6.3: Language encoding flag (EFS) enabled, using UTF-8 encoding.");
            fName = Utf8ToUnicode(fnbuf);
        } else {
            if (isValidUtf8Data((const unsigned char *)fnbuf, SizeToRead)) {
                //CRLog::trace("autodetected UTF-8 encoding.");
                fName = Utf8ToUnicode(fnbuf);
            } else {
                // {"DOS","Amiga","VAX/VMS","Unix","VM/CMS","Atari ST",
                //  "OS/2","Mac-OS","Z-System","CP/M","TOPS-20",
                //  "Win32","SMS/QDOS","Acorn RISC OS","Win32 VFAT","MVS",
                //  "BeOS","Tandem"};
                // TODO: try to detect proper charset using 0x0008 Extra Field (InfoZip APPNOTE-6.3.5, Appendix D.4).
                const lChar32 * enc_name = (ZipHeader.PackOS==0) ? U"cp866" : U"cp1251";
                //CRLog::trace("detected encoding %s", LCSTR(enc_name));
                const lChar32 * table = GetCharsetByte2UnicodeTable( enc_name );
                fName = ByteToUnicode( lString8(fnbuf), table );
            }
        }
        
        item->SetItemInfo(fName.c_str(), ZipHeader.UnpSize, (ZipHeader.getAttr() & 0x3f));
        item->SetSrc( ZipHeader.getOffset(), ZipHeader.PackSize, ZipHeader.Method );
        
        //#define DUMP_ZIP_HEADERS
#ifdef DUMP_ZIP_HEADERS
        CRLog::trace("ZIP entry '%s' unpSz=%d, pSz=%d, m=%x, offs=%x, zAttr=%x, flg=%x", LCSTR(fName), (int)ZipHeader.UnpSize, (int)ZipHeader.PackSize, (int)ZipHeader.Method, (int)ZipHeader.getOffset(), (int)ZipHeader.getZIPAttr(), (int)ZipHeader.getAttr());
        //, addL=%d, commL=%d, dn=%d
        //, (int)ZipHeader.AddLen, (int)ZipHeader.CommLen, (int)ZipHeader.DiskNum
#endif
        
        m_list.add(item);
    }
    int sz2 = m_list.length();
    return sz2;
}

LVArcContainerBase *LVZipArc::OpenArchieve(LVStreamRef stream)
{
    // read beginning of file
    const lvsize_t hdrSize = 4;
    char hdr[hdrSize];
    stream->SetPos(0);
    lvsize_t bytesRead = 0;
    if (stream->Read(hdr, hdrSize, &bytesRead)!=LVERR_OK || bytesRead!=hdrSize)
        return NULL;
    stream->SetPos(0);
    // detect arc type
    if (hdr[0]!='P' || hdr[1]!='K' || hdr[2]!=3 || hdr[3]!=4)
        return NULL;
    LVZipArc * arc = new LVZipArc( stream );
    int itemCount = arc->ReadContents();
    if ( itemCount > 0 && arc->isAltReadingMethod() ) {
        CRLog::warn("Zip file truncated: going on with possibly partial content.");
    }
    else if ( itemCount <= 0 && !arc->isAltReadingMethod() ) {
        CRLog::warn("Zip file corrupted or invalid: trying alternative processing...");
        arc->setAltReadingMethod();
        itemCount = arc->ReadContents();
    }
    if ( itemCount <= 0 )
    {
        CRLog::error("Zip file corrupted or invalid: processing failure.");
        delete arc;
        return NULL;
    }
    return arc;
}

#endif  // (USE_ZLIB==1)
