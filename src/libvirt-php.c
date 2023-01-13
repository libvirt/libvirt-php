/*
 * libvirt-php.c: Core of the PHP bindings library/module
 *
 * See COPYING for the license of this software
 */

#ifdef _MSC_VER
#define EXTWIN
#endif

#include <config.h>
#include <stdio.h>

#ifdef EXTWIN
#define PHP_COMPILER_ID  "VC9"
#endif

#include "libvirt-php.h"
#include "vncfunc.h"
#include "sockets.h"
#include "libvirt-connection.h"
#include "libvirt-domain.h"
#include "libvirt-network.h"
#include "libvirt-node.h"
#include "libvirt-nodedev.h"
#include "libvirt-nwfilter.h"
#include "libvirt-snapshot.h"
#include "libvirt-storage.h"
#include "libvirt-stream.h"

DEBUG_INIT("core");

ZEND_DECLARE_MODULE_GLOBALS(libvirt)

#ifndef EXTWIN
/* Additional binaries */
const char *features[] = { "screenshot", "create-image", "screenshot-convert", NULL };
const char *features_binaries[] = { "/usr/bin/gvnccapture", "/usr/bin/qemu-img", "/bin/convert", NULL };
#else
const char *features[] = { NULL };
const char *features_binaries[] = { NULL };
#endif

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_connect, 0, 0, 0)
ZEND_ARG_INFO(0, url)
ZEND_ARG_INFO(0, readonly)
ZEND_ARG_INFO(0, credentials)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_void, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_conn, 0, 0, 1)
ZEND_ARG_INFO(0, conn)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_conn_xpath, 0, 0, 1)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, xpath)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_connect_get_domain_capabilities, 0, 0, 1)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, emulatorbin)
ZEND_ARG_INFO(0, arch)
ZEND_ARG_INFO(0, machine)
ZEND_ARG_INFO(0, virttype)
ZEND_ARG_INFO(0, flags)
ZEND_ARG_INFO(0, xpath)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_connect_get_emulator, 0, 0, 1)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, arch)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_connect_get_soundhw_models, 0, 0, 1)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, arch)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_connect_get_all_domain_stats, 0, 0, 1)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, stats)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_stream_send, 0, 1, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(1, data)
ZEND_ARG_INFO(0, len)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_stream_recv, 0, 1, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(1, data)
ZEND_ARG_INFO(0, len)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_new, 0, 0, 9)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, name)
ZEND_ARG_INFO(0, arch)
ZEND_ARG_INFO(0, memMB)
ZEND_ARG_INFO(0, maxmemMB)
ZEND_ARG_INFO(0, vcpus)
ZEND_ARG_INFO(0, iso)
ZEND_ARG_INFO(0, disks)
ZEND_ARG_INFO(0, networks)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_change_vcpus, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, numCpus)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_change_memory, 0, 0, 3)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, allocMem)
ZEND_ARG_INFO(0, allocMax)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_change_boot_devices, 0, 0, 3)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, first)
ZEND_ARG_INFO(0, second)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_disk_add, 0, 0, 5)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, img)
ZEND_ARG_INFO(0, dev)
ZEND_ARG_INFO(0, type)
ZEND_ARG_INFO(0, driver)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_disk_remove, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, dev)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_nic_add, 0, 0, 4)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, mac)
ZEND_ARG_INFO(0, network)
ZEND_ARG_INFO(0, model)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_nic_remove, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, dev)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_attach_device, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, xml)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_detach_device, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, xml)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_lookup_by_id, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, id)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_conn_uuid, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, uuid)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_conn_xml, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, xml)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_core_dump, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, to)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_conn_flags, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_xml_from_native, 0, 0, 3)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, format)
ZEND_ARG_INFO(0, config_data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_xml_to_native, 0, 0, 3)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, format)
ZEND_ARG_INFO(0, xml_data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_memory_peek, 0, 0, 4)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, start)
ZEND_ARG_INFO(0, size)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_set_memory, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, memory)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_set_memory_flags, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, memory)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_block_commit, 0, 0, 2)
ZEND_ARG_INFO(0, res)
ZEND_ARG_INFO(0, disk)
ZEND_ARG_INFO(0, base)
ZEND_ARG_INFO(0, top)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_conn_path, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_block_resize, 0, 0, 3)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, path)
ZEND_ARG_INFO(0, size)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_block_job_abort, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, path)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_block_job_set_speed, 0, 0, 3)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, path)
ZEND_ARG_INFO(0, bandwidth)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_interface_addresses, 0, 0, 2)
ZEND_ARG_INFO(0, domain)
ZEND_ARG_INFO(0, source)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_block_job_info, 0, 0, 2)
ZEND_ARG_INFO(0, dom)
ZEND_ARG_INFO(0, disk)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_migrate, 0, 0, 3)
ZEND_ARG_INFO(0, res)
ZEND_ARG_INFO(0, dest_conn)
ZEND_ARG_INFO(0, flags)
ZEND_ARG_INFO(0, dname)
ZEND_ARG_INFO(0, bandwidth)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_migrate_to_uri, 0, 0, 3)
ZEND_ARG_INFO(0, res)
ZEND_ARG_INFO(0, dest_uri)
ZEND_ARG_INFO(0, flags)
ZEND_ARG_INFO(0, dname)
ZEND_ARG_INFO(0, bandwidth)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_migrate_to_uri2, 0, 0, 1)
ZEND_ARG_INFO(0, res)
ZEND_ARG_INFO(0, dconn_uri)
ZEND_ARG_INFO(0, mig_uri)
ZEND_ARG_INFO(0, dxml)
ZEND_ARG_INFO(0, flags)
ZEND_ARG_INFO(0, dname)
ZEND_ARG_INFO(0, bandwidth)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_xml_xpath, 0, 0, 2)
ZEND_ARG_INFO(0, res)
ZEND_ARG_INFO(0, xpath)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_get_block_info, 0, 0, 2)
ZEND_ARG_INFO(0, res)
ZEND_ARG_INFO(0, dev)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_get_network_info, 0, 0, 2)
ZEND_ARG_INFO(0, res)
ZEND_ARG_INFO(0, mac)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_get_metadata, 0, 0, 4)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, type)
ZEND_ARG_INFO(0, uri)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_set_metadata, 0, 0, 6)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, type)
ZEND_ARG_INFO(0, metadata)
ZEND_ARG_INFO(0, key)
ZEND_ARG_INFO(0, uri)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_get_screenshot, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, server)
ZEND_ARG_INFO(0, scancode)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_get_screenshot_api, 0, 0, 1)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, screenID)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_get_screen_dimensions, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, server)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_send_keys, 0, 0, 3)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, server)
ZEND_ARG_INFO(0, scancode)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_send_key_api, 0, 0, 4)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, codeset)
ZEND_ARG_INFO(0, holdime)
ZEND_ARG_INFO(0, keycodes)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_send_pointer_event, 0, 0, 5)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, server)
ZEND_ARG_INFO(0, pos_x)
ZEND_ARG_INFO(0, pox_y)
ZEND_ARG_INFO(0, clicked)
ZEND_ARG_INFO(0, release)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_update_device, 0, 0, 3)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, xml)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_conn_optflags, 0, 0, 1)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_snapshot_lookup_by_name, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, name)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_conn_name, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_storagevolume_lookup_by_path, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_storagevolume_get_xml_desc, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, xpath)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_storagevolume_create_xml_from, 0, 0, 3)
ZEND_ARG_INFO(0, pool)
ZEND_ARG_INFO(0, xml)
ZEND_ARG_INFO(0, original_volume)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_storagevolume_download, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, stream)
ZEND_ARG_INFO(0, offset)
ZEND_ARG_INFO(0, length)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_storagevolume_resize, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, capacity)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_storagepool_define_xml, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, xml)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_conn_optcpunr, 0, 0, 1)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, cpunr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_conn_opttime, 0, 0, 1)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, time)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_conn_optcap, 0, 0, 1)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, cap)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_opttype, 0, 0, 0)
ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_check_version, 0, 0, 3)
ZEND_ARG_INFO(0, major)
ZEND_ARG_INFO(0, minor)
ZEND_ARG_INFO(0, micro)
ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_name, 0, 0, 1)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_optpath, 0, 0, 0)
ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_image_create, 0, 0, 4)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, name)
ZEND_ARG_INFO(0, size)
ZEND_ARG_INFO(0, format)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_conn_image, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, image)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_logfile_set, 0, 0, 1)
ZEND_ARG_INFO(0, filename)
ZEND_ARG_INFO(0, maxsize)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_domain_qemu_agent_command, 0, 0, 2)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, cmd)
ZEND_ARG_INFO(0, timeout)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_libvirt_network_get_dhcp_leases, 0, 0, 1)
ZEND_ARG_INFO(0, conn)
ZEND_ARG_INFO(0, mac)
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

static zend_function_entry libvirt_functions[] = {
    PHP_FE_LIBVIRT_CONNECTION
    PHP_FE_LIBVIRT_STREAM
    PHP_FE_LIBVIRT_DOMAIN
    PHP_FE_LIBVIRT_SNAPSHOT
    PHP_FE_LIBVIRT_STORAGE
    PHP_FE_LIBVIRT_NETWORK
    PHP_FE_LIBVIRT_NODE
    PHP_FE_LIBVIRT_NODEDEV
    PHP_FE_LIBVIRT_NWFILTER
    /* Common functions */
    PHP_FE(libvirt_get_last_error,               arginfo_libvirt_void)
    /* Version information and common function */
    PHP_FE(libvirt_version,                      arginfo_libvirt_opttype)
    PHP_FE(libvirt_check_version,                arginfo_libvirt_check_version)
    PHP_FE(libvirt_has_feature,                  arginfo_libvirt_name)
    PHP_FE(libvirt_get_iso_images,               arginfo_libvirt_optpath)
    PHP_FE(libvirt_image_create,                 arginfo_libvirt_image_create)
    PHP_FE(libvirt_image_remove,                 arginfo_libvirt_conn_image)
    /* Debugging functions */
    PHP_FE(libvirt_logfile_set,                  arginfo_libvirt_logfile_set)
    PHP_FE(libvirt_print_binding_resources,      arginfo_libvirt_void)
    PHP_FE_END
};

/* Zend module basic definition  */
zend_module_entry libvirt_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_LIBVIRT_WORLD_EXTNAME,
    libvirt_functions,
    PHP_MINIT(libvirt),
    PHP_MSHUTDOWN(libvirt),
    PHP_RINIT(libvirt),
    PHP_RSHUTDOWN(libvirt),
    PHP_MINFO(libvirt),
#if ZEND_MODULE_API_NO >= 20010901
    PHP_LIBVIRT_WORLD_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_LIBVIRT
ZEND_GET_MODULE(libvirt)
#endif

/* PHP init options */
PHP_INI_BEGIN()
STD_PHP_INI_ENTRY("libvirt.longlong_to_string", "1", PHP_INI_ALL, OnUpdateBool, longlong_to_string_ini, zend_libvirt_globals, libvirt_globals)
STD_PHP_INI_ENTRY("libvirt.signed_longlong_to_string", "0", PHP_INI_ALL, OnUpdateBool, signed_longlong_to_string_ini, zend_libvirt_globals, libvirt_globals)
STD_PHP_INI_ENTRY("libvirt.iso_path", "/var/lib/libvirt/images/iso", PHP_INI_ALL, OnUpdateString, iso_path_ini, zend_libvirt_globals, libvirt_globals)
STD_PHP_INI_ENTRY("libvirt.image_path", "/var/lib/libvirt/images", PHP_INI_ALL, OnUpdateString, image_path_ini, zend_libvirt_globals, libvirt_globals)
STD_PHP_INI_ENTRY("libvirt.max_connections", "5", PHP_INI_ALL, OnUpdateLong, max_connections_ini, zend_libvirt_globals, libvirt_globals)
PHP_INI_END()

void change_debug(int val)
{
#ifdef DEBUG_SUPPORT
    LIBVIRT_G(debug) = val;
#endif
    setDebug(val);
}

/* PHP requires to have this function defined */
static void php_libvirt_init_globals(zend_libvirt_globals *libvirt_globals)
{
    libvirt_globals->longlong_to_string_ini = 1;
    libvirt_globals->signed_longlong_to_string_ini = 0;
    libvirt_globals->iso_path_ini = "/var/lib/libvirt/images/iso";
    libvirt_globals->image_path_ini = "/var/lib/libvirt/images";
    libvirt_globals->max_connections_ini = 5;
    libvirt_globals->binding_resources_count = 0;
    libvirt_globals->binding_resources = NULL;
#ifdef DEBUG_SUPPORT
    libvirt_globals->debug = 0;
    change_debug(0);
#endif
}

/* PHP request initialization */
PHP_RINIT_FUNCTION(libvirt)
{
    LIBVIRT_G(last_error) = NULL;
    LIBVIRT_G(vnc_location) = NULL;
    change_debug(0);
    return SUCCESS;
}

/* PHP request destruction */
PHP_RSHUTDOWN_FUNCTION(libvirt)
{
    if (LIBVIRT_G(last_error) != NULL)
        efree(LIBVIRT_G(last_error));
    if (LIBVIRT_G(vnc_location) != NULL)
        efree(LIBVIRT_G(vnc_location));
    return SUCCESS;
}

/*
 * Private function name:   set_logfile
 * Since version:           0.4.2
 * Description:             Function to set the log file. You can set log file to NULL to disable logging (default). Useful for debugging purposes.
 * Arguments:               @filename [string]: name of log file or NULL to disable
 *                          @maxsize [long]: integer value of maximum file size, file will be truncated after reaching max file size. Value is set in KiB.
 * Returns:                 0 on success, -errno otherwise
 */
