/*
 * libvirt-node.c: The PHP bindings to libvirt connection API
 *
 * See COPYING for the license of this software
 */

#include <config.h>

#include <libvirt/libvirt.h>

#include "libvirt-php.h"
#include "libvirt-connection.h"
#include "libvirt-node.h"

DEBUG_INIT("node");

/*
 * Function name:   libvirt_node_get_info
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the information about host node, mainly total memory installed, total CPUs installed and model information are useful
 * Arguments:       @conn [resource]: resource for connection
 * Returns:         array of node information or FALSE for error
 */
PHP_FUNCTION(libvirt_node_get_info)
{
    virNodeInfo info;
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int retval;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    retval = virNodeGetInfo   (conn->conn, &info);
    DPRINTF("%s: virNodeGetInfo returned %d\n", PHPFUNC, retval);
    if (retval == -1)
        RETURN_FALSE;

    array_init(return_value);
    VIRT_ADD_ASSOC_STRING(return_value, "model", info.model);
    add_assoc_long(return_value, "memory", (long)info.memory);
    add_assoc_long(return_value, "cpus", (long)info.cpus);
    add_assoc_long(return_value, "nodes", (long)info.nodes);
    add_assoc_long(return_value, "sockets", (long)info.sockets);
    add_assoc_long(return_value, "cores", (long)info.cores);
    add_assoc_long(return_value, "threads", (long)info.threads);
    add_assoc_long(return_value, "mhz", (long)info.mhz);
}

/*
 * Function name:   libvirt_node_get_cpu_stats
 * Since version:   0.4.6
 * Description:     Function is used to get the CPU stats per nodes
 * Arguments:       @conn [resource]: resource for connection
 *                  @cpunr [int]: CPU number to get information about, defaults to VIR_NODE_CPU_STATS_ALL_CPUS to get information about all CPUs
 * Returns:         array of node CPU statistics including time (in seconds since UNIX epoch), cpu number and total number of CPUs on node or FALSE for error
 */
PHP_FUNCTION(libvirt_node_get_cpu_stats)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int cpuNum = VIR_NODE_CPU_STATS_ALL_CPUS;
    virNodeCPUStatsPtr params;
    virNodeInfo info;
    zend_long cpunr = -1;
    int nparams = 0;
    int i, j, numCpus;

    GET_CONNECTION_FROM_ARGS("r|l", &zconn, &cpunr);

    if (virNodeGetInfo(conn->conn, &info) != 0) {
        set_error("Cannot get number of CPUs" TSRMLS_CC);
        RETURN_FALSE;
    }

    numCpus = info.cpus;
    if (cpunr > numCpus - 1) {
        char tmp[256] = { 0 };
        snprintf(tmp, sizeof(tmp), "Invalid CPU number, valid numbers in range 0 to %d or VIR_NODE_CPU_STATS_ALL_CPUS",
                 numCpus - 1);
        set_error(tmp TSRMLS_CC);

        RETURN_FALSE;
    }

    cpuNum = (int)cpunr;

    if (virNodeGetCPUStats(conn->conn, cpuNum, NULL, &nparams, 0) != 0) {
        set_error("Cannot get number of CPU stats" TSRMLS_CC);
        RETURN_FALSE;
    }

    if (nparams == 0)
        RETURN_TRUE;

    DPRINTF("%s: Number of parameters got from virNodeGetCPUStats is %d\n", __FUNCTION__, nparams);

    params = (virNodeCPUStatsPtr)calloc(nparams, nparams * sizeof(*params));

    array_init(return_value);
    for (i = 0; i < 2; i++) {
        zval *arr;

        if (i > 0)
#ifdef EXTWIN
            Sleep(1000);
#else
        sleep(1);
#endif

        if (virNodeGetCPUStats(conn->conn, cpuNum, params, &nparams, 0) != 0) {
            set_error("Unable to get node cpu stats" TSRMLS_CC);
            RETURN_FALSE;
        }

        VIRT_ARRAY_INIT(arr);

        for (j = 0; j < nparams; j++) {
            DPRINTF("%s: Field %s has value of %llu\n", __FUNCTION__, params[j].field, params[j].value);

            add_assoc_long(arr, params[j].field, params[j].value);
        }

        add_assoc_long(arr, "time", time(NULL));

        add_index_zval(return_value, i, arr);
    }

    add_assoc_long(return_value, "cpus", numCpus);
    if (cpuNum >= 0) {
        add_assoc_long(return_value, "cpu", cpunr);
    } else {
        if (cpuNum == VIR_NODE_CPU_STATS_ALL_CPUS)
            VIRT_ADD_ASSOC_STRING(return_value, "cpu", "all");
        else
            VIRT_ADD_ASSOC_STRING(return_value, "cpu", "unknown");
    }

    VIR_FREE(params);
}

