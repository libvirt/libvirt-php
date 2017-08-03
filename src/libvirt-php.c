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
#include <stdafx.h>

#ifdef EXTWIN
#define PHP_COMPILER_ID  "VC9"
#endif

#include "libvirt-php.h"
#include "vncfunc.h"
#include "sockets.h"
#include "libvirt-connection.h"
#include "libvirt-node.h"
#include "libvirt-stream.h"
#include "libvirt-domain.h"

DEBUG_INIT("core");

#ifndef EXTWIN
/* Additional binaries */
const char *features[] = { "screenshot", "create-image", "screenshot-convert", NULL };
const char *features_binaries[] = { "/usr/bin/gvnccapture", "/usr/bin/qemu-img", "/bin/convert", NULL };
#else
const char *features[] = { NULL };
const char *features_binaries[] = { NULL };
#endif

/* ZEND thread safe per request globals definition */
int le_libvirt_storagepool;
int le_libvirt_volume;
int le_libvirt_network;
int le_libvirt_nodedev;
int le_libvirt_snapshot;
int le_libvirt_nwfilter;

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

static zend_function_entry libvirt_functions[] = {
    /* Common functions */
    PHP_FE(libvirt_get_last_error,               arginfo_libvirt_void)
    PHP_FE_LIBVIRT_CONNECTION
    PHP_FE_LIBVIRT_STREAM
    PHP_FE_LIBVIRT_DOMAIN
    /* Domain snapshot functions */
    PHP_FE(libvirt_domain_has_current_snapshot,  arginfo_libvirt_conn_optflags)
    PHP_FE(libvirt_domain_snapshot_create,       arginfo_libvirt_conn_optflags)
    PHP_FE(libvirt_domain_snapshot_get_xml,      arginfo_libvirt_conn_optflags)
    PHP_FE(libvirt_domain_snapshot_revert,       arginfo_libvirt_conn_optflags)
    PHP_FE(libvirt_domain_snapshot_delete,       arginfo_libvirt_conn_optflags)
    PHP_FE(libvirt_domain_snapshot_lookup_by_name, arginfo_libvirt_domain_snapshot_lookup_by_name)
    /* Storagepool functions */
    PHP_FE(libvirt_storagepool_lookup_by_name,   arginfo_libvirt_conn_name)
    PHP_FE(libvirt_storagepool_lookup_by_volume, arginfo_libvirt_conn)
    PHP_FE(libvirt_storagepool_get_info,         arginfo_libvirt_conn)
    PHP_FE(libvirt_storagevolume_lookup_by_name, arginfo_libvirt_conn_name)
    PHP_FE(libvirt_storagevolume_lookup_by_path, arginfo_libvirt_storagevolume_lookup_by_path)
    PHP_FE(libvirt_storagevolume_get_name,       arginfo_libvirt_conn)
    PHP_FE(libvirt_storagevolume_get_path,       arginfo_libvirt_conn)
    PHP_FE(libvirt_storagevolume_get_info,       arginfo_libvirt_conn)
    PHP_FE(libvirt_storagevolume_get_xml_desc,   arginfo_libvirt_storagevolume_get_xml_desc)
    PHP_FE(libvirt_storagevolume_create_xml,     arginfo_libvirt_conn_xml)
    PHP_FE(libvirt_storagevolume_create_xml_from,arginfo_libvirt_storagevolume_create_xml_from)
    PHP_FE(libvirt_storagevolume_delete,         arginfo_libvirt_conn_optflags)
    PHP_FE(libvirt_storagevolume_download,       arginfo_libvirt_storagevolume_download)
    PHP_FE(libvirt_storagevolume_upload,         arginfo_libvirt_storagevolume_download)
    PHP_FE(libvirt_storagevolume_resize,         arginfo_libvirt_storagevolume_resize)
    PHP_FE(libvirt_storagepool_get_uuid_string,  arginfo_libvirt_conn)
    PHP_FE(libvirt_storagepool_get_name,         arginfo_libvirt_conn)
    PHP_FE(libvirt_storagepool_lookup_by_uuid_string, arginfo_libvirt_conn_uuid)
    PHP_FE(libvirt_storagepool_get_xml_desc,     arginfo_libvirt_conn_xpath)
    PHP_FE(libvirt_storagepool_define_xml,       arginfo_libvirt_storagepool_define_xml)
    PHP_FE(libvirt_storagepool_undefine,         arginfo_libvirt_conn)
    PHP_FE(libvirt_storagepool_create,           arginfo_libvirt_conn)
    PHP_FE(libvirt_storagepool_destroy,          arginfo_libvirt_conn)
    PHP_FE(libvirt_storagepool_is_active,        arginfo_libvirt_conn)
    PHP_FE(libvirt_storagepool_get_volume_count, arginfo_libvirt_conn)
    PHP_FE(libvirt_storagepool_refresh,          arginfo_libvirt_conn_optflags)
    PHP_FE(libvirt_storagepool_set_autostart,    arginfo_libvirt_conn_flags)
    PHP_FE(libvirt_storagepool_get_autostart,    arginfo_libvirt_conn)
    PHP_FE(libvirt_storagepool_build,            arginfo_libvirt_conn)
    PHP_FE(libvirt_storagepool_delete,           arginfo_libvirt_conn)
    /* Network functions */
    PHP_FE(libvirt_network_define_xml,           arginfo_libvirt_conn_xml)
    PHP_FE(libvirt_network_undefine,             arginfo_libvirt_conn)
    PHP_FE(libvirt_network_get,                  arginfo_libvirt_conn_name)
    PHP_FE(libvirt_network_get_xml_desc,         arginfo_libvirt_conn_xpath)
    PHP_FE(libvirt_network_get_bridge,           arginfo_libvirt_conn)
    PHP_FE(libvirt_network_get_information,      arginfo_libvirt_conn)
    PHP_FE(libvirt_network_get_active,           arginfo_libvirt_conn)
    PHP_FE(libvirt_network_set_active,           arginfo_libvirt_conn_flags)
    PHP_FE(libvirt_network_get_uuid_string,      arginfo_libvirt_conn)
    PHP_FE(libvirt_network_get_uuid,             arginfo_libvirt_conn)
    PHP_FE(libvirt_network_get_name,             arginfo_libvirt_conn)
    PHP_FE(libvirt_network_get_autostart,        arginfo_libvirt_conn)
    PHP_FE(libvirt_network_set_autostart,        arginfo_libvirt_conn_flags)
    /* Node functions */
    PHP_FE_LIBVIRT_NODE
    /* Nodedev functions */
    PHP_FE(libvirt_nodedev_get,                  arginfo_libvirt_conn)
    PHP_FE(libvirt_nodedev_capabilities,         arginfo_libvirt_conn)
    PHP_FE(libvirt_nodedev_get_xml_desc,         arginfo_libvirt_conn_xpath)
    PHP_FE(libvirt_nodedev_get_information,      arginfo_libvirt_conn)
    /* NWFilter functions */
    PHP_FE(libvirt_nwfilter_define_xml,          arginfo_libvirt_conn_xml)
    PHP_FE(libvirt_nwfilter_undefine,            arginfo_libvirt_conn)
    PHP_FE(libvirt_nwfilter_get_xml_desc,        arginfo_libvirt_conn_xpath)
    PHP_FE(libvirt_nwfilter_get_uuid_string,     arginfo_libvirt_conn)
    PHP_FE(libvirt_nwfilter_get_uuid,            arginfo_libvirt_conn)
    PHP_FE(libvirt_nwfilter_get_name,            arginfo_libvirt_conn)
    PHP_FE(libvirt_nwfilter_lookup_by_name,      arginfo_libvirt_conn_name)
    PHP_FE(libvirt_nwfilter_lookup_by_uuid_string, arginfo_libvirt_conn_uuid)
    PHP_FE(libvirt_nwfilter_lookup_by_uuid,      arginfo_libvirt_conn_uuid)
    /* List functions */
    PHP_FE(libvirt_list_domain_snapshots,        arginfo_libvirt_conn_optflags)
    PHP_FE(libvirt_list_nodedevs,                arginfo_libvirt_conn_optcap)
    PHP_FE(libvirt_list_all_networks,            arginfo_libvirt_conn_optflags)
    PHP_FE(libvirt_list_networks,                arginfo_libvirt_conn_optflags)
    PHP_FE(libvirt_list_storagepools,            arginfo_libvirt_conn)
    PHP_FE(libvirt_list_active_storagepools,     arginfo_libvirt_conn)
    PHP_FE(libvirt_list_inactive_storagepools,   arginfo_libvirt_conn)
    PHP_FE(libvirt_storagepool_list_volumes,     arginfo_libvirt_conn)
    PHP_FE(libvirt_list_all_nwfilters,           arginfo_libvirt_conn)
    PHP_FE(libvirt_list_nwfilters,               arginfo_libvirt_conn)
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
STD_PHP_INI_ENTRY("libvirt.iso_path", "/var/lib/libvirt/images/iso", PHP_INI_ALL, OnUpdateString, iso_path_ini, zend_libvirt_globals, libvirt_globals)
STD_PHP_INI_ENTRY("libvirt.image_path", "/var/lib/libvirt/images", PHP_INI_ALL, OnUpdateString, image_path_ini, zend_libvirt_globals, libvirt_globals)
STD_PHP_INI_ENTRY("libvirt.max_connections", "5", PHP_INI_ALL, OnUpdateLong, max_connections_ini, zend_libvirt_globals, libvirt_globals)
PHP_INI_END()

void change_debug(int val TSRMLS_DC)
{
#ifdef DEBUG_SUPPORT
    LIBVIRT_G(debug) = val;
#endif
    setDebug(val);
}

/* PHP requires to have this function defined */
static void php_libvirt_init_globals(zend_libvirt_globals *libvirt_globals TSRMLS_DC)
{
    libvirt_globals->longlong_to_string_ini = 1;
    libvirt_globals->iso_path_ini = "/var/lib/libvirt/images/iso";
    libvirt_globals->image_path_ini = "/var/lib/libvirt/images";
    libvirt_globals->max_connections_ini = 5;
    libvirt_globals->binding_resources_count = 0;
    libvirt_globals->binding_resources = NULL;
#ifdef DEBUG_SUPPORT
    libvirt_globals->debug = 0;
    change_debug(0 TSRMLS_CC);
#endif
}

/* PHP request initialization */
PHP_RINIT_FUNCTION(libvirt)
{
    LIBVIRT_G(last_error) = NULL;
    LIBVIRT_G(vnc_location) = NULL;
    change_debug(0 TSRMLS_CC);
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
int set_logfile(char *filename, long maxsize TSRMLS_DC)
{
    int res;
    struct stat st;

    if (filename == NULL) {
        change_debug(0 TSRMLS_CC);
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
        change_debug(1 TSRMLS_CC);
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
 * Description:             Function to tokenize string into tokens by delimiter $by
 * Arguments:               @string [string]: input string
 *                          @by [string]: string used as delimited
 * Returns:                 tTokenizer structure
 */
tTokenizer tokenize(char *string, char *by)
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
        token = strtok_r(str, by, &save);
        if (token == NULL)
            break;

        t.tokens = realloc(t.tokens, (i + 1) * sizeof(char *));
        t.tokens[i++] = strdup(token);
    }

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
        free(t.tokens[i]);
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
int resource_change_counter(int type, virConnectPtr conn, void *mem, int inc TSRMLS_DC)
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
        snprintf(version, sizeof(version), "%i.%i.%i", (long)((libVer/1000000) % 1000), (long)((libVer/1000) % 1000), (long)(libVer % 1000));
        php_info_print_table_row(2, "Libvirt version", version);
    }

    snprintf(path, sizeof(path), "%lu", (unsigned long)LIBVIRT_G(max_connections_ini));
    php_info_print_table_row(2, "Max. connections", path);

    if (!access(LIBVIRT_G(iso_path_ini), F_OK) == 0)
        snprintf(path, sizeof(path), "%s - path is invalid. To set the valid path modify the libvirt.iso_path in your php.ini configuration!",
                 LIBVIRT_G(iso_path_ini));
    else
        snprintf(path, sizeof(path), "%s", LIBVIRT_G(iso_path_ini));

    php_info_print_table_row(2, "ISO Image path", path);

    if (!access(LIBVIRT_G(image_path_ini), F_OK) == 0)
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
void set_error(char *msg TSRMLS_DC)
{
    if (LIBVIRT_G(last_error) != NULL)
        efree(LIBVIRT_G(last_error));

    if (msg == NULL) {
        LIBVIRT_G(last_error) = NULL;
        return;
    }

    php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", msg);
    LIBVIRT_G(last_error) = estrndup(msg, strlen(msg));
}

/*
 * Private function name:   set_vnc_location
 * Since version:           0.4.5
 * Description:             This private function is used to set the VNC location for the newly started installation
 * Arguments:               @msg [string]: vnc location string
 * Returns:                 None
 */
void set_vnc_location(char *msg TSRMLS_DC)
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
void set_error_if_unset(char *msg TSRMLS_DC)
{
    if (LIBVIRT_G(last_error) == NULL)
        set_error(msg TSRMLS_CC);
}

/*
 * Private function name:   reset_error
 * Since version:           0.4.2
 * Description:             Function to reset the error string set by set_error(). Same as set_error(NULL).
 * Arguments:               None
 * Returns:                 None
 */
void reset_error(TSRMLS_D)
{
    set_error(NULL TSRMLS_CC);
}


/* Error handler for receiving libvirt errors */
static void catch_error(void *userData,
                        virErrorPtr error)
{
    TSRMLS_FETCH_FROM_CTX(userData);
    set_error(error->message TSRMLS_CC);
}

/*
 * Private function name:   free_resource
 * Since version:           0.4.2
 * Description:             Function is used to free the the internal libvirt-php resource identified by it's type and memory location
 * Arguments:               @type [int]: type of the resource to be freed, INT_RESOURCE_x where x can be { CONNECTION | DOMAIN | NETWORK | NODEDEV | STORAGEPOOL | VOLUME | SNAPSHOT }
 *                          @mem [uint]: memory location of the resource to be freed
 * Returns:                 None
 */
void free_resource(int type, void *mem TSRMLS_DC)
{
    int rv;

    DPRINTF("%s: Freeing libvirt %s resource at 0x%lx\n", __FUNCTION__, translate_counter_type(type), (long) mem);

    if (type == INT_RESOURCE_DOMAIN) {
        rv = virDomainFree((virDomainPtr)mem);
        if (rv != 0) {
            DPRINTF("%s: virDomainFree(%p) returned %d (%s)\n", __FUNCTION__, (virDomainPtr)mem, rv, LIBVIRT_G(last_error));
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "virDomainFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
        } else {
            DPRINTF("%s: virDomainFree(%p) completed successfully\n", __FUNCTION__, (virDomainPtr)mem);
            resource_change_counter(INT_RESOURCE_DOMAIN, NULL, (virDomainPtr)mem, 0 TSRMLS_CC);
        }
    }

    if (type == INT_RESOURCE_STREAM) {
        rv = virStreamFree((virStreamPtr)mem);
        if (rv != 0) {
            DPRINTF("%s: virStreamFree(%p) returned %d (%s)\n", __FUNCTION__, (virStreamPtr)mem, rv, LIBVIRT_G(last_error));
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "virStreamFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
        } else {
            DPRINTF("%s: virStreamFree(%p) completed successfully\n", __FUNCTION__, (virStreamPtr)mem);
            resource_change_counter(INT_RESOURCE_STREAM, NULL, (virStreamPtr)mem, 0 TSRMLS_CC);
        }
    }

    if (type == INT_RESOURCE_NETWORK) {
        rv = virNetworkFree((virNetworkPtr)mem);
        if (rv != 0) {
            DPRINTF("%s: virNetworkFree(%p) returned %d (%s)\n", __FUNCTION__, (virNetworkPtr)mem, rv, LIBVIRT_G(last_error));
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "virNetworkFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
        } else {
            DPRINTF("%s: virNetworkFree(%p) completed successfully\n", __FUNCTION__, (virNetworkPtr)mem);
            resource_change_counter(INT_RESOURCE_NETWORK, NULL, (virNetworkPtr)mem, 0 TSRMLS_CC);
        }
    }

    if (type == INT_RESOURCE_NODEDEV) {
        rv = virNodeDeviceFree((virNodeDevicePtr)mem);
        if (rv != 0) {
            DPRINTF("%s: virNodeDeviceFree(%p) returned %d (%s)\n", __FUNCTION__, (virNodeDevicePtr)mem, rv, LIBVIRT_G(last_error));
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "virNodeDeviceFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
        } else {
            DPRINTF("%s: virNodeDeviceFree(%p) completed successfully\n", __FUNCTION__, (virNodeDevicePtr)mem);
            resource_change_counter(INT_RESOURCE_NODEDEV, NULL, (virNodeDevicePtr)mem, 0 TSRMLS_CC);
        }
    }

    if (type == INT_RESOURCE_STORAGEPOOL) {
        rv = virStoragePoolFree((virStoragePoolPtr)mem);
        if (rv != 0) {
            DPRINTF("%s: virStoragePoolFree(%p) returned %d (%s)\n", __FUNCTION__, (virStoragePoolPtr)mem, rv, LIBVIRT_G(last_error));
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "virStoragePoolFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
        } else {
            DPRINTF("%s: virStoragePoolFree(%p) completed successfully\n", __FUNCTION__, (virStoragePoolPtr)mem);
            resource_change_counter(INT_RESOURCE_STORAGEPOOL, NULL, (virStoragePoolPtr)mem, 0 TSRMLS_CC);
        }
    }

    if (type == INT_RESOURCE_VOLUME) {
        rv = virStorageVolFree((virStorageVolPtr)mem);
        if (rv != 0) {
            DPRINTF("%s: virStorageVolFree(%p) returned %d (%s)\n", __FUNCTION__, (virStorageVolPtr)mem, rv, LIBVIRT_G(last_error));
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "virStorageVolFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
        } else {
            DPRINTF("%s: virStorageVolFree(%p) completed successfully\n", __FUNCTION__, (virStorageVolPtr)mem);
            resource_change_counter(INT_RESOURCE_VOLUME, NULL, (virStorageVolPtr)mem, 0 TSRMLS_CC);
        }
    }

    if (type == INT_RESOURCE_SNAPSHOT) {
        rv = virDomainSnapshotFree((virDomainSnapshotPtr)mem);
        if (rv != 0) {
            DPRINTF("%s: virDomainSnapshotFree(%p) returned %d (%s)\n", __FUNCTION__, (virDomainSnapshotPtr)mem, rv, LIBVIRT_G(last_error));
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "virDomainSnapshotFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
        } else {
            DPRINTF("%s: virDomainSnapshotFree(%p) completed successfully\n", __FUNCTION__, (virDomainSnapshotPtr)mem);
            resource_change_counter(INT_RESOURCE_SNAPSHOT, NULL, (virDomainSnapshotPtr)mem, 0 TSRMLS_CC);
        }
    }

    if (type == INT_RESOURCE_NWFILTER) {
        rv = virNWFilterFree((virNWFilterPtr) mem);
        if (rv != 0) {
            DPRINTF("%s: virNWFilterFree(%p) returned %d (%s)\n", __FUNCTION__, (virNWFilterPtr) mem, rv, LIBVIRT_G(last_error));
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "virDomainSnapshotFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
        } else {
            DPRINTF("%s: virNWFilterFree(%p) completed successfully\n", __FUNCTION__, (virNWFilterPtr) mem);
            resource_change_counter(INT_RESOURCE_NWFILTER, NULL, (virNWFilterPtr) mem, 0 TSRMLS_CC);
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
int check_resource_allocation(virConnectPtr conn, int type, void *mem TSRMLS_DC)
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
int count_resources(int type TSRMLS_DC)
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
    char *hostname;
    char name[1024];

    hostname = virConnectGetHostname(conn);
    gethostname(name, 1024);
    ret = strcmp(name, hostname) == 0;
    free(hostname);
    return ret;
#else
    // Libvirt daemon doesn't work on Windows systems so always return 0 (FALSE)
    return 0;
#endif
}

/* Destructor for storagepool resource */
static void php_libvirt_storagepool_dtor(virt_resource *rsrc TSRMLS_DC)
{
    php_libvirt_storagepool *pool = (php_libvirt_storagepool *)rsrc->ptr;
    int rv = 0;

    if (pool != NULL) {
        if (pool->pool != NULL) {
            if (!check_resource_allocation(NULL, INT_RESOURCE_STORAGEPOOL, pool->pool TSRMLS_CC)) {
                pool->pool = NULL;
                efree(pool);
                return;
            }
            rv = virStoragePoolFree(pool->pool);
            if (rv != 0) {
                DPRINTF("%s: virStoragePoolFree(%p) returned %d (%s)\n", __FUNCTION__, pool->pool, rv, LIBVIRT_G(last_error));
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "virStoragePoolFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
            } else {
                DPRINTF("%s: virStoragePoolFree(%p) completed successfully\n", __FUNCTION__, pool->pool);
                resource_change_counter(INT_RESOURCE_STORAGEPOOL, NULL, pool->pool, 0 TSRMLS_CC);
            }
            pool->pool = NULL;
        }
        efree(pool);
    }
}

/* Destructor for volume resource */
static void php_libvirt_volume_dtor(virt_resource *rsrc TSRMLS_DC)
{
    php_libvirt_volume *volume = (php_libvirt_volume *)rsrc->ptr;
    int rv = 0;

    if (volume != NULL) {
        if (volume->volume != NULL) {
            if (!check_resource_allocation(NULL, INT_RESOURCE_VOLUME, volume->volume TSRMLS_CC)) {
                volume->volume = NULL;
                efree(volume);
                return;
            }
            rv = virStorageVolFree(volume->volume);
            if (rv != 0) {
                DPRINTF("%s: virStorageVolFree(%p) returned %d (%s)\n", __FUNCTION__, volume->volume, rv, LIBVIRT_G(last_error));
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "virStorageVolFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
            } else {
                DPRINTF("%s: virStorageVolFree(%p) completed successfully\n", __FUNCTION__, volume->volume);
                resource_change_counter(INT_RESOURCE_VOLUME, NULL, volume->volume, 0 TSRMLS_CC);
            }
            volume->volume = NULL;
        }
        efree(volume);
    }
}

/* Destructor for network resource */
static void php_libvirt_network_dtor(virt_resource *rsrc TSRMLS_DC)
{
    php_libvirt_network *network = (php_libvirt_network *)rsrc->ptr;
    int rv = 0;

    if (network != NULL) {
        if (network->network != NULL) {
            if (!check_resource_allocation(network->conn->conn, INT_RESOURCE_NETWORK, network->network TSRMLS_CC)) {
                network->network = NULL;
                efree(network);
                return;
            }
            rv = virNetworkFree(network->network);
            if (rv != 0) {
                DPRINTF("%s: virNetworkFree(%p) returned %d (%s)\n", __FUNCTION__, network->network, rv, LIBVIRT_G(last_error));
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "virStorageVolFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
            } else {
                DPRINTF("%s: virNetworkFree(%p) completed successfully\n", __FUNCTION__, network->network);
                resource_change_counter(INT_RESOURCE_NETWORK, NULL, network->network, 0 TSRMLS_CC);
            }
            network->network = NULL;
        }
        efree(network);
    }
}

/* Destructor for nodedev resource */
static void php_libvirt_nodedev_dtor(virt_resource *rsrc TSRMLS_DC)
{
    php_libvirt_nodedev *nodedev = (php_libvirt_nodedev *)rsrc->ptr;
    int rv = 0;

    if (nodedev != NULL) {
        if (nodedev->device != NULL) {
            if (!check_resource_allocation(nodedev->conn->conn, INT_RESOURCE_NODEDEV, nodedev->device TSRMLS_CC)) {
                nodedev->device = NULL;
                efree(nodedev);
                return;
            }
            rv = virNodeDeviceFree(nodedev->device);
            if (rv != 0) {
                DPRINTF("%s: virNodeDeviceFree(%p) returned %d (%s)\n", __FUNCTION__, nodedev->device, rv, LIBVIRT_G(last_error));
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "virStorageVolFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
            } else {
                DPRINTF("%s: virNodeDeviceFree(%p) completed successfully\n", __FUNCTION__, nodedev->device);
                resource_change_counter(INT_RESOURCE_NODEDEV, nodedev->conn->conn, nodedev->device, 0 TSRMLS_CC);
            }
            nodedev->device = NULL;
        }
        efree(nodedev);
    }
}

/* Destructor for snapshot resource */
static void php_libvirt_snapshot_dtor(virt_resource *rsrc TSRMLS_DC)
{
    php_libvirt_snapshot *snapshot = (php_libvirt_snapshot *)rsrc->ptr;
    int rv = 0;

    if (snapshot != NULL) {
        if (snapshot->snapshot != NULL) {
            if (!check_resource_allocation(NULL, INT_RESOURCE_SNAPSHOT, snapshot->snapshot TSRMLS_CC)) {
                snapshot->snapshot = NULL;
                efree(snapshot);
                return;
            }
            rv = virDomainSnapshotFree(snapshot->snapshot);
            if (rv != 0) {
                DPRINTF("%s: virDomainSnapshotFree(%p) returned %d\n", __FUNCTION__, snapshot->snapshot, rv);
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "virDomainSnapshotFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
            } else {
                DPRINTF("%s: virDomainSnapshotFree(%p) completed successfully\n", __FUNCTION__, snapshot->snapshot);
                resource_change_counter(INT_RESOURCE_SNAPSHOT, snapshot->domain->conn->conn, snapshot->snapshot, 0 TSRMLS_CC);
            }
            snapshot->snapshot = NULL;
        }
        efree(snapshot);
    }
}

/* Destructor for nwfilter resource */
static void php_libvirt_nwfilter_dtor(virt_resource *rsrc TSRMLS_DC)
{
    php_libvirt_nwfilter *nwfilter = (php_libvirt_nwfilter *) rsrc->ptr;
    int rv = 0;

    if (nwfilter != NULL) {
        if (nwfilter->nwfilter != NULL) {
            if (!check_resource_allocation(NULL, INT_RESOURCE_NWFILTER, nwfilter->nwfilter TSRMLS_CC)) {
                nwfilter->nwfilter = NULL;
                efree(nwfilter);

                return;
            }
            rv = virNWFilterFree(nwfilter->nwfilter);
            if (rv != 0) {
                DPRINTF("%s: virNWFilterFree(%p) returned %d\n", __FUNCTION__, nwfilter->nwfilter, rv);
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "virNWFilterFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
            } else {
                DPRINTF("%s: virNWFilterFee(%p) completed successfully\n", __FUNCTION__, nwfilter->nwfilter);
                resource_change_counter(INT_RESOURCE_NWFILTER, nwfilter->conn->conn, nwfilter->nwfilter, 0 TSRMLS_CC);
            }
            nwfilter->nwfilter = NULL;
        }
        efree(nwfilter);
    }
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

    /* XML contants */
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
    REGISTER_LONG_CONSTANT("VIR_CRED_NOECHOPROMP",      7, CONST_CS | CONST_PERSISTENT);
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

    REGISTER_INI_ENTRIES();

    /* Initialize libvirt and set up error callback */
    virInitialize();

    void *thread_ctx = NULL;
    TSRMLS_SET_CTX(thread_ctx);
    virSetErrorFunc(thread_ctx, catch_error);

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

/* Macros for obtaining resources from arguments */
#define GET_NETWORK_FROM_ARGS(args, ...)                                                        \
    do {                                                                                        \
        reset_error(TSRMLS_C);                                                                  \
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, args, __VA_ARGS__) == FAILURE) {   \
            set_error("Invalid arguments" TSRMLS_CC);                                           \
            RETURN_FALSE;                                                                       \
        }                                                                                       \
                                                                                                \
        VIRT_FETCH_RESOURCE(network, php_libvirt_network*, &znetwork, PHP_LIBVIRT_NETWORK_RES_NAME, le_libvirt_network);\
        if ((network == NULL) || (network->network == NULL))                                    \
            RETURN_FALSE;                                                                       \
    } while (0)                                                                                 \

#define GET_NODEDEV_FROM_ARGS(args, ...)                                                        \
    do {                                                                                        \
        reset_error(TSRMLS_C);                                                                  \
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, args, __VA_ARGS__) == FAILURE) {   \
            set_error("Invalid arguments" TSRMLS_CC);                                           \
            RETURN_FALSE;                                                                       \
        }                                                                                       \
                                                                                                \
        VIRT_FETCH_RESOURCE(nodedev, php_libvirt_nodedev*, &znodedev, PHP_LIBVIRT_NODEDEV_RES_NAME, le_libvirt_nodedev);\
        if ((nodedev == NULL) || (nodedev->device == NULL))                                     \
            RETURN_FALSE;                                                                       \
    } while (0)

#define GET_STORAGEPOOL_FROM_ARGS(args, ...)                                                    \
    do {                                                                                        \
        reset_error(TSRMLS_C);                                                                  \
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, args, __VA_ARGS__) == FAILURE) {   \
            set_error("Invalid arguments" TSRMLS_CC);                                           \
            RETURN_FALSE;                                                                       \
        }                                                                                       \
                                                                                                \
        VIRT_FETCH_RESOURCE(pool, php_libvirt_storagepool*, &zpool, PHP_LIBVIRT_STORAGEPOOL_RES_NAME, le_libvirt_storagepool);\
        if ((pool == NULL) || (pool->pool == NULL))                                             \
            RETURN_FALSE;                                                                       \
    } while (0)                                                                                 \

