/*
  +----------------------------------------------------------------------+
  | PECL mysqlnd_memcache                                                |
  +----------------------------------------------------------------------+
  | Copyright (c) 2012-2013 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Johannes Schlüter <johannes.schlueter@oracle.com>            |
  +----------------------------------------------------------------------+
*/

#ifndef PHP_MYSQLND_MEMCACHE_H
#define PHP_MYSQLND_MEMCACHE_H

extern zend_module_entry mysqlnd_memcache_module_entry;
#define phpext_mysqlnd_memcache_ptr &mysqlnd_memcache_module_entry

#define PHP_MYSQLND_MEMCACHE_VERSION "1.0.1"
#define MYSQLND_MEMCACHE_VERSION_ID 10001

#endif	/* PHP_MYSQLND_MEMCACHE_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
