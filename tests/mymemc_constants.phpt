--TEST--
Constants
--SKIPIF--
<?php
require_once('skipif.inc');
?>
--INI--
mysqlnd_memcache.enable=1
--FILE--
<?php
	$expected = array(
		"MYSQLND_MEMCACHE_DEFAULT_REGEXP" => true,
		"MYSQLND_MEMCACHE_VERSION" => true,
		"MYSQLND_MEMCACHE_VERSION_ID" => true,
	);


	$constants = get_defined_constants(true);
	$constants = (isset($constants['mysqlnd_memcache'])) ? $constants['mysqlnd_memcache'] : array();
	ksort($constants);
	foreach ($constants as $name => $value) {
		if (!isset($expected[$name])) {
			printf("[001] Unexpected constants: %s/%s\n", $name, $value);
		} else {
			if ($expected[$name])
				printf("%s = '%s'\n", $name, $value);
			unset($expected[$name]);
		}
	}
	if (!empty($expected)) {
		printf("[002] Dumping list of missing constants\n");
		var_dump($expected);
	}

	print "done!";
?>
--EXPECTF--
MYSQLND_MEMCACHE_DEFAULT_REGEXP = '/^\s*SELECT\s*(.+?)\s*FROM\s*`?([a-z0-9_]+)`?\s*WHERE\s*`?([a-z0-9_]+)`?\s*=\s*(?(?=["'])["']([^"']*)["']|([0-9e\.]*))\s*$/is'
MYSQLND_MEMCACHE_VERSION = '%s'
MYSQLND_MEMCACHE_VERSION_ID = '%d'
done!