#define GET_VOLUME_FROM_ARGS(args, ...)                                                         \
    do {                                                                                        \
        reset_error(TSRMLS_C);                                                                  \
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, args, __VA_ARGS__) == FAILURE) {   \
            set_error("Invalid arguments" TSRMLS_CC);                                           \
            RETURN_FALSE;                                                                       \
        }                                                                                       \
                                                                                                \
        VIRT_FETCH_RESOURCE(volume, php_libvirt_volume*, &zvolume, PHP_LIBVIRT_VOLUME_RES_NAME, le_libvirt_volume);\
        if ((volume == NULL) || (volume->volume == NULL))                                       \
            RETURN_FALSE;                                                                       \
    } while (0)                                                                                 \

#define GET_SNAPSHOT_FROM_ARGS(args, ...)                                                       \
    do {                                                                                        \
        reset_error(TSRMLS_C);                                                                  \
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, args, __VA_ARGS__) == FAILURE) {   \
            set_error("Invalid arguments" TSRMLS_CC);                                           \
            RETURN_FALSE;                                                                       \
        }                                                                                       \
                                                                                                \
        VIRT_FETCH_RESOURCE(snapshot, php_libvirt_snapshot*, &zsnapshot, PHP_LIBVIRT_SNAPSHOT_RES_NAME, le_libvirt_snapshot);\
        if ((snapshot == NULL) || (snapshot->snapshot == NULL))                                 \
            RETURN_FALSE;                                                                       \
    } while (0)                                                                                 \

