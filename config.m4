dnl $Id$
dnl config.m4 for extension mysqlnd_memcache

PHP_ARG_ENABLE(mysqlnd_memcache, whether to enable mysqlnd_memcache support,
[  --enable-mysqlnd_memcache           Enable mysqlnd_memcache support])

if test "$PHP_MYSQLND_MEMCACHE" != "no"; then

  PHP_ADD_INCLUDE("/usr/include/libmemcached")
  PHP_ADD_LIBRARY_WITH_PATH(memcached, /usr/lib, MEMCACHED_SHARED_LIBADD)

  PHP_NEW_EXTENSION(mysqlnd_memcache, mysqlnd_memcache.c, $ext_shared)

  PHP_ADD_EXTENSION_DEP(mysqlnd_memcache, mysqlnd)
  PHP_ADD_EXTENSION_DEP(mysqlnd_memcache, memcached)
  PHP_ADD_EXTENSION_DEP(mysqlnd_memcache, pcre)
fi
