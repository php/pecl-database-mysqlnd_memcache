--TEST--
mysqlnd_memcache_set(), params
--SKIPIF--
<?php
	require('skipif.inc');
	_skipif_check_extensions(array("mysqli"));

	require_once('table.inc');
	$ret = my_memcache_config::init(array('f1', 'f2', 'f3'), true, '|');
	if (true !== $ret) {
		die(sprintf("SKIP %s\n", $ret));
	}
?>
--INI--
mysqlnd_memcache.enable=1
--FILE--
<?php
	require_once('connect.inc');
	function debug_callback() {
		printf("%s()", __FUNCTION__);

		$args = func_get_args();
		foreach ($args as $k => $arg) {
			printf(" %02d: %s / %s\n", $k, gettype($arg), var_export($arg, true));
		}
	}

	$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
	if ($link->connect_errno) {
		printf("[001] [%d] %s\n", $link->connect_errno, $link->connect_error);
	}

	$memc = my_memcache_connect($memcache_host, $memcache_port);
	if (!mysqlnd_memcache_set($link, $memc, NULL, "debug_callback")) {
		printf("[002] Failed to register connection, [%d] '%s'\n",
			$link->errno, $link->error);
	}

	/* No mem access, simple function callback */
	if ($res = $link->query("SELECT f1 FROM mymem_test")) {
		$res->free_result();
	} else {
		printf("[003] [%d] %s\n", $link->errno, $link->error);
	}

	class debug {
		public static function static_db() {
			printf("%s()", __FUNCTION__);
			$args = func_get_args();
			foreach ($args as $k => $arg) {
				printf(" %02d: %s / %s\n", $k, gettype($arg), var_export($arg, true));
			}
		}
		public function cb() {
			printf("%s()", __FUNCTION__);
			$args = func_get_args();
			foreach ($args as $k => $arg) {
				printf(" %02d: %s / %s\n", $k, gettype($arg), var_export($arg, true));
			}
		}
		public function __call($a, $b) {
			printf("%s()", __FUNCTION__);
			$args = $b;
			foreach ($args as $k => $arg) {
				printf(" %02d: %s / %s\n", $k, gettype($arg), var_export($arg, true));
			}
		}
	}

	/* static class function */
	if (!mysqlnd_memcache_set($link, $memc, NULL, "debug::static_db")) {
		printf("[003] Failed to register connection, [%d] '%s'\n",
			$link->errno, $link->error);
	}

	if ($res = $link->query("SELECT f1, f2 FROM mymem_test")) {
		$res->free_result();
	} else {
		printf("[004] [%d] %s\n", $link->errno, $link->error);
	}


	$d = new debug();
	if (!mysqlnd_memcache_set($link, $memc, NULL, array($d, "cb"))) {
		printf("[005] Failed to register connection, [%d] '%s'\n",
			$link->errno, $link->error);
	}

	if ($res = $link->query("SELECT f1, f2 FROM mymem_test")) {
		$res->free_result();
	} else {
		printf("[006] [%d] %s\n", $link->errno, $link->error);
	}

	if (!mysqlnd_memcache_set($link, $memc, NULL, array($d, "gurkensalat"))) {
		printf("[007] Failed to register connection, [%d] '%s'\n",
			$link->errno, $link->error);
	}

	if ($res = $link->query("SELECT 1 FROM DUAL")) {
		$res->free_result();
	} else {
		printf("[008] [%d] %s\n", $link->errno, $link->error);
	}

	/* anonymous */
	if (!mysqlnd_memcache_set($link, $memc, NULL, function ($bar) { echo "bar = '$bar'\n"; })) {
		printf("[009] Failed to register connection, [%d] '%s'\n",
			$link->errno, $link->error);
	}

	if ($res = $link->query("SELECT 1 FROM DUAL")) {
		$res->free_result();
	} else {
		printf("[010] [%d] %s\n", $link->errno, $link->error);
	}
	print "done!";
?>
--EXPECTF--
debug_callback() 00: boolean / false
static_db() 00: boolean / false
cb() 00: boolean / false
__call() 00: boolean / false
bar = ''
done!