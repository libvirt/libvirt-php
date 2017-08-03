/*
 * util.h: common, generic utility functions
 *
 * See COPYING for the license of this software
 */

#ifndef __UTIL_H__
# define __UTIL_H__

# include <stdint.h>

# ifdef COMPILE_DL_LIBVIRT
#  undef PACKAGE_BUGREPORT
#  undef PACKAGE_NAME
#  undef PACKAGE_STRING
#  undef PACKAGE_TARNAME
#  undef PACKAGE_URL
#  undef PACKAGE_VERSION
#  include <php.h>

#  ifdef ZTS
#   include <TSRM.h>
#  endif

#  include <php_ini.h>
#  ifdef EXTWIN
#   include <ext/standard/info.h>
#  else
#   include <standard/info.h>
#  endif
# endif

# define DEBUG_SUPPORT

# ifdef DEBUG_SUPPORT
#  define DEBUG_CORE
#  define DEBUG_VNC
# endif

# define DEBUG_INIT(source)     \
    static const char *debugSource = "" source ""

# define DPRINTF(fmt, ...)      \
    debugPrint(debugSource, fmt, __VA_ARGS__)

# define ARRAY_CARDINALITY(array) (sizeof(array) / sizeof(array[0]))

# define IS_BIGENDIAN (*(uint16_t *)"\0\xff" < 0x100)

# define SWAP2_BY_ENDIAN(le, v1, v2)    \
    (((le && IS_BIGENDIAN) || (!le && !IS_BIGENDIAN)) ? ((v2 << 8) + v1) : ((v1 << 8) + v2))

# define PUT2_BYTE_ENDIAN(le, val, v1, v2)                      \
    do {                                                        \
        if ((le && IS_BIGENDIAN) || (!le && !IS_BIGENDIAN)) {   \
            v2 = val >> 8;                                      \
            v1 = val % 256;                                     \
        } else {                                                \
            v1 = val >> 8;                                      \
            v2 = val % 256;                                     \
        }                                                       \
    } while (0)

# define SWAP2_BYTES_ENDIAN(le, a, b)                           \
    do {                                                        \
        if ((le && IS_BIGENDIAN) || (!le && !IS_BIGENDIAN)) {   \
            uint8_t _tmpval;                                    \
            _tmpval = a;                                        \
            a = b;                                              \
            b = _tmpval;                                        \
        }                                                       \
    } while (0)

# define UINT32STR(var, val)        \
    var[0] = (val >> 24) & 0xff;    \
    var[1] = (val >> 16) & 0xff;    \
    var[2] = (val >>  8) & 0xff;    \
    var[3] = (val      ) & 0xff;

# define GETUINT32(var)                     \
    (uint32_t)(((uint32_t)var[0] << 24) +   \
               ((uint32_t)var[1] << 16) +   \
               ((uint32_t)var[2] <<  8) +   \
               ((uint32_t)var[3]))

# if PHP_MAJOR_VERSION >= 7
    typedef size_t strsize_t;
    typedef zend_resource virt_resource;
    typedef virt_resource *virt_resource_handle;

#  define VIRT_RETURN_RESOURCE(_resource) \
    RETVAL_RES(_resource)

#  define VIRT_REGISTER_RESOURCE(_resource, _le_resource)          \
    VIRT_RETURN_RESOURCE(zend_register_resource(_resource, _le_resource))

#  define VIRT_REGISTER_LIST_RESOURCE(_name) do { \
    zval zret; \
    ZVAL_RES(&zret, zend_register_resource(res_##_name, le_libvirt_##_name)); \
    add_next_index_zval(return_value, &zret); \
    } while(0)

#  define VIRT_RESOURCE_HANDLE(_resource) \
    Z_RES_P(_resource)

#  define VIRT_FETCH_RESOURCE(_state, _type, _zval, _name, _le) \
    if ((_state = (_type)zend_fetch_resource(Z_RES_P(*_zval), _name, _le)) == NULL) { \
        RETURN_FALSE; \
    }

#  define VIRT_RETVAL_STRING(_str)    \
    RETVAL_STRING(_str)

#  define VIRT_RETVAL_STRINGL(_str, _len) \
    RETVAL_STRINGL(_str, _len)

#  define VIRT_RETURN_STRING(_str)    \
    RETURN_STRING(_str)

#  define VIRT_RETURN_STRINGL(_str, _len) \
    RETURN_STRINGL(_str, _len)

#  define VIRT_ZVAL_STRINGL(_zv, _str, _len)  \
    ZVAL_STRINGL(_zv, _str, _len)

#  define VIRT_ADD_INDEX_STRING(_arg, _idx, _str)  \
    add_index_string(_arg, _idx, _str)

#  define VIRT_ADD_NEXT_INDEX_STRING(_arg, _str)  \
    add_next_index_string(_arg, _str)

#  define VIRT_ADD_ASSOC_STRING(_arg, _key, _str) \
    add_assoc_string(_arg, _key, _str)

#  define VIRT_ADD_ASSOC_STRING_EX(_arg, _key, _key_len, _value) \
    add_assoc_string_ex(_arg, _key, _key_len, _value)

#  define VIRT_FOREACH(_ht, _pos, _zv) \
    for (zend_hash_internal_pointer_reset_ex(_ht, &_pos); \
         (_zv = zend_hash_get_current_data_ex(_ht, &_pos)) != NULL; \
         zend_hash_move_forward_ex(_ht, &_pos)) \

#  define VIRT_FOREACH_END(_dummy)

#  define VIRT_HASH_CURRENT_KEY_INFO(_ht, _pos, _idx, _info) \
    do { \
    zend_string *tmp_key_info; \
    _info.type = zend_hash_get_current_key_ex(_ht, &tmp_key_info, &_idx, &_pos); \
    _info.name = ZSTR_VAL(tmp_key_info); \
    _info.length = ZSTR_LEN(tmp_key_info); \
    } while(0)

#  define VIRT_ARRAY_INIT(_name) do { \
    zval z##_name; \
    _name = &z##_name; \
    array_init(_name); \
    } while(0)