/*
 * Function name:   libvirt_node_get_cpu_stats_for_each_cpu
 * Since version:   0.4.6
 * Description:     Function is used to get the CPU stats for each CPU on the host node
 * Arguments:       @conn [resource]: resource for connection
 *                  @time [int]: time in seconds to get the information about, without aggregation for further processing
 * Returns:         array of node CPU statistics for each CPU including time (in seconds since UNIX epoch), cpu number and total number of CPUs on node or FALSE for error
 */
PHP_FUNCTION(libvirt_node_get_cpu_stats_for_each_cpu)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    virNodeCPUStatsPtr params;
    virNodeInfo info;
    int nparams = 0;
    zend_long avg = 0, iter = 0;
    int done = 0;
    int i, j, numCpus;
    time_t startTime = 0;
    zval *time_array;

    GET_CONNECTION_FROM_ARGS("r|l", &zconn, &avg);

    if (virNodeGetInfo(conn->conn, &info) != 0) {
        set_error("Cannot get number of CPUs" TSRMLS_CC);
        RETURN_FALSE;
    }

    if (virNodeGetCPUStats(conn->conn, VIR_NODE_CPU_STATS_ALL_CPUS, NULL, &nparams, 0) != 0) {
        set_error("Cannot get number of CPU stats" TSRMLS_CC);
        RETURN_FALSE;
    }

    if (nparams == 0)
        RETURN_TRUE;

    DPRINTF("%s: Number of parameters got from virNodeGetCPUStats is %d\n", __FUNCTION__, nparams);

    params = (virNodeCPUStatsPtr)calloc(nparams, nparams * sizeof(*params));

    numCpus = info.cpus;
    array_init(return_value);

    startTime = time(NULL);

    iter = 0;
    done = 0;
    while (!done) {
        zval *arr;
        VIRT_ARRAY_INIT(arr);

        for (i = 0; i < numCpus; i++) {
            zval *arr2;

            if (virNodeGetCPUStats(conn->conn, i, params, &nparams, 0) != 0) {
                set_error("Unable to get node cpu stats" TSRMLS_CC);
                RETURN_FALSE;
            }

            VIRT_ARRAY_INIT(arr2);

            for (j = 0; j < nparams; j++)
                add_assoc_long(arr2, params[j].field, params[j].value);

            add_assoc_long(arr, "time", time(NULL));
            add_index_zval(arr, i, arr2);
        }

        add_index_zval(return_value, iter, arr);

        if ((avg <= 0) || (iter == avg - 1)) {
            done = 1;
            break;
        }

#ifndef EXTWIN
        sleep(1);
#else
        Sleep(1000);
#endif
        iter++;
    }

    VIRT_ARRAY_INIT(time_array);
    add_assoc_long(time_array, "start", startTime);
    add_assoc_long(time_array, "finish", time(NULL));
    add_assoc_long(time_array, "duration", time(NULL) - startTime);

    add_assoc_zval(return_value, "times", time_array);

    VIR_FREE(params);
}

/*
 * Function name:   libvirt_node_get_mem_stats
 * Since version:   0.4.6
 * Description:     Function is used to get the memory stats per node
 * Arguments:       @conn [resource]: resource for connection
 * Returns:         array of node memory statistics including time (in seconds since UNIX epoch) or FALSE for error
 */
PHP_FUNCTION(libvirt_node_get_mem_stats)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int memNum = VIR_NODE_MEMORY_STATS_ALL_CELLS;
    virNodeMemoryStatsPtr params;
    int nparams = 0;
    int j;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    if (virNodeGetMemoryStats(conn->conn, memNum, NULL, &nparams, 0) != 0) {
        set_error("Cannot get number of memory stats" TSRMLS_CC);
        RETURN_FALSE;
    }

    if (nparams == 0)
        RETURN_TRUE;

    DPRINTF("%s: Number of parameters got from virNodeGetMemoryStats is %d\n", __FUNCTION__, nparams);

    params = (virNodeMemoryStatsPtr)calloc(nparams, nparams * sizeof(*params));

    array_init(return_value);
    if (virNodeGetMemoryStats(conn->conn, memNum, params, &nparams, 0) != 0) {
        set_error("Unable to get node memory stats" TSRMLS_CC);
        RETURN_FALSE;
    }

    for (j = 0; j < nparams; j++) {
        DPRINTF("%s: Field %s has value of %llu\n", __FUNCTION__, params[j].field, params[j].value);

        add_assoc_long(return_value, params[j].field, params[j].value);
    }

    add_assoc_long(return_value, "time", time(NULL));

    VIR_FREE(params);
}

/*
 * Function name:   libvirt_node_get_free_memory
 * Since version:   0.5.3
 * Description:     Function is used to get free memory available on the node.
 * Arguments:       @conn [resource]: resource for connection.
 * Returns:         The available free memory in bytes as string or FALSE for error.
 */
PHP_FUNCTION(libvirt_node_get_free_memory)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    unsigned long long ret;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    if ((ret = virNodeGetFreeMemory(conn->conn)) != 0) {
        LONGLONG_RETURN_AS_STRING(ret);
    } else {
        set_error("Cannot get the free memory for the node" TSRMLS_CC);
        RETURN_FALSE;
    }
}
