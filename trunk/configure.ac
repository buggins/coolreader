# get rid of that stupid cache mechanism
rm -f config.cache

AC_INIT(cr3, 3.0.16, vadim.lopatin@coolreader.org)
AC_PREREQ(2.52)
AC_CONFIG_SRCDIR(configure.ac)
#AC_CONFIG_SRCDIR(crengine/configure.ac)
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_ISC_POSIX

AM_INIT_AUTOMAKE(1.6 dist-bzip2)

AC_PROG_CXX
AC_PROG_CC
AC_PROG_CC_STDC
AC_HEADER_STDC
AC_C_BIGENDIAN
AC_C_CONST
AM_PROG_LIBTOOL

PKG_PROG_PKG_CONFIG


#PKG_CHECK_MODULES(EWL, [ewl >= 0.5.2.042])
PKG_CHECK_MODULES(XCB, [xcb >= 1.1])
PKG_CHECK_MODULES(XCB_AUX, [xcb-aux >= 0.2])
PKG_CHECK_MODULES(XCB_IMAGE, [xcb-image >= 0.2])
PKG_CHECK_MODULES(XCB_KEYSYMS, [xcb-keysyms >= 0.2])
PKG_CHECK_MODULES(FREETYPE, [freetype2 >= 9.10])
PKG_CHECK_MODULES(PNG, [libpng >= 1.2])

#PKG_CHECK_MODULES(CRENGINE, [crengine >= 3.0.16])
#CRENGINE_CFLAGS="-I${top_srcdir}/crengine/include -DLINUX -D_LINUX -DCOLOR_BACKBUFFER=0 "
#CRENGINE_LIBS="-L${top_srcdir}/crengine/src -lcrengine "
CRENGINE_CFLAGS="-I../crengine/include -DLINUX -D_LINUX "
CRENGINE_LIBS="-L../crengine/src -lcrengine "

dnl color backbuffer option
AC_ARG_ENABLE(color-backbuffer,
   AC_HELP_STRING(
      [enable color backbuffer (gray otherwise)]),
   [AC_DEFINE(COLOR_BACKBUFFER, 1, [Build with color backbuffer])],
   [AC_DEFINE(COLOR_BACKBUFFER, 0, [Build with gray backbuffer])]
)

AC_SUBST([CRENGINE_CFLAGS])
AC_SUBST([CRENGINE_LIBS])

AC_CONFIG_SUBDIRS(crengine)

AC_OUTPUT(Makefile src/Makefile crengine/Makefile crengine/src/Makefile)
