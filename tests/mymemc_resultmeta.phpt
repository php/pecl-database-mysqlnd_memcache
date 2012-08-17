--TEST--
Resultset meta
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

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test_meta WHERE id = 'key1'")) {
		$fields = $res->fetch_fields();
		$res->free();
	} else {
		printf("[004] [%d] %s\n", $link->errno, $link->error);
	}
	var_dump($fields);

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test_meta WHERE id = 'key1'")) {
		$fields = $res->fetch_fields();
	} else {
		printf("[005] [%d] %s\n", $link->errno, $link->error);
	}
	var_dump($fields);

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1'")) {
		$row = $res->fetch_row();

		if ($row != $columns) {
			printf("[005] Native Memcache and SQL results differ\n");
			var_dump(array_diff($row, $columns));
		}
		$fields_mapped = $res->fetch_fields();
		if ($fields != $fields_mapped) {
			var_dump(array_diff($fields, $fields_mapped));
		}

	} else {
		printf("[004] [%d] %s\n", $link->errno, $link->error);
	}

	print "done!";
?>
--XFAIL--
Test does not expose it but server returns incomplete meta
--EXPECT--
array(3) {
  [0]=>
  string(1) "a"
  [1]=>
  string(1) "b"
  [2]=>
  string(1) "c"
}
debug_callback() 00: boolean / true
debug_callback() 00: boolean / true
done!
