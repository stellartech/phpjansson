/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
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
    zend_object         std;
    json_t		*p_json;
    size_t              mysize;
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
php_jansson_ctor(TSRMLS_C)
{
    php_jansson_t *p_jansson = emalloc(sizeof(php_jansson_t));
    if(p_jansson) {
        memset(p_jansson, 0, sizeof(php_jansson_t));
        p_jansson->p_json = json_object();
        if(p_jansson->p_json == NULL) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING , 
                    "json_object() failed");
        }
    }
    return p_jansson;
}

// Forward static declarations.
static json_t* 
jansson_encode_zval_to_jansson(zval **inpp_zval TSRMLS_DC);

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
jansson_encode_zval_object_to_jansson(zval **inpp_zval TSRMLS_DC) 
{
    if(Z_TYPE_PP(inpp_zval) != IS_OBJECT) {
        return NULL;
    }
    
    php_error_docref(NULL TSRMLS_CC, E_WARNING, 
        "Cannot yet convert an object.");
    
    // ToDo (if object type is Jansson\jannson then copy, otherwise NULL)
    return NULL;
}

static json_t* 
jansson_encode_zval_array_to_jansson(zval **inpp_zval TSRMLS_DC) 
{
    zval **pp_zval;
    char *p_key = NULL;
    json_t *p_json;  
    HashTable *p_hash;
    HashPosition position;
    
    if(Z_TYPE_PP(inpp_zval) != IS_ARRAY) {
        return NULL;
    }
    
    if((p_json = json_object()) == NULL) {
        return NULL;
    }
    
    p_hash = Z_ARRVAL_PP(inpp_zval);

    if(zend_hash_num_elements(p_hash) == 0) {
        return p_json;
    }
            
    zend_hash_internal_pointer_reset_ex(p_hash, &position);
            
    while(zend_hash_get_current_data_ex(
            p_hash, (void**)&pp_zval, &position) == SUCCESS) {
                
        zend_uint klen = 0;
        zend_ulong kidx = 0L;
        p_key = NULL;
                
        int res = zend_hash_get_current_key_ex(p_hash, 
                    &p_key, &klen, &kidx, DUPLICATE_KEY, &position);
                
        switch(res) {
        case HASH_KEY_IS_STRING:
            if(klen < 1) {
                goto jansson_encode_zval_array_to_jansson_bailout;
            }
            if(json_object_set_new(p_json, p_key, 
                jansson_encode_zval_to_jansson(pp_zval TSRMLS_CC)) != SUCCESS) {
                goto jansson_encode_zval_array_to_jansson_bailout;
            }
            break;
        case HASH_KEY_IS_LONG: {
            char tmp[32] = {0};
            snprintf((char*)&tmp, 32, "%d", kidx);
            if(json_object_set_new(p_json, (const char*)&tmp, 
                jansson_encode_zval_to_jansson(pp_zval TSRMLS_CC)) != SUCCESS) {
                goto jansson_encode_zval_array_to_jansson_bailout;
            }}
            break;                    
        default:
            /* Should never happen. */
            json_decref(p_json);
            if(p_key) efree(p_key);
            return NULL;
        }
              
        if(p_key) {
            efree(p_key);
        }
              
        zend_hash_move_forward_ex(p_hash, &position);
    }
    
    return p_json;
    
    jansson_encode_zval_array_to_jansson_bailout:
    if(p_key) efree(p_key);
    if(p_json) json_decref(p_json);
    return NULL;
}

static json_t* 
jansson_encode_zval_to_jansson(zval **inpp_zval TSRMLS_DC) 
{	
    switch (Z_TYPE_PP(inpp_zval)) {
    case IS_STRING:
        return json_stringn(Z_STRVAL_PP(inpp_zval), Z_STRLEN_PP(inpp_zval));
    case IS_LONG:
        return json_integer(Z_LVAL_PP(inpp_zval));
    case IS_NULL:
        return json_null();    
    case IS_DOUBLE:
        return json_real(Z_DVAL_PP(inpp_zval));    
    case IS_BOOL:
        return json_boolean(Z_BVAL_PP(inpp_zval));
    case IS_ARRAY: 
        return jansson_encode_zval_array_to_jansson(inpp_zval TSRMLS_CC);    
    case IS_OBJECT: 
        return jansson_encode_zval_object_to_jansson(inpp_zval TSRMLS_CC);    
    default:
        return NULL;
    }
    return NULL;
}

