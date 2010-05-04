dnl PR_CHECK_CC_OPT
dnl ---------------------
dnl Check whether the C compiler accepts the given option
AC_DEFUN([PR_CHECK_CC_OPT]
  [AC_MSG_CHECKING([whether ${CC-cc} accepts -[$1]])
   echo 'void f(){}' > conftest.c
   if test -z "`${CC-cc} -c -$1 conftest.c 2>&1`"; then
     AC_MSG_RESULT(yes)
     CFLAGS="$CFLAGS -$1"
   else
     AC_MSG_RESULT(no)
   fi
   rm -f conftest*
  ])
