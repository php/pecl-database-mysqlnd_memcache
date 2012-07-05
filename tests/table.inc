<?php
require_once('connect.inc');

class my_memcache_config {
	protected $innodb_schema = 'innodb_memcache';
	protected static $my_memcache_config = NULL;
	protected $link = NULL;


	public static function init($value_columns, $id_is_string = false, $separator = '|') {
		if (is_null(self::$my_memcache_config)) {
			self::$my_memcache_config = new my_memcache_config();
		}

		if (!($ret = self::$my_memcache_config->create_test_table($id_is_string)))
			return $ret;

		if (!($res = self::$my_memcache_config->update_container($value_columns, $separator)))
			return $ret;

		return true;
	}

	public function connect($link = NULL) {
		global $host, $user, $passwd, $db, $port, $socket;

		if (!$link && !$this->link) {
			$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
			if (mysqli_connect_errno()) {
				return sprintf("[%d] %s\n", mysqli_connect_errno(), mysqli_connect_error());
			}
			$this->link = $link;
		}

		return true;
	}

	public function create_test_table($id_is_string = false) {

		$this->connect();
		if (!$this->link->query("DROP TABLE IF EXISTS mymem_test")) {
			printf("[%d] %s\n", $this->link->errno, $this->link->error);
			return false;
		}

		$sql = sprintf("CREATE TABLE mymem_test(
			id %s,
			f1 VARCHAR(255),
			f2 VARCHAR(255),
			f3 VARCHAR(255),
			flags INT NOT NULL,
			cas_column INT,
			expire_time_column INT,
			PRIMARY KEY(id)) ENGINE=InnoDB",
			($id_is_string) ? "CHAR(4)" : "INT");

		if (!$this->link->query($sql)) {
			return sprintf("(%s) [%d] %s\n", $sql, $this->link->errno, $this->link->error);
		}

		$sql = sprintf("INSERT INTO mymem_test(id, f1, f2, f3) VALUES
				('%s1', 'a', 'b', 'c'),
				('%s2', 'foo', 'bar', 'baz'),
				('%s3', 'der Hund', 'liegt in der Kueche', 'und bellt')",
			($id_is_string) ? "key" : "",
			($id_is_string) ? "key" : "",
			($id_is_string) ? "key" : "");

		if (!$this->link->query($sql)) {
			return sprintf("(%s) [%d] %s\n", $sql, $this->link->errno, $this->link->error);
		}

		return true;
	}

	public function update_container($value_columns, $separator = '|') {
		global $db;

		$this->connect();

		$plugin_loaded = false;
		if (!($res = $this->link->query("SHOW PLUGINS"))) {
			return sprintf("SHOW PLUGINS failed, [%d] %s\n", $this->link->errno, $this->link->error);
		}
		while ($row = $res->fetch_assoc()) {
			if (($row['Name'] == 'daemon_memcached') && ($row['Status'] == 'ACTIVE')) {
				$plugin_loaded = true;
				break;
			}
		}

		if (!$plugin_loaded) {
			/* Plugin needs to be installed, we don't need to check config */
			$update_needed = $this->container_needs_update($value_columns, $separator);
			if (is_string($update_needed)) {
				/* error */
				return $update_needed;
			} else if (false === $update_needed) {
				/* Yeah, we don't need to update the config and reload the plugin */
				return true;
			}
		}

		/* Bad luck... */
		$sql = sprintf("REPLACE INTO %s.containers(
				name, db_schema, db_table, key_columns,
				value_columns, flags, cas_column,
				expire_time_column, unique_idx_name_on_key
			) VALUES (
				'mymem_test', '%s', 'mymem_test', 'id',
				'%s', 'flags', 'cas_column',
				'expire_time_column', 'PRIMARY'
			)",
			$this->link->real_escape_string($this->innodb_schema),
			$this->link->real_escape_string($db),
			$this->link->real_escape_string(implode(',', $value_columns)));

		if (!$this->link->query($sql)) {
			return sprintf("%s, [%d] %s\n", $sql, $this->link->errno, $this->link->error);
		}

		$sql = sprintf("REPLACE INTO %s.config_options(name, value) VALUES ('separator', '%s')",
			$this->link->real_escape_string($this->innodb_schema),
			$this->link->real_escape_string($separator));


		if (!$this->link->query($sql)) {
			return sprintf("%s, [%d] %s\n", $sql, $this->link->errno, $this->link->error);
		}

		/* As of today, there seems no other way to notify the plugin of reloading its config */
		if (!$this->link->query("UNINSTALL PLUGIN daemon_memcached")) {
			return sprintf("UNINSTALL PLUGIN failed, [%d] %s\n", $this->link->errno, $this->link->error);
		}

		if (!$this->link->query("INSTALL PLUGIN  daemon_memcached SONAME 'libmemcached.so'")) {
			return sprintf("INSTALL PLUGIN failed, [%d] %s\n", $this->link->errno, $this->link->error);
		}

		return true;
	}

	public function container_needs_update($value_columns, $separator = '|') {
		global $db;

		$this->connect();

		$update_needed = true;

		/* Are we lucky and columns are already configured...? */
		$sql = sprintf("SELECT value_columns FROM %s.containers WHERE
			name = 'mymem_test' AND
			db_schema = '%s' AND
			db_table = 'mymem_test' AND
			key_columns = 'id' AND
			flags = 'flags' AND
			cas_column = 'cas_column' AND
			expire_time_column = 'expire_time_column' AND
			unique_idx_name_on_key = 'PRIMARY'",
			$this->link->real_escape_string($this->innodb_schema),
			$this->link->real_escape_string($db));

		if ($res = $this->link->query($sql)) {
			$row = $res->fetch_assoc();
			$columns = explode("'", $row['value_columns']);
			foreach ($columns as $k => $column) {
				$column = trim($column);
				foreach ($value_columns as $vk => $vcolumn) {
					$vcolumn = trim($vcolumn);
					if ($vcolumn == $column) {
						unset($columns[$k]);
						unset($value_columns[$vk]);
					}
				}
			}
			$update_needed = (empty($columns) && empty($value_columns)) ? false : true;
		} else if ($this->link->errno) {
			return sprintf("%s, [%d] %s\n", $sql, $this->link->errno, $this->link->error);
		}

		if (!$update_needed) {
			/* Are we lucky and separator is already configured...? */
			$sql = sprintf("SELECT value FROM %s.config_options WHERE name = 'separator'",
				$this->link->real_escape_string($this->innodb_schema));

			if ($res = $this->link->query($sql)) {
				$row = $res->fetch_assoc();
				$update_needed = ($row['value'] == $separator) ? false : true;
			} else if ($this->link->errno) {
				return sprintf("%s, [%d] %s\n", $sql, $this->link->errno, $this->link->error);
			}
		}

		return $update_needed;
	}

}

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