static zval*
jansson_to_zval(json_t **inpp_json TSRMLS_DC)
{
    const char *p_key;
    size_t nkey;
    json_t *p_json;
    
    zval *p_retval = EG(uninitialized_zval_ptr);
    
    if(!inpp_json || !(*inpp_json)) {
        Z_ADDREF_P(p_retval);
        return p_retval;
    }
    
    switch(json_typeof(*inpp_json)) {
        case JSON_STRING:
            ALLOC_INIT_ZVAL(p_retval);
            ZVAL_STRINGL(p_retval, json_string_value(*inpp_json),
                    json_string_length(*inpp_json), DUPLICATE_VAL);
            break;
        case JSON_INTEGER:
            ALLOC_INIT_ZVAL(p_retval);
            ZVAL_LONG(p_retval, json_integer_value(*inpp_json));
            break;
        case JSON_REAL:
            ALLOC_INIT_ZVAL(p_retval);
            ZVAL_DOUBLE(p_retval, json_real_value(*inpp_json));
            break;
        case JSON_NULL:
            Z_ADDREF_P(p_retval);
            break;
        case JSON_TRUE:
        case JSON_FALSE:
            ALLOC_INIT_ZVAL(p_retval);
            ZVAL_BOOL(p_retval, json_boolean_value(*inpp_json));
            break;
        case JSON_OBJECT:
            ALLOC_INIT_ZVAL(p_retval);
            array_init(p_retval);
            json_object_foreach(*inpp_json, p_key, p_json) {
                add_assoc_zval(p_retval, p_key, 
                        jansson_to_zval(&p_json TSRMLS_CC));
            }
            break;
        case JSON_ARRAY:
            ALLOC_INIT_ZVAL(p_retval);
            array_init(p_retval);
            json_array_foreach(*inpp_json, nkey, p_json) {
                add_index_zval(p_retval, nkey, 
                        jansson_to_zval(&p_json TSRMLS_CC));
            }
            break;
        default:
            php_error_docref(NULL TSRMLS_CC, E_WARNING , 
                "jansson_to_zval(): No type at line %d", __LINE__);
            
    }
    
    return p_retval;
}

