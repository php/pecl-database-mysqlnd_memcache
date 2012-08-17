--TEST--
Result meta: length
--SKIPIF--
<?php
	require('skipif.inc');
	_skipif_check_extensions(array("mysqli"));
	_skipif_connect($host, $user, $passwd, $db, $port, $socket);

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
	require_once('connect.inc');
	require_once("util.inc");

	function debug_callback() {
		printf("%s()", __FUNCTION__);

		$args = func_get_args();
		foreach ($args as $k => $arg) {
			printf(" %02d: %s / %s\n", $k, gettype($arg), var_export($arg, true));
		}
	}

	$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
	if ($link->connect_errno) {
		printf("[001] [%d] %s\n", $link->connect_errno, $link->connect_error);
	}

	$memc = my_memcache_connect($memcache_host, $memcache_port);
	if (!mysqlnd_memcache_set($link, $memc, NULL, "debug_callback")) {
		printf("[002] Failed to register connection, [%d] '%s'\n",
			$link->errno, $link->error);
	}

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1'")) {
		$row = $res->fetch_row();
		foreach ($res->lengths as $k => $v) {
			printf("Field %d: '%s' (%d)\n", $k, $row[$k], $v);
		}
	} else {
		printf("[003] [%d] %s\n", $link->errno, $link->error);
	}

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key2'")) {
		$row = $res->fetch_row();
		foreach ($res->lengths as $k => $v) {
			printf("Field %d: '%s' (%d)\n", $k, $row[$k], $v);
		}
	} else {
		printf("[004] [%d] %s\n", $link->errno, $link->error);
	}


	print "done!";
?>
--XFAIL--
Not enough length meta
--EXPECT--
debug_callback() 00: boolean / true
Field 0: 'a' (%d)
Field 1: 'b' (%d)
Field 2: 'c' (%d)
debug_callback() 00: boolean / true
Field 0: 'foo' (%d)
Field 1: 'bar' (%d)
Field 2: 'baz' (%d)
done!