--TEST--
phpinfo() section
--SKIPIF--
<?php
require_once('skipif.inc');
?>
--INI--
mysqlnd_memcache.enable=1
--FILE--
<?php
	ob_start();
	phpinfo(INFO_MODULES);
	$tmp = ob_get_contents();
	ob_end_clean();

	if (!stristr($tmp, 'mysqlnd_memcache support'))
		printf("[001] mysqlnd_memcache section seems to be missing. Check manually\n");

	if (!stristr($tmp, 'php-memcached version'))
		printf("[002] php-memcached version seems to be missing. Check manually\n");

	if (!stristr($tmp, 'libmemcached version'))
		printf("[003] libmemcached version version seems to be missing. Check manually\n");

	print "done!";
?>
--EXPECTF--
done!