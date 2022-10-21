/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007 Vadim Lopatin <coolreader.org@gmail.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

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
