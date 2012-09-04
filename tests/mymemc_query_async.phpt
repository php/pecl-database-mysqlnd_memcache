--TEST--
Async query
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

	$link1 = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
	if ($link1->connect_errno) {
		printf("[001] [%d] %s\n", $link1->connect_errno, $link1->connect_error);
	}

	$link2 = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
	if ($link2->connect_errno) {
		printf("[001] [%d] %s\n", $link2->connect_errno, $link2->connect_error);
	}


	$link1->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1'", MYSQLI_ASYNC);
	$link2->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1'", MYSQLI_ASYNC);

	$all_links = array($link1, $link2);
	$processed = 0;
	do {
		$links = $errors = $reject = array();
		foreach ($all_links as $link) {
			$links[] = $errors[] = $reject[] = $link;
		}
		if (!mysqli_poll($handles, $errors, $reject, 1)) {
			continue;
		}
		foreach ($links as $link) {
			if ($res = $link->reap_async_query()) {
				while ($row = $res->fetch_assoc()) {
					var_dump($row);
				}
				$processed++;
			}
		}
	} while ($processed < count($all_links));

	print "done!";
?>
--XFAIL--
Crash
--EXPECT--
Anything but a crash
