/*
 * libvirt-domain.h: The PHP bindings to libvirt domain API
 *
 * See COPYING for the license of this software
 */

#ifndef __LIBVIRT_DOMAIN_H__
# define __LIBVIRT_DOMAIN_H__

# include "libvirt-connection.h"

# define PHP_LIBVIRT_DOMAIN_RES_NAME "Libvirt domain"
# define INT_RESOURCE_DOMAIN 0x02

# define DOMAIN_DISK_FILE            0x01
# define DOMAIN_DISK_BLOCK           0x02
# define DOMAIN_DISK_ACCESS_ALL      0x04

# define DOMAIN_FLAG_FEATURE_ACPI    0x01
# define DOMAIN_FLAG_FEATURE_APIC    0x02
# define DOMAIN_FLAG_FEATURE_PAE     0x04
# define DOMAIN_FLAG_CLOCK_LOCALTIME 0x08
# define DOMAIN_FLAG_TEST_LOCAL_VNC  0x10
# define DOMAIN_FLAG_SOUND_AC97      0x20

# define GET_DOMAIN_FROM_ARGS(args, ...)                                       \
    do {                                                                       \
        reset_error(TSRMLS_C);                                                 \
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,                   \
                                  args,                                        \
                                  __VA_ARGS__) == FAILURE) {                   \
            set_error("Invalid arguments" TSRMLS_CC);                          \
            RETURN_FALSE;                                                      \
        }                                                                      \
                                                                               \
        VIRT_FETCH_RESOURCE(domain, php_libvirt_domain*, &zdomain,             \
                            PHP_LIBVIRT_DOMAIN_RES_NAME, le_libvirt_domain);   \
        if (domain == NULL || domain->domain == NULL)                          \
            RETURN_FALSE;                                                      \
    } while (0)                                                                \

