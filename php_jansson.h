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

#ifndef PHP_JANSSON_H_INCLUDED
#define PHP_JANSSON_H_INCLUDED

#include "php.h"

extern zend_module_entry jansson_module_entry;
#define phpext_extname_ptr &jansson_module_entry

extern PHPAPI zend_class_entry *jansson_get_exception_ce;
extern PHPAPI zend_class_entry *jansson_constructor_exception_ce;

#define PHP_JANSSON_VERSION "0.1.0"
#define PHP_JANSSON_NS	"Jansson"

#ifdef PHP_WIN32
#define PHP_JANSSON_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define PHP_JANSSON_API __attribute__ ((visibility("default")))
#else
#define PHP_JANSSON_API
#endif

ZEND_BEGIN_MODULE_GLOBALS(jansson)
    long seed;
    long use_php_memory;
ZEND_END_MODULE_GLOBALS(jansson)

#ifdef ZTS
#include "TSRM.h"
#endif

#ifdef ZTS
#define JANSSON_G(v) TSRMG(jansson_globals_id, zend_jansson_globals *, v)
extern int jansson_globals_id;
#else
#define JANSSON_G(v) (jansson_globals.v)
extern zend_jansson_globals jansson_globals;
#endif

#endif /* PHP_JANSSON_H_INCLUDED */

