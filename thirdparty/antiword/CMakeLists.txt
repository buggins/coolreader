INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET (ANTIWORD_SOURCES 
    main_u.c asc85enc.c blocklist.c chartrans.c datalist.c depot.c
    dib2eps.c doclist.c fail.c finddata.c findtext.c fmt_text.c fontlist.c
    fonts.c fonts_u.c hdrftrlist.c imgexam.c imgtrans.c jpeg2eps.c
    listlist.c misc.c notes.c options.c out2window.c output.c pdf.c
    pictlist.c png2eps.c postscript.c prop0.c prop2.c prop6.c prop8.c
    properties.c propmod.c rowlist.c sectlist.c stylelist.c stylesheet.c
    summary.c tabstop.c text.c unix.c utf8.c word2text.c worddos.c
    wordlib.c wordmac.c wordole.c wordwin.c xmalloc.c xml.c
)

if (WIN32)
SET (ANTIWORD_SOURCES ${ANTIWORD_SOURCES}
)
endif(WIN32)

ADD_LIBRARY(antiword STATIC ${ANTIWORD_SOURCES})

