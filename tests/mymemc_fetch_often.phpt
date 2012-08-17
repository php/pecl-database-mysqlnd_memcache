--TEST--
Fetch rows often
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

	$rows = array();
	if (!($res = $link->query("SELECT id, f1, f2, f3 FROM mymem_test"))) {
		printf("[003] [%d] %s\n", $link->errno, $link->error);
	}
	while ($row = $res->fetch_assoc()) {
		$rows[$row['id']] = array("f1" => $row['f1'], "f2" => $row['f2'], "f3" => $row['f3']);
	}

	for ($i = 1; $i <= 1000; $i++) {
		$key = sprintf("key%d", mt_rand(1, 3));
		$sql = sprintf("SELECT %s f1, f2, f3 FROM mymem_test WHERE id = '%s'",
				((mt_rand(0, 100) < 10) ? "SQL_NO_CACHE" : ""),
				$key
			);
		if (!($res = $link->query($sql))) {
			break;
		}

		$row = $res->fetch_assoc();
		if ($rows[$key] != $row) {
			printf("[004] Wrong results, dumping\n");
			var_dump($rows[$key]);
			var_dump($row);
			break;
		}
	}

	printf("[005] [%d] '%s'\n", $link->errno, $link->error);
	print "done!";
?>
--EXPECT--
[005] [0] ''
done!