# get rid of that stupid cache mechanism
rm -f config.cache

AC_INIT(cr3, 3.0.16, vadim.lopatin@coolreader.org)
AC_PREREQ(2.52)
AC_CONFIG_SRCDIR(src/cr3main.cpp)
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

echo "${srcdir}/crengine/configure"
if test -r "${srcdir}/crengine/configure"; then
    AC_CONFIG_SUBDIRS([crengine])
    CRENGINE_CFLAGS="-I\$(srcdir)/crengine/include -DLINUX -D_LINUX -DCOLOR_BACKBUFFER=0"
    CRENGINE_LIBS="\$(top_builddir)/crengine/src/libcrengine.la"
else
    PKG_CHECK_MODULES(CRENGINE, [crengine >= 3.0.16])
fi

dnl color backbuffer option
AC_ARG_ENABLE(color-backbuffer,
   AC_HELP_STRING(
      [enable color backbuffer (gray otherwise)]),
   [AC_DEFINE(COLOR_BACKBUFFER, 1, [Build with color backbuffer])],
   [AC_DEFINE(COLOR_BACKBUFFER, 0, [Build with gray backbuffer])]
)

dnl crengine memory manager option
AC_ARG_ENABLE(crengine-memory-manager,
   AC_HELP_STRING(
      [enable crengine internal memory manager]),
   [AC_DEFINE(LDOM_USE_OWN_MEM_MAN, 1, [Build with internal memory manager support])],
   [AC_DEFINE(LDOM_USE_OWN_MEM_MAN, 0, [Build without internal memory manager support])]
)

AC_SUBST([CRENGINE_CFLAGS])
AC_SUBST([CRENGINE_LIBS])

if test -r "${srcdir}/dict-plugin/configure"; then
    AC_DEFINE(WITH_DICT,1)
    AC_CONFIG_SUBDIRS(dict-plugin)
    with_dict=true
else
    with_dict=false
fi
AM_CONDITIONAL(WITH_DICT,$with_dict)

#AC_CONFIG_FILES([Makefile])

AC_OUTPUT(Makefile src/Makefile)
