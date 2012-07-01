<?php
require_once('connect.inc');

function init_memcache_config($value_columns, $id_is_string = false, $separator = '|') {
	global $host, $user, $passwd, $db, $port, $socket;

	if (!$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
		printf("Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n",
			$host, $user, $db, $port, $socket);
		exit(1);
	}

	mysqli_query($link, 'UNINSTALL PLUGIN daemon_memcached');

	if (!mysqli_query($link, 'DROP TABLE IF EXISTS mymem_test')) {
		printf("Failed to drop old test table: [%d] %s\n", mysqli_errno($link), mysqli_error($link));
		exit(1);
	}

	$id_type = $id_is_string ? 'CHAR(4)' : 'INT';
	if (!mysqli_query($link, 'CREATE TABLE mymem_test(id '.$id_type.', f1 VARCHAR(255), f2 VARCHAR(255), f3 VARCHAR(255), flags INT NOT NULL, cas_column INT, expire_time_column INT, PRIMARY KEY(id)) ENGINE=InnoDB DEFAULT CHARSET=latin1')) {
		printf("Failed to create test table: [%d] %s\n", mysqli_errno($link), mysqli_error($link));
		exit(1);
	}
	$id_prefix = $id_is_string ? 'key' : '';
	if (!mysqli_query($link, "INSERT INTO mymem_test(id, f1, f2, f3) VALUES ('${id_prefix}1', 'a', 'b', 'c'), ('${id_prefix}2', 'foo', 'bar', 'baz'), ('${id_prefix}3', 'der Hund', 'liegt in der Kueche', 'und bellt')")) {
		printf("[%d] %s\n",  mysqli_errno($link), mysqli_error($link));
	}
	if (!mysqli_query($link, 'TRUNCATE TABLE innodb_memcache.containers')) {
		printf("[%d] %s\n",  mysqli_errno($link), mysqli_error($link));
		exit(1);
	}

	if (!mysqli_query($link, 'INSERT INTO innodb_memcache.containers (name, db_schema, db_table, key_columns, value_columns, flags, cas_column, expire_time_column, unique_idx_name_on_key) '.
		"     VALUES ('mymem_test', '$db', 'mymem_test', 'id', '$value_columns', 'flags', 'cas_column', 'expire_time_column', 'PRIMARY')")) {
		printf("[%d] %s\n",  mysqli_errno($link), mysqli_error($link));
		exit(1);
	}
	if (!mysqli_query($link, 'TRUNCATE TABLE innodb_memcache.config_options')) {
		printf("[%d] %s\n",  mysqli_errno($link), mysqli_error($link));
		exit(1);
	}
	if (!mysqli_query($link, "INSERT INTO innodb_memcache.config_options (name, value) VALUES ('separator', '$separator')")) {
		printf("[%d] %s\n",  mysqli_errno($link), mysqli_error($link));
		exit(1);
	}

	if (!mysqli_query($link, "INSTALL PLUGIN  daemon_memcached SONAME 'libmemcached.so'")) {
		printf("[%d] %s\n",  mysqli_errno($link), mysqli_error($link));
		exit(1);
	}
}

