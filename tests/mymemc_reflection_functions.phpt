--TEST--
ReflectionFunction to check API
--SKIPIF--
<?php
require('skipif.inc');
?>
--INI--
mysqlnd_memcache.enable=1
--FILE--
<?php
	$r = new ReflectionExtension("mysqlnd_memcache");

	$ignore = array();

	$functions = $r->getFunctions();
	asort($functions);
	printf("Functions:\n");
	foreach ($functions as $func) {
		if (isset($ignore[$func->name]))
			continue;

		printf("  %s\n", $func->name);
		$rf = new ReflectionFunction($func->name);
		printf("    Deprecated: %s\n", $rf->isDeprecated() ? "yes" : "no");
		printf("    Accepted parameters: %d\n", $rf->getNumberOfParameters());
		printf("    Required parameters: %d\n", $rf->getNumberOfRequiredParameters());
		foreach( $rf->getParameters() as $param ) {
			printf("      %s\n", $param);
		}
	}

	print "done!";
?>
--EXPECT--
Functions:
  mysqlnd_memcache_get_config
    Deprecated: no
    Accepted parameters: 1
    Required parameters: 1
      Parameter #0 [ <required> $mysql_connection ]
  mysqlnd_memcache_set
    Deprecated: no
    Accepted parameters: 4
    Required parameters: 2
      Parameter #0 [ <required> $mysql_connection ]
      Parameter #1 [ <required> Memcached or NULL $memcached_connection ]
      Parameter #2 [ <optional> $select_pattern ]
      Parameter #3 [ <optional> $debug_callback ]
done!