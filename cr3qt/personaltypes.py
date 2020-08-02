############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

# This is a place to add your own dumpers for testing purposes.
# Any contents here will be picked up by GDB, LLDB, and CDB based
# debugging in Qt Creator automatically.

# NOTE: This file will get overwritten when updating Qt Creator.
#
# To add dumpers that don't get overwritten, copy this file here
# to a safe location outside the Qt Creator installation and
# make this location known to Qt Creator using the Debugger >
# Locals & Expressions > Extra Debugging Helpers setting.

# Example to display a simple type
# template<typename U, typename V> struct MapNode
# {
#     U key;
#     V data;
# }
#
# def qdump__MapNode(d, value):
#    d.putValue("This is the value column contents")
#    d.putNumChild(2)
#    if d.isExpanded():
#        with Children(d):
#            # Compact simple case.
#            d.putSubItem("key", value["key"])
#            # Same effect, with more customization possibilities.
#            with SubItem(d, "data")
#                d.putItem("data", value["data"])

# Check http://doc.qt.io/qtcreator/creator-debugging-helpers.html
# for more details or look at qttypes.py, stdtypes.py, boosttypes.py
# for more complex examples.

from dumper import *

######################## Your code below #######################

def qdump__lString8(d, value):
    pchunk = value['pchunk'].dereference()
    length = pchunk['len']
    size = pchunk['size']
    refCount = pchunk['refCount']
    buf8 = pchunk['buf8']
    length_int = length.integer()
    size_int = size.integer()
    d.check(length_int >= 0 and size_int >= 0)
    #buf8str = buf8.cast(d.charType().pointer())
    #d.putCharArrayHelper(buf8str, length_int, d.charType(), d.currentItemFormat(), True)
    bytelen = length_int*d.charType().size()
    elided, shown = d.computeLimit(bytelen, 512)
    mem = d.readMemory(buf8, shown)
    d.putValue(mem, 'utf8', elided=elided)
    d.putNumChild(3)
    if d.isExpanded():
        with Children(d):
            d.putSubItem('len', length)
            d.putSubItem('size', size)
            d.putSubItem('refCount', refCount)


def qdump__lString16(d, value):
    pchunk = value['pchunk'].dereference()
    length = pchunk['len']
    size = pchunk['size']
    refCount = pchunk['refCount']
    buf16 = pchunk['buf16']
    wcharType = d.createType('wchar_t')
    length_int = length.integer()
    size_int = size.integer()
    d.check(length_int >= 0 and size_int >= 0)
    #buf16str = buf16.cast(wcharType.pointer())
    #d.putCharArrayHelper(buf16str, size_int, wcharType, d.currentItemFormat(), False)
    bytelen = length_int*wcharType.size()
    elided, shown = d.computeLimit(bytelen, 512)
    mem = d.readMemory(buf16, shown)
    d.putValue(mem, 'ucs4', elided=elided)
    d.putNumChild(3)
    if d.isExpanded():
        with Children(d):
            d.putSubItem('len', length)
            d.putSubItem('size', size)
            d.putSubItem('refCount', refCount)
