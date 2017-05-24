/*
  +----------------------------------------------------------------------+
  | PHP Version 5/7                                                      |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2017 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Andy Kirkham <andy@spiders-lair.com>                         |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php_jansson.h>
#include <php_ini.h>
#include <ext/standard/info.h>
#include <ext/spl/spl_iterators.h>
#include <zend_interfaces.h>
#include <zend_exceptions.h>

#include <jansson.h>

#define DUPLICATE_KEY 1
#define DUPLICATE_VAL 1

PHPAPI zend_class_entry *jansson_ce;
static zend_object_handlers jansson_object_handlers;

PHPAPI zend_class_entry *jansson_get_exception_ce;
PHPAPI zend_class_entry *jansson_constructor_exception_ce;

typedef struct _php_jansson_t
{
    json_t		*p_json;
    size_t              mysize;
    zend_object         std;
} php_jansson_t;

static void 
php_jansson_free(void *inp)
{
    if(inp) {
        php_jansson_t *p_jansson = (php_jansson_t*)inp;
        if(p_jansson->p_json) {
            json_decref(p_jansson->p_json);
            p_jansson->p_json = NULL;
        }
        efree(p_jansson);
    }    
}

static void 
php_jansson_dtor(php_jansson_t **inpp)
{
    if(inpp) {
        php_jansson_free(*inpp);
        *inpp = NULL;
    }
}

static php_jansson_t*
php_jansson_ctor(zend_class_entry *inp_class_type TSRMLS_C)
{
    php_jansson_t *p_jansson = ecalloc(1, sizeof(php_jansson_t) 
		+ zend_object_properties_size(inp_class_type)
		+ sizeof(zend_object));
    if(p_jansson) {
        p_jansson->p_json = json_object();
        if(p_jansson->p_json == NULL) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING , 
                    "json_object() failed");
        }
    }
    return p_jansson;
}

static inline php_jansson_t*
jansson_from_obj(zend_object *obj) {
	return (php_jansson_t*)((char*)(obj) - XtOffsetOf(php_jansson_t, std));
}

#define Z_JANSSON_P(zv) jansson_from_obj(Z_OBJ_P((zv)))

// Forward static declarations.
static json_t* 
jansson_encode_zval_to_jansson(zval *inp_zval TSRMLS_DC);

static inline void* 
jansson_malloc(size_t len) 
{
    return emalloc(len);
}

static inline void 
jansson_free(void *block) 
{
    efree(block);
}

static json_t* 
jansson_encode_zval_object_to_jansson(zval *inp_zval TSRMLS_DC) 
{
    if(Z_TYPE_P(inp_zval) != IS_OBJECT) {
        return NULL;
    }

    php_error_docref(NULL TSRMLS_CC, E_WARNING,
        "Cannot yet convert an object.");

    return NULL;
}

static json_t* 
jansson_encode_zval_array_to_jansson(zval *inp_zval TSRMLS_DC) 
{
    zval *p_zval;
    ulong num_key;
    zend_string *p_key;
    json_t *p_json = NULL;
    HashTable *p_hash = NULL;

    if(Z_TYPE_P(inp_zval) != IS_ARRAY) {
        return NULL;
    }

    if((p_hash = Z_ARRVAL_P(inp_zval)) == NULL) {
        return NULL;
    }

    if((p_json = json_object()) == NULL) {
        return NULL;
    }

    ZEND_HASH_FOREACH_KEY_VAL(p_hash, num_key, p_key, p_zval) {
        if(p_key) {
            if(json_object_set_new(p_json, ZSTR_VAL(p_key),
                jansson_encode_zval_to_jansson(p_zval)) != SUCCESS) {
                goto jansson_encode_zval_array_to_jansson_bailout;
            }
        }
        else {
            char tmp[32] = {0};
            snprintf((char*)&tmp[0], 32, "%d", num_key);
            if(json_object_set_new(p_json, (const char*)&tmp[0],
                jansson_encode_zval_to_jansson(p_zval)) != SUCCESS) {
                goto jansson_encode_zval_array_to_jansson_bailout;
            }

        }
    } ZEND_HASH_FOREACH_END();

    return p_json;
    jansson_encode_zval_array_to_jansson_bailout:
    if(p_json) {
        json_decref(p_json);
    }
    return NULL;
}

static json_t* 
jansson_encode_zval_to_jansson(zval *inp_zval TSRMLS_DC) 
{
    switch(Z_TYPE_P(inp_zval)) {
    case IS_STRING: 
        return json_stringn(Z_STRVAL_P(inp_zval), Z_STRLEN_P(inp_zval));
    case IS_LONG:
        return json_integer(Z_LVAL_P(inp_zval));
    case IS_NULL:
        return json_null();
    case IS_DOUBLE:
        return json_real(Z_DVAL_P(inp_zval));
    case _IS_BOOL:
        return json_boolean(Z_LVAL_P(inp_zval) == 0 ? 0 : 1);
    case IS_ARRAY:
        return jansson_encode_zval_array_to_jansson(inp_zval TSRMLS_CC);
    case IS_OBJECT:
        return jansson_encode_zval_object_to_jansson(inp_zval TSRMLS_CC);
    default:
        return NULL;
    }
    return NULL;
}

static zval*
jansson_to_zval(json_t *inp_json, zval *outp_zval TSRMLS_DC)
{
    switch(json_typeof(inp_json)) {
    case JSON_STRING:
        ZVAL_STRINGL(outp_zval, json_string_value(inp_json),
            json_string_length(inp_json));
        break;
    case JSON_INTEGER:
        ZVAL_LONG(outp_zval, json_interger_value(inp_json));
        break; 
    case JSON_REAL:
        ZVAL_DOUBLE(outp_zval, json_real_value(inp_json));
        break;
    case JSON_TRUE:
    case JSON_FALSE:
        ZVAL_BOOL(outp_zval, json_boolean_value(inp_json));
        break;
    case JSON_OBJECT: {
        zval z;
        json_t *p_json;
        const char *p_key;
        array_init(outp_zval);
        json_object_foreach(inp_json, p_key, p_json) {
            add_assoc_zval(outp_zval, p_key, jansson_to_zval(p_json, &z TSRMLS_CC));
        }}
        break;
    case JSON_ARRAY: {
        zval z;
        size_t nkey;
        json_t *p_json;
        array_init(outp_zval);
        json_array_foreach(inp_json, nkey, p_json) {
            add_index_zval(outp_zval, nkey, jansson_to_zval(p_json, &z TSRMLS_CC));
        }}
        break;
    }
    return outp_zval;
}

static json_t* 
jansson_add_zval_to_jansson(json_t *inp_json, const char *inp_key, 
        zval *inp_zval TSRMLS_DC) 
{
    json_t *p_json;
    if(!json_is_object(inp_json)) {
        return NULL;
    }
    if((p_json = jansson_encode_zval_to_jansson(inp_zval TSRMLS_CC)) == NULL) {
        return NULL;
    }
    if(json_object_set_new(inp_json, inp_key, p_json) != SUCCESS) {
        json_decref(p_json);
        return NULL;
    }
    return inp_json;
}

static json_t* 
jansson_add_zval_to_jansson_ex(json_t *inp_json, const char *inp_key, 
        int inn_len, zval *inp_zval TSRMLS_DC) 
{
    if(inn_len > 0) {
        const char *p = estrndup(inp_key, inn_len);
        if(p) {
            json_t *p_json = jansson_add_zval_to_jansson(inp_json, p,
                                  inp_zval TSRMLS_CC);
            efree((void*)p);
            return p_json;
        }
    }
    return NULL;
}

static zend_bool
jansson_add_zval(json_t *inp_json, zval *inp_key, 
        zval *inp_zval TSRMLS_DC) 
{
    if(Z_TYPE_P(inp_zval) == IS_STRING) {
        if(jansson_add_zval_to_jansson_ex(inp_json,
              Z_STRVAL_P(inp_key), Z_STRLEN_P(inp_key),
              inp_zval TSRMLS_CC) != NULL)
        {
            return SUCCESS;
        }
    }
    return FAILURE;
}

ZEND_DECLARE_MODULE_GLOBALS(jansson)
PHPAPI zend_class_entry *p_ce_jansson;

static ZEND_MODULE_GLOBALS_CTOR_D(jansson)
{
    jansson_globals->seed = 0;
    jansson_globals->use_php_memory = 1;
}

static ZEND_MODULE_GLOBALS_DTOR_D(jansson)
{}

static inline json_t*
jansson_get_value(json_t *inp_json, const char *inp_key, int inn_len TSRMLS_DC)
{
    json_t *p_json = NULL;
    char *p_key = estrndup(inp_key, inn_len); // ensure use of null term string
    if(p_key) {
        p_json = json_object_get(inp_json, p_key);
        efree(p_key);
    }
    return p_json;
}

static zend_bool
jansson_set_value(json_t *inp_json, const char *inp_key, int inn_len, 
        json_t *inp_json_value TSRMLS_DC)
{
    zend_bool rval = FAILURE;
    char *p_key = estrndup(inp_key, inn_len); // ensure use of null term string
    if(p_key) {
        if(json_object_set_new(inp_json, p_key, inp_json_value) == SUCCESS) {
            rval = SUCCESS;
        }
        else {
            php_error_docref(NULL TSRMLS_CC, E_WARNING , 
                "jansson_set_value(): For %p At line %d", inp_json, __LINE__);
        }
        efree(p_key);
    }
    return rval;
}

ZEND_BEGIN_ARG_INFO(arginfo_jansson_method_count, 0)
ZEND_END_ARG_INFO()
PHP_METHOD(jansson, count)
{
    php_jansson_t *p_this;
    
    if(zend_parse_parameters_none() == FAILURE) {
        return;
    }
    
    p_this = Z_JANSSON_P(getThis());
    if(!p_this || !p_this->p_json || json_typeof(p_this->p_json) != JSON_OBJECT) {
        return;
    }
    
    RETURN_LONG(json_object_size(p_this->p_json));
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_jansson_method_has, 0, 0, 1)
    ZEND_ARG_INFO(0, string)
ZEND_END_ARG_INFO()
PHP_METHOD(jansson, has)
{
    size_t   klen;
    char *inp_key;
    json_t *p_json;
    php_jansson_t *p_this;
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(inp_key, klen)
    ZEND_PARSE_PARAMETERS_END();
    
    p_this = Z_JANSSON_P(getThis());
    if(!p_this) {
        return;
    }
    
    if(jansson_get_value(p_this->p_json, inp_key, klen TSRMLS_CC)) {
        RETURN_TRUE;
    }        
    RETURN_FALSE;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_jansson_method_get, 0, 0, 1)
    ZEND_ARG_INFO(0, string)
    ZEND_ARG_INFO(0, bool_use_exception)
ZEND_END_ARG_INFO()
PHP_METHOD(jansson, get)
{
    zval z, *p_zval;
    size_t  klen;
    char *inp_key;
    json_t *p_json;
    zend_bool use_exception = 1;
    php_jansson_t *p_this;
    
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STRING(inp_key, klen)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(use_exception)
    ZEND_PARSE_PARAMETERS_END();

    p_this = Z_JANSSON_P(getThis());
    if(p_this != NULL) {
        p_json = jansson_get_value(p_this->p_json, inp_key, klen TSRMLS_CC);
        if(p_json == NULL) {
            if(use_exception) {
                zend_throw_exception(jansson_get_exception_ce, 
                    "Jansson::get() Key not found", 0 TSRMLS_CC);
            }
            return;
        }
    
        if((p_zval = jansson_to_zval(p_json, &z TSRMLS_CC)) != NULL) {
            ZVAL_ZVAL(return_value, p_zval, 0, 1);
        }
        else {
            if(use_exception) {
                zend_throw_exception(jansson_get_exception_ce, 
                    "Jansson::get() Key not found", 0 TSRMLS_CC);
            }
        }
    }
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_jansson_method_del, 0, 0, 1)
    ZEND_ARG_INFO(0, string)
ZEND_END_ARG_INFO()
PHP_METHOD(jansson, del)
{
    size_t klen = 0;
    long rval = 0;
    char *inp_key;
    php_jansson_t *p_this;
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(inp_key, klen)
    ZEND_PARSE_PARAMETERS_END();

    p_this = Z_JANSSON_P(getThis());
    if(p_this != NULL) { 
        char *p_key = estrndup(inp_key, klen);
        rval = json_object_del(p_this->p_json, p_key);
        efree(p_key);
        RETURN_LONG(rval);
    }
    RETURN_FALSE;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_jansson_method_to_array, 0, 0, 1)
ZEND_END_ARG_INFO()
PHP_METHOD(jansson, to_array)
{
    zval z, *p_zval;
    php_jansson_t *p_this;
    
    if(zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }
    
    p_this = Z_JANSSON_P(getThis());
    if(p_this != NULL) {
        if((p_zval = jansson_to_zval(p_this->p_json, &z TSRMLS_CC)) != NULL) {
            ZVAL_ZVAL(return_value, p_zval, 0, 1);
        }
    }
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_jansson_method_set, 0, 0, 1)
    ZEND_ARG_INFO(0, key_string)
    ZEND_ARG_INFO(0, variable_to_set)
ZEND_END_ARG_INFO()
PHP_METHOD(jansson, set)
{
    size_t klen = 0;
    char *inp_key;
    zval *inp_zval;
    json_t *p_json;
    php_jansson_t *p_this;
    
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(inp_key, klen)
        Z_PARAM_ZVAL(inp_zval)
    ZEND_PARSE_PARAMETERS_END();

    p_json = jansson_encode_zval_to_jansson(inp_zval TSRMLS_CC);
    if(!p_json) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING , "jansson_encode_zval_to_jansson() failed");
        RETURN_FALSE;
    }
    
    p_this = Z_JANSSON_P(getThis());
    if(!p_this) {
        json_decref(p_json);
        RETURN_FALSE;
    }
    
    if(jansson_set_value(p_this->p_json, inp_key, klen, p_json TSRMLS_CC) != SUCCESS) {
        json_decref(p_json);
        php_error_docref(NULL TSRMLS_CC, E_WARNING , "jansson_set_value() failed");
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

typedef struct _jansson_stream_resource {
    long            byte_count;
    json_t          *p_json;
    php_stream      *p_stream;
    php_jansson_t   *p_this;
} jansson_stream_resource_t;

static int
_jansson_to_stream_callback(const char *inp_buf, size_t inn_len, void *inp_data)
{

    size_t written = 0;
    jansson_stream_resource_t *p_res = 
            (jansson_stream_resource_t*)inp_data;
#ifdef ZTS
    TSRMLS_D = p_res->TSRMLS_C;
#endif    
    
    while(written < inn_len) {
        size_t remaining = inn_len - written;
        written += php_stream_write(p_res->p_stream, &inp_buf[written], 
                remaining TSRMLS_CC);
    }
    p_res->byte_count += written;
    return SUCCESS;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_jansson_method_to_stream, 0, 0, 1)
    ZEND_ARG_INFO(0, stream_resource)
    ZEND_ARG_INFO(0, jansson_dump_flags)
    ZEND_ARG_INFO(0, indent)
    ZEND_ARG_INFO(0, precision)
ZEND_END_ARG_INFO()
PHP_METHOD(jansson, to_stream)
{
    zval *inp_zval_dst;
    zend_long flags = JSON_COMPACT, indent = 0, precision = 0;
    jansson_stream_resource_t resource;
    
    ZEND_PARSE_PARAMETERS_START(1, 4)
        Z_PARAM_RESOURCE(inp_zval_dst)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
        Z_PARAM_LONG(indent)
        Z_PARAM_LONG(precision)
    ZEND_PARSE_PARAMETERS_END();

    php_stream_from_zval(resource.p_stream, inp_zval_dst);
    if(!resource.p_stream) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, 
            "Parameter is not a stream");
        RETURN_FALSE;
    }
    
    resource.p_this = Z_JANSSON_P(getThis());
    if(!resource.p_this) {
        RETURN_FALSE;
    }
    
#ifdef ZTS
    resource.TSRMLS_C = TSRMLS_C;
#endif
    
    resource.byte_count = 0;
    
    if(indent > 0) {
        if(indent > 31) indent = 31;
        flags |= JSON_INDENT(indent);
    }
    
    if(precision > 0) {
        if(precision > 31) precision = 31;
        flags |= JSON_REAL_PRECISION(precision);
    }
    
    if(json_dump_callback(resource.p_this->p_json, 
            _jansson_to_stream_callback, &resource, flags) != SUCCESS) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, 
                "json_dump_callback failed");
        RETURN_FALSE;
    }
    
    RETURN_LONG(resource.byte_count);
}

static size_t
_jansson_from_stream_callback(void *inp_buf, size_t inn_len, void *inp_data)
{
    size_t read = 0;
    jansson_stream_resource_t *p_res = 
            (jansson_stream_resource_t*)inp_data;
#ifdef ZTS
    TSRMLS_D = p_res->TSRMLS_C;
#endif    

    read = php_stream_read(p_res->p_stream, inp_buf, inn_len TSRMLS_CC);
    p_res->byte_count += read;    
    return read;   
}

static zend_bool
_jansson_from_stream(zval *inp_zval_src, size_t flags, 
        zval *return_value, php_jansson_t *this_ptr, int with_warn TSRMLS_DC)
{
    json_t *p_json;
    json_error_t err;
    jansson_stream_resource_t resource;
    
    if(Z_TYPE_P(inp_zval_src) != IS_RESOURCE) {
        return;
    }
    
    php_stream_from_zval(resource.p_stream, inp_zval_src);
    if(!resource.p_stream) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING , 
            "Parameter is not a stream");
        if(return_value) {
            RETURN_FALSE;
        }
        return FAILURE;
    }
    
    resource.p_this = this_ptr;
    if(!resource.p_this) {
        if(return_value) {
            RETURN_FALSE;
        }
        return FAILURE;
    }
    
#ifdef ZTS
    resource.TSRMLS_C = TSRMLS_C;
#endif
    
    resource.byte_count = 0;
    
    p_json = json_load_callback(_jansson_from_stream_callback, 
                 &resource, flags, &err);
    
    if(p_json == NULL) {
        if(with_warn) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING , 
                "json_load_callback() failed: '%s' at line %d, column %d, pos %d", 
                err.text, err.line, err.column, err.position);
        }
        if(return_value) {
            RETURN_FALSE;
        }
        return FAILURE;
    }
    
    if(json_typeof(p_json) != JSON_OBJECT) {
        json_decref(p_json);
        if(with_warn) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING , 
                "json_load_callback() did not load an json object");
        }
        if(return_value) {
            RETURN_FALSE;
        }
        return FAILURE;
    }
        
    if(resource.p_this->p_json) {
        json_decref(resource.p_this->p_json);
    }
    resource.p_this->p_json = p_json;
    if(return_value) {
        RETURN_LONG(resource.byte_count);        
    }
    return SUCCESS;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_jansson_method_from_stream, 0, 0, 1)
    ZEND_ARG_INFO(0, stream_resource)
    ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()
PHP_METHOD(jansson, from_stream)
{
    zval *inp_zval_src;
    size_t flags = 0;
    
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|l", 
            &inp_zval_src, &flags) == FAILURE) {
        RETURN_FALSE;
    }
    
    //_jansson_from_stream(inp_zval_src, 
    //        flags, return_value, getThis(), 0 TSRMLS_CC);
    _jansson_from_stream(inp_zval_src, 
            flags, return_value, Z_JANSSON_P(getThis()), 0 TSRMLS_CC);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_jansson_method_construct, 0, 0, 1)
    ZEND_ARG_INFO(0, array)
ZEND_END_ARG_INFO()
PHP_METHOD(jansson, __construct)
{
    zval *inp_zval = NULL;

    return_value = getThis();
    
    if(ZEND_NUM_ARGS() != 1) {
        return;
    }
    else {    
        if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", 
                &inp_zval) == FAILURE) {
            return;
        }
        
        if(!inp_zval) {
            return;
        }
    }
    
    if(inp_zval && Z_TYPE_P(inp_zval) == IS_RESOURCE) {
        if(_jansson_from_stream(inp_zval, 0, NULL, Z_JANSSON_P(getThis()), 0 TSRMLS_CC) != SUCCESS) {
            zend_throw_exception(jansson_constructor_exception_ce, 
                "Jansson::__construct() failed to load from stream", 0 TSRMLS_CC);
        }
    }
    else if(inp_zval && Z_TYPE_P(inp_zval) == IS_ARRAY) {
        json_t *p_json;
        php_jansson_t *p_this;
        
        p_this = Z_JANSSON_P(getThis());
        if(!p_this) {
            return;
        }
        if(p_this->p_json) {
            json_decref(p_this->p_json);
            p_this->p_json = NULL;
        }
        p_json = jansson_encode_zval_array_to_jansson(inp_zval TSRMLS_CC);    
        p_this->p_json = p_json ? p_json : json_object();
    }
}

zend_function_entry jansson_methods[] = 
{
    PHP_ME(jansson, __construct, 
            arginfo_jansson_method_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(jansson, count, 
            arginfo_jansson_method_count, ZEND_ACC_PUBLIC)
    PHP_ME(jansson, get, 
            arginfo_jansson_method_get, ZEND_ACC_PUBLIC)
    PHP_ME(jansson, del, 
            arginfo_jansson_method_del, ZEND_ACC_PUBLIC)        
    PHP_ME(jansson, has, 
            arginfo_jansson_method_has, ZEND_ACC_PUBLIC)        
    PHP_ME(jansson, set, 
            arginfo_jansson_method_set, ZEND_ACC_PUBLIC)                
    PHP_ME(jansson, to_array, 
            arginfo_jansson_method_to_array, ZEND_ACC_PUBLIC)                        
    PHP_ME(jansson, to_stream, 
            arginfo_jansson_method_to_stream, ZEND_ACC_PUBLIC)                
    PHP_ME(jansson, from_stream, 
            arginfo_jansson_method_from_stream, ZEND_ACC_PUBLIC)                        
    PHP_FE_END
};

static void
jansson_free_object_storage_handler(zend_object *p_obj)
{
    php_jansson_t *p_intern = jansson_from_obj(p_obj);
    zend_object_std_dtor(&p_intern->std);
    php_jansson_free(p_intern);
}

static zend_object*
jansson_clone_object(zval *inp_zval)
{
    php_jansson_t *p_intern, *p_new_intern;
    zend_object *p_new_obj;

    p_intern = Z_JANSSON_P(inp_zval);
    p_new_obj = jansson_ce->create_object(Z_OBJCE_P(inp_zval));
    zend_object_clone_members(&p_new_intern->std, &p_intern->std);
    p_new_intern->p_json = json_deep_copy(p_intern->p_json);

    return p_new_obj;
}

static zend_object*
jansson_create_object_handler(zend_class_entry *inp_class_type TSRMLS_DC)
{
    php_jansson_t *p_intern = php_jansson_ctor(inp_class_type TSRMLS_CC);
    zend_object_std_init(&p_intern->std, inp_class_type);
    object_properties_init(&p_intern->std, inp_class_type);
    p_intern->std.handlers = &jansson_object_handlers;
    return &p_intern->std;
}

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("jansson.use_php_memory", "1", PHP_INI_SYSTEM,        
        OnUpdateLong, use_php_memory, zend_jansson_globals, jansson_globals)
    STD_PHP_INI_ENTRY("jansson.seed", "0", PHP_INI_SYSTEM,        
        OnUpdateLong, seed, zend_jansson_globals, jansson_globals)
PHP_INI_END()


PHP_MINIT_FUNCTION(jansson)
{
    zend_class_entry ce, tmp_ce;
    zend_class_entry* p_exception_ce = 
            zend_exception_get_default(TSRMLS_C);
    
    REGISTER_INI_ENTRIES();
    
    if(JANSSON_G(use_php_memory) == 1) {
        json_set_alloc_funcs(jansson_malloc, jansson_free);
    }
    
    json_object_seed(JANSSON_G(seed));
    
    INIT_NS_CLASS_ENTRY(ce, PHP_JANSSON_NS, "Jansson", jansson_methods);
    ce.create_object = jansson_create_object_handler;
    jansson_ce = zend_register_internal_class(&ce TSRMLS_CC);
    zend_class_implements(jansson_ce TSRMLS_CC, 1, spl_ce_Countable);
    memcpy(&jansson_object_handlers, zend_get_std_object_handlers(),
        sizeof(zend_object_handlers));
    jansson_object_handlers.clone_obj = jansson_clone_object;
    jansson_object_handlers.free_obj = jansson_free_object_storage_handler;

    INIT_CLASS_ENTRY(tmp_ce, "JanssonGetException", NULL);
    jansson_get_exception_ce = zend_register_internal_class_ex(
            &tmp_ce, p_exception_ce);
    
    INIT_CLASS_ENTRY(tmp_ce, "JanssonConstructorException", NULL);
    jansson_constructor_exception_ce = zend_register_internal_class_ex(
            &tmp_ce, p_exception_ce);
    
    zend_declare_class_constant_long(jansson_ce, 
            ZEND_STRL("JSON_COMPACT"), 
            JSON_COMPACT TSRMLS_CC);
    zend_declare_class_constant_long(jansson_ce, 
            ZEND_STRL("JSON_ENSURE_ASCII"), 
            JSON_ENSURE_ASCII TSRMLS_CC);
    zend_declare_class_constant_long(jansson_ce, 
            ZEND_STRL("JSON_SORT_KEYS"), 
            JSON_SORT_KEYS TSRMLS_CC);
    zend_declare_class_constant_long(jansson_ce, 
            ZEND_STRL("JSON_PRESERVE_ORDER"), 
            JSON_PRESERVE_ORDER TSRMLS_CC);
    zend_declare_class_constant_long(jansson_ce, 
            ZEND_STRL("JSON_ENCODE_ANY"), 
            JSON_ENCODE_ANY TSRMLS_CC);
    zend_declare_class_constant_long(jansson_ce, 
            ZEND_STRL("JSON_ESCAPE_SLASH"), 
            JSON_ESCAPE_SLASH TSRMLS_CC);
    zend_declare_class_constant_long(jansson_ce, 
            ZEND_STRL("JSON_REJECT_DUPLICATES"), 
            JSON_REJECT_DUPLICATES TSRMLS_CC);
    zend_declare_class_constant_long(jansson_ce, 
            ZEND_STRL("JSON_DECODE_ANY"), 
            JSON_DECODE_ANY TSRMLS_CC);
    zend_declare_class_constant_long(jansson_ce, 
            ZEND_STRL("JSON_DISABLE_EOF_CHECK"), 
            JSON_DISABLE_EOF_CHECK TSRMLS_CC);
    zend_declare_class_constant_long(jansson_ce, 
            ZEND_STRL("JSON_DECODE_INT_AS_REAL"), 
            JSON_DECODE_INT_AS_REAL TSRMLS_CC);
    zend_declare_class_constant_long(jansson_ce, 
            ZEND_STRL("JSON_ALLOW_NUL"), 
            JSON_ALLOW_NUL TSRMLS_CC);
    
    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(jansson)
{
    return SUCCESS;
}

PHP_MINFO_FUNCTION(jansson)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "Version", PHP_JANSSON_VERSION);
    DISPLAY_INI_ENTRIES();
}

PHP_RINIT_FUNCTION(jansson)
{
    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(jansson)
{
    return SUCCESS;
}

zend_function_entry jansson_functions[] = 
{
    PHP_FE_END
};
        
zend_module_entry jansson_module_entry = 
{
    STANDARD_MODULE_HEADER,
    "jansson",
    jansson_functions,
    PHP_MINIT(jansson),
    PHP_MSHUTDOWN(jansson),
    PHP_RINIT(jansson),
    PHP_RSHUTDOWN(jansson),
    PHP_MINFO(jansson),
    PHP_JANSSON_VERSION,
    PHP_MODULE_GLOBALS(jansson),
    PHP_GINIT(jansson),
    NULL,
    NULL,
    STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_JANSSON
    ZEND_GET_MODULE(jansson)
#endif

