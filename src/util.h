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

# define DEBUG_INIT(source)     \
    static const char *debugSource = "" source ""

# define DPRINTF(fmt, ...)      \
    debugPrint(debugSource, fmt, __VA_ARGS__)

# define VIR_FREE(ptr) \
    do { \
        free(ptr); \
        ptr = NULL; \
    } while (0)

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

# define VIRT_RETURN_RESOURCE(_resource) \
    RETVAL_RES(_resource)

# define VIRT_REGISTER_RESOURCE(_resource, _le_resource) \
    VIRT_RETURN_RESOURCE(zend_register_resource(_resource, _le_resource))

# define VIRT_REGISTER_LIST_RESOURCE(_name) \
    do { \
        zval zret; \
        ZVAL_RES(&zret, zend_register_resource(res_##_name, le_libvirt_##_name)); \
        add_next_index_zval(return_value, &zret); \
    } while(0)

# define VIRT_RESOURCE_HANDLE(_resource) \
    Z_RES_P(_resource)

# define VIRT_FETCH_RESOURCE(_state, _type, _zval, _name, _le) \
    if ((_state = (_type)zend_fetch_resource(Z_RES_P(*_zval), _name, _le)) == NULL) { \
        RETURN_FALSE; \
    }

# define VIRT_RETVAL_STRING(_str) \
    RETVAL_STRING(_str)

# define VIRT_RETVAL_STRINGL(_str, _len) \
    RETVAL_STRINGL(_str, _len)

# define VIRT_RETURN_STRING(_str) \
    RETURN_STRING(_str)

# define VIRT_RETURN_STRINGL(_str, _len) \
    RETURN_STRINGL(_str, _len)

# define VIRT_ZVAL_STRINGL(_zv, _str, _len) \
    ZVAL_STRINGL(_zv, _str, _len)

# define VIRT_ADD_INDEX_STRING(_arg, _idx, _str) \
    add_index_string(_arg, _idx, _str)

# define VIRT_ADD_NEXT_INDEX_STRING(_arg, _str) \
    add_next_index_string(_arg, _str)

# define VIRT_ADD_ASSOC_STRING(_arg, _key, _str) \
    add_assoc_string(_arg, _key, _str)

# define VIRT_ADD_ASSOC_STRING_WITH_NULL_POINTER_CHECK(_arg, _key, _str) \
    if ((_str)) { \
        add_assoc_string(_arg, _key, _str); \
    } else { \
        add_assoc_null(_arg, _key); \
    }

# define VIRT_ADD_ASSOC_STRING_EX(_arg, _key, _key_len, _value) \
    add_assoc_string_ex(_arg, _key, _key_len, _value)

# define VIRT_FOREACH(_ht, _pos, _zv) \
    for (zend_hash_internal_pointer_reset_ex(_ht, &_pos); \
         (_zv = zend_hash_get_current_data_ex(_ht, &_pos)) != NULL; \
         zend_hash_move_forward_ex(_ht, &_pos)) \

# define VIRT_FOREACH_END(_dummy)

# define VIRT_HASH_CURRENT_KEY_INFO(_ht, _pos, _info) \
    do { \
        zend_string *tmp_name = NULL; \
        _info.type = zend_hash_get_current_key_ex(_ht, &tmp_name, &_info.index, &_pos); \
        if (tmp_name) { \
            _info.name = ZSTR_VAL(tmp_name); \
            _info.length = ZSTR_LEN(tmp_name); \
        } \
    } while(0)

# define VIRT_ARRAY_INIT(_name) \
    zval z##_name; \
    do { \
        _name = &z##_name; \
        array_init(_name); \
    } while(0)

# define LONGLONG_ASSOC(out, key, in)                                          \
    if (LIBVIRT_G(longlong_to_string_ini)) {                                   \
        char _tmpnumber[64] = { 0 };                                           \
        snprintf(_tmpnumber, sizeof(_tmpnumber), "%llu", in);                  \
        VIRT_ADD_ASSOC_STRING(out, key, _tmpnumber);                           \
    } else {                                                                   \
        add_assoc_long(out, key, in);                                          \
    }

# define SIGNED_LONGLONG_ASSOC(out, key, in)                                   \
    if (LIBVIRT_G(signed_longlong_to_string_ini)) {                            \
        char _tmpnumber[64] = { 0 };                                           \
        snprintf(_tmpnumber, sizeof(_tmpnumber), "%lld", in);                  \
        VIRT_ADD_ASSOC_STRING(out, key, _tmpnumber);                           \
    } else {                                                                   \
        add_assoc_long(out, key, in);                                          \
    }

# define LONGLONG_INDEX(out, key, in)                                          \
    if (LIBVIRT_G(longlong_to_string_ini)) {                                   \
        char _tmpnumber[64] = { 0 };                                           \
        snprintf(_tmpnumber, sizeof(_tmpnumber), "%llu", in);                  \
        VIRT_ADD_INDEX_STRING(out, key, _tmpnumber);                           \
    } else {                                                                   \
        add_index_long(out, key, in);                                          \
    }

# define LONGLONG_RETURN_AS_STRING(in)                                         \
    do {                                                                       \
        char _tmpnumber[64] = { 0 };                                           \
        snprintf(_tmpnumber, sizeof(_tmpnumber), "%llu", in);                  \
        VIRT_RETURN_STRING(_tmpnumber);                                        \
    } while (0)

# define VIR_TYPED_PARAMETER_ASSOC(out, param)                                 \
    switch (param.type) {                                                      \
    case VIR_TYPED_PARAM_INT:                                                  \
        add_assoc_long(out, param.field, param.value.i);                       \
        break;                                                                 \
    case VIR_TYPED_PARAM_UINT:                                                 \
        add_assoc_long(out, param.field, param.value.ui);                      \
        break;                                                                 \
    case VIR_TYPED_PARAM_LLONG:                                                \
        add_assoc_long(out, param.field, param.value.l);                       \
        break;                                                                 \
    case VIR_TYPED_PARAM_ULLONG:                                               \
        LONGLONG_ASSOC(out, param.field, param.value.ul);                      \
        break;                                                                 \
    case VIR_TYPED_PARAM_DOUBLE:                                               \
        add_assoc_double(out, param.field, param.value.d);                     \
        break;                                                                 \
    case VIR_TYPED_PARAM_BOOLEAN:                                              \
        add_assoc_bool(out, param.field, param.value.b);                       \
        break;                                                                 \
    case VIR_TYPED_PARAM_STRING:                                               \
        VIRT_ADD_ASSOC_STRING(out, param.field, param.value.s);                \
        break;                                                                 \
    }

# ifndef PHP_FE_END
#  define PHP_FE_END {NULL, NULL, NULL}
# endif

void debugPrint(const char *source,
                const char *fmt, ...);

void setDebug(int level);

#endif /* __UTIL_H__ */
