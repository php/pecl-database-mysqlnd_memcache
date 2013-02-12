--TEST--
Extension interface
--SKIPIF--
<?php
require('skipif.inc');
?>
--INI--
mysqlnd_memcache.enable=1
--FILE--
<?php
	$r = new ReflectionExtension("mysqlnd_memcache");

	printf("Name: %s\n", $r->name);

	/* TODO: version constants MYSQLND_MEMCACHE_VERSION, ... */
	printf("Version: %s\n", $r->getVersion());

	$classes = $r->getClasses();
	if (!empty($classes)) {
		printf("[001] Expecting no class\n");
		asort($classes);
		var_dump($classes);
	}

	$expected = array(
		'memcached'	=> true,
		'pcre' 	=> true,
		'mysqlnd' 	=> true,
	);


	$dependencies = $r->getDependencies();
	asort($dependencies);
	printf("Dependencies:\n");
	foreach ($dependencies as $what => $how) {
		printf("  %s - %s\n", $what, $how);
		if (isset($expected[$what])) {
			unset($expected[$what]);
		} else {
			printf("Unexpected extension dependency with %s - %s\n", $what, $how);
		}
	}
	if (!empty($expected)) {
		printf("Dumping list of missing extension dependencies\n");
		var_dump($expected);
	}
	printf("\n");

	$ignore = array();

	$functions = $r->getFunctions();
	asort($functions);
	printf("Functions:\n");
	foreach ($functions as $func) {
		if (isset($ignore[$func->name])) {
			unset($ignore[$func->name]);
		} else {
			printf("  %s\n", $func->name);
		}
	}
	if (!empty($ignore)) {
		printf("Dumping version dependent and missing functions\n");
		var_dump($ignore);
	}

	print "done!";
?>
--EXPECTF--
Name: mysqlnd_memcache
Version: %s
Dependencies:
  memcached - Required
  pcre - Required
  mysqlnd - Required

Functions:
  mysqlnd_memcache_get_config
  mysqlnd_memcache_set
done!
