dnl $Id$
dnl config.m4 for extension phpjansson

PHP_ARG_WITH(jansson, for Jansson support,
[  --with-jansson             Include Jansson support])

if test "$PHP_JANSSON" != "no"; then
  SEARCH_PATH="/usr/local /usr"
  SEARCH_FOR="/include/jansson.h"
  if test -r $PHP_JANSSON/$SEARCH_FOR; then
    JANSSON_DIR=$PHP_JANSSON
  else
   AC_MSG_CHECKING([for jansson files in default paths])
    for i in $SEARCH_PATH ; do
      if test -r $i/$SEARCH_FOR; then
        JANSSON_DIR=$i
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi
  
  if test -z "$JANSSON_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the Jansson 2.7 or greater distribution])
  fi

  PHP_ADD_INCLUDE($JANSSON_DIR/include)

  LIBNAME=jansson
  LIBSYMBOL=json_stringn

  PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  [
   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $JANSSON_DIR/lib, JANSSON_SHARED_LIBADD)
    AC_DEFINE(HAVE_JANSSONLIB,1,[ ])
  ],[
     AC_MSG_ERROR([wrong Jansson lib version or lib not found, version 2.7+ required])
  ],[
     -L$JANSSON_DIR/lib
  ])

  PHP_SUBST(JANSSON_SHARED_LIBADD)

  PHP_NEW_EXTENSION(jansson, jansson.c, $ext_shared)
fi

