/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2012 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_versioning.h"
#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqlnd/mysqlnd_result.h"
#include "ext/mysqlnd/mysqlnd_ext_plugin.h"
#include "ext/mysqlnd/mysqlnd_priv.h"
#include "ext/mysqlnd/mysqlnd_alloc.h"
#include "ext/mysqlnd/mysqlnd_reverse_api.h"
#include "Zend/zend_interfaces.h"
#include "php_mysqlnd_memcache.h"
#include "ext/pcre/php_pcre.h"
#include "libmemcached/memcached.h"

/* If you declare any globals in php_mysqlnd_memcache.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(mysqlnd_memcache)
 */

#define MYSQLND_MEMCACHE_VERSION "0.1.0"

static unsigned int mysqlnd_memcache_plugin_id;

static func_mysqlnd_conn_data__query orig_mysqlnd_conn_query;
static func_mysqlnd_conn_data__dtor orig_mysqlnd_conn_dtor;

#define SQL_IDENTIFIER "`?([a-z0-9_]+)`?"
#define SQL_PATTERN "/^\\s*SELECT\\s*(\\S+)\\s*FROM\\s*" SQL_IDENTIFIER "\\s*WHERE\\s*" SQL_IDENTIFIER "\\s*=\\s*[\"']?([0-9]+)[\"']?\\s*$/im"
#define SQL_PATTERN_LEN (sizeof(SQL_PATTERN)-1)

#define MAPPING_QUERY "    SELECT c.db_schema, c.db_table, c.key_columns, c.value_columns, o.value sep" \
                      "      FROM innodb_memcache.containers c" \
                      " LEFT JOIN innodb_memcache.config_options o " \
		      "        ON o.name = 'separator'"

typedef struct {
	struct {
		zval *zv;
		memcached_st *memc;
	} connection;
	struct {
		char *str; /* TODO: Actually the compiled and cached pattern is enough, but let's keep str, len and str_is_allocated for now to ease debugging */
		int  len;
		pcre_cache_entry* pattern;
		int  str_is_allocated;
	} regexp;
	struct {
		char *schema_name;
		char *table_name;
		char *id_field_name;
		struct {
			int num;
			char *to_free;
			char **v;
		} value_columns;
		char *separator;
	} mapping;
	struct {
		zend_fcall_info fci;
		zend_fcall_info_cache fcc;
		zend_bool exists;
	} callback;
} mysqlnd_memcache_connection_data_data;

typedef struct {
	char *data;
	size_t data_len;
	int read;
	MYSQLND_FIELD *fields;
	unsigned long *lengths;
} mysqlnd_memcache_result_data;

/*
 * I'd prefer having those exported from php-memcached.
 */
typedef struct {
	zend_object zo;
	struct {
		memcached_st *memc;
	} *obj;
} fake_memcached_object;

static zend_class_entry **memcached_ce = NULL;


static int count_char(char *pos, char v) /* {{{ */
{
	int retval = 0;

	while (*pos) {
		if (*pos == v) {
			retval++;
		}
		pos++;
	}

	return retval;
}
/* }}} */