#define GET_NWFILTER_FROM_ARGS(args, ...)                                                       \
    do {                                                                                        \
        reset_error(TSRMLS_C);                                                                  \
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, args, __VA_ARGS__) == FAILURE) {   \
            set_error("Invalid arguments" TSRMLS_CC);                                           \
            RETURN_FALSE;                                                                       \
        }                                                                                       \
                                                                                                \
        VIRT_FETCH_RESOURCE(nwfilter, php_libvirt_nwfilter *, &znwfilter,                       \
                            PHP_LIBVIRT_NWFILTER_RES_NAME, le_libvirt_nwfilter);                \
        if ((nwfilter == NULL) || (nwfilter->nwfilter == NULL))                                 \
            RETURN_FALSE;                                                                       \
    } while (0)                                                                                 \


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
    strsize_t image_len;
    char *format;
    strsize_t format_len;
    long long size;
    char *size_str;
    strsize_t size_str_len;
    int cmdRet;

    if (LIBVIRT_G(image_path_ini))
        path = strdup(LIBVIRT_G(image_path_ini));

    if ((path == NULL) || (path[0] != '/')) {
        set_error("Invalid argument, path must be set and absolute (start by slash character [/])" TSRMLS_CC);
        RETURN_FALSE;
    }

    GET_CONNECTION_FROM_ARGS("rsss", &zconn, &image, &image_len, &size_str, &size_str_len, &format, &format_len);

    if (size_str == NULL)
        RETURN_FALSE;

    size = size_def_to_mbytes(size_str);

    if (!is_local_connection(conn->conn)) {
        // TODO: Try to implement remote connection somehow. Maybe using SSH tunneling
        snprintf(msg, sizeof(msg), "%s works only on local systems!", PHPFUNC);
        set_error(msg TSRMLS_CC);
        RETURN_FALSE;
    }

    snprintf(fpath, sizeof(fpath), "%s/%s", path, image);

    const char *qemu_img_cmd = get_feature_binary("create-image");
    if (qemu_img_cmd == NULL) {
        set_error("Feature 'create-image' is not supported" TSRMLS_CC);
        RETURN_FALSE;
    }

    snprintf(cmd, sizeof(cmd), "%s create -f %s %s %dM > /dev/null", qemu_img_cmd, format, fpath, size);
    DPRINTF("%s: Running '%s'...\n", PHPFUNC, cmd);
    cmdRet = system(cmd);

    if (WEXITSTATUS(cmdRet) == 0 && access(fpath, F_OK) == 0) {
        RETURN_TRUE;
    } else {
        snprintf(msg, sizeof(msg), "Cannot create image: %s", fpath);
        set_error(msg TSRMLS_CC);
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
    strsize_t image_len;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &image, &image_len);

    // Disable remote connections
    if (!is_local_connection(conn->conn)) {
        set_error("Function works only on local connection" TSRMLS_CC);
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
        set_error(msg TSRMLS_CC);
        free(hostname);
        RETURN_FALSE;
    }
    free(hostname);

    if (unlink(image) != 0) {
        snprintf(msg, sizeof(msg), "An error occured while unlinking %s: %d (%s)", image, errno, strerror(errno));
        set_error(msg TSRMLS_CC);
        RETURN_FALSE;
    } else {
        RETURN_TRUE;
    }
}

/*
 * Private function name:   get_string_from_xpath
 * Since version:           0.4.1(-1)
 * Description:             Function is used to get the XML xPath expression from the XML document. This can be added to val array if not NULL.
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
                free(value);
                value = NULL;
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
    free(binary);

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
    unsigned long index;
    long max_slot = -1;

    xml = virDomainGetXMLDesc(domain, VIR_DOMAIN_XML_INACTIVE);
    output = (zval *)emalloc(sizeof(zval));
    array_init(output);
    free(get_string_from_xpath(xml, xpath, &output, &retval));

    arr_hash = Z_ARRVAL_P(output);
    // array_count = zend_hash_num_elements(arr_hash);
    VIRT_FOREACH(arr_hash, pointer, data) {
        if (Z_TYPE_P(data) == IS_STRING) {
            php_libvirt_hash_key_info info;
            VIRT_HASH_CURRENT_KEY_INFO(arr_hash, pointer, index, info);

            if (info.type != HASH_KEY_IS_STRING) {
                long num = -1;

                sscanf(Z_STRVAL_P(data), "%lx", &num);
                if (num > max_slot)
                    max_slot = num;
            }
        }
    } VIRT_FOREACH_END();

    efree(output);
    free(xml);
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
char *connection_get_domain_type(virConnectPtr conn, char *arch TSRMLS_DC)
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
    free(tmpArch);
    free(caps);
    free(tmp);
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
char *connection_get_emulator(virConnectPtr conn, char *arch TSRMLS_DC)
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

    DPRINTF("%s: Requested emulator for arch '%s'\n",  __FUNCTION__, arch);

    snprintf(xpath, sizeof(xpath), "//capabilities/guest/arch[@name='%s']/domain/emulator", arch);
    DPRINTF("%s: Applying xPath '%s' to capabilities XML output\n", __FUNCTION__, xpath);
    tmp = get_string_from_xpath(caps, xpath, NULL, &retval);
    if (tmp && retval >= 0) {
        DPRINTF("%s: Emulator is '%s'\n",  __FUNCTION__, tmp);
        goto done;
    }

    DPRINTF("%s: No emulator found. Trying next location ...\n", __FUNCTION__);
    snprintf(xpath, sizeof(xpath), "//capabilities/guest/arch[@name='%s']/emulator", arch);
    DPRINTF("%s: Applying xPath '%s' to capabilities XML output\n",  __FUNCTION__, xpath);
    free(tmp);
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
    free(tmpArch);
    free(caps);
    free(tmp);
    return ret;
}

/*
 * Private function name:   connection_get_arch
 * Since version:           0.4.5
 * Description:             Function is required for functions that get the architecture for specific libvirt connection
 * Arguments:               @conn [virConnectPtr]: libvirt connection pointer of connection to get architecture for
 * Returns:                 path to the emulator
 */
char *connection_get_arch(virConnectPtr conn TSRMLS_DC)
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
    free(caps);
    free(tmp);
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
char *generate_uuid(virConnectPtr conn TSRMLS_DC)
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
char *get_disk_xml(unsigned long long size, char *path, char *driver, char *bus, char *dev, int disk_flags TSRMLS_DC)
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
        snprintf(cmd, sizeof(cmd), "%s create -f %s %s %ldM > /dev/null &2>/dev/null", qemu_img_cmd, driver, path, size);

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
 * Description:             Function is used to generate the installation XML description to install a new domain
 * Arguments:               @step [int]: number of step for XML output (1 or 2)
 *                          @conn [virConnectPtr]: libvirt connection pointer
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
char *installation_get_xml(int step, virConnectPtr conn, char *name, int memMB, int maxmemMB, char *arch, char *uuid, int vCpus, char *iso_image,
                           tVMDisk *disks, int numDisks, tVMNetwork *networks, int numNetworks, int domain_flags TSRMLS_DC)
{
    int i;
    char xml[32768] = { 0 };
    char disks_xml[16384] = { 0 };
    char networks_xml[16384] = { 0 };
    char features[128] = { 0 };
    char *tmp = NULL;
    char type[64] = { 0 };
    // virDomainPtr domain=NULL;

    if (conn == NULL) {
        DPRINTF("%s: Invalid libvirt connection pointer\n", __FUNCTION__);
        return NULL;
    }

    if (uuid == NULL)
        uuid = generate_uuid(conn TSRMLS_CC);

    if (domain_flags & DOMAIN_FLAG_FEATURE_ACPI)
        strcat(features, "<acpi/>");
    if (domain_flags & DOMAIN_FLAG_FEATURE_APIC)
        strcat(features, "<apic/>");
    if (domain_flags & DOMAIN_FLAG_FEATURE_PAE)
        strcat(features, "<pae/>");

    if (arch == NULL) {
        arch = connection_get_arch(conn TSRMLS_CC);
        DPRINTF("%s: No architecture defined, got host arch of '%s'\n", __FUNCTION__, arch);
    }

    if (access(iso_image, R_OK) != 0) {
        DPRINTF("%s: Installation image %s doesn't exist\n", __FUNCTION__, iso_image);
        return NULL;
    }

    tmp = connection_get_domain_type(conn, arch TSRMLS_CC);
    if (tmp != NULL)
        snprintf(type, sizeof(type), " type='%s'", tmp);

    for (i = 0; i < numDisks; i++) {
        char *disk = get_disk_xml(disks[i].size, disks[i].path, disks[i].driver, disks[i].bus, disks[i].dev, disks[i].flags TSRMLS_CC);

        if (disk != NULL)
            strcat(disks_xml, disk);

        free(disk);
    }

    for (i = 0; i < numNetworks; i++) {
        char *network = get_network_xml(networks[i].mac, networks[i].network, networks[i].model);

        if (network != NULL)
            strcat(networks_xml, network);

        free(network);
    }

    if (step == 1)
        snprintf(xml, sizeof(xml), "<domain%s>\n"
                 "\t<name>%s</name>\n"
                 "\t<currentMemory>%d</currentMemory>\n"
                 "\t<memory>%d</memory>\n"
                 "\t<uuid>%s</uuid>\n"
                 "\t<os>\n"
                 "\t\t<type arch='%s'>hvm</type>\n"
                 "\t\t<boot dev='cdrom'/>\n"
                 "\t\t<boot dev='hd'/>\n"
                 "\t</os>\n"
                 "\t<features>\n"
                 "\t\t%s\n"
                 "\t</features>\n"
                 "\t<clock offset=\"%s\"/>\n"
                 "\t<on_poweroff>destroy</on_poweroff>\n"
                 "\t<on_reboot>destroy</on_reboot>\n"
                 "\t<on_crash>destroy</on_crash>\n"
                 "\t<vcpu>%d</vcpu>\n"
                 "\t<devices>\n"
                 "\t\t<emulator>%s</emulator>\n"
                 "%s"
                 "\t\t<disk type='file' device='cdrom'>\n"
                 "\t\t\t<driver name='qemu' type='raw' />\n"
                 "\t\t\t<source file='%s' />\n"
                 "\t\t\t<target dev='hdc' bus='ide' />\n"
                 "\t\t\t<readonly />\n"
                 "\t\t</disk>\n"
                 "%s"
                 "\t\t<input type='mouse' bus='ps2' />\n"
                 "\t\t<graphics type='vnc' port='-1' />\n"
                 "\t\t<console type='pty' />\n"
                 "%s"
                 "\t\t<video>\n"
                 "\t\t\t<model type='cirrus' />\n"
                 "\t\t</video>\n"
                 "\t</devices>\n"
                 "</domain>",
            type, name, memMB * 1024, maxmemMB * 1024, uuid, arch, features,
            (domain_flags & DOMAIN_FLAG_CLOCK_LOCALTIME ? "localtime" : "utc"),
            vCpus, connection_get_emulator(conn, arch TSRMLS_CC), disks_xml, iso_image, networks_xml,
            (domain_flags & DOMAIN_FLAG_SOUND_AC97 ? "\t\t<sound model='ac97'/>\n" : "")
                );
    else
        if (step == 2)
            snprintf(xml, sizeof(xml), "<domain%s>\n"
                     "\t<name>%s</name>\n"
                     "\t<currentMemory>%d</currentMemory>\n"
                     "\t<memory>%d</memory>\n"
                     "\t<uuid>%s</uuid>\n"
                     "\t<os>\n"
                     "\t\t<type arch='%s'>hvm</type>\n"
                     "\t\t<boot dev='hd'/>\n"
                     "\t</os>\n"
                     "\t<features>\n"
                     "\t\t%s\n"
                     "\t</features>\n"
                     "\t<clock offset=\"%s\"/>\n"
                     "\t<on_poweroff>destroy</on_poweroff>\n"
                     "\t<on_reboot>destroy</on_reboot>\n"
                     "\t<on_crash>destroy</on_crash>\n"
                     "\t<vcpu>%d</vcpu>\n"
                     "\t<devices>\n"
                     "\t\t<emulator>%s</emulator>\n"
                     "%s"
                     "\t\t<disk type='file' device='cdrom'>\n"
                     "\t\t\t<driver name='qemu' type='raw' />\n"
                     "\t\t\t<target dev='hdc' bus='ide' />\n"
                     "\t\t\t<readonly />\n"
                     "\t\t</disk>\n"
                     "%s"
                     "\t\t<input type='mouse' bus='ps2' />\n"
                     "\t\t<graphics type='vnc' port='-1' />\n"
                     "\t\t<console type='pty' />\n"
                     "%s"
                     "\t\t<video>\n"
                     "\t\t\t<model type='cirrus' />\n"
                     "\t\t</video>\n"
                     "\t</devices>\n"
                     "</domain>",
                type, name, memMB * 1024, maxmemMB * 1024, uuid, arch, features,
                (domain_flags & DOMAIN_FLAG_CLOCK_LOCALTIME ? "localtime" : "utc"),
                vCpus, connection_get_emulator(conn, arch TSRMLS_CC), disks_xml, networks_xml,
                (domain_flags & DOMAIN_FLAG_SOUND_AC97 ? "\t\t<sound model='ac97'/>\n" : "")
                    );

    return (strlen(xml) > 0) ? strdup(xml) : NULL;
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
    unsigned long index;

    arr_hash = Z_ARRVAL_P(arr);
    //array_count = zend_hash_num_elements(arr_hash);

    if (disk != NULL)
        memset(disk, 0, sizeof(tVMDisk));
    if (network != NULL)
        memset(network, 0, sizeof(tVMNetwork));

    VIRT_FOREACH(arr_hash, pointer, data) {
        if ((Z_TYPE_P(data) == IS_STRING) || (Z_TYPE_P(data) == IS_LONG)) {
            VIRT_HASH_CURRENT_KEY_INFO(arr_hash, pointer, index, key);
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

/*
 * Function name:   libvirt_domain_has_current_snapshot
 * Since version:   0.4.1(-2)
 * Description:     Function is used to get the information whether domain has the current snapshot
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @flags [int]: libvirt snapshot flags
 * Returns:         TRUE is domain has the current snapshot, otherwise FALSE (you may need to check for error using libvirt_get_last_error())
 */
PHP_FUNCTION(libvirt_domain_has_current_snapshot)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int retval;
    zend_long flags = 0;

    GET_DOMAIN_FROM_ARGS("r|l", &zdomain, &flags);

    retval = virDomainHasCurrentSnapshot(domain->domain, flags);
    if (retval <= 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_snapshot_lookup_by_name
 * Since version:   0.4.1(-2)
 * Description:     This functions is used to lookup for the snapshot by it's name
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @name [string]: name of the snapshot to get the resource
 *                  @flags [int]: libvirt snapshot flags
 * Returns:         domain snapshot resource
 */
PHP_FUNCTION(libvirt_domain_snapshot_lookup_by_name)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    strsize_t name_len;
    char *name = NULL;
    zend_long flags = 0;
    php_libvirt_snapshot *res_snapshot;
    virDomainSnapshotPtr snapshot = NULL;

    GET_DOMAIN_FROM_ARGS("rs|l", &zdomain, &name, &name_len, &flags);

    if ((name == NULL) || (name_len < 1))
        RETURN_FALSE;
    snapshot=virDomainSnapshotLookupByName(domain->domain, name, flags);
    if (snapshot == NULL)
        RETURN_FALSE;

    res_snapshot = (php_libvirt_snapshot *)emalloc(sizeof(php_libvirt_snapshot));
    res_snapshot->domain = domain;
    res_snapshot->snapshot = snapshot;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_snapshot->snapshot);
    resource_change_counter(INT_RESOURCE_SNAPSHOT, domain->conn->conn, res_snapshot->snapshot, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_snapshot, le_libvirt_snapshot);
}

/*
 * Function name:   libvirt_domain_snapshot_create
 * Since version:   0.4.1(-2)
 * Description:     This function creates the domain snapshot for the domain identified by it's resource
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @flags [int]: libvirt snapshot flags
 * Returns:         domain snapshot resource
 */
PHP_FUNCTION(libvirt_domain_snapshot_create)
{
    php_libvirt_domain *domain = NULL;
    php_libvirt_snapshot *res_snapshot;
    zval *zdomain;
    virDomainSnapshotPtr snapshot = NULL;
    zend_long flags = 0;

    GET_DOMAIN_FROM_ARGS("r|l", &zdomain, &flags);

    snapshot = virDomainSnapshotCreateXML(domain->domain, "<domainsnapshot/>", flags);
    DPRINTF("%s: virDomainSnapshotCreateXML(%p, <xml>) returned %p\n", PHPFUNC, domain->domain, snapshot);
    if (snapshot == NULL)
        RETURN_FALSE;

    res_snapshot = (php_libvirt_snapshot *)emalloc(sizeof(php_libvirt_snapshot));
    res_snapshot->domain = domain;
    res_snapshot->snapshot = snapshot;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_snapshot->snapshot);
    resource_change_counter(INT_RESOURCE_SNAPSHOT, domain->conn->conn, res_snapshot->snapshot, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_snapshot, le_libvirt_snapshot);
}

/*
 * Function name:   libvirt_domain_snapshot_get_xml
 * Since version:   0.4.1(-2)
 * Description:     Function is used to get the XML description of the snapshot identified by it's resource
 * Arguments:       @res [resource]: libvirt snapshot resource
 *                  @flags [int]: libvirt snapshot flags
 * Returns:         XML description string for the snapshot
 */
PHP_FUNCTION(libvirt_domain_snapshot_get_xml)
{
    char *xml;
    zval *zsnapshot;
    php_libvirt_snapshot *snapshot;
    zend_long flags = 0;

    GET_SNAPSHOT_FROM_ARGS("r|l", &zsnapshot, &flags);

    xml = virDomainSnapshotGetXMLDesc(snapshot->snapshot, flags);
    if (xml == NULL)
        RETURN_FALSE;

    VIRT_RETVAL_STRING(xml);
    free(xml);
}

/*
 * Function name:   libvirt_domain_snapshot_revert
 * Since version:   0.4.1(-2)
 * Description:     Function is used to revert the domain state to the state identified by the snapshot
 * Arguments:       @res [resource]: libvirt snapshot resource
 *                  @flags [int]: libvirt snapshot flags
 * Returns:         TRUE on success, FALSE on error
 */
PHP_FUNCTION(libvirt_domain_snapshot_revert)
{
    zval *zsnapshot;
    php_libvirt_snapshot *snapshot;
    int ret;
    zend_long flags = 0;

    GET_SNAPSHOT_FROM_ARGS("r|l", &zsnapshot, &flags);

    ret = virDomainRevertToSnapshot(snapshot->snapshot, flags);
    DPRINTF("%s: virDomainRevertToSnapshot(%p, 0) returned %d\n", PHPFUNC, snapshot->snapshot, ret);
    if (ret == -1)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_domain_snapshot_delete
 * Since version:   0.4.1(-2)
 * Description:     Function is used to revert the domain state to the state identified by the snapshot
 * Arguments:       @res [resource]: libvirt snapshot resource
 *                  @flags [int]: 0 to delete just snapshot, VIR_SNAPSHOT_DELETE_CHILDREN to delete snapshot children as well
 * Returns:         TRUE on success, FALSE on error
 */
PHP_FUNCTION(libvirt_domain_snapshot_delete)
{
    zval *zsnapshot;
    php_libvirt_snapshot *snapshot;
    zend_long flags = 0;
    int retval;

    GET_SNAPSHOT_FROM_ARGS("r|l", &zsnapshot, &flags);

    retval = virDomainSnapshotDelete(snapshot->snapshot, flags);
    DPRINTF("%s: virDomainSnapshotDelete(%p, %d) returned %d\n", PHPFUNC, snapshot->snapshot, (int) flags, retval);
    if (retval == -1)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_list_domain_snapshots
 * Since version:   0.4.1(-2)
 * Description:     Function is used to list domain snapshots for the domain specified by it's resource
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @flags [int]: libvirt snapshot flags
 * Returns:         libvirt domain snapshot names array
 */
PHP_FUNCTION(libvirt_list_domain_snapshots)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    int count = -1;
    int expectedcount = -1;
    char **names;
    zend_long flags = 0;
    int i;

    GET_DOMAIN_FROM_ARGS("r|l", &zdomain, &flags);

    expectedcount = virDomainSnapshotNum(domain->domain, flags);
    DPRINTF("%s: virDomainSnapshotNum(%p, 0) returned %d\n", PHPFUNC, domain->domain, expectedcount);

    if (expectedcount != -1) {
        names = (char **)emalloc(expectedcount * sizeof(char *));
        count = virDomainSnapshotListNames(domain->domain, names, expectedcount, 0);
    }

    if ((count != expectedcount) || (count < 0)) {
        RETURN_FALSE;
    } else {
        array_init(return_value);
        for (i = 0; i < count; i++) {
            VIRT_ADD_NEXT_INDEX_STRING(return_value, names[i]);
            free(names[i]);
        }
    }
    efree(names);
}

/* Storagepool functions */

/*
 * Function name:   libvirt_storagepool_lookup_by_name
 * Since version:   0.4.1(-1)
 * Description:     Function is used to lookup for storage pool by it's name
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @name [string]: storage pool name
 * Returns:         libvirt storagepool resource
 */
PHP_FUNCTION(libvirt_storagepool_lookup_by_name)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    strsize_t name_len;
    char *name = NULL;
    virStoragePoolPtr pool = NULL;
    php_libvirt_storagepool *res_pool;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &name, &name_len);

    if ((name == NULL) || (name_len < 1))
        RETURN_FALSE;
    pool = virStoragePoolLookupByName(conn->conn, name);
    DPRINTF("%s: virStoragePoolLookupByName(%p, %s) returned %p\n", PHPFUNC, conn->conn, name, pool);
    if (pool == NULL)
        RETURN_FALSE;

    res_pool = (php_libvirt_storagepool *)emalloc(sizeof(php_libvirt_storagepool));
    res_pool->pool = pool;
    res_pool->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_pool->pool);
    resource_change_counter(INT_RESOURCE_STORAGEPOOL, conn->conn, res_pool->pool, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_pool, le_libvirt_storagepool);
}

