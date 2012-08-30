--TEST--
Column alias
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

	if (!($key1 = $memc->get("@@mymem_test.key1"))) {
		printf("[003] Failed to fetch 'key1' using native Memcache API.\n");
	}
	$columns = explode("|", $key1);
	var_dump($columns);


	if ($res = $link->query("SELECT f1 AS f2, f3 FROM mymem_test WHERE id = 'key1'")) {
		var_dump($res->fetch_row());
		var_dump($res->fetch_assoc());

	} else {
		printf("[004] %d %s\n", $link->errno, $link->error);
	}

	if ($res = $link->query("SELECT 1 AS f1, 2 AS f2, 3 AS f3 FROM mymem_test WHERE id = 'key1'")) {
		var_dump($res->fetch_row());
		var_dump($res->fetch_assoc());

	} else {
		printf("[005] %d %s\n", $link->errno, $link->error);
	}

	print "done!";
?>
--EXPECT--
array(3) {
  [0]=>
  string(1) "a"
  [1]=>
  string(1) "b"
  [2]=>
  string(1) "c"
}
debug_callback() 00: boolean / false
array(2) {
  [0]=>
  string(1) "a"
  [1]=>
  string(1) "c"
}
NULL
debug_callback() 00: boolean / false
array(3) {
  [0]=>
  string(1) "1"
  [1]=>
  string(1) "2"
  [2]=>
  string(1) "3"
}
NULL
done!