#define BAILOUT_IF_CONN_DATA_UNSET(connection_data) \
	if (UNEXPECTED(!connection_data)) { \
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Connection data was unset but result set using it still exists"); \
	}

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("mysqlnd_memcache.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_mysqlnd_memcache_globals, mysqlnd_memcache_globals)
    STD_PHP_INI_ENTRY("mysqlnd_memcache.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_mysqlnd_memcache_globals, mysqlnd_memcache_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_mysqlnd_memcache_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_mysqlnd_memcache_init_globals(zend_mysqlnd_memcache_globals *mysqlnd_memcache_globals)
{
	mysqlnd_memcache_globals->global_value = 0;
	mysqlnd_memcache_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ MYSQLND_MEMCACHE_RESULT */
static enum_func_status mysqlnd_memcache_result_fetch_row(MYSQLND_RES *result, void *param, unsigned int flags, zend_bool *fetched_anything TSRMLS_DC) /* {{{ */
{
	zval *row = (zval *) param;
	zval *data;
	mysqlnd_memcache_connection_data_data *connection_data = *mysqlnd_plugin_get_plugin_connection_data_data(result->conn, mysqlnd_memcache_plugin_id);
	mysqlnd_memcache_result_data *result_data = *(mysqlnd_memcache_result_data **)mysqlnd_plugin_get_plugin_result_data(result, mysqlnd_memcache_plugin_id);

	BAILOUT_IF_CONN_DATA_UNSET(connection_data)

	if (result_data->read) {
		return FAIL;
	}
	
	result_data->read = 1;
	
	if (!param) {
		return FAIL;
	}
	
	pcre_cache_entry *pattern = pcre_get_compiled_regex_cache("/\\W*,\\W*/", sizeof("/\\W*,\\W*/")-1 TSRMLS_CC);
	php_pcre_split_impl(pattern, result_data->data, result_data->data_len, row, 0, 0 TSRMLS_CC);
/*
	ALLOC_INIT(data);
	ZVAL_STRINGL(data, result_data->data, result_data->data_len, 1);
	if (flags & MYSQLND_FETCH_NUM) {
		Z_ADDREF_P(data);
		zend_hash_next_index_insert(Z_ARRVAL_P(row), &data, sizeof(zval *), NULL);
	}
	if (flags & MYSQLND_FETCH_ASSOC) {
		Z_ADDREF_P(data);
		zend_hash_add(Z_ARRVAL_P(row), connection_data->mapping.id_field_name, strlen(connection_data->mapping.id_field_name)+1, &data, sizeof(zval *), NULL);
	}
*/	
	*fetched_anything = TRUE;
	
	return PASS;
}
/* }}} */

static MYSQLND_RES *mysqlnd_memcache_result_use_result(MYSQLND_RES *const result, zend_bool ps_protocol TSRMLS_DC) /* {{{ */
{
	return result;
}
/* }}} */

static MYSQLND_RES *mysqlnd_memcache_result_store_result(MYSQLND_RES *result, MYSQLND_CONN_DATA *conn, zend_bool ps_protocol TSRMLS_DC) /* {{{ */
{
	return result;
}
/* }}} */

static void mysqlnd_memcache_result_fetch_into(MYSQLND_RES *result, unsigned int flags, zval *return_value, enum_mysqlnd_extension ext TSRMLS_DC ZEND_FILE_LINE_DC) /* {{{ */
{
	mysqlnd_memcache_connection_data_data *connection_data = *mysqlnd_plugin_get_plugin_connection_data_data(result->conn, mysqlnd_memcache_plugin_id);
	mysqlnd_memcache_result_data *result_data = *(mysqlnd_memcache_result_data **)mysqlnd_plugin_get_plugin_result_data(result, mysqlnd_memcache_plugin_id);

	int i = 0;
	zval *data;
	char *value, *value_lasts;

	BAILOUT_IF_CONN_DATA_UNSET(connection_data)

	if (result_data->read || !result_data->data) {
		return;
	}
	
	result_data->read = 1;
	
	value = strtok_r(result_data->data, connection_data->mapping.separator, &value_lasts);

	array_init(return_value);
	while (value) {
		ALLOC_INIT_ZVAL(data);
		ZVAL_STRING(data, value, 1);
		
		if (flags & MYSQLND_FETCH_NUM) {
			Z_ADDREF_P(data);
			zend_hash_next_index_insert(Z_ARRVAL_P(return_value), &data, sizeof(zval *), NULL);
		}
		if (flags & MYSQLND_FETCH_ASSOC) {
			Z_ADDREF_P(data);
			zend_hash_add(Z_ARRVAL_P(return_value), connection_data->mapping.value_columns.v[i], strlen(connection_data->mapping.value_columns.v[i])+1, &data, sizeof(zval *), NULL);
		}
		zval_ptr_dtor(&data);
		
		value = strtok_r(NULL, connection_data->mapping.separator, &value_lasts);
		i++;
	}

	CONN_SET_STATE(result->conn, CONN_READY);
}
/* }}} */

MYSQLND_ROW_C mysqlnd_memcache_result_fetch_row_c(MYSQLND_RES *result TSRMLS_DC) /* {{{ */
{
	mysqlnd_memcache_connection_data_data *connection_data = *mysqlnd_plugin_get_plugin_connection_data_data(result->conn, mysqlnd_memcache_plugin_id);
	mysqlnd_memcache_result_data *result_data = *(mysqlnd_memcache_result_data **)mysqlnd_plugin_get_plugin_result_data(result, mysqlnd_memcache_plugin_id);

	int field_count;
	char **retval;

	BAILOUT_IF_CONN_DATA_UNSET(connection_data)

	field_count = connection_data->mapping.value_columns.num;

	if (result_data->read || !result_data->data) {
		return NULL;
	}
	
	result_data->read = 1;
	retval = mnd_emalloc(field_count * sizeof(char*));

	int i = 0;
	char *value, *value_lasts;
	
	value = strtok_r(result_data->data, connection_data->mapping.separator, &value_lasts);

	while (value) {
		/* TODO - This can be optimized, data handling in general should be made binary safe ..*/
		retval[i] = strdup(value);
		result_data->lengths[i++] = strlen(value);
		value = strtok_r(NULL, connection_data->mapping.separator, &value_lasts);
	}

	CONN_SET_STATE(result->conn, CONN_READY);

	return retval;
}
/* }}} */

static void mysqlnd_memcache_result_fetch_all(MYSQLND_RES *result, unsigned int flags, zval *return_value TSRMLS_DC ZEND_FILE_LINE_DC) /* {{{ */
{
	zval *row;
	mysqlnd_memcache_result_data *result_data_p = *(mysqlnd_memcache_result_data **)mysqlnd_plugin_get_plugin_result_data(result, mysqlnd_memcache_plugin_id);
		
	array_init(return_value);
	if (result_data_p->data) {
		ALLOC_INIT_ZVAL(row);
		mysqlnd_memcache_result_fetch_into(result, flags, row, MYSQLND_MYSQLI TSRMLS_CC ZEND_FILE_LINE_CC);
		zend_hash_next_index_insert(Z_ARRVAL_P(return_value), &row, sizeof(zval *), NULL);
	}
}
/* }}} */

static uint64_t mysqlnd_memcache_result_num_rows(const MYSQLND_RES * const result TSRMLS_DC) /* {{{ */
{
	mysqlnd_memcache_result_data *result_data_p = *(mysqlnd_memcache_result_data **)mysqlnd_plugin_get_plugin_result_data(result, mysqlnd_memcache_plugin_id);
	return result_data_p->data ? 1 : 0;
}
/* }}} */

static unsigned int mysqlnd_memcache_result_num_fields(const MYSQLND_RES * const result TSRMLS_DC) /* {{{ */
{
	mysqlnd_memcache_connection_data_data *connection_data = *mysqlnd_plugin_get_plugin_connection_data_data(result->conn, mysqlnd_memcache_plugin_id);
	BAILOUT_IF_CONN_DATA_UNSET(connection_data)
	return connection_data->mapping.value_columns.num;
}
/* }}} */

static MYSQLND_FIELD_OFFSET mysqlnd_memcache_result_field_tell(const MYSQLND_RES * const result TSRMLS_DC) /* {{{ */
{
	return 0;
}
/* }}} */

const MYSQLND_FIELD *mysqlnd_memcache_result_fetch_fields(MYSQLND_RES * const result TSRMLS_DC) /* {{{ */
{
	mysqlnd_memcache_result_data *result_data = *mysqlnd_plugin_get_plugin_result_data(result, mysqlnd_memcache_plugin_id);
	return result_data->fields;
}
/* }}} */

unsigned long *mysqlnd_memcache_result_fetch_lengths(MYSQLND_RES * const result TSRMLS_DC) /* {{{ */
{
	mysqlnd_memcache_result_data *result_data = *mysqlnd_plugin_get_plugin_result_data(result, mysqlnd_memcache_plugin_id);
	return result_data->lengths;
}
/* }}} */

static enum_func_status	mysqlnd_memcache_result_free_result(MYSQLND_RES * result, zend_bool implicit TSRMLS_DC) /* {{{ */
{
	mysqlnd_memcache_result_data *result_data = *mysqlnd_plugin_get_plugin_result_data(result, mysqlnd_memcache_plugin_id);
	free(result_data->data);
	efree(result_data->fields);
	efree(result_data->lengths);
	efree(result_data);
	mnd_efree(result);
	return PASS;
}
/* }}} */

static const struct st_mysqlnd_res_methods mysqlnd_memcache_query_result_funcs = {  /* {{{ */
	mysqlnd_memcache_result_fetch_row,    /* mysqlnd_fetch_row_func	fetch_row; */
	NULL, /* mysqlnd_fetch_row_func	fetch_row_normal_buffered; -- private */
	NULL, /* mysqlnd_fetch_row_func	fetch_row_normal_unbuffered; -- private */

	mysqlnd_memcache_result_use_result,   /* func_mysqlnd_res__use_result use_result; */
	mysqlnd_memcache_result_store_result, /* func_mysqlnd_res__store_result store_result; */
	mysqlnd_memcache_result_fetch_into,   /* func_mysqlnd_res__fetch_into fetch_into; */
	mysqlnd_memcache_result_fetch_row_c,  /* func_mysqlnd_res__fetch_row_c fetch_row_c; */
	mysqlnd_memcache_result_fetch_all,    /* func_mysqlnd_res__fetch_all fetch_all; */
	NULL, /* func_mysqlnd_res__fetch_field_data fetch_field_data; */
	mysqlnd_memcache_result_num_rows,     /* func_mysqlnd_res__num_rows num_rows; */
	mysqlnd_memcache_result_num_fields,   /* func_mysqlnd_res__num_fields num_fields; */
	NULL, /* func_mysqlnd_res__skip_result skip_result; */
	NULL, /* func_mysqlnd_res__seek_data seek_data; */
	NULL, /* func_mysqlnd_res__seek_field seek_field; */
	mysqlnd_memcache_result_field_tell, /* func_mysqlnd_res__field_tell field_tell; */
	NULL, /* func_mysqlnd_res__fetch_field fetch_field; */
	NULL, /* func_mysqlnd_res__fetch_field_direct fetch_field_direct; */
	mysqlnd_memcache_result_fetch_fields, /* func_mysqlnd_res__fetch_fields fetch_fields; */
	NULL, /* func_mysqlnd_res__read_result_metadata read_result_metadata; */
	mysqlnd_memcache_result_fetch_lengths, /* func_mysqlnd_res__fetch_lengths fetch_lengths; */
	NULL, /* func_mysqlnd_res__store_result_fetch_data store_result_fetch_data; */
	NULL, /* func_mysqlnd_res__initialize_result_set_rest initialize_result_set_rest; */
	NULL, /* func_mysqlnd_res__free_result_buffers free_result_buffers; */
	mysqlnd_memcache_result_free_result,  /* func_mysqlnd_res__free_result free_result; */
	NULL, /* func_mysqlnd_res__free_result_internal free_result_internal; */
	NULL, /* func_mysqlnd_res__free_result_contents free_result_contents; */
	NULL, /* func_mysqlnd_res__free_buffered_data free_buffered_data; */
	NULL, /* func_mysqlnd_res__unbuffered_free_last_data unbuffered_free_last_data; */

	/* for decoding - binary or text protocol */
	NULL, /* func_mysqlnd_res__row_decoder row_decoder; */

	NULL, /* func_mysqlnd_res__result_meta_init result_meta_init; */

	NULL, /* void * unused1; */
	NULL, /* void * unused2; */
	NULL, /* void * unused3; */
	NULL, /* void * unused4; */
	NULL, /* void * unused5; */
};
/* }}} */
/* }}} */

static zval** mysqlnd_memcache_verify_patterns(mysqlnd_memcache_connection_data_data *connection_data, char *query, unsigned int query_len, zval *subpats TSRMLS_DC) /* {{{ */
{
	zval return_value;
	zval **tmp;
	
	INIT_ZVAL(return_value); /* This will be a long or bool, no need for a zval_dtor */
	
	php_pcre_match_impl(connection_data->regexp.pattern, query, query_len, &return_value, subpats, 0, 0, 0, 0 TSRMLS_CC);
	
	if (!Z_LVAL(return_value)) {
		return NULL;
	}
	
	if (zend_hash_index_find(Z_ARRVAL_P(subpats), 1, (void**)&tmp) == FAILURE || Z_TYPE_PP(tmp) != IS_STRING) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Pattern matched but no field list passed or not a string");
		return NULL;
	}

/* TODO - check fieldnames
	if (memcmp(Z_STRVAL_PP(tmp), connection_data->mapping.value_columns.raw, Z_STRLEN_PP(tmp))) {
		return NULL;
	}
*/

	if (zend_hash_index_find(Z_ARRVAL_P(subpats), 2, (void**)&tmp) == FAILURE || Z_TYPE_PP(tmp) != IS_STRING) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Pattern matched but no table name passed or not a string");
		return NULL;
	}

	if (memcmp(Z_STRVAL_PP(tmp), connection_data->mapping.table_name, Z_STRLEN_PP(tmp))) {
		return NULL;
	}
		
	if (zend_hash_index_find(Z_ARRVAL_P(subpats), 3, (void**)&tmp) == FAILURE || Z_TYPE_PP(tmp) != IS_STRING) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Pattern matched but no id field name passed or not a string");
		return NULL;
	}
		
	if (memcmp(Z_STRVAL_PP(tmp), connection_data->mapping.id_field_name, Z_STRLEN_PP(tmp))) {
		return NULL;
	}
		
	if (zend_hash_index_find(Z_ARRVAL_P(subpats), 4, (void**)&tmp) == FAILURE || Z_TYPE_PP(tmp) != IS_STRING) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Pattern matched but no id value passed or not a string");
		return NULL;
	}
	
	return tmp;
}
/* }}} */