static json_t* 
jansson_add_zval_to_jansson(json_t *inp_json, const char *inp_key, 
        zval **inpp_zval TSRMLS_DC) 
{
    json_t *p_json;
    
    if(!json_is_object(inp_json)) {
        return NULL;
    }
    
    if((p_json = jansson_encode_zval_to_jansson(inpp_zval TSRMLS_CC)) == NULL) {
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
        int inn_len, zval **inpp_zval TSRMLS_DC) 
{
    if(inn_len > 0) {
        const char *p_tmp = estrndup(inp_key, inn_len);
        if(p_tmp) {
            json_t *p_json = jansson_add_zval_to_jansson(inp_json, p_tmp, 
                                inpp_zval TSRMLS_CC);
            efree((char*)p_tmp);
            return p_json;
        }
    }    
    return NULL;
}

static zend_bool
jansson_add_zval(json_t *inp_json, zval *inp_key, 
        zval **inpp_zval TSRMLS_DC) 
{
    if(Z_TYPE_P(inp_key) == IS_STRING) {
        if(jansson_add_zval_to_jansson_ex(inp_json,
                Z_STRVAL_P(inp_key), Z_STRLEN_P(inp_key), 
                inpp_zval TSRMLS_CC) != NULL) 
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
    
    p_this = (php_jansson_t*)zend_object_store_get_object(getThis() TSRMLS_CC);
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
    int   klen;
    char *inp_key;
    json_t *p_json;
    php_jansson_t *p_this;
    
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", 
            &inp_key, &klen) == FAILURE) {
        RETURN_FALSE;
    }
    
    p_this = (php_jansson_t*)zend_object_store_get_object(getThis() TSRMLS_CC);
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
    zval *p_zval;
    int   klen;
    char *inp_key;
    json_t *p_json;
    zend_bool use_exception = 1;
    php_jansson_t *p_this;
    
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b", 
            &inp_key, &klen, &use_exception) == FAILURE) {
        zend_throw_exception(jansson_get_exception_ce, 
            "Jansson::get() invalid paramers", 0 TSRMLS_CC);
        RETURN_FALSE;
    }
    
    if((p_this = (php_jansson_t*)zend_object_store_get_object(
            getThis() TSRMLS_CC)) != NULL) 
    {
        p_json = jansson_get_value(p_this->p_json, inp_key, klen TSRMLS_CC);
        if(p_json == NULL) {
            if(use_exception) {
                zend_throw_exception(jansson_get_exception_ce, 
                    "Jansson::get() Key not found", 0 TSRMLS_CC);
            }
            return;
        }
    
        if((p_zval = jansson_to_zval(&p_json TSRMLS_CC)) != NULL) {
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
    int  klen = 0;
    long rval = 0;
    char *inp_key;
    php_jansson_t *p_this;
    
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", 
            &inp_key, &klen) == FAILURE || klen == 0) {
        RETURN_FALSE;
    }
    
    if((p_this = (php_jansson_t*)zend_object_store_get_object(
            getThis() TSRMLS_CC)) != NULL) 
    {
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
    zval *p_zval;
    php_jansson_t *p_this;
    
    if(zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }
    
    if((p_this = (php_jansson_t*)zend_object_store_get_object(
            getThis() TSRMLS_CC)) != NULL) 
    {
        if((p_zval = jansson_to_zval(&p_this->p_json TSRMLS_CC)) != NULL) {
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
    int  klen = 0;
    char *inp_key;
    zval *inp_zval;
    json_t *p_json;
    php_jansson_t *p_this;
    
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", 
            &inp_key, &klen, &inp_zval) == FAILURE || klen == 0) {
        RETURN_FALSE;
    }
    
    p_json = jansson_encode_zval_to_jansson(&inp_zval TSRMLS_CC);
    if(!p_json) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING , "jansson_encode_zval_to_jansson() failed");
        RETURN_FALSE;
    }
    
    p_this = (php_jansson_t*)zend_object_store_get_object(getThis() TSRMLS_CC);
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
    TSRMLS_D;
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
    long flags = JSON_COMPACT, indent = 0, precision = 0;
    jansson_stream_resource_t resource;
    
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|lll", 
            &inp_zval_dst, &flags, &indent, &precision) == FAILURE) {
        RETURN_FALSE;
    }
    
    php_stream_from_zval(resource.p_stream, &inp_zval_dst);
    if(!resource.p_stream) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, 
            "Parameter is not a stream");
        RETURN_FALSE;
    }
    
    resource.p_this = (php_jansson_t*)zend_object_store_get_object(getThis() TSRMLS_CC);
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
_jansson_from_stream(zval **inpp_zval_src, size_t flags, 
        zval *return_value, zval *this_ptr, int with_warn TSRMLS_DC)
{
    json_t *p_json;
    json_error_t err;
    jansson_stream_resource_t resource;
    
    if(Z_TYPE_PP(inpp_zval_src) != IS_RESOURCE) {
        return;
    }
    
    php_stream_from_zval(resource.p_stream, inpp_zval_src);
    if(!resource.p_stream) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING , 
            "Parameter is not a stream");
        if(return_value) {
            RETURN_FALSE;
        }
        return FAILURE;
    }
    
    resource.p_this = (php_jansson_t*)zend_object_store_get_object(getThis() TSRMLS_CC);
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
    zval **inpp_zval_src;
    size_t flags = 0;
    
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Z|l", 
            &inpp_zval_src, &flags) == FAILURE) {
        RETURN_FALSE;
    }
    
    _jansson_from_stream(inpp_zval_src, 
            flags, return_value, getThis(), 0 TSRMLS_CC);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_jansson_method_construct, 0, 0, 1)
    ZEND_ARG_INFO(0, array)
