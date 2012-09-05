dnl $Id$
dnl config.m4 for extension mysqlnd_memcache

PHP_ARG_ENABLE(mysqlnd_memcache, whether to enable mysqlnd_memcache support,
[  --enable-mysqlnd_memcache           Enable mysqlnd_memcache support])

PHP_ARG_WITH(libmemcached-dir,  for libmemcached,
[  --with-libmemcached-dir[=DIR]   Set the path to libmemcached install prefix.], yes)

if test "$PHP_MYSQLND_MEMCACHE" != "no"; then

  if test "$PHP_LIBMEMCACHED_DIR" != "no" && test "$PHP_LIBMEMCACHED_DIR" != "yes"; then
    if test -r "$PHP_LIBMEMCACHED_DIR/include/libmemcached/memcached.h"; then
      PHP_LIBMEMCACHED_DIR="$PHP_LIBMEMCACHED_DIR"
    else
      AC_MSG_ERROR([Can't find libmemcached headers under "$PHP_LIBMEMCACHED_DIR"])
    fi
  else
    PHP_LIBMEMCACHED_DIR="no"
    for i in /usr /usr/local; do
      if test -r "$i/include/libmemcached/memcached.h"; then
        PHP_LIBMEMCACHED_DIR=$i
        break
      fi
    done
  fi

  AC_MSG_CHECKING([for libmemcached location])
  if test "$PHP_LIBMEMCACHED_DIR" = "no"; then
    AC_MSG_ERROR([mysqlnd_memcached support requires libmemcached. Use --with-libmemcached-dir=<DIR> to specify the prefix where libmemcached headers and library are located])
  else
    AC_MSG_RESULT([$PHP_LIBMEMCACHED_DIR])
    PHP_LIBMEMCACHED_INCDIR="$PHP_LIBMEMCACHED_DIR/include"
    PHP_ADD_INCLUDE($PHP_LIBMEMCACHED_INCDIR)

    PHP_ADD_LIBRARY_WITH_PATH(memcached, $PHP_LIBMEMCACHED_DIR/$PHP_LIBDIR, MEMCACHED_SHARED_LIBADD)
  fi

  PHP_NEW_EXTENSION(mysqlnd_memcache, mysqlnd_memcache.c, $ext_shared)

  PHP_ADD_EXTENSION_DEP(mysqlnd_memcache, mysqlnd)
  PHP_ADD_EXTENSION_DEP(mysqlnd_memcache, memcached)
  PHP_ADD_EXTENSION_DEP(mysqlnd_memcache, pcre)
fi
