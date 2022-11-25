#!/usr/bin/perl

###########################################################################
#   CoolReader engine, dic hyphenation file converter                     #
#   Copyright (C) 2022 Aleksey Chernov <valexlin@gmail.com>               #
#                                                                         #
#   This program is free software; you can redistribute it and/or         #
#   modify it under the terms of the GNU General Public License           #
#   as published by the Free Software Foundation; either version 2        #
#   of the License, or (at your option) any later version.                #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the Free Software           #
#   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            #
#   MA 02110-1301, USA.                                                   #
###########################################################################

#
# This program is based on the mkpattern.cpp sources and uses its algorithms
# Copyright (C) 2011,2012 Vadim Lopatin <coolreader.org@gmail.com>
#

use warnings;

use Encode qw(decode encode);

sub makeConversion($$$);
sub outPattern($$);
sub my_chomp;

if (scalar(@ARGV) < 2) {
    print("Hyphenation pattern converter\n");
    print("usage: $0 <srclistfile.dic> <dstfile.pattern>\n");
    exit -1;
}

my $in_filename = $ARGV[0];
my $out_filename = $ARGV[1];

my $infh;
my $outfh;
if (!open($infh, "<", $in_filename)) {
    print "Cannot open file ${in_filename}\n";
    exit 1;
}
if (!open($outfh, ">", $out_filename)) {
    close($infh);
    print "Cannot create file ${out_filename}\n";
    exit 1;
}

print "$0: converting ${in_filename} to ${out_filename}...\n";
my $count = makeConversion($in_filename, $infh, $outfh);
if ($count > 0) {
    print "$count patterns saved.\n";
} else {
    print "0 patterns found or broken source file!\n";
}

close($outfh);
close($infh);

# End of main.
1;


sub makeConversion($$$) {
    my ($srcFileName, $infh, $outfh) = @_;
    print $outfh ("<?xml version=\"1.0\" encoding=\"utf8\"?>\n" .
                  "<!--\n" .
                  "    Hyphenations description for CoolReader\n" .
                  "    from original file: ${srcFileName}\n" .
                  "\n" .
                  "// Write here something about source file\n" .
                  "// Licence, source URL, etc...\n" .
                  "\n" .
                  "-->\n\n" .
                  "<HyphenationDescription\n" .
                  "    title=\"<please_set>\" lang=\"<please_set>\"\n" .
                  "    lefthyphenmin=\"<please_set>\" righthyphenmin=\"<please_set>\">\n");
    my $count = 0;
    my $word;
    my $ch;
    foreach (<$infh>) {
        my_chomp;
        $_ = decode('UTF-8', $_);
        $word = "";
        foreach $ch (split //) {
            if ($ch =~ m/ |\t/) {
                if (length($word) > 0) {
                    outPattern($outfh, $word);
                    $count++;
                    $word = "";
                }
            } else {
                $word = $word . $ch;
            }
        }
        if (length($word) > 0) {
            outPattern($outfh, $word);
            $count++;
        }
    }
    print $outfh ("</HyphenationDescription>\n");
    return $count;
}

sub outPattern($$) {
    my ($outfh, $pattern) = @_;
    if ($pattern =~ m/^\.(.*)$/) {
        $pattern = " $1";
    }
    if ($pattern =~ m/^(.*)\.$/) {
        $pattern = "$1 ";
    }
    $pattern = encode('UTF-8', $pattern);
    print $outfh ("  <pattern>${pattern}</pattern>\n");
}

# chomp version for '\r\n' or '\n' or '\r' at the same time
sub my_chomp
{
    my $res = 0;
    my $line;
    if (defined($_[0])) {
        $line = \$_[0];
    } else {
        $line = \$_;
    }
    my $len = length($$line);
    my $c;
    if ($len > 0) {
        $c = ord(substr($$line, $len - 1, 1)); 
        if ($c == 0x0A) {
            $$line = substr($$line, 0, $len - 1);
            $len--;
            $res++;
        }
    }
    if ($len > 0) {
        $c = ord(substr($$line, $len - 1, 1)); 
        if ($c == 0x0D) {
            $$line = substr($$line, 0, $len - 1);
            $res++;
        }
    }
    return $res;
}