ZEND_END_ARG_INFO()
PHP_METHOD(jansson, __construct)
{
    zval **inpp_zval = NULL;

    return_value = getThis();
    
    if(ZEND_NUM_ARGS() != 1) {
        return;
    }
    else {    
        if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Z", 
                &inpp_zval) == FAILURE) {
            return;
        }
        
        if(!inpp_zval) {
            return;
        }
    }
    
    if(inpp_zval && Z_TYPE_PP(inpp_zval) == IS_RESOURCE) {
        if(_jansson_from_stream(inpp_zval, 0, NULL, getThis(), 0 TSRMLS_CC) != SUCCESS) {
            zend_throw_exception(jansson_constructor_exception_ce, 
                "Jansson::__construct() failed to load from stream", 0 TSRMLS_CC);
        }
    }
    else if(inpp_zval && Z_TYPE_PP(inpp_zval) == IS_ARRAY) {
        json_t *p_json;
        php_jansson_t *p_this;
        
        p_this = (php_jansson_t*)
                zend_object_store_get_object(getThis() TSRMLS_CC);
        if(!p_this) {
            return;
        }
        if(p_this->p_json) {
            json_decref(p_this->p_json);
            p_this->p_json = NULL;
        }
        p_json = jansson_encode_zval_array_to_jansson(inpp_zval TSRMLS_CC);    
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
jansson_free_object_storage_handler(php_jansson_t *inp_intern TSRMLS_DC)
{
    zend_object_std_dtor(&inp_intern->std TSRMLS_CC);
    php_jansson_dtor(&inp_intern);
}

static void 
jansson_clone_object_storage_handler(
        php_jansson_t *inp, 
        php_jansson_t **outpp TSRMLS_DC) 
{
    php_jansson_t *p_clone = php_jansson_ctor(TSRMLS_C);
    zend_object_std_init(&p_clone->std, inp->std.ce TSRMLS_CC);
    object_properties_init(&p_clone->std, inp->std.ce);
    p_clone->p_json = json_deep_copy(inp->p_json);
    *outpp = p_clone;
}

static zend_object_value
jansson_create_object_handler(zend_class_entry *inp_class_type TSRMLS_DC)
{
    zend_object_value retval;
    php_jansson_t *p_intern = php_jansson_ctor(TSRMLS_CC);
    zend_object_std_init(&p_intern->std, inp_class_type TSRMLS_CC);
    object_properties_init(&p_intern->std, inp_class_type);
    retval.handle = zend_objects_store_put(
        p_intern,
        (zend_objects_store_dtor_t)zend_objects_destroy_object,
        (zend_objects_free_object_storage_t)jansson_free_object_storage_handler,
        (zend_objects_store_clone_t)jansson_clone_object_storage_handler
        TSRMLS_CC
    );
    retval.handlers = &jansson_object_handlers;
    return retval;
}

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("jansson.use_php_memory", "1", PHP_INI_SYSTEM,        
        OnUpdateLong, use_php_memory, zend_jansson_globals, jansson_globals)
PHP_INI_END()


PHP_MINIT_FUNCTION(jansson)
{
    zend_class_entry tmp_ce;
    zend_class_entry* p_exception_ce = 
            zend_exception_get_default(TSRMLS_C);
    
    REGISTER_INI_ENTRIES();
    
    if(JANSSON_G(use_php_memory) == 1) {
        json_set_alloc_funcs(jansson_malloc, jansson_free);
    }
    
    json_object_seed(0);
    
    INIT_NS_CLASS_ENTRY(tmp_ce, PHP_JANSSON_NS, "Jansson", jansson_methods);
    
    jansson_ce = zend_register_internal_class(&tmp_ce TSRMLS_CC);
    zend_class_implements(jansson_ce TSRMLS_CC, 1, spl_ce_Countable);
    
    jansson_ce->create_object = jansson_create_object_handler;
    
    memcpy(&jansson_object_handlers, zend_get_std_object_handlers(), 
            sizeof(zend_object_handlers));
    
    INIT_CLASS_ENTRY(tmp_ce, "JanssonGetException", NULL);
    jansson_get_exception_ce = zend_register_internal_class_ex(
            &tmp_ce, p_exception_ce, NULL TSRMLS_CC);
    
    INIT_CLASS_ENTRY(tmp_ce, "JanssonConstructorException", NULL);
    jansson_constructor_exception_ce = zend_register_internal_class_ex(
            &tmp_ce, p_exception_ce, NULL TSRMLS_CC);
    
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

