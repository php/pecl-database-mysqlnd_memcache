--TEST--
mysqlnd_memcache_get_config() - parameter
--SKIPIF--
<?php
require('skipif.inc');
_skipif_check_extensions(array("mysqli"));
?>
--INI--
mysqlnd_memcache.enable=1
--FILE--
<?php
	require_once("connect.inc");

	if (NULL != ($tmp = @mysqlnd_memcache_get_config())) {
		printf("[001] Expecting NULL, got %s\n", var_export($tmp, true));
	}
	if (NULL != ($tmp = @mysqlnd_memcache_get_config(1, 2))) {
		printf("[002] Expecting NULL, got %s\n", var_export($tmp, true));
	}

	$params = array(PHP_INT_MAX, new stdClass(), "foo", array("bar"));
	foreach ($params as $param) {
		var_dump(mysqlnd_memcache_get_config($param));
	}

	$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
	if (mysqli_connect_errno()) {
		printf("[003] [%d] %s\n", mysqli_connect_errno(),mysqli_connect_error());
	}

	var_dump(mysqlnd_memcache_get_config($link));
	$link->kill($link->thread_id);
	var_dump(mysqlnd_memcache_get_config($link));

	print "done!";
?>
--EXPECTF--
Warning: mysqlnd_memcache_get_config(): Passed variable is no mysqlnd-based MySQL connection in %s on line %d
bool(false)

Warning: mysqlnd_memcache_get_config(): Passed variable is no mysqlnd-based MySQL connection in %s on line %d
bool(false)

Warning: mysqlnd_memcache_get_config(): Passed variable is no mysqlnd-based MySQL connection in %s on line %d
bool(false)

Warning: mysqlnd_memcache_get_config(): Passed variable is no mysqlnd-based MySQL connection in %s on line %d
bool(false)
bool(false)
bool(false)
done!