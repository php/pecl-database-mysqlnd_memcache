--TEST--
mysqli multiple fields
--SKIPIF--
<?php
require('skipif.inc');
_skipif_check_extensions(array("mysqli"));
_skipif_no_plugin($host, $user, $passwd, $db, $port, $socket);
?>
--FILE--
<?php
require 'table.inc';
init_memcache_config('f1,f2,f3', true, '|');

if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
	die("Connection failed");
}

$memc = my_memcache_connect($memcache_host, $memcache_port);
mysqlnd_memcache_set($link, $memc, NULL, function ($success) { echo "Went through memcache: ".($success ? 'Yes' : 'No')."\n";});

echo "Fetching key1 via memcache:\n";
var_dump($memc->get("key1"));
echo "Querying SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1':\n";
$r = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1'");
var_dump($r->fetch_all());
?>
--EXPECT--
Fetching key1 via memcache:
string(5) "a|b|c"
Querying SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1':
Went through memcache: Yes
array(1) {
  [0]=>
  array(3) {
    [0]=>
    string(1) "a"
    [1]=>
    string(1) "b"
    [2]=>
    string(1) "c"
  }
}