# define PHP_FE_LIBVIRT_DOMAIN                                                                 \
    PHP_FE(libvirt_domain_new,                   arginfo_libvirt_domain_new)                   \
    PHP_FE(libvirt_domain_new_get_vnc,           arginfo_libvirt_void)                         \
    PHP_FE(libvirt_domain_get_counts,            arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_is_persistent,         arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_lookup_by_name,        arginfo_libvirt_conn_name)                    \
    PHP_FE(libvirt_domain_get_xml_desc,          arginfo_libvirt_conn_xpath)                   \
    PHP_FE(libvirt_domain_get_disk_devices,      arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_get_interface_devices, arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_change_vcpus,          arginfo_libvirt_domain_change_vcpus)          \
    PHP_FE(libvirt_domain_change_memory,         arginfo_libvirt_domain_change_memory)         \
    PHP_FE(libvirt_domain_change_boot_devices,   arginfo_libvirt_domain_change_boot_devices)   \
    PHP_FE(libvirt_domain_disk_add,              arginfo_libvirt_domain_disk_add)              \
    PHP_FE(libvirt_domain_disk_remove,           arginfo_libvirt_domain_disk_remove)           \
    PHP_FE(libvirt_domain_nic_add,               arginfo_libvirt_domain_nic_add)               \
    PHP_FE(libvirt_domain_nic_remove,            arginfo_libvirt_domain_nic_remove)            \
    PHP_FE(libvirt_domain_attach_device,         arginfo_libvirt_domain_attach_device)         \
    PHP_FE(libvirt_domain_detach_device,         arginfo_libvirt_domain_detach_device)         \
    PHP_FE(libvirt_domain_get_info,              arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_get_name,              arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_get_uuid,              arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_get_uuid_string,       arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_get_id,                arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_lookup_by_uuid,        arginfo_libvirt_conn_uuid)                    \
    PHP_FE(libvirt_domain_lookup_by_uuid_string, arginfo_libvirt_conn_uuid)                    \
    PHP_FE(libvirt_domain_lookup_by_id,          arginfo_libvirt_domain_lookup_by_id)          \
    PHP_FE(libvirt_domain_create,                arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_destroy,               arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_resume,                arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_core_dump,             arginfo_libvirt_domain_core_dump)             \
    PHP_FE(libvirt_domain_shutdown,              arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_suspend,               arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_managedsave,           arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_undefine,              arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_reboot,                arginfo_libvirt_conn_flags)                   \
    PHP_FE(libvirt_domain_reset,                 arginfo_libvirt_conn_flags)                   \
    PHP_FE(libvirt_domain_define_xml,            arginfo_libvirt_conn_xml)                     \
    PHP_FE(libvirt_domain_create_xml,            arginfo_libvirt_conn_xml)                     \
    PHP_FE(libvirt_domain_xml_from_native,       arginfo_libvirt_domain_xml_from_native)       \
    PHP_FE(libvirt_domain_xml_to_native,         arginfo_libvirt_domain_xml_to_native)         \
    PHP_FE(libvirt_domain_memory_peek,           arginfo_libvirt_domain_memory_peek)           \
    PHP_FE(libvirt_domain_memory_stats,          arginfo_libvirt_conn_flags)                   \
    PHP_FE(libvirt_domain_set_memory,            arginfo_libvirt_domain_set_memory)            \
    PHP_FE(libvirt_domain_set_max_memory,        arginfo_libvirt_domain_set_memory)            \
    PHP_FE(libvirt_domain_set_memory_flags,      arginfo_libvirt_domain_set_memory_flags)      \
    PHP_FE(libvirt_domain_block_commit,          arginfo_libvirt_domain_block_commit)          \
    PHP_FE(libvirt_domain_block_stats,           arginfo_libvirt_conn_path)                    \
    PHP_FE(libvirt_domain_block_resize,          arginfo_libvirt_domain_block_resize)          \
    PHP_FE(libvirt_domain_block_job_info,        arginfo_libvirt_domain_block_job_info)        \
    PHP_FE(libvirt_domain_block_job_abort,       arginfo_libvirt_domain_block_job_abort)       \
    PHP_FE(libvirt_domain_block_job_set_speed,   arginfo_libvirt_domain_block_job_set_speed)   \
    PHP_FE(libvirt_domain_interface_addresses,   arginfo_libvirt_domain_interface_addresses)   \
    PHP_FE(libvirt_domain_interface_stats,       arginfo_libvirt_conn_path)                    \
    PHP_FE(libvirt_domain_get_connect,           arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_migrate,               arginfo_libvirt_domain_migrate)               \
    PHP_FE(libvirt_domain_migrate_to_uri,        arginfo_libvirt_domain_migrate_to_uri)        \
    PHP_FE(libvirt_domain_migrate_to_uri2,       arginfo_libvirt_domain_migrate_to_uri2)       \
    PHP_FE(libvirt_domain_get_job_info,          arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_xml_xpath,             arginfo_libvirt_domain_xml_xpath)             \
    PHP_FE(libvirt_domain_get_block_info,        arginfo_libvirt_domain_get_block_info)        \
    PHP_FE(libvirt_domain_get_network_info,      arginfo_libvirt_domain_get_network_info)      \
    PHP_FE(libvirt_domain_get_autostart,         arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_set_autostart,         arginfo_libvirt_conn_flags)                   \
    PHP_FE(libvirt_domain_get_metadata,          arginfo_libvirt_domain_get_metadata)          \
    PHP_FE(libvirt_domain_set_metadata,          arginfo_libvirt_domain_set_metadata)          \
    PHP_FE(libvirt_domain_is_active,             arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_get_next_dev_ids,      arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_domain_get_screenshot,        arginfo_libvirt_domain_get_screenshot)        \
    PHP_FE(libvirt_domain_get_screenshot_api,    arginfo_libvirt_domain_get_screenshot_api)    \
    PHP_FE(libvirt_domain_get_screen_dimensions, arginfo_libvirt_domain_get_screen_dimensions) \
    PHP_FE(libvirt_domain_send_keys,             arginfo_libvirt_domain_send_keys)             \
    PHP_FE(libvirt_domain_send_key_api,          arginfo_libvirt_domain_send_key_api)          \
    PHP_FE(libvirt_domain_send_pointer_event,    arginfo_libvirt_domain_send_pointer_event)    \
    PHP_FE(libvirt_domain_update_device,         arginfo_libvirt_domain_update_device)         \
    PHP_FE(libvirt_domain_qemu_agent_command,    arginfo_libvirt_domain_qemu_agent_command)    \
    PHP_FE(libvirt_list_domains,                 arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_list_domain_resources,        arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_list_active_domain_ids,       arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_list_active_domains,          arginfo_libvirt_conn)                         \
    PHP_FE(libvirt_list_inactive_domains,        arginfo_libvirt_conn)

extern int le_libvirt_domain;

typedef struct _php_libvirt_domain {
    virDomainPtr domain;
    php_libvirt_connection* conn;
} php_libvirt_domain;

void php_libvirt_domain_dtor(virt_resource *rsrc TSRMLS_DC);

