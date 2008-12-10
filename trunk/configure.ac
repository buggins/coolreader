AC_INIT(configure.ac)

AM_INIT_AUTOMAKE(crengine, 3.0.16)

AC_PROG_CXX
AM_PROG_LIBTOOL

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(FREETYPE, [freetype2 >= 9.10])
PKG_CHECK_MODULES(PNG, [libpng >= 1.2])

want_xcb=yes

AC_DEFINE(LINUX)
AC_DEFINE(_LINUX)

dnl color backbuffer option
AC_ARG_ENABLE(color-backbuffer,
  AC_HELP_STRING(
  [enable color backbuffer (gray otherwise)]),
  [AC_DEFINE(COLOR_BACKBUFFER, 1, [Build with color backbuffer])
  AC_DEFINE(LINUX)
  AC_DEFINE(_LINUX)
  ],
  [AC_DEFINE(COLOR_BACKBUFFER, 0, [Build with gray backbuffer])
  AC_DEFINE(LINUX)
  AC_DEFINE(_LINUX)
  ]
)
	       
	       
AC_DEFINE(COLOR_BACKBUFFER,0)


AC_OUTPUT(Makefile src/Makefile)
