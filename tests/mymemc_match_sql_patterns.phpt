--TEST--
MySQL Query pattern matching
--SKIPIF--
<?php
require('skipif.inc');
?>
--INI--
mysqlnd_memcache.enable=1
--FILE--
<?php
$sqls = [
    "SELECT column FROM table WHERE key = 1",
    "SELECT column FROM table WHERE key = 'v'",
    'SELECT column FROM table WHERE key = "v"',
    "SELECT column1, column2, column3 FROM table WHERE id = 1",
    "SELECT column1, column2, column3 FROM table WHERE id = 1 AND foo = 2",
];

printf("Default regexp: %s\n", MYSQLND_MEMCACHE_DEFAULT_REGEXP);

foreach ($sqls as $sql) {
	if (preg_match(MYSQLND_MEMCACHE_DEFAULT_REGEXP, $sql, $matches)) {
		printf("'%s' - match\n", $sql);
		foreach ($matches as $k => $v) {
			printf(" %02d: '%s'\n", $k, $v);
		}
	} else {
		printf("'%s' - no match\n", $sql);
	}
}

print "done!";
?>
--EXPECT--
Default regexp: /^\s*SELECT\s*(.+?)\s*FROM\s*`?([a-z0-9_]+)`?\s*WHERE\s*`?([a-z0-9_]+)`?\s*=\s*(?(?=["'])["']([^"']*)["']|([0-9e\.]*))\s*$/is
'SELECT column FROM table WHERE key = 1' - match
 00: 'SELECT column FROM table WHERE key = 1'
 01: 'column'
 02: 'table'
 03: 'key'
 04: ''
 05: '1'
'SELECT column FROM table WHERE key = 'v'' - match
 00: 'SELECT column FROM table WHERE key = 'v''
 01: 'column'
 02: 'table'
 03: 'key'
 04: 'v'
'SELECT column FROM table WHERE key = "v"' - match
 00: 'SELECT column FROM table WHERE key = "v"'
 01: 'column'
 02: 'table'
 03: 'key'
 04: 'v'
'SELECT column1, column2, column3 FROM table WHERE id = 1' - match
 00: 'SELECT column1, column2, column3 FROM table WHERE id = 1'
 01: 'column1, column2, column3'
 02: 'table'
 03: 'id'
 04: ''
 05: '1'
'SELECT column1, column2, column3 FROM table WHERE id = 1 AND foo = 2' - no match
done!