/* Storagepool functions */

/*
 * Function name:   libvirt_storagepool_lookup_by_volume
 * Since version:   0.4.1(-1)
 * Description:     Function is used to lookup for storage pool by a volume
 * Arguments:       @res [volume]: volume resource of storage pool
 * Returns:         libvirt storagepool resource
 */
PHP_FUNCTION(libvirt_storagepool_lookup_by_volume)
{
    php_libvirt_volume *volume;
    zval *zvolume;
    virStoragePoolPtr pool = NULL;
    php_libvirt_storagepool *res_pool;

    GET_VOLUME_FROM_ARGS("r", &zvolume);

    pool = virStoragePoolLookupByVolume(volume->volume);
    DPRINTF("%s: virStoragePoolLookupByVolume(%p) returned %p\n", PHPFUNC, volume->volume, pool);
    if (pool == NULL)
        RETURN_FALSE;

    res_pool = (php_libvirt_storagepool *)emalloc(sizeof(php_libvirt_storagepool));
    res_pool->pool = pool;
    res_pool->conn = volume->conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_pool->pool);
    resource_change_counter(INT_RESOURCE_STORAGEPOOL, res_pool->conn->conn, res_pool->pool, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_pool, le_libvirt_storagepool);
}

/*
 * Function name:   libvirt_storagepool_list_volumes
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list volumes in the specified storage pool
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         list of storage volume names in the storage pool in an array using default keys (indexes)
 */
PHP_FUNCTION(libvirt_storagepool_list_volumes)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    char **names = NULL;
    int expectedcount = -1;
    int i;
    int count = -1;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    if ((expectedcount = virStoragePoolNumOfVolumes(pool->pool)) < 0)
        RETURN_FALSE;

    DPRINTF("%s: virStoragePoolNumOfVolumes(%p) returned %d\n", PHPFUNC, pool->pool, expectedcount);
    names = (char **)emalloc(expectedcount*sizeof(char *));

    count = virStoragePoolListVolumes(pool->pool, names, expectedcount);
    DPRINTF("%s: virStoragePoolListVolumes(%p, %p, %d) returned %d\n", PHPFUNC, pool->pool, names, expectedcount, count);
    array_init(return_value);

    if ((count != expectedcount) || (count < 0))
        RETURN_FALSE;
    for (i = 0; i < count; i++) {
        VIRT_ADD_NEXT_INDEX_STRING(return_value,  names[i]);
        free(names[i]);
    }

    efree(names);
}

/*
 * Function name:   libvirt_storagepool_get_info
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get information about the storage pool
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         storage pool information array of state, capacity, allocation and available space
 */
PHP_FUNCTION(libvirt_storagepool_get_info)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    virStoragePoolInfo poolInfo;
    int retval;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    retval = virStoragePoolGetInfo(pool->pool, &poolInfo);
    DPRINTF("%s: virStoragePoolGetInfo(%p, <info>) returned %d\n", PHPFUNC, pool->pool, retval);
    if (retval != 0)
        RETURN_FALSE;

    array_init(return_value);

    // @todo: fix the long long returns
    LONGLONG_INIT;
    add_assoc_long(return_value, "state", (long)poolInfo.state);
    LONGLONG_ASSOC(return_value, "capacity", poolInfo.capacity);
    LONGLONG_ASSOC(return_value, "allocation", poolInfo.allocation);
    LONGLONG_ASSOC(return_value, "available", poolInfo.available);
}

/*
 * Function name:   libvirt_storagevolume_lookup_by_name
 * Since version:   0.4.1(-1)
 * Description:     Function is used to lookup for storage volume by it's name
 * Arguments:       @res [resource]: libvirt storagepool resource
 *                  @name [string]: name of the storage volume to look for
 * Returns:         libvirt storagevolume resource
 */
PHP_FUNCTION(libvirt_storagevolume_lookup_by_name)
{
    php_libvirt_storagepool *pool = NULL;
    php_libvirt_volume *res_volume;
    zval *zpool;
    strsize_t name_len;
    char *name = NULL;
    virStorageVolPtr volume = NULL;

    GET_STORAGEPOOL_FROM_ARGS("rs", &zpool, &name, &name_len);
    if ((name == NULL) || (name_len < 1))
        RETURN_FALSE;

    volume = virStorageVolLookupByName(pool->pool, name);
    DPRINTF("%s: virStorageVolLookupByName(%p, %s) returned %p\n", PHPFUNC, pool->pool, name, volume);
    if (volume == NULL)
        RETURN_FALSE;

    res_volume = (php_libvirt_volume *)emalloc(sizeof(php_libvirt_volume));
    res_volume->volume = volume;
    res_volume->conn   = pool->conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_volume->volume);
    resource_change_counter(INT_RESOURCE_VOLUME, pool->conn->conn, res_volume->volume, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_volume, le_libvirt_volume);
}

/*
 * Function name:   libvirt_storagevolume_lookup_by_path
 * Since version:   0.4.1(-2)
 * Description:     Function is used to lookup for storage volume by it's path
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @path [string]: path of the storage volume to look for
 * Returns:         libvirt storagevolume resource
 */
PHP_FUNCTION(libvirt_storagevolume_lookup_by_path)
{
    php_libvirt_connection *conn = NULL;
    php_libvirt_volume *res_volume;
    zval *zconn;
    strsize_t name_len;
    char *name = NULL;
    virStorageVolPtr volume = NULL;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &name, &name_len);
    if ((name == NULL) || (name_len < 1))
        RETURN_FALSE;

    volume = virStorageVolLookupByPath(conn->conn, name);
    DPRINTF("%s: virStorageVolLookupByPath(%p, %s) returned %p\n", PHPFUNC, conn->conn, name, volume);
    if (volume == NULL) {
        set_error_if_unset("Cannot find storage volume on requested path" TSRMLS_CC);
        RETURN_FALSE;
    }

    res_volume = (php_libvirt_volume *)emalloc(sizeof(php_libvirt_volume));
    res_volume->volume = volume;
    res_volume->conn   = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_volume->volume);
    resource_change_counter(INT_RESOURCE_VOLUME, conn->conn, res_volume->volume, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_volume, le_libvirt_volume);
}

/*
 * Function name:   libvirt_storagevolume_get_name
 * Since version:   0.4.1(-2)
 * Description:     Function is used to get the storage volume name
 * Arguments:       @res [resource]: libvirt storagevolume resource
 * Returns:         storagevolume name
 */
PHP_FUNCTION(libvirt_storagevolume_get_name)
{
    php_libvirt_volume *volume = NULL;
    zval *zvolume;
    const char *retval;

    GET_VOLUME_FROM_ARGS("r", &zvolume);

    retval = virStorageVolGetName(volume->volume);
    DPRINTF("%s: virStorageVolGetName(%p) returned %s\n", PHPFUNC, volume->volume, retval);
    if (retval == NULL)
        RETURN_FALSE;

    VIRT_RETURN_STRING(retval);
}

/*
 * Function name:   libvirt_storagevolume_get_path
 * Since version:   0.4.1(-2)
 * Description:     Function is used to get the  storage volume path
 * Arguments:       @res [resource]: libvirt storagevolume resource
 * Returns:         storagevolume path
 */
PHP_FUNCTION(libvirt_storagevolume_get_path)
{
    php_libvirt_volume *volume = NULL;
    zval *zvolume;
    char *retval;

    GET_VOLUME_FROM_ARGS("r", &zvolume);

    retval = virStorageVolGetPath(volume->volume);
    DPRINTF("%s: virStorageVolGetPath(%p) returned %s\n", PHPFUNC, volume->volume, retval);
    if (retval == NULL)
        RETURN_FALSE;

    VIRT_RETVAL_STRING(retval);
    free(retval);
}

/*
 * Function name:   libvirt_storagevolume_get_info
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the storage volume information
 * Arguments:       @res [resource]: libvirt storagevolume resource
 * Returns:         storage volume information array of type, allocation and capacity
 */
PHP_FUNCTION(libvirt_storagevolume_get_info)
{
    php_libvirt_volume *volume = NULL;
    zval *zvolume;
    virStorageVolInfo volumeInfo;
    int retval;

    GET_VOLUME_FROM_ARGS("r", &zvolume);

    retval = virStorageVolGetInfo(volume->volume, &volumeInfo);
    DPRINTF("%s: virStorageVolGetInfo(%p, <info>) returned %d\n", PHPFUNC, volume->volume, retval);
    if (retval != 0)
        RETURN_FALSE;

    array_init(return_value);
    LONGLONG_INIT;
    add_assoc_long(return_value, "type", (long)volumeInfo.type);
    LONGLONG_ASSOC(return_value, "capacity", volumeInfo.capacity);
    LONGLONG_ASSOC(return_value, "allocation", volumeInfo.allocation);
}

/*
 * Function name:   libvirt_storagevolume_get_xml_desc
 * Since version:   0.4.1(-1), changed 0.4.2
 * Description:     Function is used to get the storage volume XML description
 * Arguments:       @res [resource]: libvirt storagevolume resource
 *                  @xpath [string]: optional xPath expression string to get just this entry, can be NULL
 *                  @flags [int]: optional flags
 * Returns:         storagevolume XML description or result of xPath expression
 */
PHP_FUNCTION(libvirt_storagevolume_get_xml_desc)
{
    php_libvirt_volume *volume = NULL;
    zval *zvolume;
    char *tmp = NULL;
    char *xml;
    char *xpath = NULL;
    strsize_t xpath_len;
    zend_long flags = 0;
    int retval = -1;

    GET_VOLUME_FROM_ARGS("rs|l", &zvolume, &xpath, &xpath_len, &flags);
    if (xpath_len < 1)
        xpath = NULL;

    DPRINTF("%s: volume = %p, xpath = %s, flags = %ld\n", PHPFUNC, volume->volume, xpath, flags);

    xml = virStorageVolGetXMLDesc(volume->volume, flags);
    if (xml == NULL) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
        RETURN_FALSE;
    }

    tmp = get_string_from_xpath(xml, xpath, NULL, &retval);
    if ((tmp == NULL) || (retval < 0)) {
        VIRT_RETVAL_STRING(xml);
    } else {
        VIRT_RETVAL_STRING(tmp);
    }

    free(xml);
    free(tmp);
}

/*
 * Function name:   libvirt_storagevolume_create_xml
 * Since version:   0.4.1(-1)
 * Description:     Function is used to create the new storage pool and return the handle to new storage pool
 * Arguments:       @res [resource]: libvirt storagepool resource
 *                  @xml [string]: XML string to create the storage volume in the storage pool
 *                  @flags [int]: virStorageVolCreateXML flags
 * Returns:         libvirt storagevolume resource
 */
