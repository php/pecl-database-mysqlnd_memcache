--TEST--
Verify mapping
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

	var_dump(mysqlnd_memcache_get_config($link)["mappings"]);

	echo "done!";
?>
--EXPECTF--
array(1) {
  ["mymem_test"]=>
  array(6) {
    ["prefix"]=>
    string(0) "@@mymem_test."
    ["schema_name"]=>
    string(4) "%s"
    ["table_name"]=>
    string(10) "mymem_test"
    ["id_field_name"]=>
    string(2) "id"
    ["separator"]=>
    string(1) "|"
    ["fields"]=>
    array(3) {
      [0]=>
      string(2) "f1"
      [1]=>
      string(2) "f2"
      [2]=>
      string(2) "f3"
    }
  }
}
done!