int set_logfile(char *filename, long maxsize)
{
    int res;
    struct stat st;

    if (filename == NULL) {
        change_debug(0);
        return 0;
    }

    /* Convert from KiB to bytes and check whether file size exceeds maxsize */
    maxsize *= 1024;
    if (access(filename, F_OK) == 0) {
        stat(filename, &st);
        if (st.st_size > maxsize)
            unlink(filename);
    }

    res = (freopen(filename, "a", stderr) != NULL) ? 0 : -errno;
    if (res == 0)
        change_debug(1);
    return res;
}

/*
 * Private function name:   translate_counter_type
 * Since version:           0.4.2
 * Description:             Function to translate the counter type into the string format
 * Arguments:               @type [int]: integer identifier of the counter type
 * Returns:                 string interpretation of the counter type
 */
static const char *
translate_counter_type(int type)
{
    switch (type) {
    case INT_RESOURCE_CONNECTION:
        return "connection";
    case INT_RESOURCE_DOMAIN:
        return "domain";
    case INT_RESOURCE_STREAM:
        return "stream";
    case INT_RESOURCE_NETWORK:
        return "network";
    case INT_RESOURCE_NODEDEV:
        return "node device";
    case INT_RESOURCE_STORAGEPOOL:
        return "storage pool";
    case INT_RESOURCE_VOLUME:
        return "storage volume";
    case INT_RESOURCE_SNAPSHOT:
        return "snapshot";
    case INT_RESOURCE_NWFILTER:
        return "nwfilter";
    }

    return "unknown";
}

/*
 * Private function name:   tokenize
 * Since version:           0.4.9
 * Description:             Function to tokenize string into tokens by delimiter $delim
 * Arguments:               @string [string]: input string
 *                          @delim [string]: string used as delimiter
 * Returns:                 tTokenizer structure
 */
tTokenizer tokenize(const char *string, char *delim)
{
#ifndef EXTWIN
    char *tmp;
    char *str;
    char *save;
    char *token;
    int i = 0;
    tTokenizer t;

    tmp = strdup(string);
    t.tokens = (char **)malloc(sizeof(char *));
    for (str = tmp; ; str = NULL) {
        token = strtok_r(str, delim, &save);
        if (token == NULL)
            break;

        t.tokens = realloc(t.tokens, (i + 1) * sizeof(char *));
        t.tokens[i++] = strdup(token);
    }
    VIR_FREE(tmp);

    t.numTokens = i;
    return t;
#else
    tTokenizer t;

    t.tokens = NULL;
    t.numTokens = 0;
    return t;
#endif
}

/*
 * Private function name:   free_tokens
 * Since version:           0.4.9
 * Description:             Function to free tokens allocated by tokenize function
 * Arguments:               @t [tTokenizer]: tokenizer structure previously allocated by tokenize function
 * Returns:                 none
 */
void free_tokens(tTokenizer t)
{
    int i;

    for (i = 0; i < t.numTokens; i++)
        VIR_FREE(t.tokens[i]);
    VIR_FREE(t.tokens);
    t.numTokens = 0;
}

/*
 * Private function name:   resource_change_counter
 * Since version:           0.4.2
 * Description:             Function to increment (inc = 1) / decrement (inc = 0) the resource pointers count including the memory location
 * Arguments:               @type [int]: type of resource (INT_RESOURCE_x defines where x can be { CONNECTION | DOMAIN | NETWORK | NODEDEV | STORAGEPOOL | VOLUME | SNAPSHOT | STREAM })
 *                          @conn [virConnectPtr]: libvirt connection pointer associated with the resource, NULL for libvirt connection objects
 *                          @mem [pointer]: Pointer to memory location for the resource. Will be converted to appropriate uint for the arch.
 *                          @inc [int/bool]: Increment the counter (1 = add memory location) or decrement the counter (0 = remove memory location) from entries.
 * Returns:                 0 on success, -errno otherwise
 */
int resource_change_counter(int type, virConnectPtr conn, void *mem, int inc)
{
    int i;
    int pos = -1;
    int binding_resources_count;
    resource_info *binding_resources = NULL;

    binding_resources_count = LIBVIRT_G(binding_resources_count);
    binding_resources = LIBVIRT_G(binding_resources);

    if (inc) {
        for (i = 0; i < binding_resources_count; i++) {
            if (binding_resources[i].overwrite) {
                pos = i;
                break;
            }
            if (pos > -1)
                DPRINTF("%s: Found match on position %d\n", __FUNCTION__, pos);
            if ((binding_resources[i].type == type) && (binding_resources[i].mem == mem)
                && (binding_resources[i].overwrite == 0)) {
                DPRINTF("%s: Pointer exists at position %d\n", __FUNCTION__, i);
                return -EEXIST;
            }
        }

        if (pos == -1) {
            if (binding_resources == NULL) {
                binding_resources_count = 1;
                binding_resources = (resource_info *)malloc(sizeof(resource_info));
            } else {
                binding_resources_count++;
                binding_resources = (resource_info *)realloc(binding_resources, binding_resources_count * sizeof(resource_info));
            }

            if (binding_resources == NULL)
                return -ENOMEM;

            pos = binding_resources_count - 1;
        }

        binding_resources[pos].type = type;
        binding_resources[pos].mem  = mem;
        binding_resources[pos].conn = conn;
        binding_resources[pos].overwrite = 0;
    } else {
        for (i = 0; i < binding_resources_count; i++) {
            if ((binding_resources[i].type == type) && (binding_resources[i].mem == mem))
                binding_resources[i].overwrite = 1;
        }
    }

    LIBVIRT_G(binding_resources_count) = binding_resources_count;
    LIBVIRT_G(binding_resources) = binding_resources;

    return 0;
}

/*
 * Private function name:   get_feature_binary
 * Since version:           0.4.1(-3)
 * Description:             Function to get the existing feature binary for the specified feature, e.g. screenshot feature
 * Arguments:               @name [string]: name of the feature to check against
 * Returns:                 Existing and executable binary name or NULL value
 */
const char *get_feature_binary(const char *name)
{
    int i, max;

    max = (ARRAY_CARDINALITY(features) < ARRAY_CARDINALITY(features_binaries)) ?
        ARRAY_CARDINALITY(features) : ARRAY_CARDINALITY(features_binaries);

    for (i = 0; i < max; i++)
        if ((features[i] != NULL) && (strcmp(features[i], name) == 0)) {
            if (access(features_binaries[i], X_OK) == 0)
                return features_binaries[i];
        }

    return NULL;
}

/*
 * Private function name:   has_builtin
 * Since version:           0.4.5
 * Description:             Function to get the information whether feature could be used as a built-in feature or not
 * Arguments:               @name [string]: name of the feature to check against
 * Returns:                 1 if feature has a builtin fallback to be used or 0 otherwise
 */
int has_builtin(char *name)
{
    int i, max;

    max = (ARRAY_CARDINALITY(features) < ARRAY_CARDINALITY(features_binaries)) ?
        ARRAY_CARDINALITY(features) : ARRAY_CARDINALITY(features_binaries);

    for (i = 0; i < max; i++)
        if ((features[i] != NULL) && (strcmp(features[i], name) == 0))
            return 1;

    return 0;
}

/* Maximum size (in KiB) of log file when DEBUG_SUPPORT is enabled */
#define DEFAULT_LOG_MAXSIZE 1024

/* Information function for phpinfo() */
PHP_MINFO_FUNCTION(libvirt)
{
    int i;
    char path[1024];
    char tmp[1024] = { 0 };
    unsigned long libVer;
    php_info_print_table_start();
    php_info_print_table_row(2, "Libvirt support", "enabled");

#ifdef DEBUG_SUPPORT
    snprintf(tmp, sizeof(tmp), "enabled, default maximum log file size: %d KiB", DEFAULT_LOG_MAXSIZE);
#else
    snprintf(tmp, sizeof(tmp), "disabled");
#endif

    php_info_print_table_row(2, "Debug support", tmp);
    php_info_print_table_row(2, "Extension version", PHP_LIBVIRT_WORLD_VERSION);

    if (virGetVersion(&libVer, NULL, NULL) == 0) {
        char version[100];
        snprintf(version, sizeof(version), "%ld.%ld.%ld", (long)((libVer/1000000) % 1000), (long)((libVer/1000) % 1000), (long)(libVer % 1000));
        php_info_print_table_row(2, "Libvirt version", version);
    }

    snprintf(path, sizeof(path), "%lu", (unsigned long)LIBVIRT_G(max_connections_ini));
    php_info_print_table_row(2, "Max. connections", path);

    if (!(access(LIBVIRT_G(iso_path_ini), F_OK) == 0))
        snprintf(path, sizeof(path), "%s - path is invalid. To set the valid path modify the libvirt.iso_path in your php.ini configuration!",
                 LIBVIRT_G(iso_path_ini));
    else
        snprintf(path, sizeof(path), "%s", LIBVIRT_G(iso_path_ini));

    php_info_print_table_row(2, "ISO Image path", path);

    if (!(access(LIBVIRT_G(image_path_ini), F_OK) == 0))
        snprintf(path, sizeof(path), "%s - path is invalid. To set the valid path modify the libvirt.image_path in your php.ini configuration!",
                 LIBVIRT_G(image_path_ini));
    else
        snprintf(path, sizeof(path), "%s", LIBVIRT_G(image_path_ini));

    php_info_print_table_row(2, "Path for images", path);

    /* Iterate all the features supported */
    char features_supported[4096] = { 0 };
    for (i = 0; i < ARRAY_CARDINALITY(features); i++) {
        const char *tmp;
        if ((features[i] != NULL) && (tmp = get_feature_binary(features[i]))) {
            strcat(features_supported, features[i]);
            strcat(features_supported, ", ");
        }
    }

    if (strlen(features_supported) > 0) {
        features_supported[strlen(features_supported) - 2] = 0;
        php_info_print_table_row(2, "Features supported", features_supported);
    }

    php_info_print_table_end();
}

/*
 * Private function name:   set_error
 * Since version:           0.4.1(-1)
 * Description:             This private function is used to set the error string to the library. This string can be obtained by libvirt_get_last_error() from the PHP application.
 * Arguments:               @msg [string]: error message string
 * Returns:                 None
 */
void set_error(char *msg)
{
    if (LIBVIRT_G(last_error) != NULL)
        efree(LIBVIRT_G(last_error));

    if (msg == NULL) {
        LIBVIRT_G(last_error) = NULL;
        return;
    }

    php_error_docref(NULL, E_WARNING, "%s", msg);
    LIBVIRT_G(last_error) = estrndup(msg, strlen(msg));
}

/*
 * Private function name:   set_vnc_location
 * Since version:           0.4.5
 * Description:             This private function is used to set the VNC location for the newly started installation
 * Arguments:               @msg [string]: vnc location string
 * Returns:                 None
 */
void set_vnc_location(char *msg)
{
    if (LIBVIRT_G(vnc_location) != NULL)
        efree(LIBVIRT_G(vnc_location));

    if (msg == NULL) {
        LIBVIRT_G(vnc_location) = NULL;
        return;
    }

    LIBVIRT_G(vnc_location) = estrndup(msg, strlen(msg));

    DPRINTF("set_vnc_location: VNC server location set to '%s'\n", LIBVIRT_G(vnc_location));
}

/*
 * Private function name:   set_error_if_unset
 * Since version:           0.4.2
 * Description:             Function to set the error only if no other error is set yet
 * Arguments:               @msg [string]: error message string
 * Returns:                 None
 */
void set_error_if_unset(char *msg)
{
    if (LIBVIRT_G(last_error) == NULL)
        set_error(msg);
}

/*
 * Private function name:   reset_error
 * Since version:           0.4.2
 * Description:             Function to reset the error string set by set_error(). Same as set_error(NULL).
 * Arguments:               None
 * Returns:                 None
 */
void reset_error(void)
{
    set_error(NULL);
}


/* Error handler for receiving libvirt errors */
static void catch_error(void *userData ATTRIBUTE_UNUSED,
                        virErrorPtr error)
{
    set_error(error->message);
}

/*
 * Private function name:   free_resource
 * Since version:           0.4.2
 * Description:             Function is used to free the the internal libvirt-php resource identified by it's type and memory location
 * Arguments:               @type [int]: type of the resource to be freed, INT_RESOURCE_x where x can be { CONNECTION | DOMAIN | NETWORK | NODEDEV | STORAGEPOOL | VOLUME | SNAPSHOT }
 *                          @mem [uint]: memory location of the resource to be freed
 * Returns:                 None
 */
