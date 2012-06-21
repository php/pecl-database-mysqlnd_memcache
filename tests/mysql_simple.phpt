--TEST--
Simple ext/mysql test
--INI--
extension=/home/johannes/src/php/php-memcached/modules/memcached.so
--SKIPIF--
<?php
if (!extension_loaded('mysql') || !extension_loaded('mysqlnd_memcache')) {
    echo "skip\n";
}
?>
--FILE--
<?php

$memc = new Memcached();
$memc->addServer("localhost", 11211);
$memc->add("23", "foo bar");

$mysql = mysql_connect(":/tmp/mysql56.sock", "root");
var_dump(mysqlnd_memcache_set($mysql, $memc));
mysql_select_db("test");

mysql_query("set session TRANSACTION ISOLATION LEVEL read uncommitted");

$r = mysql_query("SELECT * FROM demo_test WHERE c1 = 23");
var_dump($r);
var_dump(mysql_fetch_row($r));

echo "---DONE---\n";
--EXPECT--
bool(true)
resource(6) of type (mysql result)
array(1) {
  [0]=>
  string(10) "hallo welt"
}
---DONE---
