/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2012 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_MYSQLND_MEMCACHE_H
#define PHP_MYSQLND_MEMCACHE_H

extern zend_module_entry mysqlnd_memcache_module_entry;
#define phpext_mysqlnd_memcache_ptr &mysqlnd_memcache_module_entry

#ifdef PHP_WIN32
#	define PHP_MYSQLND_MEMCACHE_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_MYSQLND_MEMCACHE_API __attribute__ ((visibility("default")))
#else
#	define PHP_MYSQLND_MEMCACHE_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

/* 
ZEND_BEGIN_MODULE_GLOBALS(mysqlnd_memcache)
	long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(mysqlnd_memcache)
*/

#ifdef ZTS
#define MYSQLND_MEMCACHE_G(v) TSRMG(mysqlnd_memcache_globals_id, zend_mysqlnd_memcache_globals *, v)
#else
#define MYSQLND_MEMCACHE_G(v) (mysqlnd_memcache_globals.v)
#endif

#endif	/* PHP_MYSQLND_MEMCACHE_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