void free_resource(int type, void *mem)
{
    int rv;

    DPRINTF("%s: Freeing libvirt %s resource at 0x%lx\n", __FUNCTION__, translate_counter_type(type), (long) mem);

    if (type == INT_RESOURCE_DOMAIN) {
        rv = virDomainFree((virDomainPtr)mem);
        if (rv != 0) {
            DPRINTF("%s: virDomainFree(%p) returned %d (%s)\n", __FUNCTION__, (virDomainPtr)mem, rv, LIBVIRT_G(last_error));
            php_error_docref(NULL, E_WARNING, "virDomainFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
        } else {
            DPRINTF("%s: virDomainFree(%p) completed successfully\n", __FUNCTION__, (virDomainPtr)mem);
            resource_change_counter(INT_RESOURCE_DOMAIN, NULL, (virDomainPtr)mem, 0);
        }
    }

    if (type == INT_RESOURCE_STREAM) {
        rv = virStreamFree((virStreamPtr)mem);
        if (rv != 0) {
            DPRINTF("%s: virStreamFree(%p) returned %d (%s)\n", __FUNCTION__, (virStreamPtr)mem, rv, LIBVIRT_G(last_error));
            php_error_docref(NULL, E_WARNING, "virStreamFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
        } else {
            DPRINTF("%s: virStreamFree(%p) completed successfully\n", __FUNCTION__, (virStreamPtr)mem);
            resource_change_counter(INT_RESOURCE_STREAM, NULL, (virStreamPtr)mem, 0);
        }
    }

    if (type == INT_RESOURCE_NETWORK) {
        rv = virNetworkFree((virNetworkPtr)mem);
        if (rv != 0) {
            DPRINTF("%s: virNetworkFree(%p) returned %d (%s)\n", __FUNCTION__, (virNetworkPtr)mem, rv, LIBVIRT_G(last_error));
            php_error_docref(NULL, E_WARNING, "virNetworkFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
        } else {
            DPRINTF("%s: virNetworkFree(%p) completed successfully\n", __FUNCTION__, (virNetworkPtr)mem);
            resource_change_counter(INT_RESOURCE_NETWORK, NULL, (virNetworkPtr)mem, 0);
        }
    }

    if (type == INT_RESOURCE_NODEDEV) {
        rv = virNodeDeviceFree((virNodeDevicePtr)mem);
        if (rv != 0) {
            DPRINTF("%s: virNodeDeviceFree(%p) returned %d (%s)\n", __FUNCTION__, (virNodeDevicePtr)mem, rv, LIBVIRT_G(last_error));
            php_error_docref(NULL, E_WARNING, "virNodeDeviceFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
        } else {
            DPRINTF("%s: virNodeDeviceFree(%p) completed successfully\n", __FUNCTION__, (virNodeDevicePtr)mem);
            resource_change_counter(INT_RESOURCE_NODEDEV, NULL, (virNodeDevicePtr)mem, 0);
        }
    }

    if (type == INT_RESOURCE_STORAGEPOOL) {
        rv = virStoragePoolFree((virStoragePoolPtr)mem);
        if (rv != 0) {
            DPRINTF("%s: virStoragePoolFree(%p) returned %d (%s)\n", __FUNCTION__, (virStoragePoolPtr)mem, rv, LIBVIRT_G(last_error));
            php_error_docref(NULL, E_WARNING, "virStoragePoolFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
        } else {
            DPRINTF("%s: virStoragePoolFree(%p) completed successfully\n", __FUNCTION__, (virStoragePoolPtr)mem);
            resource_change_counter(INT_RESOURCE_STORAGEPOOL, NULL, (virStoragePoolPtr)mem, 0);
        }
    }

    if (type == INT_RESOURCE_VOLUME) {
        rv = virStorageVolFree((virStorageVolPtr)mem);
        if (rv != 0) {
            DPRINTF("%s: virStorageVolFree(%p) returned %d (%s)\n", __FUNCTION__, (virStorageVolPtr)mem, rv, LIBVIRT_G(last_error));
            php_error_docref(NULL, E_WARNING, "virStorageVolFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
        } else {
            DPRINTF("%s: virStorageVolFree(%p) completed successfully\n", __FUNCTION__, (virStorageVolPtr)mem);
            resource_change_counter(INT_RESOURCE_VOLUME, NULL, (virStorageVolPtr)mem, 0);
        }
    }

    if (type == INT_RESOURCE_SNAPSHOT) {
        rv = virDomainSnapshotFree((virDomainSnapshotPtr)mem);
        if (rv != 0) {
            DPRINTF("%s: virDomainSnapshotFree(%p) returned %d (%s)\n", __FUNCTION__, (virDomainSnapshotPtr)mem, rv, LIBVIRT_G(last_error));
            php_error_docref(NULL, E_WARNING, "virDomainSnapshotFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
        } else {
            DPRINTF("%s: virDomainSnapshotFree(%p) completed successfully\n", __FUNCTION__, (virDomainSnapshotPtr)mem);
            resource_change_counter(INT_RESOURCE_SNAPSHOT, NULL, (virDomainSnapshotPtr)mem, 0);
        }
    }

    if (type == INT_RESOURCE_NWFILTER) {
        rv = virNWFilterFree((virNWFilterPtr) mem);
        if (rv != 0) {
            DPRINTF("%s: virNWFilterFree(%p) returned %d (%s)\n", __FUNCTION__, (virNWFilterPtr) mem, rv, LIBVIRT_G(last_error));
            php_error_docref(NULL, E_WARNING, "virDomainSnapshotFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
        } else {
            DPRINTF("%s: virNWFilterFree(%p) completed successfully\n", __FUNCTION__, (virNWFilterPtr) mem);
            resource_change_counter(INT_RESOURCE_NWFILTER, NULL, (virNWFilterPtr) mem, 0);
        }
    }
}

/*
 * Private function name:   check_resource_allocation
 * Since version:           0.4.2
 * Description:             Function is used to check whether the resource identified by type and memory is allocated for connection conn or not
 * Arguments:               @conn [virConnectPtr]: libvirt connection pointer
 *                          @type [int]: type of the counter to be checked, please see free_resource() API for possible values
 *                          @memp [pointer]: pointer to the memory
 * Returns:                 1 if resource is allocated, 0 otherwise
 */
int check_resource_allocation(virConnectPtr conn, int type, void *mem)
{
    int binding_resources_count = 0;
    resource_info *binding_resources = NULL;
    int i, allocated = 0;

    binding_resources_count = LIBVIRT_G(binding_resources_count);
    binding_resources = LIBVIRT_G(binding_resources);

    if (binding_resources == NULL)
        return 0;

    for (i = 0; i < binding_resources_count; i++) {
        if ((((conn != NULL) && (binding_resources[i].conn == conn)) || (conn == NULL))
            && (binding_resources[i].type == type) && (binding_resources[i].mem == mem)
            && (binding_resources[i].overwrite == 0))
            allocated = 1;
    }

    DPRINTF("%s: libvirt %s resource 0x%lx (conn %p) is%s allocated\n", __FUNCTION__, translate_counter_type(type),
            (long) mem, conn, !allocated ? " not" : "");
    return allocated;
}

/*
 * Private function name:   count_resources
 * Since version:           0.4.2
 * Description:             Function counts the internal resources of module instance
 * Arguments:               @type [int]: integer interpretation of the type, see free_resource() API function for possible values
 * Returns:                 number of resources already used
 */
int count_resources(int type)
{
    int binding_resources_count = 0;
    resource_info *binding_resources = NULL;
    int i, count = 0;

    binding_resources_count = LIBVIRT_G(binding_resources_count);
    binding_resources = LIBVIRT_G(binding_resources);

    if (binding_resources == NULL)
        return 0;

    for (i = 0; i < binding_resources_count; i++) {
        if (binding_resources[i].type == type)
            count++;
    }

    return count;
}

/*
 * Private function name:   size_def_to_mbytes
 * Since version:           0.4.5
 * Description:             Function is used to translate the string size representation to the number of MBytes, used e.g. for domain installation
 * Arguments:               @arg [string]: input string to be converted
 * Returns:                 number of megabytes extracted from the input string
 */
unsigned long long size_def_to_mbytes(char *arg)
{
    int unit, multiplicator = 1, nodel = 0;

    if ((arg == NULL) || (strlen(arg) == 0))
        return 0;

    unit = arg[strlen(arg)-1];
    switch (unit) {
    case 'G':
        multiplicator = 1 << 10;
        break;
    case 'T':
        multiplicator = 1 << 20;
        break;
    default:
        nodel = 1;
    }

    if (nodel == 0)
        arg[strlen(arg) - 1] = 0;

    return atoi(arg) * multiplicator;
}

/*
 * Private function name:   is_local_connection
 * Since version:           0.4.5
 * Description:             Function is used to check whether the connection is the connection to the local hypervisor or to remote hypervisor
 * Arguments:               @conn [virConnectPtr]: libvirt connection pointer
 * Returns:                 1 (TRUE) for local connection, 0 (FALSE) for remote connection
 */
int is_local_connection(virConnectPtr conn)
{
#ifndef EXTWIN
    int ret;
    char *lv_hostname = NULL, *result = NULL;
    char name[1024];
    struct addrinfo hints, *info = NULL;

    name[1023] = '\0';
    gethostname(name, 1024);

    if (strcmp(name, "localhost") == 0)
        return 1;

    lv_hostname = virConnectGetHostname(conn);

    /* gethostname gave us FQDN, compare */
    if (strchr(name, '.') && strcmp(name, lv_hostname) == 0)
        return 1;

    /* need to get FQDN of the local name */
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_CANONNAME|AI_CANONIDN;
    hints.ai_family = AF_UNSPEC;

    /* could not get FQDN or got localhost, use whatever gethostname gave us */
    if (getaddrinfo(name, NULL, &hints, &info) != 0 ||
        info->ai_canonname == NULL ||
        strcmp(info->ai_canonname, "localhost") == 0)
        result = strdup(name);
    else
        result = strdup(info->ai_canonname);

    ret = strcmp(result, lv_hostname) == 0;

    freeaddrinfo(info);
    if (lv_hostname)
        VIR_FREE(lv_hostname);
    if (result)
        VIR_FREE(result);

    return ret;
#else
    // Libvirt daemon doesn't work on Windows systems so always return 0 (FALSE)
    return 0;
#endif
}

/* ZEND Module inicialization function */
PHP_MINIT_FUNCTION(libvirt)
{
    /* register resource types and their descriptors */
    le_libvirt_connection = zend_register_list_destructors_ex(php_libvirt_connection_dtor, NULL, PHP_LIBVIRT_CONNECTION_RES_NAME, module_number);
    le_libvirt_domain = zend_register_list_destructors_ex(php_libvirt_domain_dtor, NULL, PHP_LIBVIRT_DOMAIN_RES_NAME, module_number);
    le_libvirt_stream = zend_register_list_destructors_ex(php_libvirt_stream_dtor, NULL, PHP_LIBVIRT_STREAM_RES_NAME, module_number);
    le_libvirt_storagepool = zend_register_list_destructors_ex(php_libvirt_storagepool_dtor, NULL, PHP_LIBVIRT_STORAGEPOOL_RES_NAME, module_number);
    le_libvirt_volume = zend_register_list_destructors_ex(php_libvirt_volume_dtor, NULL, PHP_LIBVIRT_VOLUME_RES_NAME, module_number);
    le_libvirt_network = zend_register_list_destructors_ex(php_libvirt_network_dtor, NULL, PHP_LIBVIRT_NETWORK_RES_NAME, module_number);
    le_libvirt_nodedev = zend_register_list_destructors_ex(php_libvirt_nodedev_dtor, NULL, PHP_LIBVIRT_NODEDEV_RES_NAME, module_number);
    le_libvirt_snapshot = zend_register_list_destructors_ex(php_libvirt_snapshot_dtor, NULL, PHP_LIBVIRT_SNAPSHOT_RES_NAME, module_number);
    le_libvirt_nwfilter = zend_register_list_destructors_ex(php_libvirt_nwfilter_dtor, NULL, PHP_LIBVIRT_NWFILTER_RES_NAME, module_number);

    ZEND_INIT_MODULE_GLOBALS(libvirt, php_libvirt_init_globals, NULL);

    /* LIBVIRT CONSTANTS */

    /* XML constants */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_XML_SECURE",     1, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_XML_INACTIVE",   2, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_XML_UPDATE_CPU", 4, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_XML_MIGRATABLE", 8, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("VIR_NODE_CPU_STATS_ALL_CPUS",   VIR_NODE_CPU_STATS_ALL_CPUS, CONST_CS | CONST_PERSISTENT);

    /* Domain constants */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_NOSTATE",        0, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_RUNNING",        1, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCKED",        2, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_PAUSED",         3, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_SHUTDOWN",       4, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_SHUTOFF",        5, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_CRASHED",        6, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_PMSUSPENDED",    7, CONST_CS | CONST_PERSISTENT);

    /* Volume constants */
    REGISTER_LONG_CONSTANT("VIR_STORAGE_VOL_RESIZE_ALLOCATE",        1, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_STORAGE_VOL_RESIZE_DELTA",           2, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_STORAGE_VOL_RESIZE_SHRINK",          4, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_STORAGE_VOL_DELETE_NORMAL",          VIR_STORAGE_VOL_DELETE_NORMAL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_STORAGE_VOL_DELETE_ZEROED",          VIR_STORAGE_VOL_DELETE_ZEROED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_STORAGE_VOL_DELETE_WITH_SNAPSHOTS",  VIR_STORAGE_VOL_DELETE_WITH_SNAPSHOTS, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_STORAGE_VOL_CREATE_PREALLOC_METADATA", VIR_STORAGE_VOL_CREATE_PREALLOC_METADATA, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_STORAGE_VOL_CREATE_REFLINK", VIR_STORAGE_VOL_CREATE_REFLINK, CONST_CS | CONST_PERSISTENT);

    /* Domain vCPU flags */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_VCPU_CONFIG",    VIR_DOMAIN_VCPU_CONFIG, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_VCPU_CURRENT",   VIR_DOMAIN_VCPU_CURRENT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_VCPU_LIVE",      VIR_DOMAIN_VCPU_LIVE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_VCPU_MAXIMUM",   VIR_DOMAIN_VCPU_MAXIMUM, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_VCPU_GUEST",     VIR_DOMAIN_VCPU_GUEST, CONST_CS | CONST_PERSISTENT);

    /* Domain snapshot constants */
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_DELETE_CHILDREN",   VIR_DOMAIN_SNAPSHOT_DELETE_CHILDREN,   CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_DELETE_METADATA_ONLY",     VIR_DOMAIN_SNAPSHOT_DELETE_METADATA_ONLY,       CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_DELETE_CHILDREN_ONLY",     VIR_DOMAIN_SNAPSHOT_DELETE_CHILDREN_ONLY,       CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_CREATE_REDEFINE",   VIR_DOMAIN_SNAPSHOT_CREATE_REDEFINE,   CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_CREATE_CURRENT",   VIR_DOMAIN_SNAPSHOT_CREATE_CURRENT,     CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_CREATE_NO_METADATA",       VIR_DOMAIN_SNAPSHOT_CREATE_NO_METADATA, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_CREATE_HALT",      VIR_DOMAIN_SNAPSHOT_CREATE_HALT,        CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_CREATE_DISK_ONLY", VIR_DOMAIN_SNAPSHOT_CREATE_DISK_ONLY,   CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_CREATE_REUSE_EXT", VIR_DOMAIN_SNAPSHOT_CREATE_REUSE_EXT,   CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_CREATE_QUIESCE",   VIR_DOMAIN_SNAPSHOT_CREATE_QUIESCE,     CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_CREATE_ATOMIC",    VIR_DOMAIN_SNAPSHOT_CREATE_ATOMIC,      CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_CREATE_LIVE",      VIR_DOMAIN_SNAPSHOT_CREATE_LIVE,        CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_LIST_DESCENDANTS", VIR_DOMAIN_SNAPSHOT_LIST_DESCENDANTS,   CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_LIST_ROOTS",       VIR_DOMAIN_SNAPSHOT_LIST_ROOTS, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_LIST_METADATA",    VIR_DOMAIN_SNAPSHOT_LIST_METADATA,      CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_LIST_LEAVES",      VIR_DOMAIN_SNAPSHOT_LIST_LEAVES,        CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_LIST_NO_LEAVES",   VIR_DOMAIN_SNAPSHOT_LIST_NO_LEAVES,     CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_LIST_NO_METADATA", VIR_DOMAIN_SNAPSHOT_LIST_NO_METADATA,   CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_LIST_INACTIVE",    VIR_DOMAIN_SNAPSHOT_LIST_INACTIVE,      CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_LIST_ACTIVE",      VIR_DOMAIN_SNAPSHOT_LIST_ACTIVE,        CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_LIST_DISK_ONLY",   VIR_DOMAIN_SNAPSHOT_LIST_DISK_ONLY,     CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_LIST_INTERNAL",    VIR_DOMAIN_SNAPSHOT_LIST_INTERNAL,      CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_LIST_EXTERNAL",    VIR_DOMAIN_SNAPSHOT_LIST_EXTERNAL,      CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_REVERT_RUNNING",   VIR_DOMAIN_SNAPSHOT_REVERT_RUNNING,     CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_REVERT_PAUSED",    VIR_DOMAIN_SNAPSHOT_REVERT_PAUSED,      CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_SNAPSHOT_REVERT_FORCE",     VIR_DOMAIN_SNAPSHOT_REVERT_FORCE,       CONST_CS | CONST_PERSISTENT);

    /* Create flags */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_NONE", VIR_DOMAIN_NONE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_START_PAUSED", VIR_DOMAIN_START_PAUSED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_START_AUTODESTROY", VIR_DOMAIN_START_AUTODESTROY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_START_BYPASS_CACHE", VIR_DOMAIN_START_BYPASS_CACHE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_START_FORCE_BOOT", VIR_DOMAIN_START_FORCE_BOOT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_START_VALIDATE", VIR_DOMAIN_START_VALIDATE, CONST_CS | CONST_PERSISTENT);

    /* Memory constants */
    REGISTER_LONG_CONSTANT("VIR_MEMORY_VIRTUAL",        1, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_MEMORY_PHYSICAL",       2, CONST_CS | CONST_PERSISTENT);

    /* Version checking constants */
    REGISTER_LONG_CONSTANT("VIR_VERSION_BINDING",           VIR_VERSION_BINDING,    CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_VERSION_LIBVIRT",           VIR_VERSION_LIBVIRT,    CONST_CS | CONST_PERSISTENT);

    /* Network constants */
    REGISTER_LONG_CONSTANT("VIR_NETWORKS_ACTIVE",       VIR_NETWORKS_ACTIVE,    CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_NETWORKS_INACTIVE",     VIR_NETWORKS_INACTIVE,  CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_NETWORKS_ALL",      VIR_NETWORKS_ACTIVE |
                           VIR_NETWORKS_INACTIVE,  CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_CONNECT_LIST_NETWORKS_INACTIVE",     VIR_CONNECT_LIST_NETWORKS_INACTIVE,     CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_CONNECT_LIST_NETWORKS_ACTIVE",       VIR_CONNECT_LIST_NETWORKS_ACTIVE,       CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_CONNECT_LIST_NETWORKS_PERSISTENT",   VIR_CONNECT_LIST_NETWORKS_PERSISTENT,   CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_CONNECT_LIST_NETWORKS_TRANSIENT",    VIR_CONNECT_LIST_NETWORKS_TRANSIENT,    CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_CONNECT_LIST_NETWORKS_AUTOSTART",    VIR_CONNECT_LIST_NETWORKS_AUTOSTART,    CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_CONNECT_LIST_NETWORKS_NO_AUTOSTART", VIR_CONNECT_LIST_NETWORKS_NO_AUTOSTART, CONST_CS | CONST_PERSISTENT);


    /* Credential constants */
    REGISTER_LONG_CONSTANT("VIR_CRED_USERNAME",     1, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_CRED_AUTHNAME",     2, CONST_CS | CONST_PERSISTENT);
    /* RFC 1766 languages */
    REGISTER_LONG_CONSTANT("VIR_CRED_LANGUAGE",     3, CONST_CS | CONST_PERSISTENT);
    /* Client supplied a nonce */
    REGISTER_LONG_CONSTANT("VIR_CRED_CNONCE",       4, CONST_CS | CONST_PERSISTENT);
    /* Passphrase secret */
    REGISTER_LONG_CONSTANT("VIR_CRED_PASSPHRASE",       5, CONST_CS | CONST_PERSISTENT);
    /* Challenge response */
    REGISTER_LONG_CONSTANT("VIR_CRED_ECHOPROMPT",       6, CONST_CS | CONST_PERSISTENT);
    /* Challenge responce */
    REGISTER_LONG_CONSTANT("VIR_CRED_NOECHOPROMPT",     7, CONST_CS | CONST_PERSISTENT);
    /* Authentication realm */
    REGISTER_LONG_CONSTANT("VIR_CRED_REALM",        8, CONST_CS | CONST_PERSISTENT);
    /* Externally managed credential More may be added - expect the unexpected */
    REGISTER_LONG_CONSTANT("VIR_CRED_EXTERNAL",     9, CONST_CS | CONST_PERSISTENT);

    /* Domain memory constants */
    /* The total amount of memory written out to swap space (in kB). */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_MEMORY_STAT_SWAP_IN",    0, CONST_CS | CONST_PERSISTENT);
    /* Page faults occur when a process makes a valid access to virtual memory that is not available. */
    /* When servicing the page fault, if disk IO is * required, it is considered a major fault. If not, */
    /* it is a minor fault. * These are expressed as the number of faults that have occurred. */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_MEMORY_STAT_SWAP_OUT",   1, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_MEMORY_STAT_MAJOR_FAULT",    2, CONST_CS | CONST_PERSISTENT);
    /* The amount of memory left completely unused by the system. Memory that is available but used for */
    /* reclaimable caches should NOT be reported as free. This value is expressed in kB. */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_MEMORY_STAT_MINOR_FAULT",    3, CONST_CS | CONST_PERSISTENT);
    /* The total amount of usable memory as seen by the domain. This value * may be less than the amount */
    /* of memory assigned to the domain if a * balloon driver is in use or if the guest OS does not initialize */
    /* all * assigned pages. This value is expressed in kB.  */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_MEMORY_STAT_UNUSED",     4, CONST_CS | CONST_PERSISTENT);
    /* The number of statistics supported by this version of the interface. To add new statistics, add them */
    /* to the enum and increase this value. */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_MEMORY_STAT_AVAILABLE",  5, CONST_CS | CONST_PERSISTENT);
    /* Current balloon value (in KB). */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON",  6, CONST_CS | CONST_PERSISTENT);
    /* Resident Set Size of the process running the domain. This value is in kB */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_MEMORY_STAT_RSS",  7, CONST_CS | CONST_PERSISTENT);
    /* The number of statistics supported by this version of the interface. */
    /* To add new statistics, add them to the enum and increase this value. */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_MEMORY_STAT_NR",     8, CONST_CS | CONST_PERSISTENT);

    /* Job constants */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_JOB_NONE",       0, CONST_CS | CONST_PERSISTENT);
    /* Job with a finite completion time */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_JOB_BOUNDED",    1, CONST_CS | CONST_PERSISTENT);
    /* Job without a finite completion time */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_JOB_UNBOUNDED",  2, CONST_CS | CONST_PERSISTENT);
    /* Job has finished but it's not cleaned up yet */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_JOB_COMPLETED",  3, CONST_CS | CONST_PERSISTENT);
    /* Job hit error but it's not cleaned up yet */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_JOB_FAILED",     4, CONST_CS | CONST_PERSISTENT);
    /* Job was aborted but it's not cleanup up yet */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_JOB_CANCELLED",  5, CONST_CS | CONST_PERSISTENT);


    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_COMMIT_SHALLOW", VIR_DOMAIN_BLOCK_COMMIT_SHALLOW, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_COMMIT_DELETE", VIR_DOMAIN_BLOCK_COMMIT_DELETE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_COMMIT_ACTIVE", VIR_DOMAIN_BLOCK_COMMIT_ACTIVE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_COMMIT_RELATIVE", VIR_DOMAIN_BLOCK_COMMIT_RELATIVE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_COMMIT_BANDWIDTH_BYTES", VIR_DOMAIN_BLOCK_COMMIT_BANDWIDTH_BYTES, CONST_CS | CONST_PERSISTENT);


    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_COPY_SHALLOW", VIR_DOMAIN_BLOCK_COPY_SHALLOW, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_COPY_REUSE_EXT", VIR_DOMAIN_BLOCK_COPY_REUSE_EXT, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_JOB_ABORT_ASYNC",  VIR_DOMAIN_BLOCK_JOB_ABORT_ASYNC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_JOB_ABORT_PIVOT",  VIR_DOMAIN_BLOCK_JOB_ABORT_PIVOT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_JOB_SPEED_BANDWIDTH_BYTES",    VIR_DOMAIN_BLOCK_JOB_SPEED_BANDWIDTH_BYTES, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_JOB_INFO_BANDWIDTH_BYTES",    VIR_DOMAIN_BLOCK_JOB_INFO_BANDWIDTH_BYTES, CONST_CS | CONST_PERSISTENT);


    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_JOB_TYPE_UNKNOWN", VIR_DOMAIN_BLOCK_JOB_TYPE_UNKNOWN, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_JOB_TYPE_PULL", VIR_DOMAIN_BLOCK_JOB_TYPE_PULL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_JOB_TYPE_COPY", VIR_DOMAIN_BLOCK_JOB_TYPE_COPY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_JOB_TYPE_COMMIT", VIR_DOMAIN_BLOCK_JOB_TYPE_COMMIT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_JOB_TYPE_ACTIVE_COMMIT", VIR_DOMAIN_BLOCK_JOB_TYPE_ACTIVE_COMMIT, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_PULL_BANDWIDTH_BYTES", VIR_DOMAIN_BLOCK_PULL_BANDWIDTH_BYTES, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_REBASE_SHALLOW", VIR_DOMAIN_BLOCK_REBASE_SHALLOW, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_REBASE_REUSE_EXT", VIR_DOMAIN_BLOCK_REBASE_REUSE_EXT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_REBASE_COPY_RAW", VIR_DOMAIN_BLOCK_REBASE_COPY_RAW, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_REBASE_COPY", VIR_DOMAIN_BLOCK_REBASE_COPY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_REBASE_RELATIVE", VIR_DOMAIN_BLOCK_REBASE_RELATIVE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_REBASE_COPY_DEV", VIR_DOMAIN_BLOCK_REBASE_COPY_DEV, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_REBASE_BANDWIDTH_BYTES", VIR_DOMAIN_BLOCK_REBASE_BANDWIDTH_BYTES, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("VIR_DOMAIN_BLOCK_RESIZE_BYTES", VIR_DOMAIN_BLOCK_RESIZE_BYTES, CONST_CS | CONST_PERSISTENT);

    /* Migration constants */
    REGISTER_LONG_CONSTANT("VIR_MIGRATE_LIVE",        1, CONST_CS | CONST_PERSISTENT);
    /* direct source -> dest host control channel Note the less-common spelling that we're stuck with: */
    /* VIR_MIGRATE_TUNNELLED should be VIR_MIGRATE_TUNNELED */
    REGISTER_LONG_CONSTANT("VIR_MIGRATE_PEER2PEER",       2, CONST_CS | CONST_PERSISTENT);
    /* tunnel migration data over libvirtd connection */
    REGISTER_LONG_CONSTANT("VIR_MIGRATE_TUNNELLED",       4, CONST_CS | CONST_PERSISTENT);
    /* persist the VM on the destination */
    REGISTER_LONG_CONSTANT("VIR_MIGRATE_PERSIST_DEST",    8, CONST_CS | CONST_PERSISTENT);
    /* undefine the VM on the source */
    REGISTER_LONG_CONSTANT("VIR_MIGRATE_UNDEFINE_SOURCE",    16, CONST_CS | CONST_PERSISTENT);
    /* pause on remote side */
    REGISTER_LONG_CONSTANT("VIR_MIGRATE_PAUSED",         32, CONST_CS | CONST_PERSISTENT);
    /* migration with non-shared storage with full disk copy */
    REGISTER_LONG_CONSTANT("VIR_MIGRATE_NON_SHARED_DISK",    64, CONST_CS | CONST_PERSISTENT);
    /* migration with non-shared storage with incremental copy (same base image shared between source and destination) */
    REGISTER_LONG_CONSTANT("VIR_MIGRATE_NON_SHARED_INC",    128, CONST_CS | CONST_PERSISTENT);
    /* protect for changing domain configuration through the whole migration process; this will be used automatically when supported */
    REGISTER_LONG_CONSTANT("VIR_MIGRATE_CHANGE_PROTECTION",    256, CONST_CS | CONST_PERSISTENT);
    /* force migration even if it is considered unsafe */
    REGISTER_LONG_CONSTANT("VIR_MIGRATE_UNSAFE",    512, CONST_CS | CONST_PERSISTENT);
    /* offline migrate */
    REGISTER_LONG_CONSTANT("VIR_MIGRATE_OFFLINE",    1024, CONST_CS | CONST_PERSISTENT);
    /* compress data during migration */
    REGISTER_LONG_CONSTANT("VIR_MIGRATE_COMPRESSED",    2048, CONST_CS | CONST_PERSISTENT);
    /* abort migration on I/O errors happened during migration */
    REGISTER_LONG_CONSTANT("VIR_MIGRATE_ABORT_ON_ERROR",    4096, CONST_CS | CONST_PERSISTENT);
    /* force convergence */
    REGISTER_LONG_CONSTANT("VIR_MIGRATE_AUTO_CONVERGE",    8192, CONST_CS | CONST_PERSISTENT);

    /* Modify device allocation based on current domain state */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_DEVICE_MODIFY_CURRENT",  0, CONST_CS | CONST_PERSISTENT);
    /* Modify live device allocation */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_DEVICE_MODIFY_LIVE",     1, CONST_CS | CONST_PERSISTENT);
    /* Modify persisted device allocation */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_DEVICE_MODIFY_CONFIG",   2, CONST_CS | CONST_PERSISTENT);
    /* Forcibly modify device (ex. force eject a cdrom) */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_DEVICE_MODIFY_FORCE",    4, CONST_CS | CONST_PERSISTENT);

    /* REGISTER_LONG_CONSTANT */
    REGISTER_LONG_CONSTANT("VIR_STORAGE_POOL_BUILD_NEW",        0, CONST_CS | CONST_PERSISTENT);
    /* Repair / reinitialize */
    REGISTER_LONG_CONSTANT("VIR_STORAGE_POOL_BUILD_REPAIR",     1, CONST_CS | CONST_PERSISTENT);
    /* Extend existing pool */
    REGISTER_LONG_CONSTANT("VIR_STORAGE_POOL_BUILD_RESIZE",     2, CONST_CS | CONST_PERSISTENT);

    /* Domain flags */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_FLAG_FEATURE_ACPI",      DOMAIN_FLAG_FEATURE_ACPI, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_FLAG_FEATURE_APIC",      DOMAIN_FLAG_FEATURE_APIC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_FLAG_FEATURE_PAE",       DOMAIN_FLAG_FEATURE_PAE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_FLAG_CLOCK_LOCALTIME",   DOMAIN_FLAG_CLOCK_LOCALTIME, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_FLAG_TEST_LOCAL_VNC",    DOMAIN_FLAG_TEST_LOCAL_VNC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_FLAG_SOUND_AC97",        DOMAIN_FLAG_SOUND_AC97, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_DISK_FILE",          DOMAIN_DISK_FILE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_DISK_BLOCK",             DOMAIN_DISK_BLOCK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_DISK_ACCESS_ALL",        DOMAIN_DISK_ACCESS_ALL, CONST_CS | CONST_PERSISTENT);

    /* Domain metadata constants */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_METADATA_DESCRIPTION",   0, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_METADATA_TITLE",     1, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_METADATA_ELEMENT",       2, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_AFFECT_CURRENT",     VIR_DOMAIN_AFFECT_CURRENT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_AFFECT_LIVE",        VIR_DOMAIN_AFFECT_LIVE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_AFFECT_CONFIG",      VIR_DOMAIN_AFFECT_CONFIG, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("VIR_DOMAIN_STATS_STATE",        VIR_DOMAIN_STATS_STATE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_STATS_CPU_TOTAL",        VIR_DOMAIN_STATS_CPU_TOTAL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_STATS_BALLOON",      VIR_DOMAIN_STATS_BALLOON, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_STATS_VCPU",     VIR_DOMAIN_STATS_VCPU, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_STATS_INTERFACE",        VIR_DOMAIN_STATS_INTERFACE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_STATS_BLOCK",        VIR_DOMAIN_STATS_BLOCK, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("VIR_CONNECT_GET_ALL_DOMAINS_STATS_ACTIVE",  VIR_CONNECT_GET_ALL_DOMAINS_STATS_ACTIVE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_CONNECT_GET_ALL_DOMAINS_STATS_INACTIVE",    VIR_CONNECT_GET_ALL_DOMAINS_STATS_INACTIVE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_CONNECT_GET_ALL_DOMAINS_STATS_OTHER",   VIR_CONNECT_GET_ALL_DOMAINS_STATS_OTHER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_CONNECT_GET_ALL_DOMAINS_STATS_PAUSED",  VIR_CONNECT_GET_ALL_DOMAINS_STATS_PAUSED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_CONNECT_GET_ALL_DOMAINS_STATS_PERSISTENT",  VIR_CONNECT_GET_ALL_DOMAINS_STATS_PERSISTENT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_CONNECT_GET_ALL_DOMAINS_STATS_RUNNING", VIR_CONNECT_GET_ALL_DOMAINS_STATS_RUNNING, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_CONNECT_GET_ALL_DOMAINS_STATS_SHUTOFF", VIR_CONNECT_GET_ALL_DOMAINS_STATS_SHUTOFF, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_CONNECT_GET_ALL_DOMAINS_STATS_TRANSIENT", VIR_CONNECT_GET_ALL_DOMAINS_STATS_TRANSIENT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_CONNECT_GET_ALL_DOMAINS_STATS_ENFORCE_STATS", VIR_CONNECT_GET_ALL_DOMAINS_STATS_ENFORCE_STATS, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("VIR_DOMAIN_MEM_CONFIG", VIR_DOMAIN_MEM_CONFIG, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_MEM_CURRENT",    VIR_DOMAIN_MEM_CURRENT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_MEM_LIVE",   VIR_DOMAIN_MEM_LIVE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_MEM_MAXIMUM",    VIR_DOMAIN_MEM_MAXIMUM, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("VIR_DOMAIN_INTERFACE_ADDRESSES_SRC_LEASE", VIR_DOMAIN_INTERFACE_ADDRESSES_SRC_LEASE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_INTERFACE_ADDRESSES_SRC_AGENT", VIR_DOMAIN_INTERFACE_ADDRESSES_SRC_AGENT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_INTERFACE_ADDRESSES_SRC_ARP", VIR_DOMAIN_INTERFACE_ADDRESSES_SRC_LEASE, CONST_CS | CONST_PERSISTENT);

    /* Connect flags */
    REGISTER_LONG_CONSTANT("VIR_CONNECT_FLAG_SOUNDHW_GET_NAMES",    CONNECT_FLAG_SOUNDHW_GET_NAMES, CONST_CS | CONST_PERSISTENT);

    /* Keycodeset constants */
    REGISTER_LONG_CONSTANT("VIR_KEYCODE_SET_LINUX", VIR_KEYCODE_SET_LINUX, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_KEYCODE_SET_XT", VIR_KEYCODE_SET_XT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_KEYCODE_SET_ATSET1", VIR_KEYCODE_SET_ATSET1, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_KEYCODE_SET_ATSET2", VIR_KEYCODE_SET_ATSET2, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_KEYCODE_SET_ATSET3", VIR_KEYCODE_SET_ATSET3, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_KEYCODE_SET_OSX", VIR_KEYCODE_SET_OSX, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_KEYCODE_SET_XT_KBD", VIR_KEYCODE_SET_XT_KBD, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_KEYCODE_SET_USB", VIR_KEYCODE_SET_USB, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_KEYCODE_SET_WIN32", VIR_KEYCODE_SET_WIN32, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_KEYCODE_SET_RFB", VIR_KEYCODE_SET_RFB, CONST_CS | CONST_PERSISTENT);

    /* virDomainUndefineFlagsValues */
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_UNDEFINE_MANAGED_SAVE", VIR_DOMAIN_UNDEFINE_MANAGED_SAVE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_UNDEFINE_SNAPSHOTS_METADATA", VIR_DOMAIN_UNDEFINE_SNAPSHOTS_METADATA, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_UNDEFINE_NVRAM", VIR_DOMAIN_UNDEFINE_NVRAM, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("VIR_DOMAIN_UNDEFINE_KEEP_NVRAM", VIR_DOMAIN_UNDEFINE_KEEP_NVRAM, CONST_CS | CONST_PERSISTENT);

    REGISTER_INI_ENTRIES();

    /* Initialize libvirt and set up error callback */
    virInitialize();

    virSetErrorFunc(NULL, catch_error);

    return SUCCESS;
}

/* Zend module destruction */
PHP_MSHUTDOWN_FUNCTION(libvirt)
{
    UNREGISTER_INI_ENTRIES();

    /* return error callback back to default (outouts to STDOUT) */
    virSetErrorFunc(NULL, NULL);
    return SUCCESS;
}

/* Common functions */

/*
 * Function name:   libvirt_get_last_error
 * Since version:   0.4.1(-1)
 * Description:     This function is used to get the last error coming either from libvirt or the PHP extension itself
 * Returns:         last error string
 */
PHP_FUNCTION(libvirt_get_last_error)
{
    if (LIBVIRT_G (last_error) == NULL)
        RETURN_NULL();
    VIRT_RETURN_STRING(LIBVIRT_G(last_error));
}

/*
 * Function name:   libvirt_image_create
 * Since version:   0.4.2
 * Description:     Function is used to create the image of desired name, size and format. The image will be created in the image path (libvirt.image_path INI variable). Works only o
 * Arguments:       @conn [resource]: libvirt connection resource
 *                  @name [string]: name of the image file that will be created in the libvirt.image_path directory
 *                  @size [int]: size of the image in MiBs
 *                  @format [string]: format of the image, may be raw, qcow or qcow2
 * Returns:         hostname of the host node or FALSE for error
 */
PHP_FUNCTION(libvirt_image_create)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    char msg[1024];
    char cmd[4096] = { 0 };
    char *path = NULL;
    char fpath[4096] = { 0 };
    char *image = NULL;
    size_t image_len;
    char *format;
    size_t format_len;
    unsigned long long size;
    char *size_str;
    size_t size_str_len;
    int cmdRet;

    if (LIBVIRT_G(image_path_ini))
        path = strdup(LIBVIRT_G(image_path_ini));

    if ((path == NULL) || (path[0] != '/')) {
        set_error("Invalid argument, path must be set and absolute (start by slash character [/])");
        RETURN_FALSE;
    }

    GET_CONNECTION_FROM_ARGS("rsss", &zconn, &image, &image_len, &size_str, &size_str_len, &format, &format_len);

    if (size_str == NULL)
        RETURN_FALSE;

    size = size_def_to_mbytes(size_str);

    if (!is_local_connection(conn->conn)) {
        // TODO: Try to implement remote connection somehow. Maybe using SSH tunneling
        snprintf(msg, sizeof(msg), "%s works only on local systems!", PHPFUNC);
        set_error(msg);
        RETURN_FALSE;
    }

    snprintf(fpath, sizeof(fpath), "%s/%s", path, image);

    const char *qemu_img_cmd = get_feature_binary("create-image");
    if (qemu_img_cmd == NULL) {
        set_error("Feature 'create-image' is not supported");
        RETURN_FALSE;
    }

    snprintf(cmd, sizeof(cmd), "%s create -f %s %s %lluM > /dev/null", qemu_img_cmd, format, fpath, size);
    DPRINTF("%s: Running '%s'...\n", PHPFUNC, cmd);
    cmdRet = system(cmd);

    if (WEXITSTATUS(cmdRet) == 0 && access(fpath, F_OK) == 0) {
        RETURN_TRUE;
    } else {
        snprintf(msg, sizeof(msg), "Cannot create image: %s", fpath);
        set_error(msg);
        RETURN_FALSE;
    }
}

/*
 * Function name:   libvirt_image_remove
 * Since version:   0.4.2
 * Description:     Function is used to create the image of desired name, size and format. The image will be created in the image path (libvirt.image_path INI variable). Works only on local systems!
 * Arguments:       @conn [resource]: libvirt connection resource
 *                  @image [string]: name of the image file that should be deleted
 * Returns:         hostname of the host node or FALSE for error
 */
PHP_FUNCTION(libvirt_image_remove)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    char *hostname;
    char name[1024];
    char msg[4096] = { 0 };
    char *image = NULL;
    size_t image_len;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &image, &image_len);

    // Disable remote connections
    if (!is_local_connection(conn->conn)) {
        set_error("Function works only on local connection");
        RETURN_FALSE;
    }

    hostname = virConnectGetHostname(conn->conn);

#ifndef EXTWIN
    /* Code should never go there for Windows systems however we need to allow compilation */
    /* Get the current hostname to check if we're on local machine */
    gethostname(name, 1024);
#endif
    if (strcmp(name, hostname) != 0) {
        snprintf(msg, sizeof(msg), "%s works only on local systems!", PHPFUNC);
        set_error(msg);
        VIR_FREE(hostname);
        RETURN_FALSE;
    }
    VIR_FREE(hostname);

    if (unlink(image) != 0) {
        snprintf(msg, sizeof(msg), "An error occurred while unlinking %s: %d (%s)", image, errno, strerror(errno));
        set_error(msg);
        RETURN_FALSE;
    } else {
        RETURN_TRUE;
    }
}

/*
 * Private function name:   get_string_from_xpath
 * Since version:           0.4.1(-1)
 * Description:             Function is used to get the XML xPath expression from the XML document. This can be added to val array if not NULL. Note that the result of @xpath is viewed as "string(@xpath)" which may not be what you want. See get_node_string_from_xpath().
 * Arguments:               @xml [string]: input XML document
 *                          @xpath [string]: xPath expression to find nodes in the XML document
 *                          @val [array]: Zend array resource to put data to
 *                          @retVal [int]: return value of the parsing
 * Returns:                 string containing data of last match found
 */
char *get_string_from_xpath(char *xml, char *xpath, zval **val, int *retVal)
{
    xmlParserCtxtPtr xp = NULL;
    xmlDocPtr doc = NULL;
    xmlXPathContextPtr context = NULL;
    xmlXPathObjectPtr result = NULL;
    xmlNodeSetPtr nodeset = NULL;
    int ret = 0, i;
    char *value = NULL;
    char key[8] = { 0 };

    if (!xpath || !xml)
        return NULL;

    xp = xmlCreateDocParserCtxt((xmlChar *)xml);
    if (!xp) {
        ret = -1;
        goto cleanup;
    }

    doc = xmlCtxtReadDoc(xp, (xmlChar *)xml, NULL, NULL, 0);
    if (!doc) {
        ret = -2;
        goto cleanup;
    }

    context = xmlXPathNewContext(doc);
    if (!context) {
        ret = -3;
        goto cleanup;
    }

    result = xmlXPathEvalExpression((xmlChar *)xpath, context);
    if (!result) {
        ret = -4;
        goto cleanup;
    }

    if (xmlXPathNodeSetIsEmpty(result->nodesetval))
        goto cleanup;

    nodeset = result->nodesetval;
    ret = nodeset->nodeNr;

    if (!ret)
        goto cleanup;

    if (val != NULL) {
        ret = 0;
        for (i = 0; i < nodeset->nodeNr; i++) {
            if ((value = (char *) xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1))) {
                snprintf(key, sizeof(key), "%d", i);
                VIRT_ADD_ASSOC_STRING(*val, key, value);
                VIR_FREE(value);
                ret++;
            }
        }
        add_assoc_long(*val, "num", (long)ret);
    } else {
        value = (char *) xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
    }

 cleanup:
    if (retVal)
        *retVal = ret;
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(context);
    xmlFreeParserCtxt(xp);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return value;
}


/*
 * Private function name:   get_node_string_from_xpath
 * Since version:           0.5.5
 * Description:             Evaluate @xpath and convert retuned node to string. The difference to get_string_from_xpath() is that get_string_from_xpath() puts implicit string() around @xpath and get_node_string_from_xpath() evaluates @xpath as is and only then translates xml node into a C string.
 *
 * Arguments:               @xml [string]: input XML document
 *                          @xpath [string]: xPath expression to find nodes in the XML document
 * Returns:                 stringified result of @xpath evaluation
 */
char *get_node_string_from_xpath(char *xml, char *xpath)
{
    xmlParserCtxtPtr xp = NULL;
    xmlDocPtr doc = NULL;
    xmlXPathContextPtr context = NULL;
    xmlXPathObjectPtr result = NULL;
    xmlBufferPtr xmlbuf = NULL;
    char *ret = NULL;


    if (!xpath || !xml)
        return NULL;

    if (!(xp = xmlCreateDocParserCtxt((xmlChar *)xml)))
        return NULL;

    if (!(doc = xmlCtxtReadDoc(xp, (xmlChar *)xml, NULL, NULL, 0)) ||
        !(context = xmlXPathNewContext(doc)) ||
        !(result = xmlXPathEvalExpression((xmlChar *)xpath, context)))
        goto cleanup;

    if (xmlXPathNodeSetIsEmpty(result->nodesetval))
        goto cleanup;

    if (result->nodesetval->nodeNr > 1) {
        set_error("XPATH returned too much nodes, expeced only 1");
        goto cleanup;
    }

    if (!(xmlbuf = xmlBufferCreate()) ||
        xmlNodeDump(xmlbuf, doc, result->nodesetval->nodeTab[0], 0, 1) == 0 ||
        !(ret = strdup((const char *)xmlBufferContent(xmlbuf)))) {
        set_error("failed to convert the XML node tree");
        goto cleanup;
    }

 cleanup:
    xmlBufferFree(xmlbuf);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);
    xmlFreeParserCtxt(xp);
    xmlCleanupParser();
    return ret;
}


/*
 * Private function name:   get_array_from_xpath
 * Since version:           0.4.9
 * Description:             Function is used to get all XPath elements from XML and return in array (character pointer)
 * Arguments:               @xml [string]: input XML document
 *                          @xpath [string]: xPath expression to find nodes in the XML document
 *                          @num [int *]: number of elements
 * Returns:                 pointer to char ** if successful or NULL for error
 */
char **get_array_from_xpath(char *xml, char *xpath, int *num)
{
    xmlParserCtxtPtr xp;
    xmlDocPtr doc;
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;
    xmlNodeSetPtr nodeset;
    int ret = 0, i;
    char *value = NULL;
    char **val = NULL;

    if ((xpath == NULL) || (xml == NULL))
        return NULL;

    xp = xmlCreateDocParserCtxt((xmlChar *)xml);
    if (!xp)
        return NULL;

    doc = xmlCtxtReadDoc(xp, (xmlChar *)xml, NULL, NULL, 0);
    if (!doc) {
        xmlCleanupParser();
        return NULL;
    }

    context = xmlXPathNewContext(doc);
    if (!context) {
        xmlCleanupParser();
        return NULL;
    }

    result = xmlXPathEvalExpression((xmlChar *)xpath, context);
    if (!result) {
        xmlXPathFreeContext(context);
        xmlCleanupParser();
        return NULL;
    }

    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        xmlXPathFreeObject(result);
        xmlXPathFreeContext(context);
        xmlCleanupParser();
        return NULL;
    }

    nodeset = result->nodesetval;
    ret = nodeset->nodeNr;

    if (ret == 0) {
        xmlXPathFreeObject(result);
        xmlFreeDoc(doc);
        xmlXPathFreeContext(context);
        xmlCleanupParser();
        if (num != NULL)
            *num = 0;
        return NULL;
    }

    ret = 0;
    val = (char **)malloc(nodeset->nodeNr  * sizeof(char *));
    for (i = 0; i < nodeset->nodeNr; i++) {
        if ((value = (char *) xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1)))
            val[ret++] = value;
    }

    xmlXPathFreeContext(context);
    xmlXPathFreeObject(result);
    xmlFreeDoc(doc);
    xmlCleanupParser();

    if (num != NULL)
        *num = ret;

    return val;
}

/*
 * Private function name:   dec_to_bin
 * Since version:           0.4.1(-1)
 * Description:             Function dec_to_bin() converts the unsigned long long decimal (used e.g. for IPv4 address) to it's binary representation
 * Arguments:               @decimal [int]: decimal value to be converted to binary interpretation
 *                          @binary [string]: output binary string with the binary interpretation
 * Returns:                 None
 */
void dec_to_bin(long long decimal, char *binary)
{
    int  k = 0, n = 0;
    int  neg_flag = 0;
    long long  remain;
    // int  old_decimal;
    char temp[128] = { 0 };

    if (decimal < 0) {
        decimal = -decimal;
        neg_flag = 1;
    }
    do {
        // old_decimal = decimal;
        remain    = decimal % 2;
        decimal   = decimal / 2;
        temp[k++] = remain + '0';
    } while (decimal > 0);

    if (neg_flag)
        temp[k++] = '-';
    else
        temp[k++] = ' ';

    while (k >= 0)
        binary[n++] = temp[--k];

    binary[n-1] = 0;
}

/*
 * Private function name:   get_subnet_bits
 * Since version:           0.4.1(-1)
 * Description:             Function is used to get number of bits used by subnet determined by IP. Useful to get the CIDR IPv4 address representation
 * Arguments:               @ip [string]: IP address to calculate subnet bits from
 * Returns:                 number of bits used by subnet mask
 */
#ifndef EXTWIN
int get_subnet_bits(char *ip)
{
    char tmp[4] = { 0 };
    int i, part = 0, ii = 0, skip = 0;
    unsigned long long retval = 0;
    char *binary;
    int maxBits = 64;

    for (i = 0; i < (int)strlen(ip); i++) {
        if (ip[i] == '.') {
            ii = 0;
            retval += (atoi(tmp) * pow(256, 3 - part));
            part++;
            memset(tmp, 0, 4);
        } else {
            tmp[ii++] = ip[i];
        }
    }

    retval += (atoi(tmp) * pow(256, 3 - part));
    binary = (char *)malloc(maxBits * sizeof(char));
    dec_to_bin(retval, binary);

    for (i = 0; i < (int)strlen(binary); i++) {
        if ((binary[i] != '1') && (binary[i] != '0'))
            skip++;
        else
            if (binary[i] != '1')
                break;
    }
    VIR_FREE(binary);

    return i - skip;
}
#else
// Always return -1 on Windows systems
int get_subnet_bits(char *ip)
{
    return -1;
}
#endif

/*
 * Private function name:   get_next_free_numeric_value
 * Since version:           0.4.2
 * Description:             Function is used to get the next free slot to be used for adding new NIC device or others
 * Arguments:               @res [virDomainPtr]: standard libvirt domain pointer identified by virDomainPtr
 *                          @xpath [string]: xPath expression of items to get the next free value of
 * Returns:                 next free numeric value
 */
long get_next_free_numeric_value(virDomainPtr domain, char *xpath)
{
    zval *output = NULL;
    char *xml;
    int retval = -1;
    HashTable *arr_hash;
    HashPosition pointer;
    // int array_count;
    zval *data;
    long max_slot = -1;
    char *tmp;

    xml = virDomainGetXMLDesc(domain, VIR_DOMAIN_XML_INACTIVE);
    output = (zval *)emalloc(sizeof(zval));
    array_init(output);

    tmp = get_string_from_xpath(xml, xpath, &output, &retval);
    VIR_FREE(tmp);

    arr_hash = Z_ARRVAL_P(output);
    // array_count = zend_hash_num_elements(arr_hash);
    VIRT_FOREACH(arr_hash, pointer, data) {
        if (Z_TYPE_P(data) == IS_STRING) {
            php_libvirt_hash_key_info info;
            VIRT_HASH_CURRENT_KEY_INFO(arr_hash, pointer, info);

            if (info.type != HASH_KEY_IS_STRING) {
                long num = -1;

                sscanf(Z_STRVAL_P(data), "%lx", &num);
                if (num > max_slot)
                    max_slot = num;
            }
        }
    } VIRT_FOREACH_END();

    efree(output);
    VIR_FREE(xml);
    return max_slot + 1;
}

/*
 * Private function name:   connection_get_domain_type
 * Since version:           0.4.5
 * Description:             Function is required for functions that get the emulator for specific libvirt connection
 * Arguments:               @conn [virConnectPtr]: libvirt connection pointer of connection to get emulator for
 *                          @arch [string]: optional architecture string, can be NULL to get default
 * Returns:                 path to the emulator
 */
char *connection_get_domain_type(virConnectPtr conn, char *arch)
{
    char *ret = NULL;
    char *tmp = NULL;
    char *caps = NULL;
    char *tmpArch = NULL;
    char xpath[1024] = { 0 };
    int retval = -1;

    caps = virConnectGetCapabilities(conn);
    if (caps == NULL)
        return NULL;

    if (arch == NULL) {
        tmpArch = get_string_from_xpath(caps, "//capabilities/host/cpu/arch", NULL, &retval);
        DPRINTF("%s: No architecture defined, got '%s' from capabilities XML\n", __FUNCTION__, tmpArch);
        if (!tmpArch || retval < 0)
            goto cleanup;
        arch = tmpArch;
    }

    DPRINTF("%s: Requested domain type for arch '%s'\n",  __FUNCTION__, arch);

    snprintf(xpath, sizeof(xpath), "//capabilities/guest/arch[@name='%s']/domain/@type", arch);
    DPRINTF("%s: Applying xPath '%s' to capabilities XML output\n", __FUNCTION__, xpath);
    tmp = get_string_from_xpath(caps, xpath, NULL, &retval);
    if ((tmp == NULL) || (retval < 0)) {
        DPRINTF("%s: No domain type found in XML...\n", __FUNCTION__);
        goto cleanup;
    }

    ret = tmp;
    tmp = NULL;
    DPRINTF("%s: Domain type is '%s'\n",  __FUNCTION__, ret);
 cleanup:
    VIR_FREE(tmpArch);
    VIR_FREE(caps);
    VIR_FREE(tmp);
    return ret;
}

/*
 * Private function name:   connection_get_emulator
 * Since version:           0.4.5
 * Description:             Function is required for functions that get the emulator for specific libvirt connection
 * Arguments:               @conn [virConnectPtr]: libvirt connection pointer of connection to get emulator for
 *                          @arch [string]: optional architecture string, can be NULL to get default
 * Returns:                 path to the emulator
 */
char *connection_get_emulator(virConnectPtr conn, char *arch)
{
    char *ret = NULL;
    char *tmp = NULL;
    char *caps = NULL;
    char *tmpArch = NULL;
    char *xpath = NULL;
    int retval = -1;

    caps = virConnectGetCapabilities(conn);
    if (caps == NULL)
        return NULL;

    if (arch == NULL) {
        tmpArch = get_string_from_xpath(caps, "//capabilities/host/cpu/arch", NULL, &retval);
        DPRINTF("%s: No architecture defined, got '%s' from capabilities XML\n", __FUNCTION__, tmpArch);
        if (!tmpArch || retval < 0)
            goto cleanup;
        arch = tmpArch;
    }

    DPRINTF("%s: Requested emulator for arch '%s'\n",  __FUNCTION__, arch);

    if (asprintf(&xpath, "//capabilities/guest/arch[@name='%s']/emulator", arch) < 0)
        goto cleanup;

    DPRINTF("%s: Applying xPath '%s' to capabilities XML output\n",  __FUNCTION__, xpath);
    tmp = get_string_from_xpath(caps, xpath, NULL, &retval);
    if (!tmp || retval < 0) {
        DPRINTF("%s: None emulator found\n",  __FUNCTION__);
        goto cleanup;
    }

 done:
    ret = tmp;
    tmp = NULL;
    DPRINTF("%s: Emulator is '%s'\n",  __FUNCTION__, ret);
 cleanup:
    VIR_FREE(xpath);
    VIR_FREE(tmpArch);
    VIR_FREE(caps);
    VIR_FREE(tmp);
    return ret;
}

/*
 * Private function name:   connection_get_arch
 * Since version:           0.4.5
 * Description:             Function is required for functions that get the architecture for specific libvirt connection
 * Arguments:               @conn [virConnectPtr]: libvirt connection pointer of connection to get architecture for
 * Returns:                 path to the emulator
 */
char *connection_get_arch(virConnectPtr conn)
{
    char *ret = NULL;
    char *tmp = NULL;
    char *caps = NULL;
    int retval = -1;

    caps = virConnectGetCapabilities(conn);
    if (caps == NULL)
        return NULL;

    tmp = get_string_from_xpath(caps, "//capabilities/host/cpu/arch", NULL, &retval);
    if ((tmp == NULL) || (retval < 0)) {
        DPRINTF("%s: Cannot get host CPU architecture from capabilities XML\n", __FUNCTION__);
        goto cleanup;
    }

    ret = tmp;
    tmp = NULL;
    DPRINTF("%s: Host CPU architecture is '%s'\n",  __FUNCTION__, ret);

 cleanup:
    VIR_FREE(caps);
    VIR_FREE(tmp);
    return ret;
}

/*
 * Private function name:   generate_uuid_any
 * Since version:           0.4.5
 * Description:             Function is used to generate a new random UUID string
 * Arguments:               None
 * Returns:                 a new random UUID string
 */
char *generate_uuid_any()
{
    int i;
    char a[37] = { 0 };
    char hexa[] = "0123456789abcdef";
    // virDomainPtr domain = NULL;

    srand(time(NULL));
    for (i = 0; i < 36; i++) {
        if ((i == 8) || (i == 13) || (i == 18) || (i == 23))
            a[i] = '-';
        else
            a[i] = hexa[rand() % strlen(hexa)];
    }

    return strdup(a);
}

/*
 * Private function name:   generate_uuid
 * Since version:           0.4.5
 * Description:             Function is used to generate a new unused UUID string
 * Arguments:               @conn [virConnectPtr]: libvirt connection pointer
 * Returns:                 a new unused random UUID string
 */
char *generate_uuid(virConnectPtr conn)
{
    virDomainPtr domain = NULL;
    char *uuid = NULL;
    int old_error_reporting = EG(error_reporting);
    EG(error_reporting) = 0;

    uuid = generate_uuid_any();
    while ((domain = virDomainLookupByUUIDString(conn, uuid)) != NULL) {
        virDomainFree(domain);
        uuid = generate_uuid_any();
    }
    EG(error_reporting) = old_error_reporting;

    DPRINTF("%s: Generated new UUID '%s'\n", __FUNCTION__, uuid);
    return uuid;
}

/*
 * Private function name:   get_disk_xml
 * Since version:           0.4.5
 * Description:             Function is used to format single disk XML
 * Arguments:               @size [unsigned long long]: size of disk for generating a new one (can be -1 not to generate even if it doesn't exist)
 *                          @path [string]: path to the storage on the host system
 *                          @driver [string]: driver to be used to access the disk
 *                          @dev [string]: device to be presented to the guest
 *                          @disk_flags [int]: disk type, VIR_DOMAIN_DISK_FILE or VIR_DOMAIN_DISK_BLOCK
 * Returns:                 XML output for the disk
 */
char *get_disk_xml(unsigned long long size, char *path, char *driver, char *bus, char *dev, int disk_flags)
{
    char xml[4096] = { 0 };

    if ((path == NULL) || (driver == NULL) || (bus == NULL))
        return NULL;

    if (access(path, R_OK) != 0) {
        if (disk_flags & DOMAIN_DISK_BLOCK) {
            DPRINTF("%s: Cannot access block device %s\n", __FUNCTION__, path);
            return NULL;
        }

        int ret = 0;
        char cmd[4096] = { 0 };
        DPRINTF("%s: Cannot access disk image %s\n", __FUNCTION__, path);

        if (size == -1) {
            DPRINTF("%s: Invalid size. Cannot create image\n", __FUNCTION__);
            return NULL;
        }

        const char *qemu_img_cmd = get_feature_binary("create-image");
        if (qemu_img_cmd == NULL) {
            DPRINTF("%s: Binary for creating disk images doesn't exist\n", __FUNCTION__);
            return NULL;
        }

        // TODO: implement backing file handling: -o backing_file = RAW_IMG_FILE QCOW_IMG
        snprintf(cmd, sizeof(cmd), "%s create -f %s %s %lluM > /dev/null &2>/dev/null", qemu_img_cmd, driver, path, size);

#ifndef EXTWIN
        int cmdRet = system(cmd);
        ret = WEXITSTATUS(cmdRet);
        DPRINTF("%s: Command '%s' finished with error code %d\n", __FUNCTION__, cmd, ret);
        if (ret != 0) {
            DPRINTF("%s: File creation failed\n", path);
            return NULL;
        }
#endif

        if (disk_flags & DOMAIN_DISK_ACCESS_ALL) {
            DPRINTF("%s: Disk flag for all user access found, setting up %s' permissions to 0666\n", __FUNCTION__, path);
            chmod(path, 0666);
        }
    }

    snprintf(xml, sizeof(xml), "\t\t<disk type='%s' device='disk'>\n"
             "\t\t\t<driver name='qemu' type='%s' />\n"
             "\t\t\t<source file='%s'/>\n"
             "\t\t\t<target bus='%s' dev='%s' />\n"
             "\t\t</disk>\n",
             (disk_flags & DOMAIN_DISK_FILE) ? "file" :
             ((disk_flags & DOMAIN_DISK_BLOCK) ? "block" : ""),
             driver, path, bus, dev);
    return strdup(xml);
}

/*
 * Private function name:   get_network_xml
 * Since version:           0.4.5
 * Description:             Function is used to format single network interface XML
 * Arguments:               @mac [string]: MAC address of the new interface
 *                          @network [string]: network name
 *                          @model [string]: optional model name
 * Returns:                 XML output for the network interface
 */
char *get_network_xml(char *mac, char *network, char *model)
{
    char xml[4096] = { 0 };

    if ((mac == NULL) || (network == NULL))
        return NULL;

    if (model == NULL)
        snprintf(xml, sizeof(xml), "\t\t<interface type='network'>\n"
                 "\t\t\t<mac address='%s'/>\n"
                 "\t\t\t<source network='%s'/>\n"
                 "\t\t</interface>\n",
                 mac, network);
    else
        snprintf(xml, sizeof(xml), "\t\t<interface type='network'>\n"
                 "\t\t\t<mac address='%s'/>\n"
                 "\t\t\t<source network='%s'/>\n"
                 "\t\t\t<model type='%s'/>\n"
                 "\t\t</interface>\n",
                 mac, network, model);

    return strdup(xml);
}

/*
 * Private function name:   installation_get_xml
 * Since version:           0.4.5
 * Description:             Function is used to generate the installation XML
 *                          description to install a new domain. If @iso_image
 *                          is not NULL, domain XML is created so that it boots
 *                          from it.
 * Arguments:               @conn [virConnectPtr]: libvirt connection pointer
 *                          @name [string]: name of the new virtual machine
 *                          @memMB [int]: memory in Megabytes
 *                          @maxmemMB [int]: maximum memory in Megabytes
 *                          @arch [string]: architecture to be used for the new domain, may be NULL to use the hypervisor default
 *                          @uuid [string]: UUID to be used or NULL to generate a new one
 *                          @vCpus [int]: number of virtual CPUs for the domain
 *                          @iso_image [string]: ISO image for the installation
 *                          @disks [tVMDisk]: disk structure with all the disks defined
 *                          @numDisks [int]: number of disks in the disk structure
 *                          @networks [tVMNetwork]: network structure with all the networks defined
 *                          @numNetworks [int]: number of networks in the network structure
 *                          @domain_flags [int]: flags for the domain installation
 * Returns:                 full XML output for installation
 */
char *installation_get_xml(virConnectPtr conn, char *name, int memMB,
                           int maxmemMB, char *arch, char *uuid, int vCpus,
                           char *iso_image, tVMDisk *disks, int numDisks,
                           tVMNetwork *networks, int numNetworks,
                           int domain_flags)
{
    int i;
    char *xml = NULL;
    char *emulator = NULL;
    char disks_xml[16384] = { 0 };
    char networks_xml[16384] = { 0 };
    char features[128] = { 0 };
    char *tmp = NULL;
    char *generated_uuid = NULL;
    char type[64] = { 0 };
    int rv;

    if (conn == NULL) {
        DPRINTF("%s: Invalid libvirt connection pointer\n", __FUNCTION__);
        return NULL;
    }

    if (domain_flags & DOMAIN_FLAG_FEATURE_ACPI)
        strcat(features, "<acpi/>");
    if (domain_flags & DOMAIN_FLAG_FEATURE_APIC)
        strcat(features, "<apic/>");
    if (domain_flags & DOMAIN_FLAG_FEATURE_PAE)
        strcat(features, "<pae/>");

    if (arch == NULL) {
        arch = connection_get_arch(conn);
        DPRINTF("%s: No architecture defined, got host arch of '%s'\n", __FUNCTION__, arch);
    }

    if (!(emulator = connection_get_emulator(conn, arch))) {
        DPRINTF("%s: Cannot get emulator\n", __FUNCTION__);
        return NULL;
    }

    if (iso_image && access(iso_image, R_OK) != 0) {
        DPRINTF("%s: Installation image %s doesn't exist\n", __FUNCTION__, iso_image);
        return NULL;
    }

    tmp = connection_get_domain_type(conn, arch);
    if (tmp != NULL)
        snprintf(type, sizeof(type), " type='%s'", tmp);

    for (i = 0; i < numDisks; i++) {
        char *disk = get_disk_xml(disks[i].size, disks[i].path, disks[i].driver, disks[i].bus, disks[i].dev, disks[i].flags);

        if (disk != NULL)
            strcat(disks_xml, disk);

        VIR_FREE(disk);
    }

    for (i = 0; i < numNetworks; i++) {
        char *network = get_network_xml(networks[i].mac, networks[i].network, networks[i].model);

        if (network != NULL)
            strcat(networks_xml, network);

        VIR_FREE(network);
    }

    if (uuid == NULL) {
        generated_uuid = uuid = generate_uuid(conn);
    }

    if (iso_image) {
        rv = asprintf(&xml,
                      "<domain%s>\n"
                      "  <name>%s</name>\n"
                      "  <currentMemory>%d</currentMemory>\n"
                      "  <memory>%d</memory>\n"
                      "  <uuid>%s</uuid>\n"
                      "  <os>\n"
                      "    <type arch='%s'>hvm</type>\n"
                      "    <boot dev='cdrom'/>\n"
                      "    <boot dev='hd'/>\n"
                      "  </os>\n"
                      "  <features>\n"
                      "    %s\n"
                      "  </features>\n"
                      "  <clock offset=\"%s\"/>\n"
                      "  <on_reboot>destroy</on_reboot>\n"
                      "  <vcpu>%d</vcpu>\n"
                      "  <devices>\n"
                      "    <emulator>%s</emulator>\n"
                      "    %s"
                      "    <disk type='file' device='cdrom'>\n"
                      "      <driver name='qemu' type='raw' />\n"
                      "      <source file='%s' />\n"
                      "      <target dev='hdc' bus='ide' />\n"
                      "      <readonly />\n"
                      "    </disk>\n"
                      "    %s"
                      "    <input type='mouse' bus='ps2' />\n"
                      "    <graphics type='vnc' port='-1' />\n"
                      "    <console type='pty' />\n"
                      "    %s"
                      "    <video>\n"
                      "      <model type='cirrus' />\n"
                      "    </video>\n"
                      "  </devices>\n"
                      "</domain>",
            type, name, memMB * 1024, maxmemMB * 1024, uuid, arch, features,
            (domain_flags & DOMAIN_FLAG_CLOCK_LOCALTIME ? "localtime" : "utc"),
            vCpus, emulator, disks_xml,
            iso_image, networks_xml,
            (domain_flags & DOMAIN_FLAG_SOUND_AC97 ? "<sound model='ac97'/>\n" : ""));
    } else {
        rv = asprintf(&xml,
                      "<domain%s>\n"
                      "  <name>%s</name>\n"
                      "  <currentMemory>%d</currentMemory>\n"
                      "  <memory>%d</memory>\n"
                      "  <uuid>%s</uuid>\n"
                      "  <os>\n"
                      "    <type arch='%s'>hvm</type>\n"
                      "    <boot dev='hd'/>\n"
                      "  </os>\n"
                      "  <features>\n"
                      "    %s\n"
                      "  </features>\n"
                      "  <clock offset=\"%s\"/>\n"
                      "  <vcpu>%d</vcpu>\n"
                      "  <devices>\n"
                      "    <emulator>%s</emulator>\n"
                      "    %s"
                      "    <disk type='file' device='cdrom'>\n"
                      "      <driver name='qemu' type='raw' />\n"
                      "      <target dev='hdc' bus='ide' />\n"
                      "      <readonly />\n"
                      "    </disk>\n"
                      "    %s"
                      "    <input type='mouse' bus='ps2' />\n"
                      "    <graphics type='vnc' port='-1' />\n"
                      "    <console type='pty' />\n"
                      "    %s"
                      "    <video>\n"
                      "      <model type='cirrus' />\n"
                      "    </video>\n"
                      "  </devices>\n"
                      "</domain>",
            type, name, memMB * 1024, maxmemMB * 1024, uuid, arch, features,
            (domain_flags & DOMAIN_FLAG_CLOCK_LOCALTIME ? "localtime" : "utc"),
            vCpus, emulator, disks_xml,
            networks_xml,
            (domain_flags & DOMAIN_FLAG_SOUND_AC97 ? "<sound model='ac97'/>\n" : ""));
    }

    VIR_FREE(generated_uuid);
    VIR_FREE(emulator);
    VIR_FREE(tmp);
    VIR_FREE(arch);
    if (rv < 0)
        return NULL;

    return xml;
}

/*
 * Private function name:   streamSink
 * Since version:           0.4.5
 * Description:             Function to write stream to file, borrowed from libvirt
 * Arguments:               @st [virStreamPtr]: stream pointer
 *                          @bytes [void *]: buffer array
 *                          @nbytes [int]: size of buffer
 *                          @opaque [void *]: used for file descriptor
 * Returns:                 write() error code as it's calling write
 */
int streamSink(virStreamPtr st ATTRIBUTE_UNUSED,
               const char *bytes, size_t nbytes, void *opaque)
{
    int *fd = (int *)opaque;

    return write(*fd, bytes, nbytes);
}

void parse_array(zval *arr, tVMDisk *disk, tVMNetwork *network)
{
    HashTable *arr_hash;
    // int array_count;
    zval *data;
    php_libvirt_hash_key_info key;
    HashPosition pointer;

    arr_hash = Z_ARRVAL_P(arr);
    //array_count = zend_hash_num_elements(arr_hash);

    if (disk != NULL)
        memset(disk, 0, sizeof(tVMDisk));
    if (network != NULL)
        memset(network, 0, sizeof(tVMNetwork));

    VIRT_FOREACH(arr_hash, pointer, data) {
        if ((Z_TYPE_P(data) == IS_STRING) || (Z_TYPE_P(data) == IS_LONG)) {
            VIRT_HASH_CURRENT_KEY_INFO(arr_hash, pointer, key);
            if (key.type == HASH_KEY_IS_STRING) {
                if (disk != NULL) {
                    if ((Z_TYPE_P(data) == IS_STRING) && strcmp(key.name, "path") == 0)
                        disk->path = strdup(Z_STRVAL_P(data));
                    else if ((Z_TYPE_P(data) == IS_STRING) && strcmp(key.name, "driver") == 0)
                        disk->driver = strdup(Z_STRVAL_P(data));
                    else if ((Z_TYPE_P(data) == IS_STRING) && strcmp(key.name, "bus") == 0)
                        disk->bus = strdup(Z_STRVAL_P(data));
                    else if ((Z_TYPE_P(data) == IS_STRING) && strcmp(key.name, "dev") == 0)
                        disk->dev = strdup(Z_STRVAL_P(data));
                    else if (strcmp(key.name, "size") == 0) {
                        if (Z_TYPE_P(data) == IS_LONG) {
                            disk->size = Z_LVAL_P(data);
                        } else {
                            disk->size = size_def_to_mbytes(Z_STRVAL_P(data));
                        }
                    } else if ((Z_TYPE_P(data) == IS_LONG) && strcmp(key.name, "flags") == 0)
                        disk->flags = Z_LVAL_P(data);
                } else {
                    if (network != NULL) {
                        if ((Z_TYPE_P(data) == IS_STRING) && strcmp(key.name, "mac") == 0)
                            network->mac = strdup(Z_STRVAL_P(data));
                        else if ((Z_TYPE_P(data) == IS_STRING) && strcmp(key.name, "network") == 0)
                            network->network = strdup(Z_STRVAL_P(data));
                        else if ((Z_TYPE_P(data) == IS_STRING) && strcmp(key.name, "model") == 0)
                            network->model = strdup(Z_STRVAL_P(data));
                    }
                }
            }
        }
    } VIRT_FOREACH_END();
}


void tVMDiskClear(tVMDisk *disk)
{
    VIR_FREE(disk->path);
    VIR_FREE(disk->driver);
    VIR_FREE(disk->bus);
    VIR_FREE(disk->dev);
    memset(disk, 0, sizeof(*disk));
}


void tVMNetworkClear(tVMNetwork *network)
{
    VIR_FREE(network->mac);
    VIR_FREE(network->network);
    VIR_FREE(network->model);
    memset(network, 0, sizeof(*network));
}

/*
 * Function name:   libvirt_version
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get libvirt, driver and libvirt-php version numbers. Can be used for information purposes, for version checking please use libvirt_check_version() defined below
 * Arguments:       @type [string]: optional type string to identify driver to look at
 * Returns:         libvirt, type (driver) and connector (libvirt-php) version numbers array
 */
PHP_FUNCTION(libvirt_version)
{
    unsigned long libVer;
    unsigned long typeVer;
    size_t type_len;
    char *type = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|s", &type, &type_len) == FAILURE) {
        set_error("Invalid arguments");
        RETURN_FALSE;
    }

    if (ZEND_NUM_ARGS() == 0) {
        if (virGetVersion(&libVer, NULL, NULL) != 0)
            RETURN_FALSE;
    } else {
        if (virGetVersion(&libVer, type, &typeVer) != 0)
            RETURN_FALSE;
    }

    /* The version is returned as: major * 1,000,000 + minor * 1,000 + release. */
    array_init(return_value);

    add_assoc_long(return_value, "libvirt.release", (long)(libVer % 1000));
    add_assoc_long(return_value, "libvirt.minor", (long)((libVer/1000) % 1000));
    add_assoc_long(return_value, "libvirt.major", (long)((libVer/1000000) % 1000));

    VIRT_ADD_ASSOC_STRING(return_value, "connector.version", PHP_LIBVIRT_WORLD_VERSION);
    add_assoc_long(return_value, "connector.major", VERSION_MAJOR);
    add_assoc_long(return_value, "connector.minor", VERSION_MINOR);
    add_assoc_long(return_value, "connector.release", VERSION_MICRO);

    if (ZEND_NUM_ARGS() > 0) {
        add_assoc_long(return_value, "type.release", (long)(typeVer % 1000));
        add_assoc_long(return_value, "type.minor", (long)((typeVer/1000) % 1000));
        add_assoc_long(return_value, "type.major", (long)((typeVer/1000000) % 1000));
    }
}

/*
 * Function name:   libvirt_check_version
 * Since version:   0.4.1(-1)
 * Description:     Function is used to check major, minor and micro (also sometimes called release) versions of libvirt-php or libvirt itself. This could useful when you want your application to support only versions of libvirt or libvirt-php higher than some version specified.
 * Arguments:       @major [long]: major version number to check for
 *                  @minor [long]: minor version number to check for
 *                  @micro [long]: micro (also release) version number to check for
 *                  @type [long]: type of checking, VIR_VERSION_BINDING to check against libvirt-php binding or VIR_VERSION_LIBVIRT to check against libvirt version
 * Returns:         TRUE if version is equal or higher than required, FALSE if not, FALSE with error [for libvirt_get_last_error()] on unsupported version type check
 */
PHP_FUNCTION(libvirt_check_version)
{
    unsigned long libVer;
    zend_long major = 0, minor = 0, micro = 0, type = VIR_VERSION_BINDING;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "lll|l", &major, &minor, &micro, &type) == FAILURE) {
        set_error("Invalid arguments");
        RETURN_FALSE;
    }

    if (virGetVersion(&libVer, NULL, NULL) != 0)
        RETURN_FALSE;

    DPRINTF("%s: Checking for version %lu.%lu.%lu of %s\n", PHPFUNC, major, minor, micro,
            (type == VIR_VERSION_BINDING) ? "php bindings" :
            ((type == VIR_VERSION_LIBVIRT) ? "libvirt" : "unknown"));

    if (type == VIR_VERSION_BINDING) {
        if ((VERSION_MAJOR > major) ||
            ((VERSION_MAJOR == major) && (VERSION_MINOR > minor)) ||
            ((VERSION_MAJOR == major) && (VERSION_MINOR == minor) &&
             (VERSION_MICRO >= micro)))
            RETURN_TRUE;
    } else {
        if (type == VIR_VERSION_LIBVIRT) {
            if ((((libVer/1000000) % 1000) > major) ||
                ((((libVer/1000000) % 1000) == major) && (((libVer/1000) % 1000) > minor)) ||
                ((((libVer/1000000) % 1000) == major) && (((libVer/1000) % 1000) == minor) &&
                 ((libVer % 1000) >= micro)))
                RETURN_TRUE;
        } else {
            set_error("Invalid version type");
        }
    }

    RETURN_FALSE;
}

