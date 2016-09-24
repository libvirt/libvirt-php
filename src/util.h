/*
 * util.h: common, generic utility functions
 *
 * See COPYING for the license of this software
 *
 * Written by:
 *      Michal Privoznik <mprivozn@redhat.com>
 */

#ifndef __UTIL_H__
# define __UTIL_H__

# include <stdint.h>

# define DEBUG_SUPPORT

# ifdef DEBUG_SUPPORT
#  define DEBUG_CORE
#  define DEBUG_VNC
extern int gdebug;
# endif

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

#endif /* __UTIL_H__ */
