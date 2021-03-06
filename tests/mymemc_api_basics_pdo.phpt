--TEST--
Simple PDO test
--SKIPIF--
<?php
	require('skipif.inc');
	_skipif_check_extensions(array("pdo_mysql", "mysqli"));
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
	require_once("connect.inc");
	function debug_callback() {
		printf("%s()", __FUNCTION__);

		$args = func_get_args();
		foreach ($args as $k => $arg) {
			printf(" %02d: %s / %s\n", $k, gettype($arg), var_export($arg, true));
		}
	}

	if (!$link = my_pdo_connect($host, $user, $passwd, $db, $port, $socket)) {
		printf("[001] Connection failed\n");
	}

	/* Disable PS emulation - we do not monitor prepare() C API call */
	$link->setAttribute(PDO::ATTR_EMULATE_PREPARES, 1);

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


	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1'")) {
		$row = $res->fetch(PDO::FETCH_NUM);
		if ($row != $columns) {
			printf("[005] Native Memcache and SQL results differ\n");
			var_dump(array_diff($row, $columns));
		}

		var_dump($res->fetch());
	} else {
		printf("[004] Fetch failed, %s\n", var_export($link->errorInfo(), true));
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
debug_callback() 00: boolean / true
bool(false)
done!
