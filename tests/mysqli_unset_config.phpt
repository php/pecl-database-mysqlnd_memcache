--TEST--
Unsetting/Resetting mysqlnd_memcache data with mysqli
--SKIPIF--
<?php
require('skipif.inc');
_skipif_check_extensions(array("mysqli"));
_skipif_connect($host, $user, $passwd, $db, $port, $socket);
?>
--FILE--
<?php
require 'table.inc';
init_memcache_config('f1', true, '|');

if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
	die("Connection failed");
}

function callback($success) {
	echo "Went through memcache: ".($success ? 'Yes' : 'No')."\n";
}

$memc = my_memcache_connect($memcache_host, $memcache_port);

echo "Setting mysqlnd_memcache:\n";
var_dump(mysqlnd_memcache_set($link, $memc, NULL, "callback"));

echo "Querying SELECT f1 FROM mymem_test WHERE id = 'key1', then fetch_all:\n";
$r = $link->query("SELECT f1 FROM mymem_test WHERE id = 'key1'");
var_dump($r->fetch_all());

echo "Unsetting mysqlnd_memcache:\n";
var_dump(mysqlnd_memcache_set($link, NULL));

echo "Querying SELECT f1 FROM mymem_test WHERE id = 'key1', then fetch_all:\n";
$r = $link->query("SELECT f1 FROM mymem_test WHERE id = 'key1'");
var_dump($r->fetch_all());

echo "Resetting mysqlnd_memcache:\n";
var_dump(mysqlnd_memcache_set($link, $memc, NULL, "callback"));

echo "Querying SELECT f1 FROM mymem_test WHERE id = 'key1', then fetch_all:\n";
$r = $link->query("SELECT f1 FROM mymem_test WHERE id = 'key1'");
var_dump($r->fetch_all());

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
