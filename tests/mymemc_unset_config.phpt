--TEST--
Unsetting/Resetting mysqlnd_memcache data
--SKIPIF--
<?php
	require('skipif.inc');
	_skipif_check_extensions(array("mysqli"));
	_skipif_no_plugin($host, $user, $passwd, $db, $port, $socket);

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

	function callback($success) {
		echo "Went through memcache: ".($success ? 'Yes' : 'No')."\n";
	}

	$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
	if ($link->connect_errno) {
		printf("[001] [%d] %s\n", $link->connect_errno, $link->connect_error);
	}

	$memc = my_memcache_connect($memcache_host, $memcache_port);

	echo "Setting mysqlnd_memcache:\n";
	var_dump(mysqlnd_memcache_set($link, $memc, NULL, "callback"));

	echo "Querying SELECT f1 FROM mymem_test WHERE id = 'key1', then fetch_all:\n";
	if ($r = $link->query("SELECT f1 FROM mymem_test WHERE id = 'key1'")) {
		var_dump($r->fetch_all());
	} else {
		printf("[002] [%d] %s\n", $link->errno, $link->error);
	}

	echo "Unsetting mysqlnd_memcache:\n";
	var_dump(mysqlnd_memcache_set($link, NULL));

	echo "Querying SELECT f1 FROM mymem_test WHERE id = 'key1', then fetch_all:\n";
	if ($r = $link->query("SELECT f1 FROM mymem_test WHERE id = 'key1'")) {
		var_dump($r->fetch_all());
	} else {
		printf("[003] [%d] %s\n", $link->errno, $link->error);
	}

	echo "Resetting mysqlnd_memcache:\n";
	var_dump(mysqlnd_memcache_set($link, $memc, NULL, "callback"));

	echo "Querying SELECT f1 FROM mymem_test WHERE id = 'key1', then fetch_all:\n";
	if ($r = $link->query("SELECT f1 FROM mymem_test WHERE id = 'key1'")) {
		var_dump($r->fetch_all());
	} else {
		printf("[004] [%d] %s\n", $link->errno, $link->error);
	}

?>
--EXPECT--
Setting mysqlnd_memcache:
bool(true)
Querying SELECT f1 FROM mymem_test WHERE id = 'key1', then fetch_all:
Went through memcache: Yes
array(1) {
  [0]=>
  array(1) {
    [0]=>
    string(1) "a"
  }
}
Unsetting mysqlnd_memcache:
bool(true)
Querying SELECT f1 FROM mymem_test WHERE id = 'key1', then fetch_all:
array(1) {
  [0]=>
  array(1) {
    [0]=>
    string(1) "a"
  }
}
Resetting mysqlnd_memcache:
bool(true)
Querying SELECT f1 FROM mymem_test WHERE id = 'key1', then fetch_all:
Went through memcache: Yes
array(1) {
  [0]=>
  array(1) {
    [0]=>
    string(1) "a"
  }
}