PHP_FUNCTION(libvirt_storagevolume_create_xml)
{
    php_libvirt_volume *res_volume = NULL;
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    virStorageVolPtr volume = NULL;
    char *xml;
    zend_long flags = 0;
    strsize_t xml_len;

    GET_STORAGEPOOL_FROM_ARGS("rs|l", &zpool, &xml, &xml_len, &flags);

    volume = virStorageVolCreateXML(pool->pool, xml, flags);
    DPRINTF("%s: virStorageVolCreateXML(%p, <xml>, 0) returned %p\n", PHPFUNC, pool->pool, volume);
    if (volume == NULL)
        RETURN_FALSE;

    res_volume = (php_libvirt_volume *)emalloc(sizeof(php_libvirt_volume));
    res_volume->volume = volume;
    res_volume->conn   = pool->conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_volume->volume);
    resource_change_counter(INT_RESOURCE_VOLUME, pool->conn->conn, res_volume->volume, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_volume, le_libvirt_volume);
}

/*
 * Function name:   libvirt_storagevolume_create_xml_from
 * Since version:   0.4.1(-2)
 * Description:     Function is used to clone the new storage volume into pool from the orignial volume
 * Arguments:       @pool [resource]: libvirt storagepool resource
 *                  @xml [string]: XML string to create the storage volume in the storage pool
 *                  @original_volume [resource]: libvirt storagevolume resource
 * Returns:         libvirt storagevolume resource
 */
PHP_FUNCTION(libvirt_storagevolume_create_xml_from)
{
    php_libvirt_volume *res_volume = NULL;
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;

    php_libvirt_volume *pl_volume = NULL;
    zval *zvolume;

    virStorageVolPtr volume = NULL;
    char *xml;
    strsize_t xml_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsr", &zpool, &xml, &xml_len, &zvolume) == FAILURE) {
        set_error("Invalid pool resource, XML or volume resouce" TSRMLS_CC);
        RETURN_FALSE;
    }

    VIRT_FETCH_RESOURCE(pool, php_libvirt_storagepool*, &zpool, PHP_LIBVIRT_STORAGEPOOL_RES_NAME, le_libvirt_storagepool);
    if ((pool == NULL) || (pool->pool == NULL))
        RETURN_FALSE;
    VIRT_FETCH_RESOURCE(pl_volume, php_libvirt_volume*, &zvolume, PHP_LIBVIRT_VOLUME_RES_NAME, le_libvirt_volume);
    if ((pl_volume == NULL) || (pl_volume->volume == NULL))
        RETURN_FALSE;
    resource_change_counter(INT_RESOURCE_VOLUME, NULL, pl_volume->volume, 1 TSRMLS_CC);

    volume = virStorageVolCreateXMLFrom(pool->pool, xml, pl_volume->volume, 0);
    DPRINTF("%s: virStorageVolCreateXMLFrom(%p, <xml>, %p, 0) returned %p\n", PHPFUNC, pool->pool, pl_volume->volume, volume);
    if (volume == NULL)
        RETURN_FALSE;

    res_volume = (php_libvirt_volume *)emalloc(sizeof(php_libvirt_volume));
    res_volume->volume = volume;
    res_volume->conn   = pool->conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_volume->volume);
    resource_change_counter(INT_RESOURCE_VOLUME, pool->conn->conn, res_volume->volume, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_volume, le_libvirt_volume);
}

/*
 * Function name:   libvirt_storagevolume_delete
 * Since version:   0.4.2
 * Description:     Function is used to delete to volume identified by it's resource
 * Arguments:       @res [resource]: libvirt storagevolume resource
 *                  @flags [int]: optional flags for the storage volume deletion for virStorageVolDelete()
 * Returns:         TRUE for success, FALSE on error
 */
PHP_FUNCTION(libvirt_storagevolume_delete)
{
    php_libvirt_volume *volume = NULL;
    zval *zvolume;
    zend_long flags = 0;
    int retval = 0;

    GET_VOLUME_FROM_ARGS("r|l", &zvolume, &flags);

    retval = virStorageVolDelete(volume->volume, flags);
    DPRINTF("%s: virStorageVolDelete(%p, %d) returned %d\n", PHPFUNC, volume->volume, (int) flags, retval);
    if (retval != 0) {
        set_error_if_unset("Cannot delete storage volume" TSRMLS_CC);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_storagevolume_resize
 * Since version:   0.5.0
 * Description:     Function is used to resize volume identified by it's resource
 * Arguments:       @res [resource]: libvirt storagevolume resource
 *                  @capacity [int]: capacity for the storage volume
 *                  @flags [int]: optional flags for the storage volume resize for virStorageVolResize()
 * Returns:         int
 */
PHP_FUNCTION(libvirt_storagevolume_resize)
{
    php_libvirt_volume *volume = NULL;
    zval *zvolume;
    zend_long flags = 0;
    zend_long capacity = 0;
    int retval = -1;

    GET_VOLUME_FROM_ARGS("rl|l", &zvolume, &capacity, &flags);

    retval = virStorageVolResize(volume->volume, capacity, flags);
    DPRINTF("%s: virStorageVolResize(%p, %d, %d) returned %d\n", PHPFUNC, volume->volume, (int) capacity, (int) flags, retval);
    if (retval != 0) {
        set_error_if_unset("Cannot resize storage volume" TSRMLS_CC);
        RETURN_LONG(retval);
    }

    RETURN_LONG(retval);
}

/*
 * Function name:   libvirt_storagevolume_download
 * Since version:   0.5.0
 * Description:     Function is used to download volume identified by it's resource
 * Arguments:       @res [resource]: libvirt storagevolume resource
 *                  @stream [resource]: stream to use as output
 *                  @offset [int]: position to start reading from
 *                  @length [int] : limit on amount of data to download
 *                  @flags [int]: optional flags for the storage volume download for virStorageVolDownload()
 * Returns:         int
 */
PHP_FUNCTION(libvirt_storagevolume_download)
{
    php_libvirt_volume *volume = NULL;
    php_libvirt_stream *stream = NULL;
    zval *zvolume;
    zval *zstream;
    zend_long flags = 0;
    zend_long offset = 0;
    zend_long length = 0;
    int retval = -1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rr|l|l|l", &zvolume, &zstream, &offset, &length, &flags) == FAILURE)
        RETURN_LONG(retval);
    VIRT_FETCH_RESOURCE(volume, php_libvirt_volume*, &zvolume, PHP_LIBVIRT_VOLUME_RES_NAME, le_libvirt_volume);
    if ((volume == NULL) || (volume->volume == NULL))
        RETURN_LONG(retval);
    VIRT_FETCH_RESOURCE(stream, php_libvirt_stream*, &zstream, PHP_LIBVIRT_STREAM_RES_NAME, le_libvirt_stream);
    if ((stream == NULL) || (stream->stream == NULL))
        RETURN_LONG(retval);

    retval = virStorageVolDownload(volume->volume, stream->stream, offset, length, flags);
    DPRINTF("%s: virStorageVolDownload(%p, %p, %d, %d, %d) returned %d\n", PHPFUNC, volume->volume, stream->stream, (int) offset, (int) length, (int) flags, retval);

    if (retval == -1) {
        set_error("Cannot download from stream" TSRMLS_CC);
        RETURN_LONG(retval);
    }

    RETURN_LONG(retval);
}

/*
 * Function name:   libvirt_storagevolume_upload
 * Since version:   0.5.0
 * Description:     Function is used to upload volume identified by it's resource
 * Arguments:       @res [resource]: libvirt storagevolume resource
 *                  @stream [resource]: stream to use as input
 *                  @offset [int]: position to start writing to
 *                  @length [int] : limit on amount of data to upload
 *                  @flags [int]: optional flags for the storage volume upload for virStorageVolUpload()
 * Returns:         int
 */
PHP_FUNCTION(libvirt_storagevolume_upload)
{
    php_libvirt_volume *volume = NULL;
    php_libvirt_stream *stream = NULL;
    zval *zvolume;
    zval *zstream;
    zend_long flags = 0;
    zend_long offset = 0;
    zend_long length = 0;
    int retval = -1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rr|l|l|l", &zvolume, &zstream, &offset, &length, &flags) == FAILURE)
        RETURN_LONG(retval);
    VIRT_FETCH_RESOURCE(volume, php_libvirt_volume*, &zvolume, PHP_LIBVIRT_VOLUME_RES_NAME, le_libvirt_volume);
    if ((volume == NULL) || (volume->volume == NULL))
        RETURN_LONG(retval);
    VIRT_FETCH_RESOURCE(stream, php_libvirt_stream*, &zstream, PHP_LIBVIRT_STREAM_RES_NAME, le_libvirt_stream);
    if ((stream == NULL) || (stream->stream == NULL))
        RETURN_LONG(retval);

    retval = virStorageVolUpload(volume->volume, stream->stream, offset, length, flags);
    DPRINTF("%s: virStorageVolUpload(%p, %p, %d, %d, %d) returned %d\n", PHPFUNC, volume->volume, stream->stream, (int) offset, (int) length, (int) flags, retval);

    if (retval == -1) {
        set_error_if_unset("Cannot upload storage volume" TSRMLS_CC);
        RETURN_LONG(retval);
    }

    RETURN_LONG(retval);
}

/*
 * Function name:   libvirt_storagepool_get_uuid_string
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get storage pool by UUID string
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         storagepool UUID string
 */
PHP_FUNCTION(libvirt_storagepool_get_uuid_string)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    char *uuid;
    int retval;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    uuid = (char *)emalloc(VIR_UUID_STRING_BUFLEN);
    retval = virStoragePoolGetUUIDString(pool->pool, uuid);
    DPRINTF("%s: virStoragePoolGetUUIDString(%p, %p) returned %d (%s)\n", PHPFUNC, pool->pool, uuid, retval, uuid);
    if (retval != 0)
        RETURN_FALSE;

    VIRT_RETVAL_STRING(uuid);
    efree(uuid);
}

/*
 * Function name:   libvirt_storagepool_get_name
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get storage pool name from the storage pool resource
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         storagepool name string
 */
PHP_FUNCTION(libvirt_storagepool_get_name)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    const char *name = NULL;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    name = virStoragePoolGetName(pool->pool);
    DPRINTF("%s: virStoragePoolGetName(%p) returned %s\n", PHPFUNC, pool->pool, name);
    if (name == NULL)
        RETURN_FALSE;

    VIRT_RETURN_STRING(name);
}

/*
 * Function name:   libvirt_storagepool_lookup_by_uuid_string
 * Since version:   0.4.1(-1)
 * Description:     Function is used to lookup for storage pool identified by UUID string
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @uuid [string]: UUID string to look for storagepool
 * Returns:         libvirt storagepool resource
 */
PHP_FUNCTION(libvirt_storagepool_lookup_by_uuid_string)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    char *uuid = NULL;
    strsize_t uuid_len;
    virStoragePoolPtr storage = NULL;
    php_libvirt_storagepool *res_pool;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &uuid, &uuid_len);

    if ((uuid == NULL) || (uuid_len < 1))
        RETURN_FALSE;

    storage = virStoragePoolLookupByUUIDString(conn->conn, uuid);
    DPRINTF("%s: virStoragePoolLookupByUUIDString(%p, %s) returned %p\n", PHPFUNC, conn->conn, uuid, storage);
    if (storage == NULL)
        RETURN_FALSE;

    res_pool = (php_libvirt_storagepool *)emalloc(sizeof(php_libvirt_storagepool));
    res_pool->pool = storage;
    res_pool->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_pool->pool);
    resource_change_counter(INT_RESOURCE_STORAGEPOOL, conn->conn, res_pool->pool, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_pool, le_libvirt_storagepool);
}

/*
 * Function name:   libvirt_storagepool_get_xml_desc
 * Since version:   0.4.1(-1), changed 0.4.2
 * Description:     Function is used to get the XML description for the storage pool identified by res
 * Arguments:       @res [resource]: libvirt storagepool resource
 *                  @xpath [string]: optional xPath expression string to get just this entry, can be NULL
 * Returns:         storagepool XML description string or result of xPath expression
 */
PHP_FUNCTION(libvirt_storagepool_get_xml_desc)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    char *xml;
    char *xpath = NULL;
    char *tmp = NULL;
    zend_long flags = 0;
    strsize_t xpath_len;
    int retval = -1;

    GET_STORAGEPOOL_FROM_ARGS("r|s", &zpool, &xpath, &xpath_len, &flags);
    if (xpath_len < 1)
        xpath = NULL;

    DPRINTF("%s: pool = %p, flags = %ld, xpath = %s\n", PHPFUNC, pool->pool, flags, xpath);

    xml = virStoragePoolGetXMLDesc(pool->pool, flags);
    if (xml == NULL) {
        set_error_if_unset("Cannot get the XML description" TSRMLS_CC);
        RETURN_FALSE;
    }

    tmp = get_string_from_xpath(xml, xpath, NULL, &retval);
    if ((tmp == NULL) || (retval < 0)) {
        VIRT_RETVAL_STRING(xml);
    } else {
        VIRT_RETVAL_STRING(tmp);
    }

    free(xml);
    free(tmp);
}

/*
 * Function name:   libvirt_storagepool_define_xml
 * Since version:   0.4.1(-1)
 * Description:     Function is used to define the storage pool from XML string and return it's resource
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @xml [string]: XML string definition of storagepool
 *                  @flags [int]: flags to define XML
 * Returns:         libvirt storagepool resource
 */
PHP_FUNCTION(libvirt_storagepool_define_xml)
{
    php_libvirt_storagepool *res_pool = NULL;
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    virStoragePoolPtr pool = NULL;
    char *xml;
    strsize_t xml_len;
    zend_long flags = 0;


    GET_CONNECTION_FROM_ARGS("rs|l", &zconn, &xml, &xml_len, &flags);

    pool = virStoragePoolDefineXML(conn->conn, xml, (unsigned int)flags);
    DPRINTF("%s: virStoragePoolDefineXML(%p, <xml>) returned %p\n", PHPFUNC, conn->conn, pool);
    if (pool == NULL)
        RETURN_FALSE;

    res_pool = (php_libvirt_storagepool *)emalloc(sizeof(php_libvirt_storagepool));
    res_pool->pool = pool;
    res_pool->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_pool->pool);
    resource_change_counter(INT_RESOURCE_STORAGEPOOL, conn->conn, res_pool->pool, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_pool, le_libvirt_storagepool);
}

/*
 * Function name:   libvirt_storagepool_undefine
 * Since version:   0.4.1(-1)
 * Description:     Function is used to undefine the storage pool identified by it's resource
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         TRUE if success, FALSE on error
 */
PHP_FUNCTION(libvirt_storagepool_undefine)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    int retval = 0;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    retval = virStoragePoolUndefine(pool->pool);
    DPRINTF("%s: virStoragePoolUndefine(%p) returned %d\n", PHPFUNC, pool->pool, retval);
    if (retval != 0)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_storagepool_create
 * Since version:   0.4.1(-1)
 * Description:     Function is used to create/start the storage pool
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         TRUE if success, FALSE on error
 */
PHP_FUNCTION(libvirt_storagepool_create)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    int retval;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    retval = virStoragePoolCreate(pool->pool, 0);
    DPRINTF("%s: virStoragePoolCreate(%p, 0) returned %d\n", PHPFUNC, pool->pool, retval);
    if (retval != 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_storagepool_destroy
 * Since version:   0.4.1(-1)
 * Description:     Function is used to destory the storage pool
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         TRUE if success, FALSE on error
 */
PHP_FUNCTION(libvirt_storagepool_destroy)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    int retval;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    retval = virStoragePoolDestroy(pool->pool);
    DPRINTF("%s: virStoragePoolDestroy(%p) returned %d\n", PHPFUNC, pool->pool, retval);
    if (retval != 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_storagepool_is_active
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get information whether storage pool is active or not
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         result of virStoragePoolIsActive
 */
PHP_FUNCTION(libvirt_storagepool_is_active)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    RETURN_LONG(virStoragePoolIsActive(pool->pool));
}

/*
 * Function name:   libvirt_storagepool_get_volume_count
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get storage volume count in the storage pool
 * Arguments:           @res [resource]: libvirt storagepool resource
 * Returns:             number of volumes in the pool
 */
PHP_FUNCTION(libvirt_storagepool_get_volume_count)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    RETURN_LONG(virStoragePoolNumOfVolumes(pool->pool));
}