/*
 * Function name:       libvirt_has_feature
 * Since version:       0.4.1(-3)
 * Description:         Function to check for feature existence for working libvirt instance
 * Arguments:           @name [string]: feature name
 * Returns:             TRUE if feature is supported, FALSE otherwise
 */
PHP_FUNCTION(libvirt_has_feature)
{
    char *name = NULL;
    size_t name_len = 0;
    const char *binary = NULL;
    int ret = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &name, &name_len) == FAILURE) {
        set_error("Invalid argument");
        RETURN_FALSE;
    }

    binary = get_feature_binary(name);
    ret = ((binary != NULL) || (has_builtin(name)));

    if (ret)
        RETURN_TRUE;

    RETURN_FALSE;
}

/*
 * Function name:       libvirt_get_iso_images
 * Since version:       0.4.1(-3)
 * Description:         Function to get the ISO images on path and return them in the array
 * Arguments:           @path [string]: string of path where to look for the ISO images
 * Returns:             ISO image array on success, FALSE otherwise
 */
PHP_FUNCTION(libvirt_get_iso_images)
{
    char *path = NULL;
    size_t path_len = 0;
#ifndef EXTWIN
    struct dirent *entry;
    DIR *d = NULL;
#endif
    int num = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|s", &path, &path_len) == FAILURE) {
        set_error("Invalid argument");
        RETURN_FALSE;
    }

    if (LIBVIRT_G(iso_path_ini))
        path = strdup(LIBVIRT_G(iso_path_ini));

    if ((path == NULL) || (path[0] != '/')) {
        set_error("Invalid argument, path must be set and absolute (start by slash character [/])");
        RETURN_FALSE;
    }

    DPRINTF("%s: Getting ISO images on path %s\n", PHPFUNC, path);