static void mysqlnd_memcache_fill_field_data(mysqlnd_memcache_connection_data_data *connection_data, mysqlnd_memcache_result_data *result_data) /* {{{ */
{
	int i;
	int field_count = connection_data->mapping.value_columns.num;
	result_data->fields = safe_emalloc(field_count, sizeof(MYSQLND_FIELD), 0);
	memset(result_data->fields, 0, field_count*sizeof(MYSQLND_FIELD));
	
	result_data->lengths = safe_emalloc(field_count, sizeof(unsigned long), 0);
	memset(result_data->lengths, 0, field_count*sizeof(unsigned long));
	
	for (i = 0; i < field_count; ++i) {
		result_data->fields[i].db = connection_data->mapping.schema_name;
		result_data->fields[i].db_length = strlen(connection_data->mapping.schema_name);
		result_data->fields[i].org_table = result_data->fields[i].table = connection_data->mapping.table_name;
		result_data->fields[i].org_table_length = result_data->fields[i].table_length = strlen(connection_data->mapping.table_name);
		result_data->fields[i].name = connection_data->mapping.value_columns.v[i];
		result_data->fields[i].name_length = strlen(connection_data->mapping.value_columns.v[i]);
		result_data->fields[i].catalog = "";
		result_data->fields[i].catalog_length = 0;
		result_data->fields[i].type = MYSQL_TYPE_STRING;
	}
}
/* }}} */

