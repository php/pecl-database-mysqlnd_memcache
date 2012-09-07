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

		if (is_string($ret = self::$my_memcache_config->create_test_table($id_is_string)))
			return $ret;

		if (is_string($res = self::$my_memcache_config->update_container($value_columns, $separator)))
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

	public function create_test_table($id_is_string = false, $max_retry = 1) {
		if ($ret = $this->_create_test_table("mymem_test", $id_is_string, $max_retry)) {
			$ret = $this->_create_test_table("mymem_test_meta", $id_is_string, $max_retry);
		}
		return $ret;
	}

	protected function _create_test_table($table_name, $id_is_string = false, $max_retry = 1) {

		$this->connect();
		$sql = sprintf("DROP TABLE IF EXISTS %s", $this->link->real_escape_string($table_name));
		if (!$this->link->query($sql)) {
			printf("[%d] %s\n", $this->link->errno, $this->link->error);
			return false;
		}

		$sql = sprintf("CREATE TABLE %s(
			id %s,
			f1 VARCHAR(255),
			f2 VARCHAR(255),
			f3 VARCHAR(255),
			flags INT NOT NULL,
			cas_column INT,
			expire_time_column INT,
			PRIMARY KEY(id)) ENGINE=InnoDB",
			$this->link->real_escape_string($table_name),
			($id_is_string) ? "CHAR(255)" : "INT");

		if (!$this->link->query($sql)) {

			if (($max_retry > 0) && (1050 == $this->link->errno)) {
				/*
					In the early days InnoDB did a background drop and it required
					a plugin reload to get re-create a table
				*/
				if (is_string($ret = $this->reload_plugin()))
					return $ret;

				$max_retry--;
				return $this->_create_test_table($table_name, $id_is_string, $max_retry);
			}
			return sprintf("(%s) [%d] %s\n", $sql, $this->link->errno, $this->link->error);
		}

		if ($id_is_string) {
			$sql = sprintf("INSERT INTO %s(id, f1, f2, f3) VALUES
					('key1', 'a', 'b', 'c'),
					('key2', 'foo', 'bar', 'baz'),
					('key3', 'der Hund', 'liegt in der Kueche', 'und bellt')",
				$this->link->real_escape_string($table_name));
		} else {
			$sql = sprintf("INSERT INTO %s(id, f1, f2, f3) VALUES
					(1, 'a', 'b', 'c'),
					(2, 'foo', 'bar', 'baz'),
					(3, 'der Hund', 'liegt in der Kueche', 'und bellt')",
				$this->link->real_escape_string($table_name));
		}

		if (!$this->link->query($sql)) {
			return sprintf("(%s) [%d] %s\n", $sql, $this->link->errno, $this->link->error);
		}

		return true;
	}

	public function update_container($value_columns, $separator = '|', $table_map_delimiter = '.') {
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

		if ($plugin_loaded) {
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

		/* early plugin version */
		$sql = sprintf("REPLACE INTO %s.config_options(name, value) VALUES ('separator', '%s')",
			$this->link->real_escape_string($this->innodb_schema),
			$this->link->real_escape_string($separator));

		if (!$this->link->query($sql)) {
			return sprintf("%s, [%d] %s\n", $sql, $this->link->errno, $this->link->error);
		}

		/* later plugin version  */
		$sql = sprintf("REPLACE INTO %s.config_options(name, value) VALUES ('table_map_delimiter', '%s')",
			$this->link->real_escape_string($this->innodb_schema),
			$this->link->real_escape_string($table_map_delimiter));

		if (!$this->link->query($sql)) {
			return sprintf("%s, [%d] %s\n", $sql, $this->link->errno, $this->link->error);
		}

		return $this->reload_plugin();
	}


	public function reload_plugin() {
		/* As of today, there seems no other way to notify the plugin of reloading its config */
		$this->link->query("UNINSTALL PLUGIN daemon_memcached");
		sleep(1);

		if (!$this->link->query("INSTALL PLUGIN  daemon_memcached SONAME 'libmemcached.so'")) {
			return sprintf("INSTALL PLUGIN failed, [%d] %s\n", $this->link->errno, $this->link->error);
		}

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

		if (!$plugin_loaded)
			return sprintf("Failed to install pluginn");

		return $plugin_loaded;
	}

	public function container_needs_update($value_columns, $separator = '|', $table_map_delimiter = '.') {
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
			$columns = explode(",", $row['value_columns']);
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


			$sql = sprintf("SELECT value FROM %s.config_options WHERE name = 'table_map_delimiter'",
				$this->link->real_escape_string($this->innodb_schema));

			if ($res = $this->link->query($sql)) {
				$row = $res->fetch_assoc();
				$update_needed = ($row['value'] == $table_map_delimiter) ? false : true;
			} else if ($this->link->errno) {
				return sprintf("%s, [%d] %s\n", $sql, $this->link->errno, $this->link->error);
			}

		}

		return $update_needed;
	}

}
