--TEST--
Don't connect to memcached host
--SKIPIF--
<?php
require('skipif.inc');
?>
--FILE--
<?php
require_once('connect.inc');
require_once('table.inc');
init_memcache_config('f1,f2,f3', true, '|');
if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
	die("Connection failed");
}

$memc = new memcached();
mysqlnd_memcache_set($link, $memc);

echo "Run query:\n";
var_dump($link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 1"));
?>
--EXPECTF--
Run query:

Warning: mysqli::query(): libmemcached error code 20 in %s on line %d
bool(false)