static void myslqnd_memcache_notify_decision(mysqlnd_memcache_connection_data_data *conn_data, zend_bool using_memcache TSRMLS_DC) /* {{{ */
{
	zval *retval = NULL, *arg;
	zval **args[1];
	ALLOC_INIT_ZVAL(arg);
	ZVAL_BOOL(arg, using_memcache);
	args[0] = &arg;

	conn_data->callback.fci.no_separation = 0;
	conn_data->callback.fci.retval_ptr_ptr = &retval;
	conn_data->callback.fci.param_count = 1;
	conn_data->callback.fci.params = args;

	if (zend_call_function(&conn_data->callback.fci, &conn_data->callback.fcc TSRMLS_CC) == SUCCESS && retval) {
		zval_ptr_dtor(&retval);
	}
	zval_ptr_dtor(&arg);
}
/* }}} */

static enum_func_status MYSQLND_METHOD(mysqlnd_memcache_conn, query)(MYSQLND_CONN_DATA *conn, const char *query, unsigned int query_len TSRMLS_DC) /* {{{ */
{
	zval subpats;
	zval **tmp = NULL;
	
	INIT_ZVAL(subpats);
	mysqlnd_memcache_connection_data_data *connection_data = *mysqlnd_plugin_get_plugin_connection_data_data(conn, mysqlnd_memcache_plugin_id);

	if (connection_data) {
		tmp = mysqlnd_memcache_verify_patterns(connection_data, (char*)query, query_len, &subpats TSRMLS_CC);

		if (UNEXPECTED(connection_data->callback.exists)) {
			myslqnd_memcache_notify_decision(connection_data, tmp ? TRUE : FALSE TSRMLS_CC);
		}
	}
	if (tmp) {
		void **result_data_vpp;
		mysqlnd_memcache_result_data *result_data_p;
		
		uint32_t flags;
		size_t value_len;
		memcached_return error;
		char* res = memcached_get(connection_data->connection.memc, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), &value_len, &flags, &error);
		
		zval_dtor(&subpats);
		
		if (conn->current_result) {
			conn->current_result->m.free_result(conn->current_result, TRUE TSRMLS_CC);
			mnd_efree(conn->current_result);
		}
		
		conn->current_result = mysqlnd_result_init(1, conn->persistent TSRMLS_CC);
		result_data_vpp = mysqlnd_plugin_get_plugin_result_data(conn->current_result, mysqlnd_memcache_plugin_id);
		*result_data_vpp = emalloc(sizeof(mysqlnd_memcache_result_data));
		result_data_p = *(mysqlnd_memcache_result_data **)result_data_vpp;
		
		result_data_p->data = res;
		result_data_p->data_len = value_len;
		result_data_p->read = 0;
		
		mysqlnd_memcache_fill_field_data(connection_data, result_data_p);
		
		conn->upsert_status->affected_rows = (uint64_t)-1;
		conn->upsert_status->warning_count = 0;
		conn->upsert_status->server_status = 0;
		
		conn->current_result->conn = conn;
		conn->current_result->field_count = 1;
		conn->current_result->m = mysqlnd_memcache_query_result_funcs;
		
		conn->last_query_type = QUERY_SELECT;
		CONN_SET_STATE(conn, CONN_FETCHING_DATA);
		
		return PASS;
	} else {
		zval_dtor(&subpats);
		return orig_mysqlnd_conn_query(conn, query, query_len TSRMLS_CC);
	}
}
/* }}} */

