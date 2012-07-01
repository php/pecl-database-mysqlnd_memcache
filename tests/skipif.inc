<?php
if (!extension_loaded('memcached')) {
	die('skip memcached extension not available');
}
if (!extension_loaded('mysqlnd_memcache')) {
	die('skip mysqlnd_memcache extension not available');
}

if (!getenv('MYSQLND_MEMCACHE_I_KNOW_WHAT_I_DO')) {
	die('skip MYSQLND_MEMCACHE_I_KNOW_WHAT_I_DO not set in environment');
}

require_once('connect.inc');
$link = @my_mysqli_connect($host, $user, $passwd, $db, $port, $socket); 
if (!is_object($link)) {
	die(sprintf("skip Can't connect to MySQL Server - [%d] %s", mysqli_connect_errno(), mysqli_connect_error()));
}
mysqli_close($link);
