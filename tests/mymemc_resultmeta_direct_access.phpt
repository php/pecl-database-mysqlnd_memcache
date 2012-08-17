--TEST--
Resultset meta (fetch field direct)
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
	require_once("util.inc");

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

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1'")) {
		$row = $res->fetch_row();
		$fields_mapped = $res->fetch_fields();
	} else {
		printf("[003] [%d] %s\n", $link->errno, $link->error);
	}

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1'")) {
		$row = $res->fetch_row();
		$fields_direct = array();
		for ($i = $res->field_count - 1; $i >= 0; $i--) {
			$fields_direct[$i] = $res->fetch_field_direct($i);
		}
	} else {
		printf("[004] [%d] %s\n", $link->errno, $link->error);
	}

	my_memcache_compare_meta(5, $fields_direct, $fields_mapped);

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1'")) {
		$row = $res->fetch_row();
		$fields_direct = array();
		for ($i = $res->field_count - 1; $i >= 0; $i--) {
			$res->field_seek($i);
			if ($res->current_field != $i) {
				printf("[006] Expecting %d got %d\n", $i, $res->current_field);
			}
			$fields_direct[$i] = $res->fetch_field();
		}
	} else {
		printf("[007] [%d] %s\n", $link->errno, $link->error);
	}

	my_memcache_compare_meta(8, $fields_direct, $fields_mapped);

	print "done!";
?>
--EXPECT--
debug_callback() 00: boolean / true
debug_callback() 00: boolean / true
debug_callback() 00: boolean / true
done!