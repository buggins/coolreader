/*******************************************************

   CoolReader Engine

   rtfimp.cpp:  RTF import implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/


RTF_CHR( "\n", par_n, 13 )
RTF_CHR( "\r", par_r, 13 )
RTF_CHR( "~", nbsp, 160 )
RTF_IPR( b, pi_ch_bold, 1 )
RTF_IPR( i, pi_ch_italic, 1 )
RTF_CHC( bullet, 'o' )
RTF_CHC( emdash, 8212 )
RTF_CHC( emspace, 160 )
RTF_CHC( endash, 8211 )
RTF_CHC( enspace, 160 )
RTF_CHC( ldblquote, 0x201C )
RTF_CHC( lquote, 0x2018 )
RTF_CHC( rdblquote, 0x201D )
RTF_CHC( rquote, 0x2019 )
RTF_CHC( par, 13 )
RTF_CHC( sect, 13 )
RTF_CHC( tab, ' ' )
