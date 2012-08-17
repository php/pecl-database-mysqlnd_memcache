--TEST--
Seek on result set
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
	$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
	if ($link->connect_errno) {
		printf("[001] [%d] %s\n", $link->connect_errno, $link->connect_error);
	}

	$memc = my_memcache_connect($memcache_host, $memcache_port);
	if (!mysqlnd_memcache_set($link, $memc)) {
		printf("[002] Failed to register connection, [%d] '%s'\n",
			$link->errno, $link->error);
	}

	if (!($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1'"))) {
		printf("[003] [%d] '%s'\n", $link->errno, $link->error);
	}

	var_dump($res->num_rows);

	print "done!";
?>
--EXPECT--
int(1)
done!
