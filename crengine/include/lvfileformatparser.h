/** @file lvfileformatparser.h
    @brief base class for all document format parsers

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __LVFILEFORMATPARSER_H_INCLUDED__
#define __LVFILEFORMATPARSER_H_INCLUDED__

#include "lvtypes.h"

class LVDocViewCallback;

/// base class for all document format parsers
class LVFileFormatParser
{
public:
    /// returns pointer to loading progress callback object
    virtual LVDocViewCallback * getProgressCallback() { return NULL; }
    /// sets pointer to loading progress callback object
    virtual void setProgressCallback( LVDocViewCallback * /*callback*/ ) { }
    /// returns true if format is recognized by parser
    virtual bool CheckFormat() = 0;
    /// parses input stream
    virtual bool Parse() = 0;
    /// resets parsing, moves to beginning of stream
    virtual void Reset() = 0;
    /// stops parsing in the middle of file, to read header only
    virtual void Stop() = 0;
    /// sets charset by name
    virtual void SetCharset( const lChar32 * name ) = 0;
    /// sets 8-bit charset conversion table (128 items, for codes 128..255)
    virtual void SetCharsetTable( const lChar32 * table ) = 0;
    /// returns 8-bit charset conversion table (128 items, for codes 128..255)
    virtual lChar32 * GetCharsetTable( ) = 0;
    /// changes space mode
    virtual void SetSpaceMode( bool ) { }
    /// returns space mode
    virtual bool GetSpaceMode() { return false; }
    /// virtual destructor
    virtual ~LVFileFormatParser() {}
};

#endif  // __LVFILEFORMATPARSER_H_INCLUDED__
