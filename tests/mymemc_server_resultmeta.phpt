--TEST--
Resultset meta from the server
--SKIPIF--
<?php
	require('skipif.inc');
	_skipif_check_extensions(array("mysqli"));
	_skipif_connect($host, $user, $passwd, $db, $port, $socket);

	require_once('table.inc');
	$ret = my_memcache_config::init(array('f1', 'f2', 'f3'), true, '|');
	if (true !== $ret) {
		die(sprintf("SKIP %s\n", $ret));
	}
?>
--FILE--
<?php
	require_once('connect.inc');

	function compare_meta($fields1, $fields2) {
		foreach ($fields1 as $k => $field1) {

			if (isset($fields2[$k])) {
				$field2 = $fields2[$k];

				$msg = '';
				if ($field1->name != $field2->name) {
					$msg.= sprintf("  name: '%s' != '%s'\n", $field1->name, $field2->name);
				}
				if ($field1->orgname != $field2->orgname) {
					$msg.= sprintf("  orgname: '%s' != '%s'\n", $field1->orgname, $field2->orgname);
				}
				if ($field1->table != $field2->table) {
					$msg.= sprintf("  table: '%s' != '%s'\n", $field1->table, $field2->table);
				}
				if ($field1->orgtable != $field2->orgtable) {
					$msg.= sprintf("  orgtable: '%s' != '%s'\n", $field1->orgtable, $field2->orgtable);
				}
				if ($field1->def != $field2->def) {
					$msg.= sprintf("  def: '%s' != '%s'\n", $field1->def, $field2->def);
				}
				if ($field1->db != $field2->db) {
					$msg.= sprintf("  db: '%s' != '%s'\n", $field1->db, $field2->db);
				}
				if ($field1->catalog != $field2->catalog) {
					$msg.= sprintf("  name: '%s' != '%s'\n", $field1->name, $field2->name);
				}
				if ($field1->max_length != $field2->max_length) {
					$msg.= sprintf("  max_length: '%s' != '%s'\n", $field1->max_length, $field2->max_length);
				}
				if ($field1->length != $field2->length) {
					$msg.= sprintf("  length: '%s' != '%s'\n", $field1->length, $field2->length);
				}
				if ($field1->charsetnr != $field2->charsetnr) {
					$msg.= sprintf("  charsetnr: '%s' != '%s'\n", $field1->charsetnr, $field2->charsetnr);
				}
				if ($field1->flags != $field2->flags) {
					$msg.= sprintf("  flags: '%s' != '%s'\n", $field1->flags, $field2->flags);
				}
				if ($field1->type != $field2->type) {
					$msg.= sprintf("  type: '%s' != '%s'\n", $field1->type, $field2->type);
				}
				if ($field1->decimals != $field2->decimals) {
					$msg.= sprintf("  decimals: '%s' != '%s'\n", $field1->decimals, $field2->decimals);
				}

				unset($fields2[$k]);

				if (!$msg)
					continue;

				printf("Field %d\n", $k);
				echo $msg;
			}
		}

		if (!empty($fields2))  {
			var_dump($fields);
		}
	}



	$link = my_mysqli_connect($host, $user, $passwd, $db, $port, $socket);
	if ($link->connect_errno) {
		printf("[001] [%d] %s\n", $link->connect_errno, $link->connect_error);
	}

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test_meta WHERE id = 'key1'")) {
		$fields = $res->fetch_fields();
		$res->free();
	} else {
		printf("[004] [%d] %s\n", $link->errno, $link->error);
	}

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test_meta WHERE id = 'key1'")) {
		$fields_meta = $res->fetch_fields();
	} else {
		printf("[005] [%d] %s\n", $link->errno, $link->error);
	}

	if ($res = $link->query("SELECT f1, f2, f3 FROM mymem_test WHERE id = 'key1'")) {
		$fields_memc = $res->fetch_fields();
	} else {
		printf("[006] [%d] %s\n", $link->errno, $link->error);
	}

	printf("Shadow table checking\n");
	compare_meta($fields_meta, $fields);
	printf("Meta of Memcache mapped and shadow table\n");
	compare_meta($fields_meta, $fields_memc);

	print "done!";
?>
--EXPECT--
Shadow table checking
Meta of Memcache mapped and shadow table
Field 0
  table: 'mymem_test_meta' != 'mymem_test'
  orgtable: 'mymem_test_meta' != 'mymem_test'
Field 1
  table: 'mymem_test_meta' != 'mymem_test'
  orgtable: 'mymem_test_meta' != 'mymem_test'
Field 2
  table: 'mymem_test_meta' != 'mymem_test'
  orgtable: 'mymem_test_meta' != 'mymem_test'
done!