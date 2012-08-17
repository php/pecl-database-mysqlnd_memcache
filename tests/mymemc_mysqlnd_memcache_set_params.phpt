--TEST--
mysqlnd_memcache_set(), params
--SKIPIF--
<?php
	require('skipif.inc');
	_skipif_check_extensions(array("mysqli"));

	require_once('table.inc');
	$ret = my_memcache_config::init(array('f1', 'f2', 'f3'), true, '|');
	if (true !== $ret) {
		die(sprintf("SKIP %s\n", $ret));
	}
?>
--INI--
mysqlnd_memcache.enable=1
--FILE--
<?php
require_once("connect.inc");
require 'table.inc';
set_error_handler('my_error_handler');

if (NULL !== ($tmp = @mysqlnd_memcache_set())) {
	printf("[001] Expecting NULL got %s\n", var_export($tmp, true));
}
if (NULL !== ($tmp = @mysqlnd_memcache_set(NULL))) {
	printf("[002] Expecting NULL got %s\n", var_export($tmp, true));
}
if (NULL !== ($tmp = @mysqlnd_memcache_set(NULL, NULL, NULL, NULL, NULL))) {
	printf("[003] Expecting NULL got %s\n", var_export($tmp, true));
}


if (NULL !== ($tmp = mysqlnd_memcache_set("a", "b"))) {
	printf("[004] Expecting NULL got %s\n", var_export($tmp, true));
}

if (!$link = my_mysql_connect($host, $user, $passwd, $db, $port, $socket)) {
	printf("[005] [%d] %s\n", mysql_errno(), mysql_error());
}
$memc = my_memcache_connect($memcache_host, $memcache_port);

if (false !== ($tmp = mysqlnd_memcache_set("b", $memc))) {
	printf("[006] Expecting false got %s\n", var_export($tmp, true));
}

if (true !== ($tmp = mysqlnd_memcache_set($link, $memc))) {
	printf("[007] Expecting true got %s\n", var_export($tmp, true));
}

print "done!";
?>
--EXPECTF--
[E_WARNING] mysqlnd_memcache_set() expects at least 2 parameters, 0 given in %s on line %d
[E_WARNING] mysqlnd_memcache_set() expects at least 2 parameters, 1 given in %s on line %d
[E_WARNING] mysqlnd_memcache_set() expects at most 4 parameters, 5 given in %s on line %d
[E_RECOVERABLE_ERROR] Argument 2 passed to mysqlnd_memcache_set() must be an instance of Memcached, string given in %s on line %d
[E_WARNING] mysqlnd_memcache_set() expects parameter 2 to be Memcached, string given in %s on line %d
[E_WARNING] mysqlnd_memcache_set(): Passed variable is no mysqlnd-based MySQL connection in %s on line %d
done!