static void mysqlnd_memcache_free_connection_data_data(MYSQLND_CONN_DATA *conn TSRMLS_DC) /* {{{ */
{
	mysqlnd_memcache_connection_data_data **conn_data_p = (mysqlnd_memcache_connection_data_data **)mysqlnd_plugin_get_plugin_connection_data_data(conn, mysqlnd_memcache_plugin_id);
	mysqlnd_memcache_connection_data_data *conn_data = *conn_data_p;

	if (conn_data) {
		efree(conn_data->mapping.schema_name);
		efree(conn_data->mapping.table_name);
		efree(conn_data->mapping.id_field_name);
		efree(conn_data->mapping.value_columns.to_free);
		efree(conn_data->mapping.value_columns.v);
		efree(conn_data->mapping.separator);
		
		zval_ptr_dtor(&conn_data->connection.zv);

		if (conn_data->regexp.str_is_allocated) {
			efree(conn_data->regexp.str);
		}
		
		if (conn_data->callback.exists) {
			zval_ptr_dtor(&conn_data->callback.fci.function_name);
			conn_data->callback.fci.function_name = NULL;
			if (conn_data->callback.fci.object_ptr) {
				zval_ptr_dtor(&conn_data->callback.fci.object_ptr);
				conn_data->callback.fci.object_ptr = NULL;
			}
		}

		efree(conn_data);

		*conn_data_p = NULL;
	}
}

