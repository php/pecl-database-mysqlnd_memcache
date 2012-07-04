<?php
	/*
	Default values are "localhost", "root",
	database "test" and empty password.
	Change the MYSQL_TEST environment values
	if you want to use another configuration

	memcache will use the MySQL host and port 11211
	if no more specific configuration is set
	*/

	$driver    = new mysqli_driver;

	$host      = getenv("MYSQL_TEST_HOST")     ? getenv("MYSQL_TEST_HOST") : "localhost";
	$port      = getenv("MYSQL_TEST_PORT")     ? getenv("MYSQL_TEST_PORT") : 3306;
	$user      = getenv("MYSQL_TEST_USER")     ? getenv("MYSQL_TEST_USER") : "root";
	$passwd    = getenv("MYSQL_TEST_PASSWD")   ? getenv("MYSQL_TEST_PASSWD") : "";
	$db        = getenv("MYSQL_TEST_DB")       ? getenv("MYSQL_TEST_DB") : "test";
	$engine    = getenv("MYSQL_TEST_ENGINE")   ? getenv("MYSQL_TEST_ENGINE") : "MyISAM";
	$socket    = getenv("MYSQL_TEST_SOCKET")   ? getenv("MYSQL_TEST_SOCKET") : null;
	if (!($memcache_host = getenv("MYSQL_TEST_MEMCACHE_HOST"))) {
		$memcache_host = $host;
	}
	$memcache_port = getenv("MYSQL_TEST_MEMCACHE_PORT") ? getenv("MYSQL_TEST_MEMCACHE_PORT") : 11211;

	$skip_on_connect_failure  = getenv("MYSQL_TEST_SKIP_CONNECT_FAILURE") ? getenv("MYSQL_TEST_SKIP_CONNECT_FAILURE") : true;
	$connect_flags = getenv("MYSQL_TEST_CONNECT_FLAGS") ? (int)getenv("MYSQL_TEST_CONNECT_FLAGS") : 0;

	if (!function_exists('my_mysqli_connect')) {

		/**
		* Whenever possible, please use this wrapper to make testing ot MYSQLI_CLIENT_COMPRESS (and potentially SSL) possible
		*
		* @param enable_env_flags Enable setting of connection flags through 	env(MYSQL_TEST_CONNECT_FLAGS)?
		*/
		function my_mysqli_connect($host, $user, $passwd, $db, $port, $socket, $enable_env_flags = true) {
			global $connect_flags;

			$flags = ($enable_env_flags) ? $connect_flags : false;

			if ($flags !== false) {
				$link = mysqli_init();
				if (!mysqli_real_connect($link, $host, $user, $passwd, $db, $port, $socket, $flags))
					$link = false;
			} else {
				$link = mysqli_connect($host, $user, $passwd, $db, $port, $socket);
			}
			if ($link) {
				$link->query("set session TRANSACTION ISOLATION LEVEL read uncommitted");
			}

			return $link;
		}

		/**
		* Whenever possible, please use this wrapper to make testing ot MYSQLI_CLIENT_COMPRESS (and potentially SSL) possible
		*
		* @param enable_env_flags Enable setting of connection flags through env(MYSQL_TEST_CONNECT_FLAGS)
		*/
		function my_mysqli_real_connect($link, $host, $user, $passwd, $db, $port, $socket, $flags = 0, $enable_env_flags = true) {
			global $connect_flags;

			if ($enable_env_flags)
				$flags & $connect_flags;

			return mysqli_real_connect($link, $host, $user, $passwd, $db, $port, $socket, $flags);
		}

		class my_mysqli extends mysqli {
			public function __construct($host, $user, $passwd, $db, $port, $socket, $enable_env_flags = true) {
				global $connect_flags;

				$flags = ($enable_env_flags) ? $connect_flags : false;

				if ($flags !== false) {
					parent::init();
					$this->real_connect($host, $user, $passwd, $db, $port, $socket, $flags);
				} else {
					parent::__construct($host, $user, $passwd, $db, $port, $socket);
				}
				$this->query("set session TRANSACTION ISOLATION LEVEL read uncommitted");
			}
		}

		function my_memcache_connect($host, $port) {
			$memc = new Memcached();
			$memc->addServer($host, $port);
			return $memc;
		}

		function my_error_handler($errno, $error, $file, $line) {
			/* KLUDGE */
			static $errcodes = array();

			if (empty($errcodes)) {
				$constants = get_defined_constants();
				foreach ($constants as $name => $value) {
					if (substr($name, 0, 2) == "E_")
						$errcodes[$value] = $name;
				}
			}
			$msg = sprintf("[%s] %s in %s on line %s\n",
				(isset($errcodes[$errno])) ? $errcodes[$errno] : $errno,
				$error, $file, $line);

			echo $msg;

			return true;
		}

	} else {
		printf("skip Eeeek/BUG/FIXME I - connect.inc included twice! skipif bug?\n");
	}

	if (extension_loaded('mysql')) {
		if (!function_exists('my_mysql_connect')) {
			/* wrapper to simplify test porting */
			function my_mysql_connect($host, $user, $passwd, $db, $port, $socket, $flags = NULL, $persistent = false) {
				global $connect_flags;

				$flags = ($flags === NULL) ? $connect_flags : $flags;

				if ($socket) {
					$host = sprintf("%s:%s", $host, $socket);
				} else if ($port) {
					$host = sprintf("%s:%s", $host, $port);
				}
				if ($persistent) {
					$link = mysql_pconnect($host, $user, $passwd, $flags);
				} else {
					$link = mysql_connect($host, $user, $passwd, true, $flags);
				}

				if (!$link) {
					printf("[000-a] Cannot connect using host '%s', user '%s', password '****', persistent = %d, [%d] %s\n",
						$host, $user, ($persistent) ? 1 : 0,
						mysql_errno(), mysql_error());
					return false;
				}

				if (!mysql_select_db($db, $link)) {
					printf("[000-b] [%d] %s\n", mysql_errno($link), mysql_error($link));
					return false;
				}
				mysql_query("set session TRANSACTION ISOLATION LEVEL read uncommitted", $link);

				return $link;
			}
		} else {
			printf("skip Eeeek/BUG/FIXME II - connect.inc included twice! skipif bug?\n");
		}
	}

	if (extension_loaded('pdo_mysql')) {
		function my_pdo_connect($host, $user, $passwd, $db, $port, $socket, $flags = NULL, $persistent = false) {
			$pdo = new PDO("mysql:hostname=$host;unix_socket=$socket;port=$port;dbname=$db", $user, $passwd);
			$pdo->query("set session TRANSACTION ISOLATION LEVEL read uncommitted");
			return $pdo;
		}
	}


?>
