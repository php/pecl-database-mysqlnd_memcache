--TEST--
Simple ext/mysql test
--INI--
extension=/home/johannes/src/php/php-memcached/modules/memcached.so
--SKIPIF--
<?php
if (!extension_loaded('mysqli') || !extension_loaded('mysqlnd_memcache')) {
    echo "skip\n";
}
?>
--FILE--
<?php

$memc = new Memcached();
$memc->addServer("localhost", 11211);
$memc->add("23", "foo bar", 10);

$m = new mysqli("localhost", "root", "", "test", 3356, "/tmp/mysql56.sock");

var_dump(mysqlnd_memcache_set($m, $memc));

$result = $m->query("SELECT * FROM demo_test WHERE c1 =23");
var_dump($result->num_rows);
var_dump($result->fetch_all());
?>
--EXPECT--
bool(true)
int(1)
array(1) {
  [0]=>
  array(1) {
    [0]=>
    string(10) "hallo welt"
  }
}
