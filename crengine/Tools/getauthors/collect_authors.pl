#!/usr/bin/perl

###########################################################################
#   CoolReader, script to collect authors list from sources               #
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

use warnings;
use strict;
use utf8;

use Fcntl ':mode';

# Directories to scan
use constant SRC_DIRS => (
    'crengine',
    'cr3gui/src',
    'cr3qt/src',
    'cr3wx/src',
    'fb2props',
    'tinydict',
    'android/src',
    'android/app/src/main/java',
    'android/genrescollection/src/main/java',
    'android/jni'
);

# File patterns to scan
use constant SRC_TYPES => (
    '*.h', '*.c', '*.cpp', '*.java', '*.idl',
    '*.sh', '*.py', '*.pl'
);

use constant EXCLUDE_LIST => (
    'crengine/include/encodings',
    'crengine/src/xxhash.h',
    'crengine/src/xxhash.c',
    'crengine/src/locale_data/files',
    'android/jni/coffeecatch'
);

use constant TOP_SRCDIR => '../../..';

sub collect_authors($$);
sub analyze_file($$);
sub glob2pattern($);
sub my_chomp;

my %authors = ();

for (SRC_DIRS) {
    my $dir = TOP_SRCDIR . '/' . $_;
    collect_authors($dir, \%authors);
}

# print result
my @sorted_authors = sort { $authors{$b} <=> $authors{$a} } keys %authors;
foreach my $author (@sorted_authors) {
    #print("${author}: ${authors{${author}}}\n");
    print("${author}\n");
}

# End of main.
1;


sub collect_authors($$) {
    my ($dirname, $authorsref) = @_;
    # $dirname - directory name (path)
    # $authorsref - ref to %authors
    print("Collecting authors in \'${dirname}\'...\n");
    my $pattern;
    my ($hdir, $entry, $fname, @st, $eligible, $to_skip, $relpath);
    my $path_prefix_len = length(TOP_SRCDIR . '/');
    if (opendir($hdir, $dirname)) {
        while (1) {
            $entry = readdir($hdir);
            last if (!$entry);
            next if ($entry eq '.' || $entry eq '..');
            $fname = $dirname . '/' . $entry;
            @st = stat($fname);
            if (scalar(@st) != 0) {
                $relpath = substr($fname, $path_prefix_len);
                $to_skip = undef;
                for (EXCLUDE_LIST) {
                    if ($relpath eq $_) {
                        $to_skip = 1;
                    }
                }
                next if $to_skip;
                if (S_ISDIR($st[2])) {
                    #printf("dir: ${fname}\n");
                    collect_authors($fname, $authorsref);
                } else {
                    #printf("file: ${fname}\n");
                    $eligible = undef;
                    for (SRC_TYPES) {
                        $pattern = glob2pattern($_);
                        if ($entry =~ m/^$pattern$/) {
                            $eligible = 1;
                            last;
                        }
                    }
                    for (EXCLUDE_LIST) {
                        if ($entry eq $_) {
                            $eligible = undef;
                        }
                    }
                    analyze_file($fname, $authorsref) if $eligible;
                }
            } else {
                print("stat failed for \'${fname}\'!\n");
            }
        }
        closedir($hdir);
    } else {
        print("Failed to open dir \"$dirname\", skipping!\n");
    }
}

sub analyze_file($$) {
    my ($filename, $authorsref) = @_;
    # $filename - file name to parse
    # $authorsref - ref to %authors

    print("Processing file \'${filename}\'\n");
    my ($fh, $lineno, $line);
    if (open($fh, "< ${filename}")) {
        $lineno = 0;
        foreach(<$fh>) {
            my_chomp;
            $line = $_;
            #printf("%04d: %s\n", $lineno, $line);
            $lineno++;
            last if ($lineno > 100);
            if (m/^ *\* *(.*) *\*$/) {
                $line = $1;
            } elsif (m/^ *\* *(.*) *$/) {
                $line = $1;
            } else {
                next;
            }
            $line =~ s/^\s+|\s+$//g;
            if ($line =~ m/^Copyright +\([Cc©]\) +(.*)$/) {
                $line = $1;
            } else {
                next;
            }
            if ($line =~ m/^20[012][0-9,-]+ +(.*)$/) {
                $line = $1;
            }
            if (!$$authorsref{$line}) {
                $$authorsref{$line} = 1;
            } else {
                $$authorsref{$line}++;
            }
        }
        close($fh);
    } else {
        print("Failed to open file!\n");
    }
}

sub glob2pattern($) {
    my $pattern = @_[0];
    $pattern =~ s/\./\\\./g;
    $pattern =~ s/\*/\.\*/g;
    $pattern =~ s/(.*)/\^$1\$/;
    return $pattern;
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