PHP_FUNCTION(libvirt_domain_new);
PHP_FUNCTION(libvirt_domain_new_get_vnc);
PHP_FUNCTION(libvirt_domain_get_counts);
PHP_FUNCTION(libvirt_domain_is_persistent);
PHP_FUNCTION(libvirt_domain_lookup_by_name);
PHP_FUNCTION(libvirt_domain_get_xml_desc);
PHP_FUNCTION(libvirt_domain_get_disk_devices);
PHP_FUNCTION(libvirt_domain_get_interface_devices);
PHP_FUNCTION(libvirt_domain_get_screenshot);
PHP_FUNCTION(libvirt_domain_get_screenshot_api);
PHP_FUNCTION(libvirt_domain_get_screen_dimensions);
PHP_FUNCTION(libvirt_domain_change_vcpus);
PHP_FUNCTION(libvirt_domain_change_memory);
PHP_FUNCTION(libvirt_domain_change_boot_devices);
PHP_FUNCTION(libvirt_domain_disk_add);
PHP_FUNCTION(libvirt_domain_disk_remove);
PHP_FUNCTION(libvirt_domain_nic_add);
PHP_FUNCTION(libvirt_domain_nic_remove);
PHP_FUNCTION(libvirt_domain_attach_device);
PHP_FUNCTION(libvirt_domain_detach_device);
PHP_FUNCTION(libvirt_domain_get_info);
PHP_FUNCTION(libvirt_domain_get_uuid);
PHP_FUNCTION(libvirt_domain_get_uuid_string);
PHP_FUNCTION(libvirt_domain_get_name);
PHP_FUNCTION(libvirt_domain_get_id);
PHP_FUNCTION(libvirt_domain_lookup_by_uuid);
PHP_FUNCTION(libvirt_domain_lookup_by_uuid_string);
PHP_FUNCTION(libvirt_domain_lookup_by_id);
PHP_FUNCTION(libvirt_domain_create);
PHP_FUNCTION(libvirt_domain_destroy);
PHP_FUNCTION(libvirt_domain_resume);
PHP_FUNCTION(libvirt_domain_core_dump);
PHP_FUNCTION(libvirt_domain_shutdown);
PHP_FUNCTION(libvirt_domain_suspend);
PHP_FUNCTION(libvirt_domain_managedsave);
PHP_FUNCTION(libvirt_domain_undefine);
PHP_FUNCTION(libvirt_domain_reboot);
PHP_FUNCTION(libvirt_domain_reset);
PHP_FUNCTION(libvirt_domain_define_xml);
PHP_FUNCTION(libvirt_domain_create_xml);
PHP_FUNCTION(libvirt_domain_xml_from_native);
PHP_FUNCTION(libvirt_domain_xml_to_native);
PHP_FUNCTION(libvirt_domain_set_max_memory);
PHP_FUNCTION(libvirt_domain_set_memory);
PHP_FUNCTION(libvirt_domain_set_memory_flags);
PHP_FUNCTION(libvirt_domain_memory_peek);
PHP_FUNCTION(libvirt_domain_memory_stats);
PHP_FUNCTION(libvirt_domain_block_commit);
PHP_FUNCTION(libvirt_domain_block_stats);
PHP_FUNCTION(libvirt_domain_block_resize);
PHP_FUNCTION(libvirt_domain_block_job_info);
PHP_FUNCTION(libvirt_domain_block_job_abort);
PHP_FUNCTION(libvirt_domain_block_job_set_speed);
PHP_FUNCTION(libvirt_domain_interface_addresses);
PHP_FUNCTION(libvirt_domain_interface_stats);
PHP_FUNCTION(libvirt_domain_get_connect);
PHP_FUNCTION(libvirt_domain_migrate);
PHP_FUNCTION(libvirt_domain_migrate_to_uri);
PHP_FUNCTION(libvirt_domain_migrate_to_uri2);
PHP_FUNCTION(libvirt_domain_get_job_info);
PHP_FUNCTION(libvirt_domain_xml_xpath);
PHP_FUNCTION(libvirt_domain_get_block_info);
PHP_FUNCTION(libvirt_domain_get_network_info);
PHP_FUNCTION(libvirt_domain_get_autostart);
PHP_FUNCTION(libvirt_domain_set_autostart);
PHP_FUNCTION(libvirt_domain_get_metadata);
PHP_FUNCTION(libvirt_domain_set_metadata);
PHP_FUNCTION(libvirt_domain_is_active);
PHP_FUNCTION(libvirt_domain_get_next_dev_ids);
PHP_FUNCTION(libvirt_domain_send_keys);
PHP_FUNCTION(libvirt_domain_send_key_api);
PHP_FUNCTION(libvirt_domain_send_pointer_event);
PHP_FUNCTION(libvirt_domain_update_device);
PHP_FUNCTION(libvirt_domain_qemu_agent_command);
PHP_FUNCTION(libvirt_list_domains);
PHP_FUNCTION(libvirt_list_domain_resources);
PHP_FUNCTION(libvirt_list_active_domain_ids);
PHP_FUNCTION(libvirt_list_active_domains);
PHP_FUNCTION(libvirt_list_inactive_domains);

#endif
