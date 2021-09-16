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

int LVZipArc::ReadContents() {
    lvByteOrderConv cnv;
    //bool arcComment = false;
    bool truncated = false;

    m_list.clear();
    if (!m_stream || m_stream->Seek(0, LVSEEK_SET, NULL) != LVERR_OK)
        return -1;

    SetName(m_stream->GetName());

    lvsize_t fileSize = 0;
    if (m_stream->GetSize(&fileSize) != LVERR_OK)
        return -1;

    char ReadBuf[1024];
    lUInt32 NextPosition;
    lvoffset_t NextOffset;
    lvpos_t CurPos;
    lvsize_t ReadSize;
    bool found = false;
    bool found64 = false;
    bool require64 = false;
    bool zip64 = false;
    lUInt64 NextPosition64 = 0;
    CurPos = 0;
    NextPosition = 0;
    if (fileSize < sizeof(ReadBuf) - 18)
        CurPos = -(lvpos_t)fileSize;
    else
        CurPos = -(lvpos_t)sizeof(ReadBuf) + 18;
    // Find End of central directory record (EOCD)
    for (int bufNo = 0; bufNo < 64; bufNo++) {
        if (m_stream->Seek(CurPos, LVSEEK_END, NULL) != LVERR_OK)
            break;
        if (m_stream->Read(ReadBuf, sizeof(ReadBuf), &ReadSize) != LVERR_OK)
            break;
        if (ReadSize == 0)
            break;
        for (int i = (int)ReadSize - 4; i >= 0; i--) {
            if (ReadBuf[i] == 0x50 && ReadBuf[i + 1] == 0x4b &&
                ReadBuf[i + 2] == 0x05 && ReadBuf[i + 3] == 0x06) {
                if (m_stream->Seek(CurPos + i + 16, LVSEEK_END, NULL) != LVERR_OK)
                    break;
                if (m_stream->Read(&NextPosition, sizeof(NextPosition), &ReadSize) != LVERR_OK)
                    break;
                cnv.lsf(&NextPosition);
                found = true;
                if (0xFFFFFFFFUL == NextPosition) {
                    require64 = true;
                    if (found64)
                        break;
                } else {
                    break;
                }
            }
            if (ReadBuf[i] == 0x50 && ReadBuf[i + 1] == 0x4b &&
                ReadBuf[i + 2] == 0x06 && ReadBuf[i + 3] == 0x06) {
                if (m_stream->Seek(CurPos + i + 48, LVSEEK_END, NULL) != LVERR_OK)
                    break;
                if (m_stream->Read(&NextPosition64, sizeof(NextPosition64), &ReadSize) != LVERR_OK)
                    break;
                cnv.lsf(&NextPosition64);
                found64 = true;
                break;
            }
        }
        if (CurPos <= -fileSize)
            break;
        if (fileSize < sizeof(ReadBuf) - 4)
            CurPos = -fileSize;
        else
            CurPos -= (lvpos_t)sizeof(ReadBuf) - 4;
    }
    zip64 = found64 || require64;

#if LVLONG_FILE_SUPPORT == 1
    if (found64)
        NextOffset = NextPosition64;
    else if (found && !require64)
        NextOffset = NextPosition;
    else
        truncated = true;
#else
    if (zip64) {
        CRLog::error("zip64 signature found, but large file support is not enabled, stop processing.");
        return -1;
    }
    truncated = !found;
    NextOffset = NextPosition;
#endif

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
        NextOffset = 0;

    //================================================================
    // get file list

    lverror_t err;
    ZipLocalFileHdr ZipHd1;
    ZipHd2 ZipHeader = { 0 };
    unsigned ZipHeader_size = 0x2E; //sizeof(ZipHd2); //0x34; //
    unsigned ZipHd1_size = 0x1E; //sizeof(ZipHd1); //sizeof(ZipHd1)

    for (;;) {
        if (m_stream->Seek(NextOffset, LVSEEK_SET, NULL) != LVERR_OK)
            return 0;
        if (truncated) {
            // The offset (that we don't find in a local header, but
            // that we will store in the ZipHeader we're building)
            // happens to be the current position here.
            lUInt32 offset = (lUInt32)m_stream->GetPos();

            err = m_stream->Read(&ZipHd1, ZipHd1_size, &ReadSize);
            ZipHd1.byteOrderConv();
            if (err != LVERR_OK || ReadSize != ZipHd1_size) {
                if (ReadSize == 0 && NextOffset == (lvoffset_t)fileSize)
                    return m_list.length();
                if (ReadSize == 0)
                    return m_list.length();
                return 0;
            }

            ZipHeader.UnpVer = ZipHd1.UnpVer;
            ZipHeader.UnpOS = ZipHd1.UnpOS;
            ZipHeader.Flags = ZipHd1.Flags;
            ZipHeader.ftime = ZipHd1.getftime();
            ZipHeader.PackSize = ZipHd1.getPackSize();
            ZipHeader.UnpSize = ZipHd1.getUnpSize();
            ZipHeader.NameLen = ZipHd1.getNameLen();
            ZipHeader.AddLen = ZipHd1.getAddLen();
            ZipHeader.Method = ZipHd1.getMethod();
            ZipHeader.setOffset(offset);
            // We may get a last invalid record with NameLen=0, which shouldn't hurt.
            // If it does, use:
            // if (ZipHeader.NameLen == 0) break;
        } else {
            err = m_stream->Read(&ZipHeader, ZipHeader_size, &ReadSize);
            ZipHeader.byteOrderConv();
            if (err != LVERR_OK || ReadSize != ZipHeader_size) {
                if (ReadSize > 16 && (ZipHeader.Mark == 0x06054B50 || ZipHeader.Mark == 0x06064b50)) {
                    break;
                }
                return 0;
            }
        }
        if (ReadSize == 0 || ZipHeader.Mark == 0x06054b50 || ZipHeader.Mark == 0x06064b50 ||
            (truncated && ZipHeader.Mark == 0x02014b50)) {
            //                if (!truncated && *(lUInt16 *)((char *)&ZipHeader+20)!=0)
            //                    arcComment=true;
            break; //(GETARC_EOF);
        }
#if LVLONG_FILE_SUPPORT == 1
        int extraPosUnpSize = -1;
        int extraPosPackSize = -1;
        int extraPosOffset = -1;
        int extraLastPos = 0;
        Zip64ExtInfo *zip64ExtInfo = NULL;
        if (0xFFFFFFFF == ZipHeader.UnpSize) {
            extraPosUnpSize = extraLastPos;
            extraLastPos += 8;
        }
        if (0xFFFFFFFF == ZipHeader.PackSize) {
            extraPosPackSize = extraLastPos;
            extraLastPos += 8;
        }
        if (0xFFFFFFFF == ZipHeader.getOffset()) {
            extraPosOffset = extraLastPos;
            extraLastPos += 8;
        }
        if (!zip64 && extraLastPos > 0)
            zip64 = true;
#endif

        //const lvsize_t NM = 513;
        const lvsize_t max_NM = 4096;
        if (ZipHeader.NameLen > max_NM) {
            CRLog::error("ZIP entry name length is too big: %d, trunc to %d",
                         (int)ZipHeader.NameLen, (int)max_NM);
        }
        lvsize_t fnameSizeToRead = (ZipHeader.NameLen < max_NM) ? ZipHeader.NameLen : max_NM;
        lvoffset_t NM_skipped_sz = (ZipHeader.NameLen > max_NM) ? (lvoffset_t)(ZipHeader.NameLen - max_NM) : 0;
        char fnbuf[max_NM + 1];
        err = m_stream->Read(fnbuf, fnameSizeToRead, &ReadSize);
        if (err != LVERR_OK || ReadSize != fnameSizeToRead) {
            CRLog::error("error while reading zip entry name");
            return 0;
        }
        fnbuf[fnameSizeToRead] = 0;
        if (NM_skipped_sz > 0) {
            if (m_stream->Seek(NM_skipped_sz, LVSEEK_CUR, NULL) != LVERR_OK) {
                CRLog::error("error while skipping the long zip entry name");
                return 0;
            }
        }

        // read extra data
        const lvsize_t max_EXTRA = 512;
        if (ZipHeader.AddLen > max_EXTRA) {
            CRLog::error("ZIP entry extra length is too big: %d", (int)ZipHeader.AddLen);
            return 0;
        }
        lvsize_t extraSizeToRead = (ZipHeader.AddLen < max_EXTRA) ? ZipHeader.AddLen : max_EXTRA;
        lUInt8 extra[max_EXTRA];
        err = m_stream->Read(extra, extraSizeToRead, &ReadSize);
        if (err != LVERR_OK || ReadSize != extraSizeToRead) {
            CRLog::error("error while reading zip entry extra data");
            return 0;
        }
#if LVLONG_FILE_SUPPORT == 1
        // Find Zip64 extension if required
        lvsize_t offs = 0;
        Zip64ExtInfo *ext;
        if (zip64) {
            while (offs + 4 < extraSizeToRead) {
                ext = (Zip64ExtInfo *)&extra[offs];
                ext->byteOrderConv();
                if (0x0001 == ext->Tag) {
                    zip64ExtInfo = ext;
                    break;
                } else {
                    offs += 4 + ext->Size;
                }
            }
        }
#endif

        lUInt32 SeekLen = ZipHeader.CommLen;
        if (truncated)
            SeekLen += ZipHeader.PackSize;
        NextOffset = (lvoffset_t)m_stream->GetPos();
        NextOffset += SeekLen;
        if (NextOffset >= (lvoffset_t)fileSize) {
            CRLog::error("invalid offset, stop to read contents.");
            break;
        }

        lString32 fName;
        if (ZipHeader.PackVer >= 63 && (ZipHeader.Flags & 0x0800) == 0x0800) {
            // Language encoding flag (EFS).  If this bit is set,
            // the filename and comment fields for this file
            // MUST be encoded using UTF-8. (InfoZip APPNOTE-6.3.0)
            //CRLog::trace("ZIP 6.3: Language encoding flag (EFS) enabled, using UTF-8 encoding.");
            fName = Utf8ToUnicode(fnbuf);
        } else {
            if (isValidUtf8Data((const unsigned char *)fnbuf, fnameSizeToRead)) {
                //CRLog::trace("autodetected UTF-8 encoding.");
                fName = Utf8ToUnicode(fnbuf);
            } else {
                // {"DOS","Amiga","VAX/VMS","Unix","VM/CMS","Atari ST",
                //  "OS/2","Mac-OS","Z-System","CP/M","TOPS-20",
                //  "Win32","SMS/QDOS","Acorn RISC OS","Win32 VFAT","MVS",
                //  "BeOS","Tandem"};
                // TODO: try to detect proper charset using 0x0008 Extra Field (InfoZip APPNOTE-6.3.5, Appendix D.4).
                const lChar32 *enc_name = (ZipHeader.PackOS == 0) ? U"cp866" : U"cp1251";
                //CRLog::trace("detected encoding %s", LCSTR(enc_name));
                const lChar32 *table = GetCharsetByte2UnicodeTable(enc_name);
                fName = ByteToUnicode(lString8(fnbuf), table);
            }
        }

        LVCommonContainerItemInfo *item = new LVCommonContainerItemInfo();
#if LVLONG_FILE_SUPPORT == 1
        lvsize_t fileUnpSize = (lvsize_t)ZipHeader.UnpSize;
        lvsize_t filePackSize = (lvsize_t)ZipHeader.PackSize;
        lvpos_t fileOffset = (lvpos_t)ZipHeader.getOffset();
        if (zip64ExtInfo != NULL) {
            if (extraPosUnpSize >= 0)
                fileUnpSize = zip64ExtInfo->getField64(extraPosUnpSize);
            if (extraPosPackSize >= 0)
                filePackSize = zip64ExtInfo->getField64(extraPosPackSize);
            if (extraPosOffset >= 0)
                fileOffset = zip64ExtInfo->getField64(extraPosOffset);
        }
        item->SetItemInfo(fName.c_str(), fileUnpSize, (ZipHeader.getAttr() & 0x3f));
        item->SetSrc(fileOffset, filePackSize, ZipHeader.Method);
#else
        item->SetItemInfo(fName.c_str(), ZipHeader.UnpSize, (ZipHeader.getAttr() & 0x3f));
        item->SetSrc(ZipHeader.getOffset(), ZipHeader.PackSize, ZipHeader.Method);
#endif
        m_list.add(item);

        //#define DUMP_ZIP_HEADERS
#ifdef DUMP_ZIP_HEADERS
#if LVLONG_FILE_SUPPORT == 1
        CRLog::trace("ZIP entry '%s' unpSz=%llu, pSz=%llu, m=%x, offs=%llu, zAttr=%x, flg=%x, addL=%d, commL=%d, dn=%d", LCSTR(fName), fileUnpSize, filePackSize, (int)ZipHeader.Method, fileOffset, (int)ZipHeader.getZIPAttr(), (int)ZipHeader.getAttr(), (int)ZipHeader.AddLen, (int)ZipHeader.CommLen, (int)ZipHeader.DiskNum);
#else
        CRLog::trace("ZIP entry '%s' unpSz=%d, pSz=%d, m=%x, offs=%x, zAttr=%x, flg=%x, addL=%d, commL=%d, dn=%d", LCSTR(fName), (int)ZipHeader.UnpSize, (int)ZipHeader.PackSize, (int)ZipHeader.Method, (int)ZipHeader.getOffset(), (int)ZipHeader.getZIPAttr(), (int)ZipHeader.getAttr(), (int)ZipHeader.AddLen, (int)ZipHeader.CommLen, (int)ZipHeader.DiskNum);
#endif
        //, addL=%d, commL=%d, dn=%d
        //, (int)ZipHeader.AddLen, (int)ZipHeader.CommLen, (int)ZipHeader.DiskNum
#define EXTRA_DEC_MAX   (1536+1)
        if (extraSizeToRead > 0) {
            char extra_buff[EXTRA_DEC_MAX];
            memset(extra_buff, 0, EXTRA_DEC_MAX);
            char* ptr = &extra_buff[0];
            for (lvsize_t i = 0; i < extraSizeToRead; i++) {
                sprintf(ptr, ":%02X", extra[i]);
                ptr += 3;
            }
            *ptr = 0;
            CRLog::trace("  ZIP entry extra data: %s", extra_buff);
        }
#endif
    }
    return m_list.length();
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
    else if ( itemCount == 0 && !arc->isAltReadingMethod() ) {
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