static void MYSQLND_METHOD(mysqlnd_memcache_conn, dtor)(MYSQLND_CONN_DATA *conn TSRMLS_DC) /* {{{ */
{
	mysqlnd_memcache_free_connection_data_data(conn TSRMLS_CC);
	orig_mysqlnd_conn_dtor(conn TSRMLS_CC);
}
/* }}} */

static void mysqlnd_memcache_split_columns(mysqlnd_memcache_connection_data_data *connection_data, char *names, int names_len) /* {{{ */
{
	int i = 0;
	char *pos_from = names, *pos_to;
	
	int count = count_char(names, ',') + 1;
	
	connection_data->mapping.value_columns.num = count;
	pos_to = connection_data->mapping.value_columns.to_free = emalloc(names_len + 1);
	connection_data->mapping.value_columns.v = safe_emalloc(count, sizeof(char*), 0);
	
	connection_data->mapping.value_columns.v[0] = connection_data->mapping.value_columns.to_free;
	while (*pos_from) {
		if (*pos_from == ',') {
			*pos_to = '\0';
			pos_to++;
			pos_from++;
			connection_data->mapping.value_columns.v[++i] = pos_to;
		}
		*pos_to = *pos_from;
		pos_from++;
		pos_to++;
	}
	*pos_to = '\0';
}
/* }}} */

