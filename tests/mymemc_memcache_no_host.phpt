--TEST--
Don't connect to memcached host
--SKIPIF--
<?php
	require('skipif.inc');
	_skipif_check_extensions(array("mysqli"));

	require_once('table.inc');
	$ret = my_memcache_config::init(array('f1'), true, '|');
	if (true !== $ret) {
		die(sprintf("SKIP %s\n", $ret));
	}
?>
--INI--
mysqlnd_memcache.enable=1
--FILE--
<?php
	require_once('connect.inc');

	$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
	if ($link->connect_errno) {
		printf("[001] [%d] %s\n", $link->connect_errno, $link->connect_error);
	}

	/* Unconnected memcache */
	$memc = new memcached();
	if (!mysqlnd_memcache_set($link, $memc)) {
		printf("[002] Failed to register connection\n");
	}

	/* Query is value but no memcache connection */
	if ($res = $link->query("SELECT f1 FROM mymem_test WHERE id = 1")) {
		while ($row = $res->fetch_assoc()) {
			var_dump($row);
		}
	} else {
		printf("[003] %s, [%d] '%s'\n", var_export($res, true), $link->errno, $link->error);
	}

	print "done!";
?>
--EXPECTF--
Warning: mysqli::query(): libmemcached error %s in %s on line %d
[003] false, [0] ''
done!