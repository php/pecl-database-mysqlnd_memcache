<?php
require_once("connect.inc");

function _skipif_check_extensions($extensions) {
	if (!is_array($extensions))
		$extensions = array($extensions);

	foreach ($extensions as $k => $extension) {
		if (!extension_loaded($extension))
			die(sprintf("SKIP ext/%s not available", $extension));
	}

}

function _skipif_connect($host, $user, $passwd, $db, $port, $socket) {
	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket))
		die(sprintf("SKIP (h=%s,u=%s,p=%s,db=%s,P=%s,S=%s), [%d] %s\n",
			$host, $user, ($passwd) ? "***" : '', $db, $port, $socket, mysqli_connect_errno(), mysqli_connect_error()));
}

function _skipif_no_plugin($host, $user, $passwd, $db, $port, $socket) {
	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket))
		die(sprintf("SKIP (h=%s,u=%s,p=%s,db=%s,P=%s,S=%s), [%d] %s\n",
			$host, $user, ($passwd) ? "***" : '', $db, $port, $socket, mysqli_connect_errno(), mysqli_connect_error()));

	if (!($res = $link->query("SHOW PLUGINS")))  {
		die(sprintf("SKIP SHOW PLUGINS failed, [%d] %s\n", $link->errno, $link->error));
	}

	$found = false;
	$plugins = array("daemon_memcached" => true);
	while ($row = $res->fetch_assoc()) {
		if (isset($plugins[$row['Name']])) {
			$found = true;
			break;
		}
	}

	if (!$found)
		die("SKIP No memcache server plugin installed");
}



_skipif_check_extensions("memcached");
_skipif_check_extensions("mysqlnd_memcache");