static mysqlnd_memcache_connection_data_data *mysqlnd_memcache_init_mysqlnd(MYSQLND *conn TSRMLS_DC) /* {{{ */
{
	void **plugin_data_vpp;
	mysqlnd_memcache_connection_data_data *plugin_data_p;

	MYSQLND_ROW_C row;
	if (FAIL == orig_mysqlnd_conn_query(conn->data, MAPPING_QUERY, sizeof(MAPPING_QUERY) TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "MySQL query failed: %s", mysqlnd_error(conn));
		return NULL;
	}
	MYSQLND_RES *res = mysqlnd_store_result(conn);
	if (!res) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to store result");
		return NULL;
	}
	if (mysqlnd_num_rows(res) != 1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Got %l configuration rows, expected a single one. Either your configuration is invalid or you're using a unsupported innodb_memcached plugin", mysqlnd_num_rows(res));
		mysqlnd_free_result(res, 0);
		return NULL;
	}
	row = mysqlnd_fetch_row_c(res);
	if (!row || 5 != mysqlnd_num_fields(res)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Got an invalid result have_row=%i, num_fields=%i", row ? 1 : 0, mysqlnd_num_fields(res));
		if (row) {
		    mnd_free(row);
		}
		mysqlnd_free_result(res, 0);
		return NULL;
	}
		
	/*
	For highest performance we might cache this persistently, globally,
	this creates the risk of stuff going wrong if servers don't match
	but if there's one PK lookup only per request we're gonna loose.
	*/
	plugin_data_vpp = mysqlnd_plugin_get_plugin_connection_data_data(conn->data, mysqlnd_memcache_plugin_id);
	*plugin_data_vpp = pemalloc(sizeof(mysqlnd_memcache_connection_data_data), conn->persistent);
	plugin_data_p = *(mysqlnd_memcache_connection_data_data **)plugin_data_vpp;
	
	plugin_data_p->mapping.schema_name = estrdup(row[0]);
	plugin_data_p->mapping.table_name = estrdup(row[1]);
	plugin_data_p->mapping.id_field_name = estrdup(row[2]);
	mysqlnd_memcache_split_columns(plugin_data_p, row[3], strlen(row[3]));
	plugin_data_p->mapping.separator = estrdup(row[4]);

	mnd_free(row);
	mysqlnd_free_result(res, 0);
	
	return plugin_data_p;
}
/* }}} */

/* {{{ proto string mysqlnd_memcache_set(mixed mysql_connection, memcached memcached, string pattern, callback debug_notify)
   Link a memcache connection to a MySQLconnection */
PHP_FUNCTION(mysqlnd_memcache_set)
{
	zval *mysqlnd_conn_zv;
	MYSQLND *mysqlnd_conn;
	zval *memcached_zv = NULL;
	fake_memcached_object *memcached_obj;
	char *regexp = NULL;
	int regexp_len;
	mysqlnd_memcache_connection_data_data *conn_data;
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zO!|s!f", &mysqlnd_conn_zv, &memcached_zv, *memcached_ce, &regexp, &regexp_len, &fci, &fcc) == FAILURE) {
		return;
	}
	
	if (!(mysqlnd_conn = zval_to_mysqlnd(mysqlnd_conn_zv TSRMLS_CC))) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Passed variable is no mysqlnd-based MySQL connection");
		RETURN_FALSE;
	}
	
	if (!memcached_zv) {
		mysqlnd_memcache_free_connection_data_data(mysqlnd_conn->data TSRMLS_CC);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Connection plugin data was unset, old result sets might still try to access it. This has to be fixed. Be careful");
		RETURN_TRUE;
	}

	memcached_obj = (fake_memcached_object *)zend_object_store_get_object(memcached_zv TSRMLS_CC);

	conn_data = mysqlnd_memcache_init_mysqlnd(mysqlnd_conn TSRMLS_CC);
	if (!conn_data) {
		RETURN_FALSE;
	}
	
	conn_data->connection.memc = memcached_obj->obj->memc;
	Z_ADDREF_P(memcached_zv);
	conn_data->connection.zv = memcached_zv;
	
	if (regexp) {
		conn_data->regexp.str = estrndup(regexp, regexp_len);
		conn_data->regexp.len = regexp_len;
		conn_data->regexp.str_is_allocated = 1;
	} else {
		conn_data->regexp.str = SQL_PATTERN;
		conn_data->regexp.len = SQL_PATTERN_LEN;
		conn_data->regexp.str_is_allocated = 0;
	}
	
	conn_data->regexp.pattern = pcre_get_compiled_regex_cache(conn_data->regexp.str, conn_data->regexp.len TSRMLS_CC);
	
	if (ZEND_NUM_ARGS() == 4) {
		Z_ADDREF_P(fci.function_name);
		if (fci.object_ptr) {
			Z_ADDREF_P(fci.object_ptr);
		}
		conn_data->callback.exists = TRUE;
		conn_data->callback.fcc = fcc;
		conn_data->callback.fci = fci;
	} else {
		conn_data->callback.exists = FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ ARGINFO */
ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlnd_memcache_set, 0, 0, 2)
	ZEND_ARG_INFO(0, mysql_connection)
	ZEND_ARG_OBJ_INFO(0, memcached_connection, Memcached, 1)
	ZEND_ARG_INFO(0, select_pattern)
	ZEND_ARG_INFO(0, debug_callback)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ mysqlnd_memcache_functions[] */