/*
 * Function name:   libvirt_storagepool_refresh
 * Since version:   0.4.1(-1)
 * Description:     Function is used to refresh the storage pool information
 * Arguments:       @res [resource]: libvirt storagepool resource
 *                  @flags [int]: refresh flags
 * Returns:         TRUE if success, FALSE on error
 */
PHP_FUNCTION(libvirt_storagepool_refresh)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    zend_long flags = 0;
    int retval;

    GET_STORAGEPOOL_FROM_ARGS("r|l", &zpool, &flags);

    retval = virStoragePoolRefresh(pool->pool, flags);
    DPRINTF("%s: virStoragePoolRefresh(%p, %ld) returned %d\n", PHPFUNC, pool->pool, flags, retval);
    if (retval < 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_storagepool_set_autostart
 * Since version:   0.4.1(-1)
 * Description:     Function is used to set autostart of the storage pool
 * Arguments:       @res [resource]: libvirt storagepool resource
 *                  @flags [int]: flags to set autostart
 * Returns:         result on setting storagepool autostart value
 */
PHP_FUNCTION(libvirt_storagepool_set_autostart)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    zend_bool flags = 0;
    int retval;

    GET_STORAGEPOOL_FROM_ARGS("rb", &zpool, &flags);

    retval = virStoragePoolSetAutostart(pool->pool, flags);
    DPRINTF("%s: virStoragePoolSetAutostart(%p, %d) returned %d\n", PHPFUNC, pool->pool, flags, retval);
    if (retval != 0)
        RETURN_FALSE;
    RETURN_TRUE;
}

/*
 * Function name:   libvirt_storagepool_get_autostart
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get autostart of the storage pool
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         TRUE for autostart enabled, FALSE for autostart disabled, FALSE with last_error set for error
 */
PHP_FUNCTION(libvirt_storagepool_get_autostart)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    int autostart;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    if (virStoragePoolGetAutostart(pool->pool, &autostart) == 0 &&
        autostart != 0)
        RETURN_TRUE;

    RETURN_FALSE;
}

/*
 * Function name:   libvirt_storagepool_build
 * Since version:   0.4.2
 * Description:     Function is used to Build the underlying storage pool, e.g. create the destination directory for NFS
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         TRUE if success, FALSE on error
 */
PHP_FUNCTION(libvirt_storagepool_build)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    int flags = 0;
    int retval;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    retval = virStoragePoolBuild(pool->pool, flags);
    DPRINTF("%s: virStoragePoolBuild(%p, %d) returned %d\n", PHPFUNC, pool->pool, flags, retval);
    if (retval == 0)
        RETURN_TRUE;

    RETURN_FALSE;
}

/*
 * Function name:   libvirt_storagepool_delete
 * Since version:   0.4.6
 * Description:     Function is used to Delete the underlying storage pool, e.g. remove the destination directory for NFS
 * Arguments:       @res [resource]: libvirt storagepool resource
 * Returns:         TRUE if success, FALSE on error
 */
PHP_FUNCTION(libvirt_storagepool_delete)
{
    php_libvirt_storagepool *pool = NULL;
    zval *zpool;
    int flags = 0;
    int retval;

    GET_STORAGEPOOL_FROM_ARGS("r", &zpool);

    retval = virStoragePoolDelete(pool->pool, flags);
    DPRINTF("%s: virStoragePoolDelete(%p, %d) returned %d\n", PHPFUNC, pool->pool, flags, retval);
    if (retval == 0)
        RETURN_TRUE;

    RETURN_FALSE;
}

/* Listing functions */
/*
 * Function name:   libvirt_list_storagepools
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list storage pools on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 * Returns:         libvirt storagepool names array for the connection
 */
PHP_FUNCTION(libvirt_list_storagepools)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int count = -1;
    int expectedcount = -1;
    char **names;
    int i;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    if ((expectedcount = virConnectNumOfStoragePools(conn->conn)) < 0)
        RETURN_FALSE;

    names = (char **)emalloc(expectedcount*sizeof(char *));
    count = virConnectListStoragePools(conn->conn, names, expectedcount);

    if ((count != expectedcount) || (count < 0)) {
        efree(names);
        RETURN_FALSE;
    }

    array_init(return_value);
    for (i = 0; i < count; i++) {
        VIRT_ADD_NEXT_INDEX_STRING(return_value,  names[i]);
        free(names[i]);
    }
    efree(names);


    if ((expectedcount = virConnectNumOfDefinedStoragePools(conn->conn)) < 0)
        RETURN_FALSE;
    names = (char **)emalloc(expectedcount * sizeof(char *));
    count = virConnectListDefinedStoragePools(conn->conn, names, expectedcount);
    if ((count != expectedcount) || (count < 0)) {
        efree(names);
        RETURN_FALSE;
    }

    for (i = 0; i < count; i++) {
        VIRT_ADD_NEXT_INDEX_STRING(return_value, names[i]);
        free(names[i]);
    }
    efree(names);
}

/*
 * Function name:   libvirt_list_active_storagepools
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list active storage pools on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 * Returns:         libvirt storagepool names array for the connection
 */
PHP_FUNCTION(libvirt_list_active_storagepools)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int count = -1;
    int expectedcount = -1;
    char **names;
    int i;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    if ((expectedcount = virConnectNumOfStoragePools(conn->conn)) < 0)
        RETURN_FALSE;

    names = (char **)emalloc(expectedcount*sizeof(char *));
    count = virConnectListStoragePools(conn->conn, names, expectedcount);

    if ((count != expectedcount) || (count < 0)) {
        efree(names);
        RETURN_FALSE;
    }
    array_init(return_value);
    for (i = 0; i < count; i++) {
        VIRT_ADD_NEXT_INDEX_STRING(return_value,  names[i]);
        free(names[i]);
    }
    efree(names);
}

/*
 * Function name:   libvirt_list_inactive_storagepools
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list inactive storage pools on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 * Returns:         libvirt storagepool names array for the connection
 */
PHP_FUNCTION(libvirt_list_inactive_storagepools)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int count = -1;
    int expectedcount = -1;
    char **names;
    int i;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    if ((expectedcount = virConnectNumOfDefinedStoragePools(conn->conn)) < 0)
        RETURN_FALSE;

    names = (char **)emalloc(expectedcount * sizeof(char *));
    count = virConnectListDefinedStoragePools(conn->conn, names, expectedcount);
    if ((count != expectedcount) || (count < 0)) {
        efree(names);
        RETURN_FALSE;
    }

    array_init(return_value);
    for (i = 0; i < count; i++) {
        VIRT_ADD_NEXT_INDEX_STRING(return_value, names[i]);
        free(names[i]);
    }
    efree(names);
}

/*
 * Function name:   libvirt_list_all_networks
 * Since version:   0.5.3
 * Description:     Function is used to list networks on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @flags [int]: optional flags to filter the results for a smaller list of targetted networks (bitwise-OR VIR_CONNECT_LIST_NETWORKS_* constants)
 * Returns:         libvirt network resources array for the connection
 */
PHP_FUNCTION(libvirt_list_all_networks)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    zend_long flags = VIR_CONNECT_LIST_NETWORKS_ACTIVE |
                      VIR_CONNECT_LIST_NETWORKS_INACTIVE;
    int count = -1;
    size_t i = 0;
    virNetworkPtr *nets = NULL;
    virNetworkPtr network = NULL;
    php_libvirt_network *res_network;

    GET_CONNECTION_FROM_ARGS("r|l", &zconn, &flags);

    if ((count = virConnectListAllNetworks(conn->conn, &nets, flags)) < 0)
        RETURN_FALSE;

    DPRINTF("%s: Found %d networks\n", PHPFUNC, count);

    array_init(return_value);

    for (i = 0; i < count; i++) {
        network = nets[i];
        res_network = (php_libvirt_network *) emalloc(sizeof(php_libvirt_network));
        res_network->network = network;
        res_network->conn = conn;

        VIRT_REGISTER_LIST_RESOURCE(network);
        resource_change_counter(INT_RESOURCE_NETWORK, conn->conn,
                                res_network->network, 1 TSRMLS_CC);
    }
}

/*
 * Function name:   libvirt_list_networks
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list networks on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @flags [int]: flags whether to list active, inactive or all networks (VIR_NETWORKS_{ACTIVE|INACTIVE|ALL} constants)
 * Returns:         libvirt network names array for the connection
 */
PHP_FUNCTION(libvirt_list_networks)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    zend_long flags = VIR_NETWORKS_ACTIVE | VIR_NETWORKS_INACTIVE;
    int count = -1;
    int expectedcount = -1;
    char **names;
    int i, done = 0;

    GET_CONNECTION_FROM_ARGS("r|l", &zconn, &flags);

    array_init(return_value);
    if (flags & VIR_NETWORKS_ACTIVE) {
        if ((expectedcount = virConnectNumOfNetworks(conn->conn)) < 0)
            RETURN_FALSE;

        names = (char **)emalloc(expectedcount*sizeof(char *));
        count = virConnectListNetworks(conn->conn, names, expectedcount);
        if ((count != expectedcount) || (count < 0)) {
            efree(names);
            RETURN_FALSE;
        }

        for (i = 0; i < count; i++) {
            VIRT_ADD_NEXT_INDEX_STRING(return_value,  names[i]);
            free(names[i]);
        }

        efree(names);
        done++;
    }

    if (flags & VIR_NETWORKS_INACTIVE) {
        if ((expectedcount = virConnectNumOfDefinedNetworks(conn->conn)) < 0)
            RETURN_FALSE;
        names = (char **)emalloc(expectedcount*sizeof(char *));
        count = virConnectListDefinedNetworks(conn->conn, names, expectedcount);
        if ((count != expectedcount) || (count < 0)) {
            efree(names);
            RETURN_FALSE;
        }

        for (i = 0; i < count; i++) {
            VIRT_ADD_NEXT_INDEX_STRING(return_value, names[i]);
            free(names[i]);
        }

        efree(names);
        done++;
    }

    if (!done)
        RETURN_FALSE;
}

/*
 * Function name:   libvirt_list_nodedevs
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list node devices on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @cap [string]: optional capability string
 * Returns:         libvirt nodedev names array for the connection
 */
PHP_FUNCTION(libvirt_list_nodedevs)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int count = -1;
    int expectedcount = -1;
    char *cap = NULL;
    char **names;
    int i;
    strsize_t cap_len;

    GET_CONNECTION_FROM_ARGS("r|s", &zconn, &cap, &cap_len);

    if ((expectedcount = virNodeNumOfDevices(conn->conn, cap, 0)) < 0)
        RETURN_FALSE;
    names = (char **)emalloc(expectedcount*sizeof(char *));
    count = virNodeListDevices(conn->conn, cap, names, expectedcount, 0);
    if ((count != expectedcount) || (count < 0)) {
        efree(names);
        RETURN_FALSE;
    }

    array_init(return_value);
    for (i = 0; i < count; i++) {
        VIRT_ADD_NEXT_INDEX_STRING(return_value,  names[i]);
        free(names[i]);
    }

    efree(names);
}


/*
 * Function name:   libvirt_list_all_nwfilters
 * Since version:   0.5.4
 * Description:     Function is used to list nwfilters on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 * Returns:         libvirt nwfilter resources array for the connection
 */
PHP_FUNCTION(libvirt_list_all_nwfilters)
{
    php_libvirt_nwfilter *res_nwfilter;
    php_libvirt_connection *conn = NULL;
    virNWFilterPtr *filters = NULL;
    virNWFilterPtr nwfilter = NULL;
    zval *zconn;
    int count = -1;
    size_t i = 0;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    /* in current libvirt version, flags are not used for this, so passing 0 */
    if ((count = virConnectListAllNWFilters(conn->conn, &filters, 0)) < 0)
        RETURN_FALSE;

    DPRINTF("%s: Found %d nwfilters\n", PHPFUNC, count);

    array_init(return_value);

    for (i = 0; i < count; i++) {
        nwfilter = filters[i];
        res_nwfilter = (php_libvirt_nwfilter *) emalloc(sizeof(php_libvirt_nwfilter));
        res_nwfilter->nwfilter = nwfilter;
        res_nwfilter->conn = conn;

        resource_change_counter(INT_RESOURCE_NWFILTER, conn->conn,
                                res_nwfilter->nwfilter, 1 TSRMLS_CC);
        VIRT_REGISTER_LIST_RESOURCE(nwfilter);
    }
}

/*
 * Function name:   libvirt_list_nwfilters
 * Since version:   0.5.4
 * Description:     Function is used to list nwfilters on the connection
 * Arguments:       @res [resource]: libvirt connection resource
 * Returns:         libvirt nwfilter names array for the connection
 */
PHP_FUNCTION(libvirt_list_nwfilters)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int count = -1;
    int expectedcount = -1;
    char **names;
    int i, done = 0;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    array_init(return_value);

    if ((expectedcount = virConnectNumOfNWFilters(conn->conn)) < 0)
        RETURN_FALSE;

    names = (char **) emalloc(expectedcount * sizeof(char *));
    count = virConnectListNWFilters(conn->conn, names, expectedcount);

    if (count != expectedcount || count < 0) {
        efree(names);
        DPRINTF("%s: virConnectListNWFilters returned %d filters, while %d was "
                "expected\n", PHPFUNC, count, expectedcount);
        RETURN_FALSE;
    }

    for (i = 0; i < count; i++) {
        VIRT_ADD_NEXT_INDEX_STRING(return_value,  names[i]);
        free(names[i]);
    }

    efree(names);
    done++;


    if (!done)
        RETURN_FALSE;
}
/* Nodedev functions */

/*
 * Function name:   libvirt_nodedev_get
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the node device by it's name
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @name [string]: name of the nodedev to get resource
 * Returns:         libvirt nodedev resource
 */
PHP_FUNCTION(libvirt_nodedev_get)
{
    php_libvirt_connection *conn = NULL;
    php_libvirt_nodedev *res_dev = NULL;
    virNodeDevice *dev;
    zval *zconn;
    char *name;
    strsize_t name_len;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &name, &name_len);

    if ((dev = virNodeDeviceLookupByName(conn->conn, name)) == NULL) {
        set_error("Cannot get find requested node device" TSRMLS_CC);
        RETURN_FALSE;
    }

    res_dev = (php_libvirt_nodedev *)emalloc(sizeof(php_libvirt_nodedev));
    res_dev->device = dev;
    res_dev->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_dev->device);
    resource_change_counter(INT_RESOURCE_NODEDEV, conn->conn, res_dev->device, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_dev, le_libvirt_nodedev);
}

/*
 * Function name:   libvirt_nodedev_capabilities
 * Since version:   0.4.1(-1)
 * Description:     Function is used to list node devices by capabilities
 * Arguments:       @res [resource]: libvirt nodedev resource
 * Returns:         nodedev capabilities array
 */
PHP_FUNCTION(libvirt_nodedev_capabilities)
{
    php_libvirt_nodedev *nodedev = NULL;
    zval *znodedev;
    int count = -1;
    int expectedcount = -1;
    char **names;
    int i;

    GET_NODEDEV_FROM_ARGS("r", &znodedev);

    if ((expectedcount = virNodeDeviceNumOfCaps(nodedev->device)) < 0)
        RETURN_FALSE;
    names = (char **)emalloc(expectedcount*sizeof(char *));
    count = virNodeDeviceListCaps(nodedev->device, names, expectedcount);
    if ((count != expectedcount) || (count < 0))
        RETURN_FALSE;

    array_init(return_value);
    for (i = 0; i < count; i++) {
        VIRT_ADD_NEXT_INDEX_STRING(return_value, names[i]);
        free(names[i]);
    }

    efree(names);
}

