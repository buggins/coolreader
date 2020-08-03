#noinst_PROGRAMS = filter_dup
include_HEADERS = \
	unibreakbase.h \
	unibreakdef.h \
	linebreak.h \
	linebreakdef.h \
	graphemebreak.h \
	wordbreak.h
lib_LTLIBRARIES = libunibreak.la

libunibreak_la_CFLAGS = -W -Wall
libunibreak_la_LDFLAGS = -no-undefined -version-info 4:3:1
libunibreak_la_SOURCES = \
	unibreakbase.c \
	unibreakdef.c \
	linebreak.c \
	linebreakdata.c \
	linebreakdef.c \
	emojidef.c \
	graphemebreak.c \
	wordbreak.c

EXTRA_DIST = \
	LineBreak1.sed \
	LineBreak2.sed \
	linebreakdata1.tmpl \
	linebreakdata2.tmpl \
	linebreakdata3.tmpl \
	wordbreakdata1.tmpl \
	wordbreakdata2.tmpl \
	wordbreakdata.c \
	graphemebreakdata1.tmpl \
	graphemebreakdata2.tmpl \
	graphemebreakdata.c \
	emojidata1.tmpl \
	emojidata2.tmpl \
	emojidata.c \
	Makefile.gcc \
	Makefile.msvc \
	filter_dup.c \
	sort_numeric_hex.py \
	wordbreakdef.h \
	graphemebreakdef.h \
	emojidef.h \
	test_skips.h

install-exec-hook:
	- [ -f $(DESTDIR)/${libdir}/libunibreak.a ] && \
	rm -f $(DESTDIR)/${libdir}/liblinebreak.a && \
	${LN_S} libunibreak.a $(DESTDIR)/${libdir}/liblinebreak.a

distclean-local:
	rm -f LineBreak.txt WordBreakData.txt GraphemeBreakData.txt emoji-data.txt LineBreakTest.txt WordBreakTest.txt GraphemeBreakTest.txt filter_dup${EXEEXT}


LineBreak.txt:
	wget http://unicode.org/Public/UNIDATA/LineBreak.txt

WordBreakProperty.txt:
	wget http://www.unicode.org/Public/UNIDATA/auxiliary/WordBreakProperty.txt

GraphemeBreakProperty.txt:
	wget http://www.unicode.org/Public/UNIDATA/auxiliary/GraphemeBreakProperty.txt

emoji-data.txt:
	wget https://www.unicode.org/Public/UNIDATA/emoji/emoji-data.txt


linebreakdata: ${builddir}/filter_dup LineBreak.txt
	sed -E -n -f ${srcdir}/LineBreak1.sed LineBreak.txt > tmp.txt
	sed -E -f ${srcdir}/LineBreak2.sed tmp.txt | ${builddir}/filter_dup > tmp.c
	head -2 LineBreak.txt > tmp.txt
	cat ${srcdir}/linebreakdata1.tmpl tmp.txt ${srcdir}/linebreakdata2.tmpl tmp.c ${srcdir}/linebreakdata3.tmpl > ${srcdir}/linebreakdata.c
	rm tmp.txt tmp.c

wordbreakdata: WordBreakProperty.txt
	sed -E -n 's/(^[0-9A-F.]+)/\1/p' WordBreakProperty.txt > tmp2.txt
	sed -E -i.bak 's/^([0-9A-F]+) +/\1..\1/' tmp2.txt
	${srcdir}/sort_numeric_hex.py tmp2.txt > tmp.txt
	rm tmp2.txt tmp2.txt.bak
	sed -E -i.bak -n 's/^([0-9A-F]+)..([0-9A-F]+) *; *([A-Za-z_]+).*/'$$'\t''{0x\1, 0x\2, WBP_\3},/p' tmp.txt
	echo "/* The content of this file is generated from:" > ${srcdir}/wordbreakdata.c
	head -2 WordBreakProperty.txt >> ${srcdir}/wordbreakdata.c
	echo "*/" >> ${srcdir}/wordbreakdata.c
	cat ${srcdir}/wordbreakdata1.tmpl tmp.txt ${srcdir}/wordbreakdata2.tmpl >> ${srcdir}/wordbreakdata.c
	rm tmp.txt tmp.txt.bak

graphemebreakdata: GraphemeBreakProperty.txt
	sed -E -n 's/(^[0-9A-F.]+)/\1/p' GraphemeBreakProperty.txt > tmp2.txt
	sed -E -i.bak 's/^([0-9A-F]+) +/\1..\1/' tmp2.txt
	${srcdir}/sort_numeric_hex.py tmp2.txt > tmp.txt
	rm tmp2.txt tmp2.txt.bak
	sed -E -i.bak -n 's/^([0-9A-F]+)..([0-9A-F]+) *; *([A-Za-z_]+).*/'$$'\t''{0x\1, 0x\2, GBP_\3},/p' tmp.txt
	echo "/* The content of this file is generated from:" > ${srcdir}/graphemebreakdata.c
	head -2 GraphemeBreakProperty.txt >> ${srcdir}/graphemebreakdata.c
	echo "*/" >> ${srcdir}/graphemebreakdata.c
	cat ${srcdir}/graphemebreakdata1.tmpl tmp.txt ${srcdir}/graphemebreakdata2.tmpl >> ${srcdir}/graphemebreakdata.c
	rm tmp.txt tmp.txt.bak

emojidata: emoji-data.txt
	echo "/* The content of this file is generated from:" > ${srcdir}/emojidata.c
	head -2 emoji-data.txt >> ${srcdir}/emojidata.c
	echo "*/" >> ${srcdir}/emojidata.c
	grep "; Extended_Pictographic#" emoji-data.txt > tmp1.txt
	sed -E -n 's/(^[0-9A-F.]+)/\1/p' tmp1.txt > tmp2.txt
	rm tmp1.txt
	sed -E -i.bak 's/^([0-9A-F]+) +/\1..\1/' tmp2.txt
	${srcdir}/sort_numeric_hex.py tmp2.txt > tmp.txt
	rm tmp2.txt tmp2.txt.bak
	sed -E -i.bak -n 's/^([0-9A-F]+)..([0-9A-F]+) *; .*/'$$'\t''{0x\1, 0x\2},/p' tmp.txt
	cat ${srcdir}/emojidata1.tmpl tmp.txt ${srcdir}/emojidata2.tmpl >> ${srcdir}/emojidata.c
	rm tmp.txt tmp.txt.bak


# Tests

check_PROGRAMS = tests

tests_SOURCES = tests.c
tests_CPPFLAGS = -I$(srcdir)
tests_LDADD = libunibreak.la
tests_DEPENDENCIES = libunibreak.la

check: LineBreakTest.txt WordBreakTest.txt GraphemeBreakTest.txt $(check_PROGRAMS)
	./tests line
	./tests word
	./tests grapheme

LineBreakTest.txt:
	wget http://www.unicode.org/Public/UNIDATA/auxiliary/LineBreakTest.txt

WordBreakTest.txt:
	wget http://www.unicode.org/Public/UNIDATA/auxiliary/WordBreakTest.txt

GraphemeBreakTest.txt:
	wget http://www.unicode.org/Public/UNIDATA/auxiliary/GraphemeBreakTest.txt
