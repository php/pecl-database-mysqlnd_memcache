--TEST--
Resultset meta
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

	if ($res = $link->query("SELECT SQL_NO_CACHE f1, f2, f3 FROM mymem_test_meta WHERE id = 'key1'")) {
		$fields_sql_shadow = $res->fetch_fields();
		$res->free();
	} else {
		printf("[003] [%d] %s\n", $link->errno, $link->error);
	}

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1'")) {
		$row = $res->fetch_row();
		var_dump($row);

		$fields_mapped = $res->fetch_fields();

		my_memcache_compare_meta(4, $fields_sql_shadow, $fields_mapped);

	} else {
		printf("[005] [%d] %s\n", $link->errno, $link->error);
	}

	print "done!";
?>
--XFAIL--
Meta incomplete, type looks suspicious, orgname missing
--EXPECT--
debug_callback() 00: boolean / false
debug_callback() 00: boolean / true
array(3) {
  [0]=>
  string(1) "a"
  [1]=>
  string(1) "b"
  [2]=>
  string(1) "c"
}
[004] Field values for field 0 differ
  orgname: 'f1' != ''
  table: 'mymem_test_meta' != 'mymem_test'
  orgtable: 'mymem_test_meta' != 'mymem_test'
  max_length: '%d' != '0'
  length: '%d' != '0'
  charsetnr: '%d' != '0'
  type: '253' != '254'
[004] Field values for field 1 differ
  orgname: 'f2' != ''
  table: 'mymem_test_meta' != 'mymem_test'
  orgtable: 'mymem_test_meta' != 'mymem_test'
  max_length: '%d' != '0'
  length: '%d' != '0'
  charsetnr: '%d' != '0'
  type: '253' != '254'
[004] Field values for field 2 differ
  orgname: 'f3' != ''
  table: 'mymem_test_meta' != 'mymem_test'
  orgtable: 'mymem_test_meta' != 'mymem_test'
  max_length: '%d' != '0'
  length: '%d' != '0'
  charsetnr: '%d' != '0'
  type: '253' != '254'
done!