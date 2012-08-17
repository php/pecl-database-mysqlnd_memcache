<?php
	function my_memcache_compare_meta($offset, $fields1, $fields2) {
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

				printf("[%03d] Field values for field %d differ\n", $offset, $k);
				echo $msg;
			}
		}

		if (!empty($fields2))  {
			printf("[%03d] Fields missing\n", $offset);
			var_dump($fields2);
		}
	}