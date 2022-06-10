/*
 * libvirt-php.h: libvirt PHP bindings header file
 *
 * See COPYING for the license of this software
 */

#ifndef PHP_LIBVIRT_H
#define PHP_LIBVIRT_H 1

/* Network constants */
#define VIR_NETWORKS_ACTIVE     1
#define VIR_NETWORKS_INACTIVE       2

/* Version constants */
#define VIR_VERSION_BINDING     1
#define VIR_VERSION_LIBVIRT     2

#ifdef _MSC_VER
#define EXTWIN
#endif

#ifdef EXTWIN
#define COMPILE_DL_LIBVIRT
#endif

#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <libvirt/libvirt-qemu.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <fcntl.h>
#include <sys/types.h>
#include "util.h"

#ifndef EXTWIN
#include <inttypes.h>
#include <dirent.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <stdint.h>
#include <libgen.h>

#else

#define PRIx32       "I32x"
#define PRIx64       "I64x"

#ifdef EXTWIN
#if (_MSC_VER < 1300)
typedef signed char       int8_t;
typedef signed short      int16_t;
typedef signed int        int32_t;
typedef unsigned char     uint8_t;
typedef unsigned short    uint16_t;
typedef unsigned int      uint32_t;
#else
typedef signed __int8     int8_t;
typedef signed __int16    int16_t;
typedef signed __int32    int32_t;
typedef unsigned __int8   uint8_t;
typedef unsigned __int16  uint16_t;
typedef unsigned __int32  uint32_t;
#endif
typedef signed __int64       int64_t;
typedef unsigned __int64     uint64_t;
#endif

#endif

#ifdef __i386__
typedef uint32_t arch_uint;
#define UINTx PRIx32
#else
typedef uint64_t arch_uint;
#define UINTx PRIx64
#endif

# define DEBUG_SUPPORT

# ifdef DEBUG_SUPPORT
#  define DEBUG_CORE
#  define DEBUG_VNC
# endif

/* PHP functions are prefixed with `zif_` so strip it */
# define PHPFUNC (__FUNCTION__ + 4)

#ifdef ZTS
#define LIBVIRT_G(v) TSRMG(libvirt_globals_id, zend_libvirt_globals *, v)
#else
#define LIBVIRT_G(v) (libvirt_globals.v)
#endif

#define PHP_LIBVIRT_WORLD_VERSION VERSION
#define PHP_LIBVIRT_WORLD_EXTNAME "libvirt"

typedef struct tTokenizer {
    char **tokens;
    int numTokens;
} tTokenizer;

typedef struct _resource_info {
    int type;
    virConnectPtr conn;
    void *mem;
    int overwrite;
} resource_info;

typedef struct tVMDisk {
    char *path;
    char *driver;
    char *bus;
    char *dev;
    unsigned long long size;
    int flags;
} tVMDisk;

typedef struct tVMNetwork {
    char *mac;
    char *network;
    char *model;
} tVMNetwork;

typedef struct _php_libvirt_hash_key_info {
    char *name;
    unsigned int length;
    zend_ulong index;
    unsigned int type;
} php_libvirt_hash_key_info;

ZEND_BEGIN_MODULE_GLOBALS(libvirt)
    char *last_error;
    char *vnc_location;
    zend_bool longlong_to_string_ini;
    zend_bool signed_longlong_to_string_ini;
    char *iso_path_ini;
    char *image_path_ini;
    zend_long max_connections_ini;
# ifdef DEBUG_SUPPORT
    int debug;
# endif
    resource_info *binding_resources;
    int binding_resources_count;
ZEND_END_MODULE_GLOBALS(libvirt)

ZEND_EXTERN_MODULE_GLOBALS(libvirt)

/* Private definitions */
void set_error(char *msg);
void set_error_if_unset(char *msg);
void reset_error(void);
int count_resources(int type);
int resource_change_counter(int type, virConnectPtr conn, void *mem,
                            int inc);
int check_resource_allocation(virConnectPtr conn, int type,
                              void *mem);
void free_resource(int type, void *mem);
char *connection_get_emulator(virConnectPtr conn, char *arch);
int is_local_connection(virConnectPtr conn);
tTokenizer tokenize(char *string, char *by);
void free_tokens(tTokenizer t);
int set_logfile(char *filename, long maxsize);
char *get_string_from_xpath(char *xml, char *xpath, zval **val, int *retVal);
char *get_node_string_from_xpath(char *xml, char *xpath);
char **get_array_from_xpath(char *xml, char *xpath, int *num);
void parse_array(zval *arr, tVMDisk *disk, tVMNetwork *network);
char *installation_get_xml(virConnectPtr conn, char *name, int memMB,
                           int maxmemMB, char *arch, char *uuid, int vCpus,
                           char *iso_image, tVMDisk *disks, int numDisks,
                           tVMNetwork *networks, int numNetworks,
                           int domain_flags);
void set_vnc_location(char *msg);
int streamSink(virStreamPtr st ATTRIBUTE_UNUSED,
               const char *bytes, size_t nbytes, void *opaque);
const char *get_feature_binary(const char *name);
long get_next_free_numeric_value(virDomainPtr domain, char *xpath);
int get_subnet_bits(char *ip);

PHP_MINIT_FUNCTION(libvirt);
PHP_MSHUTDOWN_FUNCTION(libvirt);
PHP_RINIT_FUNCTION(libvirt);
PHP_RSHUTDOWN_FUNCTION(libvirt);
PHP_MINFO_FUNCTION(libvirt);

/* Common functions */
PHP_FUNCTION(libvirt_get_last_error);
PHP_FUNCTION(libvirt_version);
PHP_FUNCTION(libvirt_check_version);
PHP_FUNCTION(libvirt_has_feature);
PHP_FUNCTION(libvirt_get_iso_images);
PHP_FUNCTION(libvirt_image_create);
PHP_FUNCTION(libvirt_image_remove);
/* Debugging functions */
PHP_FUNCTION(libvirt_logfile_set);
PHP_FUNCTION(libvirt_print_binding_resources);

extern zend_module_entry libvirt_module_entry;
#define phpext_libvirt_ptr &libvirt_module_entry

#endif