# else /* PHP_MAJOR_VERSION < 7 */
    typedef int strsize_t;
    typedef long zend_long;
    typedef unsigned long zend_ulong;
    typedef zend_rsrc_list_entry virt_resource;
    typedef long virt_resource_handle;

#  define VIRT_RETURN_RESOURCE(_resource) \
    RETVAL_RESOURCE((long) _resource)

#  define VIRT_REGISTER_RESOURCE(_resource, _le_resource) \
    ZEND_REGISTER_RESOURCE(return_value, _resource, _le_resource)

#  define VIRT_REGISTER_LIST_RESOURCE(_name) do { \
    zval *zret; \
    ALLOC_INIT_ZVAL(zret); \
    ZEND_REGISTER_RESOURCE(zret, res_##_name, le_libvirt_##_name); \
    add_next_index_zval(return_value, zret); \
    } while(0)

#  define VIRT_RESOURCE_HANDLE(_resource) \
    Z_LVAL_P(_resource)

#  define VIRT_FETCH_RESOURCE(_state, _type, _zval, _name, _le) \
    ZEND_FETCH_RESOURCE(_state, _type, _zval, -1, _name, _le);

#  define VIRT_RETVAL_STRING(_str)    \
    RETVAL_STRING(_str, 1)

#  define VIRT_RETVAL_STRINGL(_str, _len) \
    RETVAL_STRINGL(_str, _len, 1)

#  define VIRT_RETURN_STRING(_str)    \
    RETURN_STRING(_str, 1)

#  define VIRT_RETURN_STRINGL(_str, _len) \
    RETURN_STRINGL(_str, _len, 1)

#  define VIRT_ZVAL_STRINGL(_zv, _str, _len)  \
    ZVAL_STRINGL(_zv, _str, _len, 1)

#  define VIRT_ADD_INDEX_STRING(_arg, _idx, _str)  \
    add_index_string(_arg, _idx, _str, 1)

#  define VIRT_ADD_NEXT_INDEX_STRING(_arg, _str)  \
    add_next_index_string(_arg, _str, 1)

#  define VIRT_ADD_ASSOC_STRING(_arg, _key, _str) \
    add_assoc_string(_arg, _key, _str, 1)

#  define VIRT_ADD_ASSOC_STRING_EX(_arg, _key, _key_len, _value) \
    add_assoc_string_ex(_arg, _key, _key_len, _value, 1)

#  define VIRT_FOREACH(_ht, _pos, _zv) \
    { \
    zval **pzv = &_zv; \
    for (zend_hash_internal_pointer_reset_ex(_ht, &_pos); \
         zend_hash_get_current_data_ex(_ht, (void **) &pzv, &_pos) == SUCCESS; \
         zend_hash_move_forward_ex(_ht, &_pos)) { \
        _zv = *pzv;

#  define VIRT_FOREACH_END(_dummy) \
    }}

#  define VIRT_HASH_CURRENT_KEY_INFO(_ht, _pos, _idx, _info) \
    do { \
    _info.type = zend_hash_get_current_key_ex(_ht, &_info.name, &_info.length, &_idx, 0, &_pos); \
    } while(0)

#  define VIRT_ARRAY_INIT(_name) do {\
    ALLOC_INIT_ZVAL(_name); \
    array_init(_name); \
    } while(0)

# endif /* PHP_MAJOR_VERSION < 7 */

# ifndef PHP_FE_END
#  define PHP_FE_END {NULL, NULL, NULL}
# endif

void debugPrint(const char *source,
                const char *fmt, ...);

void setDebug(int level);

#endif /* __UTIL_H__ */