static const zend_function_entry mysqlnd_memcache_functions[] = {
	PHP_FE(mysqlnd_memcache_set, arginfo_mysqlnd_memcache_set)
	PHP_FE_END
};
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(mysqlnd_memcache)
{
	struct st_mysqlnd_conn_data_methods *data_methods;
	char *pmversion;
	
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	
	if (zend_hash_find(CG(class_table), "memcached", sizeof("memcached"), (void **) &memcached_ce)==FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "mysqlnd_memcache failed to get Memcached class");
		return FAILURE;
	}

	pmversion = (char*)((*memcached_ce)->info.internal.module->version);
	if (php_version_compare("2.0.0", pmversion) > 0 || php_version_compare(pmversion, "2.1.0") >= 0) {
		/* As long as we're blindly casting a void* to fake_memcached_object* we have to be extra careful */
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "mysqlnd_memcache is only tested with php_memcached 2.0.x, %s might cause errors", pmversion);
	}

	mysqlnd_memcache_plugin_id = mysqlnd_plugin_register();

	data_methods = mysqlnd_conn_data_get_methods();

	orig_mysqlnd_conn_query = data_methods->query;
        data_methods->query = MYSQLND_METHOD(mysqlnd_memcache_conn, query);

	orig_mysqlnd_conn_dtor = data_methods->dtor;
        data_methods->dtor = MYSQLND_METHOD(mysqlnd_memcache_conn, dtor);
	
	REGISTER_STRINGL_CONSTANT("MYSQLND_MEMCACHE_DEFAULT_REGEXP", SQL_PATTERN, SQL_PATTERN_LEN, CONST_CS | CONST_PERSISTENT);
	
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(mysqlnd_memcache)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
    
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(mysqlnd_memcache)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "mysqlnd_memcache support", "enabled v." MYSQLND_MEMCACHE_VERSION);
	php_info_print_table_row(2, "mysqlnd version", MYSQLND_VERSION);
	php_info_print_table_row(2, "php-memcached version", (*memcached_ce)->info.internal.module->version);
	php_info_print_table_row(2, "libmemcached version", LIBMEMCACHED_VERSION_STRING);
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

static const zend_module_dep mysqlnd_memcache_deps[] = { /* {{{ */
	ZEND_MOD_REQUIRED("mysqlnd")
	ZEND_MOD_REQUIRED("pcre")
	ZEND_MOD_REQUIRED("memcached")
	ZEND_MOD_END
};
/* }}} */

/* {{{ mysqlnd_memcache_module_entry
 */
zend_module_entry mysqlnd_memcache_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	mysqlnd_memcache_deps,
	"mysqlnd_memcache",
	mysqlnd_memcache_functions,
	PHP_MINIT(mysqlnd_memcache),
	PHP_MSHUTDOWN(mysqlnd_memcache),
	NULL,
	NULL,
	PHP_MINFO(mysqlnd_memcache),
	MYSQLND_MEMCACHE_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_MYSQLND_MEMCACHE
ZEND_GET_MODULE(mysqlnd_memcache)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
