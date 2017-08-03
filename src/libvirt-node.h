/*
 * libvirt-node.h: The PHP bindings to libvirt node API
 *
 * See COPYING for the license of this software
 */

#ifndef __LIBVIRT_NODE_H__
# define __LIBVIRT_NODE_H__

# define PHP_FE_LIBVIRT_NODE                                                       \
    PHP_FE(libvirt_node_get_info,                   arginfo_libvirt_conn)          \
    PHP_FE(libvirt_node_get_cpu_stats,              arginfo_libvirt_conn_optcpunr) \
    PHP_FE(libvirt_node_get_cpu_stats_for_each_cpu, arginfo_libvirt_conn_opttime)  \
    PHP_FE(libvirt_node_get_mem_stats,              arginfo_libvirt_conn)          \
    PHP_FE(libvirt_node_get_free_memory,            arginfo_libvirt_conn)

PHP_FUNCTION(libvirt_node_get_info);
PHP_FUNCTION(libvirt_node_get_cpu_stats);
PHP_FUNCTION(libvirt_node_get_cpu_stats_for_each_cpu);
PHP_FUNCTION(libvirt_node_get_mem_stats);
PHP_FUNCTION(libvirt_node_get_free_memory);

#endif
