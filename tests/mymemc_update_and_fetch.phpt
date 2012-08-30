--TEST--
Update (SQL) and fetch (memcache)
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
		$args = func_get_args();
		if (!$args[0])
			return;

		printf("%s()", __FUNCTION__);
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
		printf("[004] Failed to register connection, [%d] '%s'\n",
			$link->errno, $link->error);
	}

	for ($i = 0; $i < 100; $i++) {
		if (!$link->query("UPDATE mymem_test SET f1='a' WHERE id='key2'")) {
			printf("[005] %d %s\n", $link->errno, $link->error);
		}
	}

	if (!($key1 = $memc->get("@@mymem_test.key2"))) {
		printf("[006] Failed to fetch 'key2' using native Memcache API.\n");
	}
	$columns = explode("|", $key1);
	var_dump($columns);

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key2'")) {
		if (!is_object($res)) {
			printf("[007] No resultset but %s\n", var_export($res, true));
		} else {
			var_dump($res->fetch_assoc());
		}
	} else {
		printf("[008] %d %s\n", $link->errno, $link->error);
	}

	print "done!";
?>
--XFAIL--
No results
--EXPECT--
debug_callback() 00: boolean / false
debug_callback() 00: boolean / true
some resultset
done!