/*
 * Function name:   libvirt_nodedev_get_xml_desc
 * Since version:   0.4.1(-1), changed 0.4.2
 * Description:     Function is used to get the node device's XML description
 * Arguments:       @res [resource]: libvirt nodedev resource
 *                  @xpath [string]: optional xPath expression string to get just this entry, can be NULL
 * Returns:         nodedev XML description string or result of xPath expression
 */
PHP_FUNCTION(libvirt_nodedev_get_xml_desc)
{
    php_libvirt_nodedev *nodedev = NULL;
    zval *znodedev;
    char *tmp = NULL;
    char *xml = NULL;
    char *xpath = NULL;
    strsize_t xpath_len;
    int retval = -1;

    GET_NODEDEV_FROM_ARGS("r|s", &znodedev, &xpath, &xpath_len);
    if (xpath_len < 1)
        xpath = NULL;

    xml = virNodeDeviceGetXMLDesc(nodedev->device, 0);
    if (!xml) {
        set_error("Cannot get the device XML information" TSRMLS_CC);
        RETURN_FALSE;
    }

    tmp = get_string_from_xpath(xml, xpath, NULL, &retval);
    if ((tmp == NULL) || (retval < 0))
        VIRT_RETVAL_STRING(xml);
    else
        VIRT_RETVAL_STRING(tmp);

    free(xml);
    free(tmp);
}

/*
 * Function name:   libvirt_nodedev_get_information
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the node device's information
 * Arguments:       @res [resource]: libvirt nodedev resource
 * Returns:         nodedev information array
 */
PHP_FUNCTION(libvirt_nodedev_get_information)
{
    php_libvirt_nodedev *nodedev = NULL;
    zval *znodedev;
    int retval = -1;
    char *xml = NULL;
    char *tmp = NULL;
    char *cap = NULL;

    GET_NODEDEV_FROM_ARGS("r", &znodedev);

    xml = virNodeDeviceGetXMLDesc(nodedev->device, 0);
    if (!xml) {
        set_error("Cannot get the device XML information" TSRMLS_CC);
        RETURN_FALSE;
    }

    array_init(return_value);

    /* Get name */
    tmp = get_string_from_xpath(xml, "//device/name", NULL, &retval);
    if (tmp == NULL) {
        set_error("Invalid XPath node for device name" TSRMLS_CC);
        goto error;
    }

    if (retval < 0) {
        set_error("Cannot get XPath expression result for device name" TSRMLS_CC);
        goto error;
    }

    VIRT_ADD_ASSOC_STRING(return_value, "name", tmp);

    /* Get parent name */
    free(tmp);
    tmp = get_string_from_xpath(xml, "//device/parent", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "parent", tmp);

    /* Get capability */
    cap = get_string_from_xpath(xml, "//device/capability/@type", NULL, &retval);
    if ((cap != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "capability", cap);

    /* System capability is having hardware and firmware sub-blocks */
    if (strcmp(cap, "system") == 0) {
        /* Get hardware vendor */
        free(tmp);
        tmp = get_string_from_xpath(xml, "//device/capability/hardware/vendor", NULL, &retval);
        if ((tmp != NULL) && (retval > 0))
            VIRT_ADD_ASSOC_STRING(return_value, "hardware_vendor", tmp);

        /* Get hardware version */
        free(tmp);
        tmp = get_string_from_xpath(xml, "//device/capability/hardware/version", NULL, &retval);
        if ((tmp != NULL) && (retval > 0))
            VIRT_ADD_ASSOC_STRING(return_value, "hardware_version", tmp);

        /* Get hardware serial */
        free(tmp);
        tmp = get_string_from_xpath(xml, "//device/capability/hardware/serial", NULL, &retval);
        if ((tmp != NULL) && (retval > 0))
            VIRT_ADD_ASSOC_STRING(return_value, "hardware_serial", tmp);

        /* Get hardware UUID */
        free(tmp);
        tmp = get_string_from_xpath(xml, "//device/capability/hardware/uuid", NULL, &retval);
        if (tmp != NULL)
            VIRT_ADD_ASSOC_STRING(return_value, "hardware_uuid", tmp);

        /* Get firmware vendor */
        free(tmp);
        tmp = get_string_from_xpath(xml, "//device/capability/firmware/vendor", NULL, &retval);
        if ((tmp != NULL) && (retval > 0))
            VIRT_ADD_ASSOC_STRING(return_value, "firmware_vendor", tmp);

        /* Get firmware version */
        free(tmp);
        tmp = get_string_from_xpath(xml, "//device/capability/firmware/version", NULL, &retval);
        if ((tmp != NULL) && (retval > 0))
            VIRT_ADD_ASSOC_STRING(return_value, "firmware_version", tmp);

        /* Get firmware release date */
        free(tmp);
        tmp = get_string_from_xpath(xml, "//device/capability/firmware/release_date", NULL, &retval);
        if ((tmp != NULL) && (retval > 0))
            VIRT_ADD_ASSOC_STRING(return_value, "firmware_release_date", tmp);
    }

    /* Get product_id */
    free(tmp);
    tmp = get_string_from_xpath(xml, "//device/capability/product/@id", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "product_id", tmp);

    /* Get product_name */
    free(tmp);
    tmp = get_string_from_xpath(xml, "//device/capability/product", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "product_name", tmp);

    /* Get vendor_id */
    free(tmp);
    tmp = get_string_from_xpath(xml, "//device/capability/vendor/@id", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "vendor_id", tmp);

    /* Get vendor_name */
    free(tmp);
    tmp = get_string_from_xpath(xml, "//device/capability/vendor", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "vendor_name", tmp);

    /* Get driver name */
    free(tmp);
    tmp = get_string_from_xpath(xml, "//device/driver/name", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "driver_name", tmp);

    /* Get driver name */
    free(tmp);
    tmp = get_string_from_xpath(xml, "//device/capability/interface", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "interface_name", tmp);

    /* Get driver name */
    free(tmp);
    tmp = get_string_from_xpath(xml, "//device/capability/address", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "address", tmp);

    /* Get driver name */
    free(tmp);
    tmp = get_string_from_xpath(xml, "//device/capability/capability/@type", NULL, &retval);
    if ((tmp != NULL) && (retval > 0))
        VIRT_ADD_ASSOC_STRING(return_value, "capabilities", tmp);

    free(cap);
    free(tmp);
    free(xml);
    return;

 error:
    free(cap);
    free(tmp);
    free(xml);
    RETURN_FALSE;
}

/* Network functions */

/*
 * Function name:   libvirt_network_define_xml
 * Since version:   0.4.2
 * Description:     Function is used to define a new virtual network based on the XML description
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @xml [string]: XML string definition of network to be defined
 * Returns:         libvirt network resource of newly defined network
 */
PHP_FUNCTION(libvirt_network_define_xml)
{
    php_libvirt_connection *conn = NULL;
    php_libvirt_network *res_net = NULL;
    virNetwork *net;
    zval *zconn;
    char *xml = NULL;
    strsize_t xml_len;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &xml, &xml_len);

    if ((net = virNetworkDefineXML(conn->conn, xml)) == NULL) {
        set_error_if_unset("Cannot define a new network" TSRMLS_CC);
        RETURN_FALSE;
    }

    res_net = (php_libvirt_network *)emalloc(sizeof(php_libvirt_network));
    res_net->network = net;
    res_net->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_net->network);
    resource_change_counter(INT_RESOURCE_NETWORK, conn->conn, res_net->network, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_net, le_libvirt_network);
}

/*
 * Function name:   libvirt_network_undefine
 * Since version:   0.4.2
 * Description:     Function is used to undefine already defined network
 * Arguments:       @res [resource]: libvirt network resource
 * Returns:         TRUE for success, FALSE on error
 */
PHP_FUNCTION(libvirt_network_undefine)
{
    php_libvirt_network *network = NULL;
    zval *znetwork;

    GET_NETWORK_FROM_ARGS("r", &znetwork);

    if (virNetworkUndefine(network->network) != 0)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_network_get
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the network resource from name
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @name [string]: network name string
 * Returns:         libvirt network resource
 */
PHP_FUNCTION(libvirt_network_get)
{
    php_libvirt_connection *conn = NULL;
    php_libvirt_network *res_net = NULL;
    virNetwork *net;
    zval *zconn;
    char *name;
    strsize_t name_len;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &name, &name_len);

    if ((net = virNetworkLookupByName(conn->conn, name)) == NULL) {
        set_error_if_unset("Cannot get find requested network" TSRMLS_CC);
        RETURN_FALSE;
    }

    res_net = (php_libvirt_network *)emalloc(sizeof(php_libvirt_network));
    res_net->network = net;
    res_net->conn = conn;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_net->network);
    resource_change_counter(INT_RESOURCE_NETWORK, conn->conn, res_net->network, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_net, le_libvirt_network);
}

/*
 * Function name:   libvirt_network_get_bridge
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the bridge associated with the network
 * Arguments:       @res [resource]: libvirt network resource
 * Returns:         bridge name string
 */
PHP_FUNCTION(libvirt_network_get_bridge)
{
    php_libvirt_network *network;
    zval *znetwork;
    char *name;

    GET_NETWORK_FROM_ARGS("r", &znetwork);

    name = virNetworkGetBridgeName(network->network);

    if (name == NULL) {
        set_error_if_unset("Cannot get network bridge name" TSRMLS_CC);
        RETURN_FALSE;
    }

    VIRT_RETVAL_STRING(name);
    free(name);
}

/*
 * Function name:   libvirt_network_get_active
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the activity state of the network
 * Arguments:       @res [resource]: libvirt network resource
 * Returns:         1 when active, 0 when inactive, FALSE on error
 */
PHP_FUNCTION(libvirt_network_get_active)
{
    php_libvirt_network *network;
    zval *znetwork;
    int res;

    GET_NETWORK_FROM_ARGS("r", &znetwork);

    res = virNetworkIsActive(network->network);

    if (res == -1) {
        set_error_if_unset("Error getting virtual network state" TSRMLS_CC);
        RETURN_FALSE;
    }

    RETURN_LONG(res);
}

/*
 * Function name:   libvirt_network_get_information
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the network information
 * Arguments:       @res [resource]: libvirt network resource
 * Returns:         network information array
 */
PHP_FUNCTION(libvirt_network_get_information)
{
    php_libvirt_network *network = NULL;
    zval *znetwork;
    int retval = 0;
    char *xml  = NULL;
    char *name = NULL;
    char *ipaddr = NULL;
    char *netmask = NULL;
    char *mode = NULL;
    char *dev = NULL;
    char *dhcp_start = NULL;
    char *dhcp_end = NULL;
    char fixedtemp[32] = { 0 };

    GET_NETWORK_FROM_ARGS("r", &znetwork);

    xml = virNetworkGetXMLDesc(network->network, 0);

    if (xml == NULL) {
        set_error_if_unset("Cannot get network XML" TSRMLS_CC);
        RETURN_FALSE;
    }

    array_init(return_value);

    /* Get name */
    name = get_string_from_xpath(xml, "//network/name", NULL, &retval);
    if (name == NULL) {
        set_error("Invalid XPath node for network name" TSRMLS_CC);
        RETURN_FALSE;
    }

    if (retval < 0) {
        set_error("Cannot get XPath expression result for network name" TSRMLS_CC);
        RETURN_FALSE;
    }

    VIRT_ADD_ASSOC_STRING(return_value, "name", name);

    /* Get gateway IP address */
    ipaddr = get_string_from_xpath(xml, "//network/ip/@address", NULL, &retval);
    if (ipaddr && retval > 0)
        VIRT_ADD_ASSOC_STRING(return_value, "ip", ipaddr);

    /* Get netmask */
    netmask = get_string_from_xpath(xml, "//network/ip/@netmask", NULL, &retval);
    if (netmask && retval > 0) {
        int subnet_bits = get_subnet_bits(netmask);
        VIRT_ADD_ASSOC_STRING(return_value, "netmask", netmask);
        add_assoc_long(return_value, "netmask_bits", (long) subnet_bits);

        /* Format CIDR address representation */
        ipaddr[strlen(ipaddr) - 1] = ipaddr[strlen(ipaddr) - 1] - 1;
        snprintf(fixedtemp, sizeof(fixedtemp), "%s/%d", ipaddr, subnet_bits);
        VIRT_ADD_ASSOC_STRING(return_value, "ip_range", fixedtemp);
    }

    /* Get forwarding settings */
    mode = get_string_from_xpath(xml, "//network/forward/@mode", NULL, &retval);
    if (mode && retval > 0)
        VIRT_ADD_ASSOC_STRING(return_value, "forwarding", mode);

    /* Get forwarding settings */
    dev = get_string_from_xpath(xml, "//network/forward/@dev", NULL, &retval);
    if (dev && retval > 0)
        VIRT_ADD_ASSOC_STRING(return_value, "forward_dev", dev);

    /* Get DHCP values */
    dhcp_start = get_string_from_xpath(xml, "//network/ip/dhcp/range/@start", NULL, &retval);
    dhcp_end = get_string_from_xpath(xml, "//network/ip/dhcp/range/@end", NULL, &retval);
    if (dhcp_start && dhcp_end && retval > 0) {
        VIRT_ADD_ASSOC_STRING(return_value, "dhcp_start", dhcp_start);
        VIRT_ADD_ASSOC_STRING(return_value, "dhcp_end", dhcp_end);
    }

    free(dhcp_end);
    free(dhcp_start);
    free(dev);
    free(mode);
    free(netmask);
    free(ipaddr);
    free(name);
    free(xml);
}

/*
 * Function name:   libvirt_network_set_active
 * Since version:   0.4.1(-1)
 * Description:     Function is used to set the activity state of the network
 * Arguments:       @res [resource]: libvirt network resource
 *                  @flags [int]: active
 * Returns:         TRUE if success, FALSE on error
 */
PHP_FUNCTION(libvirt_network_set_active)
{
    php_libvirt_network *network;
    zval *znetwork;
    zend_long act = 0;

    DPRINTF("%s: Setting network activity...\n", PHPFUNC);

    GET_NETWORK_FROM_ARGS("rl", &znetwork, &act);

    if ((act != 0) && (act != 1)) {
        set_error("Invalid network activity state" TSRMLS_CC);
        RETURN_FALSE;
    }

    DPRINTF("%s: %sabling network...\n", PHPFUNC, (act == 1) ? "En" : "Dis");

    if (act == 1) {
        if (virNetworkCreate(network->network) == 0) {
            // Network is up and running
            RETURN_TRUE;
        } else {
            // We don't have to set error since it's caught by libvirt error handler itself
            RETURN_FALSE;
        }
    }

    if (virNetworkDestroy(network->network) == 0) {
        // Network is down
        RETURN_TRUE;
    } else {
        // Caught by libvirt error handler too
        RETURN_FALSE;
    }
}

/*
 * Function name:   libvirt_network_get_xml_desc
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the XML description for the network
 * Arguments:       @res [resource]: libvirt network resource
 *                  @xpath [string]: optional xPath expression string to get just this entry, can be NULL
 * Returns:         network XML string or result of xPath expression
 */
PHP_FUNCTION(libvirt_network_get_xml_desc)
{
    php_libvirt_network *network;
    zval *znetwork;
    char *xml = NULL;
    char *xpath = NULL;
    char *tmp;
    strsize_t xpath_len;
    int retval = -1;

    GET_NETWORK_FROM_ARGS("r|s", &znetwork, &xpath, &xpath_len);
    if (xpath_len < 1)
        xpath = NULL;

    xml = virNetworkGetXMLDesc(network->network, 0);

    if (xml == NULL) {
        set_error_if_unset("Cannot get network XML" TSRMLS_CC);
        RETURN_FALSE;
    }

    tmp = get_string_from_xpath(xml, xpath, NULL, &retval);
    if ((tmp == NULL) || (retval < 0)) {
        VIRT_RETVAL_STRING(xml);
    } else {
        VIRT_RETVAL_STRING(tmp);
    }

    free(xml);
    free(tmp);
}

