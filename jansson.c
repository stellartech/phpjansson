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

// wip 
// General : https://wiki.php.net/phpng-upgrading
// Classes and objects : 
//	http://jpauli.github.io/2015/03/24/zoom-on-php-objects.html
//	http://jpauli.github.io/2016/01/14/php-7-objects.html

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
php_jansson_ctor(TSRMLS_C)
{
    php_jansson_t *p_jansson = emalloc(sizeof(php_jansson_t) + sizeof(zend_class_entry));
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
    return NULL;
}

static json_t* 
jansson_encode_zval_array_to_jansson(zval *inp_zval TSRMLS_DC) 
{
    return NULL;
}

static json_t* 
jansson_encode_zval_to_jansson(zval *inp_zval TSRMLS_DC) 
{	
    return NULL;
}

static zval*
jansson_to_zval(json_t **inpp_json TSRMLS_DC)
{
    return NULL;
}

static json_t* 
jansson_add_zval_to_jansson(json_t *inp_json, const char *inp_key, 
        zval *inp_zval TSRMLS_DC) 
{
    return NULL;
}

static json_t* 
jansson_add_zval_to_jansson_ex(json_t *inp_json, const char *inp_key, 
        int inn_len, zval *inp_zval TSRMLS_DC) 
{
    return NULL;
}

static zend_bool
jansson_add_zval(json_t *inp_json, zval *inp_key, 
        zval *inp_zval TSRMLS_DC) 
{
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
    int   klen;
    char *inp_key;
    json_t *p_json;
    php_jansson_t *p_this;
    
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", 
            &inp_key, &klen) == FAILURE) {
        RETURN_FALSE;
    }
    
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
    zval *p_zval;
    php_jansson_t *p_this;
    
    if(zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }
    
    p_this = Z_JANSSON_P(getThis());
    if(p_this != NULL) {
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

#ifdef NO_STREAM
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
        zval *return_value, zval *this_ptr, int with_warn TSRMLS_DC)
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
    
    resource.p_this = Z_JANSSON_P(getThis());
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
    
    _jansson_from_stream(inp_zval_src, 
            flags, return_value, getThis(), 0 TSRMLS_CC);
}
#endif /* NO_STREAM */

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
        if(_jansson_from_stream(inp_zval, 0, NULL, getThis(), 0 TSRMLS_CC) != SUCCESS) {
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
    //PHP_ME(jansson, to_stream, 
    //        arginfo_jansson_method_to_stream, ZEND_ACC_PUBLIC)                
    //PHP_ME(jansson, from_stream, 
    //        arginfo_jansson_method_from_stream, ZEND_ACC_PUBLIC)                        
    PHP_FE_END
};

static void
jansson_free_object_storage_handler(zend_object *p_obj)
{
    php_jansson_t *p_intern = jansson_from_obj(p_obj);
    zend_object_std_dtor(&p_intern->std);
    php_jansson_dtor(&p_intern);
}

zend_object*
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

static zend_object
jansson_create_object_handler(zend_class_entry *inp_class_type TSRMLS_DC)
{
    zend_object retval;
    php_jansson_t *p_intern = php_jansson_ctor(TSRMLS_CC);

    zend_object_std_init(&p_intern->std, inp_class_type);
    object_properties_init(&p_intern->std, inp_class_type);
    p_intern->std.handlers = &jansson_object_handlers;
    retval.handlers = &jansson_object_handlers;
    return retval;
}

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("jansson.use_php_memory", "1", PHP_INI_SYSTEM,        
        OnUpdateLong, use_php_memory, zend_jansson_globals, jansson_globals)
    STD_PHP_INI_ENTRY("jansson.seed", "0", PHP_INI_SYSTEM,        
        OnUpdateLong, seed, zend_jansson_globals, jansson_globals)
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
    
    json_object_seed(JANSSON_G(seed));
    
    INIT_NS_CLASS_ENTRY(tmp_ce, PHP_JANSSON_NS, "Jansson", jansson_methods);
    
    jansson_ce = zend_register_internal_class(&tmp_ce TSRMLS_CC);
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

