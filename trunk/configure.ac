AC_INIT(configure.ac)

AM_INIT_AUTOMAKE(crengine, 3.0.18)

AC_PROG_CXX
AM_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])

AC_ARG_WITH(jinke,AC_HELP_STRING([--with-jinke],[path to jinke SDK]),
    [
        ac_cv_use_jinke=$withval
        with_jinke=true
        case ${host_cpu} in
            i?86) cpu_libdir=i386 ;;
            *) cpu_libdir="${host_cpu}";;
        esac
        JINKEPATH="$withval"
        CPPFLAGS="${CPPFLAGS} -I${JINKEPATH}/include"
        #LDFLAGS="${LDFLAGS} -L${JINKEPATH}/lib/${cpu_libdir}"
        export CPPFLAGS
        export LDFLAGS
        export JINKEPATH
        with_jinke=true
    ],[
        with_jinke=false
        ac_cv_use_jinke=none
    PKG_PROG_PKG_CONFIG
    PKG_CHECK_MODULES(FREETYPE, [freetype2 >= 9.10])
    PKG_CHECK_MODULES(PNG, [libpng >= 1.2])
])

AM_CONDITIONAL(WITH_JINKE,$with_jinke)

want_xcb=yes

AC_DEFINE(LINUX)
AC_DEFINE(_LINUX)

dnl color backbuffer option
AC_ARG_ENABLE(color-backbuffer,
  AC_HELP_STRING(
  [enable color backbuffer (gray otherwise)]),
  [AC_DEFINE(COLOR_BACKBUFFER, 1, [Build with color backbuffer])
  #AC_DEFINE(LINUX)
  #AC_DEFINE(_LINUX)
  ],
  [AC_DEFINE(COLOR_BACKBUFFER, 0, [Build with gray backbuffer])
  #AC_DEFINE(LINUX)
  #AC_DEFINE(_LINUX)
  ]
)
	       
dnl crengine memory manager option
AC_ARG_ENABLE(crengine-memory-manager,
   AC_HELP_STRING(
      [enable crengine internal memory manager]),
   [AC_DEFINE(LDOM_USE_OWN_MEM_MAN, 1, [Build with internal memory manager support])],
   [AC_DEFINE(LDOM_USE_OWN_MEM_MAN, 0, [Build without internal memory manager support])]
)
	       
AC_OUTPUT(Makefile src/Makefile)
