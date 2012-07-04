--TEST--
MySQL Query pattern matching
--SKIPIF--
<?php
require('skipif.inc');
_skipif_check_extensions(array("mysqli"));
_skipif_no_plugin($host, $user, $passwd, $db, $port, $socket);
?>
--FILE--
<?php
require_once('connect.inc');
if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
	die("Connection failed");
}

$memc = my_memcache_connect($memcache_host, $memcache_port);
mysqlnd_memcache_set($link, $memc);

$regexp = mysqlnd_memcache_get_config($link)["pattern"];

$sqls = [
    "SELECT f FROM t WHERE i = 1",
    "SELECT f FROM t WHERE i = 'v'",
    'SELECT f FROM t WHERE i = "v"',
    "   	SELECT 	f1, f2,f3 FROM f WHERE id = 1",
    "SELECT f1, f2, f3 FROM t WHERE id = 1 AND foo = 2",
];

foreach ($sqls as $sql) {
    echo $sql."\n";
    var_dump(preg_match($regexp, $sql));
    echo "\n";
}
?>
--EXPECT--
SELECT f FROM t WHERE i = 1
int(1)

SELECT f FROM t WHERE i = 'v'
int(1)

SELECT f FROM t WHERE i = "v"
int(1)

   	SELECT 	f1, f2,f3 FROM f WHERE id = 1
int(1)

SELECT f1, f2, f3 FROM t WHERE id = 1 AND foo = 2
int(0)

