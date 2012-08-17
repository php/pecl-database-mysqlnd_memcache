--TEST--
Resultset meta - plugin disabled
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
mysqlnd_memcache.enable=0
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

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1'")) {
		$fields_mapped1 = $res->fetch_fields();
	} else {
		printf("[002] [%d] %s\n", $link->errno, $link->error);
	}

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1'")) {
		$fields_mapped2 = $res->fetch_fields();
		my_memcache_compare_meta(3, $fields_mapped1, $fields_mapped2);
	} else {
		printf("[004] [%d] %s\n", $link->errno, $link->error);
	}

	if ($res = $link->query("SELECT SQL_NO_CACHE f1, f2, f3 FROM mymem_test_meta WHERE id = 'key1'")) {
		$fields_sql_shadow = $res->fetch_fields();
		my_memcache_compare_meta(3, $fields_mapped1, $fields_sql_shadow);
		$res->free();
	} else {
		printf("[005] [%d] %s\n", $link->errno, $link->error);
	}

	print "done!";
?>
--EXPECT--
[003] Field values for field 0 differ
  table: 'mymem_test' != 'mymem_test_meta'
  orgtable: 'mymem_test' != 'mymem_test_meta'
[003] Field values for field 1 differ
  table: 'mymem_test' != 'mymem_test_meta'
  orgtable: 'mymem_test' != 'mymem_test_meta'
[003] Field values for field 2 differ
  table: 'mymem_test' != 'mymem_test_meta'
  orgtable: 'mymem_test' != 'mymem_test_meta'
done!