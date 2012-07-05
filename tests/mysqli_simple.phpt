--TEST--
Simple mysqli test
--SKIPIF--
<?php
require('skipif.inc');
_skipif_check_extensions(array("mysqli"));
_skipif_connect($host, $user, $passwd, $db, $port, $socket);
?>
--FILE--
<?php
require 'table.inc';
my_memcache_config::init(array('f1'), true, '|');

if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
	die("Connection failed");
}

$memc = my_memcache_connect($memcache_host, $memcache_port);
mysqlnd_memcache_set($link, $memc, NULL, function ($success) { echo "Went through memcache: ".($success ? 'Yes' : 'No')."\n";});

echo "Fetching key1 via memcache:\n";
var_dump($memc->get("key1"));
echo "Querying SELECT f1 FROM mymem_test WHERE id = 'key1', then fetch_all:\n";
$r = $link->query("SELECT f1 FROM mymem_test WHERE id = 'key1'");
var_dump($r->fetch_all());

echo "Querying SELECT f1 FROM mymem_test WHERE id = 'key1', then while fetch_row:\n";
$r = $link->query("SELECT f1 FROM mymem_test WHERE id = 'key1'");
while ($row = $r->fetch_row()) {
   var_dump($row);
}
?>
--EXPECT--
Fetching key1 via memcache:
string(1) "a"
Querying SELECT f1 FROM mymem_test WHERE id = 'key1', then fetch_all:
Went through memcache: Yes
array(1) {
  [0]=>
  array(1) {
    [0]=>
    string(1) "a"
  }
}
Querying SELECT f1 FROM mymem_test WHERE id = 'key1', then while fetch_row:
Went through memcache: Yes
array(1) {
  [0]=>
  string(1) "a"
}
