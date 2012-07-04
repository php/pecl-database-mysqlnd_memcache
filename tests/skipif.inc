<?php
function _skipif_check_extensions($extensions) {
	if (!is_array($extensions))
		$extensions = array($extensions);

	foreach ($extensions as $k => $extension) {
		if (!extension_loaded($extension))
			die(sprintf("SKIP ext/%s not available", $extension));
	}

}

function _skipif_connect($host, $user, $passwd, $db, $port, $socket) {
	if (!$link = mst_mysqli_connect($host, $user, $passwd, $db, $port, $socket))
		die(sprintf("SKIP (h=%s,u=%s,p=%s,db=%s,P=%s,S=%s), [%d] %s\n",
			$host, $user, ($passwd) ? "***" : '', $db, $port, $socket, mysqli_connect_errno(), mysqli_connect_error()));
}

_skipif_check_extensions("memcached");
_skipif_check_extensions("mysqlnd_memcache");

