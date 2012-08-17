--TEST--
Resultset meta from the server
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
	require_once('util.inc');

	$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
	if ($link->connect_errno) {
		printf("[001] [%d] %s\n", $link->connect_errno, $link->connect_error);
	}

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test_meta WHERE id = 'key1'")) {
		$fields = $res->fetch_fields();
		$res->free();
	} else {
		printf("[004] [%d] %s\n", $link->errno, $link->error);
	}

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test_meta WHERE id = 'key1'")) {
		$fields_meta = $res->fetch_fields();
	} else {
		printf("[005] [%d] %s\n", $link->errno, $link->error);
	}

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1'")) {
		$fields_memc = $res->fetch_fields();
	} else {
		printf("[006] [%d] %s\n", $link->errno, $link->error);
	}

	printf("Shadow table checking\n");
	my_memcache_compare_meta(7, $fields_meta, $fields);
	printf("Meta of Memcache mapped and shadow table\n");
	my_memcache_compare_meta(8, $fields_meta, $fields_memc);

	print "done!";
?>
--EXPECT--
Shadow table checking
Meta of Memcache mapped and shadow table
[008] Field values for field 0 differ
  table: 'mymem_test_meta' != 'mymem_test'
  orgtable: 'mymem_test_meta' != 'mymem_test'
[008] Field values for field 1 differ
  table: 'mymem_test_meta' != 'mymem_test'
  orgtable: 'mymem_test_meta' != 'mymem_test'
[008] Field values for field 2 differ
  table: 'mymem_test_meta' != 'mymem_test'
  orgtable: 'mymem_test_meta' != 'mymem_test'
done!