#ifndef EXTWIN
    if ((d = opendir(path)) != NULL) {
        array_init(return_value);
        while ((entry = readdir(d)) != NULL) {
            if (strcasecmp(entry->d_name + strlen(entry->d_name) - 4, ".iso") == 0) {
                VIRT_ADD_NEXT_INDEX_STRING(return_value, entry->d_name);
                num++;
            }
        }
        closedir(d);
    } else {
        printf("Error: %d\n", errno);
    }
#endif

    if (num == 0)
        RETURN_FALSE;
}

/*
 * Function name:       libvirt_print_binding_resources
 * Since version:       0.4.2
 * Description:         Function to print the binding resources, although the resource information are printed, they are returned in the return_value
 * Arguments:           None
 * Returns:             bindings resource information
 */
PHP_FUNCTION(libvirt_print_binding_resources)
{
    int binding_resources_count = 0;
    resource_info *binding_resources;
    char tmp[256] = { 0 };
    int i;

    binding_resources_count = LIBVIRT_G(binding_resources_count);
    binding_resources = LIBVIRT_G(binding_resources);

    array_init(return_value);
    for (i = 0; i < binding_resources_count; i++) {
        if (binding_resources[i].overwrite == 0) {
            if (binding_resources[i].conn != NULL)
                snprintf(tmp, sizeof(tmp), "Libvirt %s resource at 0x%lx (connection %lx)", translate_counter_type(binding_resources[i].type),
                         (long)binding_resources[i].mem, (long)binding_resources[i].conn);
            else
                snprintf(tmp, sizeof(tmp), "Libvirt %s resource at 0x%lx", translate_counter_type(binding_resources[i].type),
                         (long)binding_resources[i].mem);
            VIRT_ADD_NEXT_INDEX_STRING(return_value, tmp);
        }
    }

    if (binding_resources_count == 0)
        RETURN_FALSE;
}

#ifdef DEBUG_SUPPORT
/*
 * Function name:       libvirt_logfile_set
 * Since version:       0.4.2
 * Description:         Function to set the log file for the libvirt module instance
 * Arguments:           @filename [string]: log filename or NULL to disable logging
 *                      @maxsize [long]: optional maximum log file size argument in KiB, default value can be found in PHPInfo() output
 * Returns:             TRUE if log file has been successfully set, FALSE otherwise
 */
PHP_FUNCTION(libvirt_logfile_set)
{
    char *filename = NULL;
    zend_long maxsize = DEFAULT_LOG_MAXSIZE;
    size_t filename_len = 0;
    int err;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|l", &filename, &filename_len, &maxsize) == FAILURE) {
        set_error("Invalid argument");
        RETURN_FALSE;
    }

    if ((filename == NULL) || (strcasecmp(filename, "null") == 0))
        err = set_logfile(NULL, 0);
    else
        err = set_logfile(filename, maxsize);

    if (err < 0) {
        char tmp[1024] = { 0 };
        snprintf(tmp, sizeof(tmp), "Cannot set the log file to %s, error code = %d (%s)", filename, err, strerror(-err));
        set_error(tmp);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
#endif