/*
 * Function name:   libvirt_network_get_uuid_string
 * Since version:   0.5.3
 * Description:     Function is used to get network's UUID in string format
 * Arguments:       @res [resource]: libvirt network resource
 * Returns:         network UUID string or FALSE on failure
 */
PHP_FUNCTION(libvirt_network_get_uuid_string)
{
    php_libvirt_network *network = NULL;
    zval *znetwork;
    char *uuid = NULL;
    int ret = -1;

    GET_NETWORK_FROM_ARGS("r", &znetwork);

    uuid = (char *) emalloc(VIR_UUID_STRING_BUFLEN);
    ret = virNetworkGetUUIDString(network->network, uuid);

    DPRINTF("%s: virNetworkGetUUIDString(%p) returned %d (%s)\n", PHPFUNC,
            network->network, ret, uuid);

    if (ret != 0)
        RETURN_FALSE;

    VIRT_RETURN_STRING(uuid);
    efree(uuid);
}

/*
 * Function name:   libvirt_network_get_uuid
 * Since version:   0.5.3
 * Descirption:     Function is used to get network's UUID in binary format
 * Arguments:       @res [resource]: libvirt netowrk resource
 * Returns:         network UUID in binary format or FALSE on failure
 */
PHP_FUNCTION(libvirt_network_get_uuid)
{
    php_libvirt_network *network = NULL;
    zval *znetwork;
    char *uuid = NULL;
    int ret = -1;

    GET_NETWORK_FROM_ARGS("r", &znetwork);

    uuid = (char *) emalloc(VIR_UUID_BUFLEN);
    ret = virNetworkGetUUID(network->network, (unsigned char *)uuid);

    DPRINTF("%s: virNetworkGetUUID(%p, %p) returned %d\n", PHPFUNC,
            network->network, uuid, ret);

    if (ret != 0)
        RETURN_FALSE;

    VIRT_RETVAL_STRING(uuid);
    efree(uuid);
}

/*
 * Function name:   libvirt_network_get_name
 * Since version:   0.5.3
 * Description:     Function is used to get network's name
 * Arguments:       @res [resource]: libvirt network resource
 * Returns:         network name string or FALSE on failure
 */
PHP_FUNCTION(libvirt_network_get_name)
{
    php_libvirt_network *network = NULL;
    zval *znetwork;
    const char *name = NULL;

    GET_NETWORK_FROM_ARGS("r", &znetwork);
    name = virNetworkGetName(network->network);

    DPRINTF("%s: virNetworkGetName(%p) returned %s\n", PHPFUNC,
            network->network, name);

    if (name == NULL)
        RETURN_FALSE;

    /* name should not be freed as its lifetime is the same as network resource */
    VIRT_RETURN_STRING(name);
}

/*
 * Function name:   libvirt_network_get_autostart
 * Since version:   0.5.4
 * Description:     Function is getting the autostart value for the network
 * Arguments:       @res [resource]: libvirt network resource
 * Returns:         autostart value or -1 on error
 */
PHP_FUNCTION(libvirt_network_get_autostart)
{
    php_libvirt_network *network = NULL;
    zval *znetwork;
    int autostart;

    GET_NETWORK_FROM_ARGS("r", &znetwork);

    if (virNetworkGetAutostart(network->network, &autostart) != 0)
        RETURN_LONG(-1);

    RETURN_LONG((long) autostart);
}

/*
 * Function name:   libvirt_network_set_autostart
 * Since version:   0.5.4
 * Description:     Function is setting the autostart value for the network
 * Arguments:       @res [resource]: libvirt network resource
 *                  @flags [int]: flag to enable/disable autostart
 * Returns:         TRUE on success, FALSE on error
 */
PHP_FUNCTION(libvirt_network_set_autostart)
{
    php_libvirt_network *network = NULL;
    zval *znetwork;
    zend_long autostart = 0;

    GET_NETWORK_FROM_ARGS("rl", &znetwork, &autostart);

    if (virNetworkSetAutostart(network->network, autostart) < 0)
        RETURN_FALSE;

    RETURN_TRUE;
}

/* NWFilter functions */

/*
 * Function name:   libvirt_nwfilter_define_xml
 * Since version:   0.5.4
 * Description:     Function is used to define a new nwfilter based on the XML description
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @xml [string]: XML string definition of nwfilter to be defined
 * Returns:         libvirt nwfilter resource of newly defined nwfilter
 */
PHP_FUNCTION(libvirt_nwfilter_define_xml)
{
    php_libvirt_connection *conn = NULL;
    php_libvirt_nwfilter *res_nwfilter = NULL;
    virNWFilter *nwfilter;
    zval *zconn;
    char *xml = NULL;
    strsize_t xml_len;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &xml, &xml_len);

    if ((nwfilter = virNWFilterDefineXML(conn->conn, xml)) == NULL) {
        set_error_if_unset("Cannot define a new NWFilter" TSRMLS_CC);
        RETURN_FALSE;
    }

    res_nwfilter = (php_libvirt_nwfilter *) emalloc(sizeof(php_libvirt_nwfilter));
    res_nwfilter->nwfilter = nwfilter;
    res_nwfilter->conn = conn;

    resource_change_counter(INT_RESOURCE_NWFILTER, conn->conn,
                            res_nwfilter->nwfilter, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_nwfilter, le_libvirt_nwfilter);
}

/*
 * Function name:   libvirt_nwfilter_undefine
 * Since version:   0.5.4
 * Description:     Function is used to undefine already defined nwfilter
 * Arguments:       @res [resource]: libvirt nwfilter resource
 * Returns:         TRUE for success, FALSE on error
 */
PHP_FUNCTION(libvirt_nwfilter_undefine)
{
    php_libvirt_nwfilter *nwfilter = NULL;
    zval *znwfilter;

    GET_NWFILTER_FROM_ARGS("r", &znwfilter);

    if (virNWFilterUndefine(nwfilter->nwfilter) != 0)
        RETURN_FALSE;

    RETURN_TRUE;
}

/*
 * Function name:   libvirt_nwfilter_get_xml_desc
 * Since version:   0.5.4
 * Description:     Function is used to get the XML description for the nwfilter
 * Arguments:       @res [resource]: libvirt nwfilter resource
 *                  @xpath [string]: optional xPath expression string to get just this entry, can be NULL
 * Returns:         nwfilter XML string or result of xPath expression
 */
PHP_FUNCTION(libvirt_nwfilter_get_xml_desc)
{
    php_libvirt_nwfilter *nwfilter = NULL;
    zval *znwfilter;
    char *xml = NULL;
    char *xpath = NULL;
    char *tmp;
    strsize_t xpath_len;
    int retval = -1;

    GET_NWFILTER_FROM_ARGS("r|s", &znwfilter, &xpath, &xpath_len);

    if (xpath_len < 1)
        xpath = NULL;

    xml = virNWFilterGetXMLDesc(nwfilter->nwfilter, 0);

    if (xml == NULL) {
        set_error_if_unset("Cannot get nwfilter XML" TSRMLS_CC);
        RETURN_FALSE;
    }

    tmp = get_string_from_xpath(xml, xpath, NULL, &retval);

    if (tmp == NULL || retval < 0)
        VIRT_RETVAL_STRING(xml);
    else
        VIRT_RETVAL_STRING(tmp);

    free(xml);
    free(tmp);
}

/*
 * Function name:   libvirt_nwfilter_get_uuid_string
 * Since version:   0.5.4
 * Description:     Function is used to get nwfilter's UUID in string format
 * Arguments:       @res [resource]: libvirt nwfilter resource
 * Returns:         nwfilter UUID string or FALSE on failure
 */
PHP_FUNCTION(libvirt_nwfilter_get_uuid_string)
{
    php_libvirt_nwfilter *nwfilter = NULL;
    zval *znwfilter;
    char *uuid = NULL;
    int ret = -1;

    GET_NWFILTER_FROM_ARGS("r", &znwfilter);

    uuid = (char *) emalloc(VIR_UUID_STRING_BUFLEN);
    ret = virNWFilterGetUUIDString(nwfilter->nwfilter, uuid);

    DPRINTF("%s: virNWFilterGetUUIDString(%p) returned %d (%s)\n", PHPFUNC,
            nwfilter->nwfilter, ret, uuid);

    if (ret != 0)
        RETURN_FALSE;

    VIRT_RETURN_STRING(uuid);
    efree(uuid);
}

/*
 * Function name:   libvirt_nwfilter_get_uuid
 * Since version:   0.5.3
 * Descirption:     Function is used to get nwfilter's UUID in binary format
 * Arguments:       @res [resource]: libvirt netowrk resource
 * Returns:         nwfilter UUID in binary format or FALSE on failure
 */
PHP_FUNCTION(libvirt_nwfilter_get_uuid)
{
    php_libvirt_nwfilter *nwfilter = NULL;
    zval *znwfilter;
    char *uuid = NULL;
    int ret = -1;

    GET_NWFILTER_FROM_ARGS("r", &znwfilter);

    uuid = (char *) emalloc(VIR_UUID_BUFLEN);
    ret = virNWFilterGetUUID(nwfilter->nwfilter, (unsigned char *) uuid);

    DPRINTF("%s: virNWFilterUUID(%p, %p) returned %d\n", PHPFUNC,
            nwfilter->nwfilter, uuid, ret);

    if (ret != 0)
        RETURN_FALSE;

    VIRT_RETVAL_STRING(uuid);
    efree(uuid);
}

/*
 * Function name:   libvirt_nwfilter_get_name
 * Since version:   0.5.4
 * Description:     Function is used to get nwfilter's name
 * Arguments:       @res [resource]: libvirt nwfilter resource
 * Returns:         nwfilter name string or FALSE on failure
 */
PHP_FUNCTION(libvirt_nwfilter_get_name)
{
    php_libvirt_nwfilter *nwfilter = NULL;
    zval *znwfilter;
    const char *name = NULL;

    GET_NWFILTER_FROM_ARGS("r", &znwfilter);
    name = virNWFilterGetName(nwfilter->nwfilter);

    DPRINTF("%s: virNWFilterGetName(%p) returned %s\n", PHPFUNC,
            nwfilter->nwfilter, name);

    if (name == NULL)
        RETURN_FALSE;

    /* name should not be freed as its lifetime is the same as nwfilter resource */
    VIRT_RETURN_STRING(name);
}

/*
 * Function name:   libvirt_nwfilter_lookup_by_name
 * Since version:   0.5.4
 * Description:     This functions is used to lookup for the nwfilter by it's name
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @name [string]: name of the nwfilter to get the resource
 * Returns:         libvirt nwfilter resource
 */
PHP_FUNCTION(libvirt_nwfilter_lookup_by_name)
{
    php_libvirt_nwfilter *res_nwfilter = NULL;
    php_libvirt_connection *conn = NULL;
    virNWFilterPtr nwfilter = NULL;
    zval *zconn;
    strsize_t name_len;
    char *name = NULL;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &name, &name_len);

    if (name == NULL || name_len < 1)
        RETURN_FALSE;

    nwfilter = virNWFilterLookupByName(conn->conn, name);

    if (nwfilter == NULL)
        RETURN_FALSE;

    res_nwfilter = (php_libvirt_nwfilter *) emalloc(sizeof(php_libvirt_nwfilter));
    res_nwfilter->conn = conn;
    res_nwfilter->nwfilter = nwfilter;

    resource_change_counter(INT_RESOURCE_NWFILTER, conn->conn,
                            res_nwfilter->nwfilter, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_nwfilter, le_libvirt_nwfilter);
}

/*
 * Function name:   libvirt_nwfilter_lookup_by_uuid_string
 * Since version:   0.5.4
 * Description:     Function is used to lookup for nwfilter identified by UUID string
 * Arguments:       @res [resource]: libvirt connection resource
 *                  @uuid [string]: UUID string to look for nwfilter
 * Returns:         libvirt nwfilter resource
 */
PHP_FUNCTION(libvirt_nwfilter_lookup_by_uuid_string)
{
    php_libvirt_nwfilter *res_nwfilter = NULL;
    php_libvirt_connection *conn = NULL;
    virNWFilterPtr nwfilter = NULL;
    zval *zconn;
    char *uuid = NULL;
    strsize_t uuid_len;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &uuid, &uuid_len);

    if (uuid == NULL || uuid_len < 1)
        RETURN_FALSE;

    nwfilter = virNWFilterLookupByUUIDString(conn->conn, uuid);

    if (nwfilter == NULL)
        RETURN_FALSE;

    res_nwfilter = (php_libvirt_nwfilter *) emalloc(sizeof(php_libvirt_nwfilter));
    res_nwfilter->conn = conn;
    res_nwfilter->nwfilter = nwfilter;

    resource_change_counter(INT_RESOURCE_NWFILTER, conn->conn,
                            res_nwfilter->nwfilter, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_nwfilter, le_libvirt_nwfilter);
}

/*
 * Function name:   libvirt_nwfilter_lookup_by_uuid
 * Since version:   0.5.4
 * Description:     Function is used to lookup for nwfilter by it's UUID in the binary format
 * Arguments:       @res [resource]: libvirt connection resource from libvirt_connect()
 *                  @uuid [string]: binary defined UUID to look for
 * Returns:         libvirt nwfilter resource
 */
PHP_FUNCTION(libvirt_nwfilter_lookup_by_uuid)
{
    php_libvirt_nwfilter *res_nwfilter = NULL;
    php_libvirt_connection *conn = NULL;
    virNWFilterPtr nwfilter = NULL;
    zval *zconn;
    strsize_t uuid_len;
    unsigned char *uuid = NULL;

    GET_CONNECTION_FROM_ARGS("rs", &zconn, &uuid, &uuid_len);

    if ((uuid == NULL) || (uuid_len < 1))
        RETURN_FALSE;

    nwfilter = virNWFilterLookupByUUID(conn->conn, uuid);

    if (nwfilter == NULL)
        RETURN_FALSE;

    res_nwfilter = (php_libvirt_nwfilter *) emalloc(sizeof(php_libvirt_nwfilter));
    res_nwfilter->conn = conn;
    res_nwfilter->nwfilter = nwfilter;

    resource_change_counter(INT_RESOURCE_NWFILTER, conn->conn,
                            res_nwfilter->nwfilter, 1 TSRMLS_CC);

    VIRT_REGISTER_RESOURCE(res_nwfilter, le_libvirt_nwfilter);
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
    strsize_t type_len;
    char *type = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &type, &type_len) == FAILURE) {
        set_error("Invalid arguments" TSRMLS_CC);
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

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lll|l", &major, &minor, &micro, &type) == FAILURE) {
        set_error("Invalid arguments" TSRMLS_CC);
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
            set_error("Invalid version type" TSRMLS_CC);
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
    strsize_t name_len = 0;
    const char *binary = NULL;
    int ret = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) == FAILURE) {
        set_error("Invalid argument" TSRMLS_CC);
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
    strsize_t path_len = 0;
#ifndef EXTWIN
    struct dirent *entry;
    DIR *d = NULL;
#endif
    int num = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &path, &path_len) == FAILURE) {
        set_error("Invalid argument" TSRMLS_CC);
        RETURN_FALSE;
    }

    if (LIBVIRT_G(iso_path_ini))
        path = strdup(LIBVIRT_G(iso_path_ini));

    if ((path == NULL) || (path[0] != '/')) {
        set_error("Invalid argument, path must be set and absolute (start by slash character [/])" TSRMLS_CC);
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
    strsize_t filename_len = 0;
    int err;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &filename, &filename_len, &maxsize) == FAILURE) {
        set_error("Invalid argument" TSRMLS_CC);
        RETURN_FALSE;
    }

    if ((filename == NULL) || (strcasecmp(filename, "null") == 0))
        err = set_logfile(NULL, 0 TSRMLS_CC);
    else
        err = set_logfile(filename, maxsize TSRMLS_CC);

    if (err < 0) {
        char tmp[1024] = { 0 };
        snprintf(tmp, sizeof(tmp), "Cannot set the log file to %s, error code = %d (%s)", filename, err, strerror(-err));
        set_error(tmp TSRMLS_CC);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
#endif
