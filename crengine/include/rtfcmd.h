/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2008,2011,2012 Vadim Lopatin <coolreader.org@gmail.com>
 *   Copyright (C) 2020 Konstantin Potapov <pkbo@users.sourceforge.net>    *
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

RTF_CHR( "\n", par_n, 13 )
RTF_CHR( "\r", par_r, 13 )
RTF_CHR( "_", nb_hyphen, '-' )
RTF_IPR( ansicpg, pi_ansicpg, 1252 )
RTF_IPR( b, pi_ch_bold, 1 )
RTF_IPR( bin, pi_bin, 0 )
RTF_CHC( bullet, 'o' )
RTF_TPR( cell, tpi_cell, 0 )     // Denotes the end of a table cell.
RTF_TPR( clmgf, tpi_clmgf, 0 )   // The first cell in a range of table cells to be merged.
RTF_TPR( clmrg, tpi_clmrg, 0 )   // Contents of the table cell are merged with those of the preceding cell.
RTF_TPR( clvmgf, tpi_clvmgf, 0 ) // The first cell in a range of table cells to be vertically merged.
RTF_TPR( clvmrg, tpi_clvmrg, 0 ) // Contents of the table cell are vertically merged with those of the preceding cell.
RTF_DST( colortbl, dest_colortbl )
RTF_IPR( deflang, pi_deflang, 1024 )
RTF_IPR( deflangfe, pi_deflang, 1024 )
RTF_CHC( emdash, 8212 )
RTF_CHC( emspace, 160 )
RTF_CHC( endash, 8211 )
RTF_CHC( enspace, 160 )
//RTF_IPR( fcharset, pi_ansicpg, 1252 )
RTF_DST( fonttbl, dest_fonttbl )
RTF_DST( footer, dest_footer )
RTF_DST( footnote, dest_footnote )
RTF_DST( header, dest_header )
RTF_IPR( i, pi_ch_italic, 1 )
RTF_DST( info, dest_info )
RTF_IPR( intbl,pi_intbl, 1 )     // in table
RTF_TPR( irow, tpi_irowN, 0 )    // N is the row index of this row.
RTF_TPR( irowband, tpi_irowbandN, 0 )// N is the row index of the row, adjusted to account for header rows. A header row has a value of –1.
RTF_IPR( jpegblip, pi_imgfmt, rtf_img_jpeg )
RTF_IPR( lang, pi_lang, 1024 )
RTF_TPR( lastrow, tpi_lastrow, 0 )// Output if this is the last row in the table.
RTF_CHC( ldblquote, 0x201C )
RTF_CHC( lquote, 0x2018 )
RTF_CHC( par, 13 )
RTF_ACT( pard, LVRtfDestination::RA_PARD )
RTF_DST( pict, dest_pict )
RTF_IPR( pngblip, pi_imgfmt, rtf_img_png )
RTF_IPR( qc, pi_align, ha_center )
RTF_IPR( qd, pi_align, ha_distributed )
RTF_IPR( qj, pi_align, ha_justified )
RTF_IPR( ql, pi_align, ha_left )
RTF_IPR( qr, pi_align, ha_right )
RTF_IPR( qt, pi_align, ha_thai )
RTF_CHC( rdblquote, 0x201D )
RTF_TPR( row, tpi_row, 0 )       // Denotes the end of a row.
RTF_CHC( rquote, 0x2019 )
RTF_CHC( sect, 13 )
RTF_DST( stylesheet, dest_stylesheet )
RTF_IPR( sub, pi_ch_sub, 1 )
RTF_IPR( super, pi_ch_super, 1 )
RTF_CHC( tab, ' ' )
RTF_TPR( tcelld, tpi_tcelld, 0 ) // Sets table cell defaults.
RTF_TPR( trowd, tpi_trowd, 0 )   //
RTF_IPR( uc, pi_uc_count, 1 )
RTF_DST( ud, dest_ud )
RTF_DST( upr, dest_upr )
RTF_CHR( "~", nbsp, 160 )

