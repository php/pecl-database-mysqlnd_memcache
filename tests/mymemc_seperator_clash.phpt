--TEST--
Seperator clash (wrong results as expected)
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

	if (!$link->query("UPDATE mymem_test SET f1 = 'Hu|hu' WHERE id = 'key2'")) {
		printf("[002] [%d] %s\n", $link->errno, $link->error);
	}

	if (!($res = $link->query("SELECT SQL_NO_CACHE f1, f2, f3 FROM mymem_test WHERE id = 'key2'"))) {
		printf("[003] [%d] %s\n", $link->errno, $link->error);
	}
	$data = $res->fetch_assoc();
	var_dump($data);

	$memc = my_memcache_connect($memcache_host, $memcache_port);
	if (!mysqlnd_memcache_set($link, $memc, NULL, "debug_callback")) {
		printf("[004] Failed to register connection, [%d] '%s'\n",
			$link->errno, $link->error);
	}

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key2'")) {
		var_dump($res->fetch_row());
	} else {
		printf("[005] %d %s\n", $link->errno, $link->error);
	}

	if (!$link->query("UPDATE mymem_test SET f1 = NULL, f2 = '|', f3 = NULL WHERE id = 'key3'")) {
		printf("[006] [%d] %s\n", $link->errno, $link->error);
	}

	if (!($res = $link->query("SELECT SQL_NO_CACHE f1, f2, f3 FROM mymem_test WHERE id = 'key3'"))) {
		printf("[007] [%d] %s\n", $link->errno, $link->error);
	}
	$data = $res->fetch_assoc();
	var_dump($data);

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key3'")) {
		var_dump($res->fetch_row());
	} else {
		printf("[008] %d %s\n", $link->errno, $link->error);
	}


	print "done!";
?>
--EXPECT--
array(3) {
  ["f1"]=>
  string(5) "Hu|hu"
  ["f2"]=>
  string(3) "bar"
  ["f3"]=>
  string(3) "baz"
}
debug_callback() 00: boolean / true
array(4) {
  [0]=>
  string(2) "Hu"
  [1]=>
  string(2) "hu"
  [2]=>
  string(3) "bar"
  [3]=>
  string(3) "baz"
}
debug_callback() 00: boolean / false
debug_callback() 00: boolean / false
array(3) {
  ["f1"]=>
  NULL
  ["f2"]=>
  string(1) "|"
  ["f3"]=>
  NULL
}
debug_callback() 00: boolean / true
array(0) {
}
done!