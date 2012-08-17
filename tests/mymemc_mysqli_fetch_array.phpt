--TEST--
mysqli fetch array
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
		printf("[004] Failed to register connection, [%d] '%s'\n",
			$link->errno, $link->error);
	}

	if (!($res = $link->query("SELECT SQL_NO_CACHE f1, f2, f3 FROM mymem_test WHERE id = 'key2'"))) {
		printf("[006] %d %s\n", $link->errno, $link->error);
	}
	$data = $res->fetch_array(MYSQLI_BOTH);

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key2'")) {
		if (!is_object($res)) {
			printf("[007] No resultset but %s\n", var_export($res, true));
		} else {
			$row = $res->fetch_array(MYSQLI_BOTH);
			if ($row != $data) {
				printf("[008] Wrong results\n");
				var_dump($row);
				var_dump($data);
				var_dump(array_diff($row, $data));
			}
		}
	} else {
		printf("[009] %d %s\n", $link->errno, $link->error);
	}

	print "done!";
?>
--EXPECT--
debug_callback() 00: boolean / false
debug_callback() 00: boolean / true
done!