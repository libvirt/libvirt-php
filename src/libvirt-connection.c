/*
 * libvirt-connection.c: The PHP bindings to libvirt connection API
 *
 * See COPYING for the license of this software
 */

#include <config.h>

#include <libvirt/libvirt.h>

#include "libvirt-php.h"
#include "libvirt-connection.h"
#include <stdbool.h>

DEBUG_INIT("connection");

int le_libvirt_connection;

/*
 * Private function name:   free_resources_on_connection
 * Since version:           0.4.2
 * Description:             Function is used to free all the resources assigned to the connection identified by conn
 * Arguments:               @conn [virConnectPtr]: libvirt connection pointer
 * Returns:                 None
 */
static void
free_resources_on_connection(virConnectPtr conn)
{
    int binding_resources_count = 0;
    resource_info *binding_resources;
    int i;

    binding_resources_count = LIBVIRT_G(binding_resources_count);
    binding_resources = LIBVIRT_G(binding_resources);

    for (i = 0; i < binding_resources_count; i++) {
        if ((binding_resources[i].overwrite == 0) && (binding_resources[i].conn == conn))
            free_resource(binding_resources[i].type, binding_resources[i].mem);
    }
}

/* Destructor for connection resource */
void
php_libvirt_connection_dtor(zend_resource *rsrc)
{
    php_libvirt_connection *conn = (php_libvirt_connection *) rsrc->ptr;
    int rv = 0;

    if (conn != NULL) {
        if (conn->conn != NULL) {
            free_resources_on_connection(conn->conn);

            rv = virConnectClose(conn->conn);
            if (rv == -1) {
                DPRINTF("%s: virConnectClose(%p) returned %d (%s)\n", __FUNCTION__, conn->conn, rv, LIBVIRT_G(last_error));
                php_error_docref(NULL, E_WARNING, "virConnectClose failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
            } else {
                DPRINTF("%s: virConnectClose(%p) completed successfully\n", __FUNCTION__, conn->conn);
                resource_change_counter(INT_RESOURCE_CONNECTION, conn->conn, conn->conn, 0);
            }
            conn->conn = NULL;
        }
        efree(conn);
    }
}

/* Authentication callback function.
 *
 * Should receive list of credentials via cbdata and pass the requested one to
 * libvirt
 */
static int libvirt_virConnectAuthCallback(virConnectCredentialPtr cred,
                                          unsigned int ncred, void *cbdata)
{
    unsigned int i, j;
    php_libvirt_cred_value *creds = (php_libvirt_cred_value *) cbdata;
    for (i = 0; i < (unsigned int)ncred; i++) {
        DPRINTF("%s: cred %d, type %d, prompt %s challenge %s\n ", __FUNCTION__, i, cred[i].type, cred[i].prompt, cred[i].challenge);
        if (creds != NULL)
            for (j = 0; j < (unsigned int)creds[0].count; j++) {
                if (creds[j].type == cred[i].type) {
                    cred[i].resultlen = creds[j].resultlen;
                    cred[i].result = (char *)malloc(creds[j].resultlen + 1);
                    memset(cred[i].result, 0, creds[j].resultlen + 1);
                    strncpy(cred[i].result, creds[j].result, creds[j].resultlen);
                }
            }
        DPRINTF("%s: result %s (%d)\n", __FUNCTION__, cred[i].result, cred[i].resultlen);
    }

    return 0;
}

static int libvirt_virConnectCredType[] = {
    VIR_CRED_AUTHNAME,
    VIR_CRED_ECHOPROMPT,
    VIR_CRED_REALM,
    VIR_CRED_PASSPHRASE,
    VIR_CRED_NOECHOPROMPT,
    //VIR_CRED_EXTERNAL,
};

/*
 * Function name:   libvirt_connect
 * Since version:   0.4.1(-1)
 * Description:     libvirt_connect() is used to connect to the specified libvirt daemon using the specified URL, user can also set the readonly flag and/or set credentials for connection
 * Arguments:       @url [string]: URI for connection
 *                  @readonly [bool]: flag whether to use read-only connection or not
 *                  @credentials [array]: array of connection credentials
 * Returns:         libvirt connection resource
 */
PHP_FUNCTION(libvirt_connect)
{
    php_libvirt_connection *conn;
    php_libvirt_cred_value *creds = NULL;
    zval *zcreds = NULL;
    zval *data;
    int i;
    int j;
    int credscount = 0;

    virConnectAuth libvirt_virConnectAuth = {
        libvirt_virConnectCredType,
        sizeof(libvirt_virConnectCredType) / sizeof(int),
        libvirt_virConnectAuthCallback,
        NULL
    };

    char *url = NULL;
    size_t url_len = 0;
    bool readonly = 1;

    HashTable *arr_hash;
    HashPosition pointer;
    int array_count;

    unsigned long libVer;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|sba", &url, &url_len, &readonly, &zcreds) == FAILURE) {
        RETURN_FALSE;
    }

    if (virGetVersion(&libVer, NULL, NULL)!= 0)
        RETURN_FALSE;

    if (libVer < 6002) {
        set_error("Only libvirt 0.6.2 and higher supported. Please upgrade your libvirt");
        RETURN_FALSE;
    }

    if ((count_resources(INT_RESOURCE_CONNECTION) + 1) > LIBVIRT_G(max_connections_ini)) {
        DPRINTF("%s: maximum number of connections allowed exceeded (max %lu)\n", PHPFUNC, (unsigned long)LIBVIRT_G(max_connections_ini));
        set_error("Maximum number of connections allowed exceeded");
        RETURN_FALSE;
    }

    /* If 'null' value has been passed as URL override url to NULL value to autodetect the hypervisor */
    if ((url == NULL) || (strcasecmp(url, "NULL") == 0))
        url = NULL;

    conn = (php_libvirt_connection *)emalloc(sizeof(php_libvirt_connection));
    if (zcreds == NULL) {
        /* connecting without providing authentication */
        if (readonly)
            conn->conn = virConnectOpenReadOnly(url);
        else
            conn->conn = virConnectOpen(url);
    } else {
        /* connecting with authentication (using callback) */
        arr_hash = Z_ARRVAL_P(zcreds);
        array_count = zend_hash_num_elements(arr_hash);

        credscount = array_count;
        creds = (php_libvirt_cred_value *)emalloc(credscount * sizeof(php_libvirt_cred_value));
        j = 0;
        /* parse the input Array and create list of credentials. The list (array) is passed to callback function. */
        VIRT_FOREACH(arr_hash, pointer, data) {
            if (Z_TYPE_P(data) == IS_STRING) {
                php_libvirt_hash_key_info info;
                VIRT_HASH_CURRENT_KEY_INFO(arr_hash, pointer, info);

                if (info.type == HASH_KEY_IS_STRING) {
                    PHPWRITE(info.name, info.length);
                } else {
                    DPRINTF("%s: credentials index %d\n", PHPFUNC, info.index);
                    creds[j].type = info.index;
                    creds[j].result = (char *)emalloc(Z_STRLEN_P(data) + 1);
                    memset(creds[j].result, 0, Z_STRLEN_P(data) + 1);
                    creds[j].resultlen = Z_STRLEN_P(data);
                    strncpy(creds[j].result, Z_STRVAL_P(data), Z_STRLEN_P(data));
                    j++;
                }
            }
        } VIRT_FOREACH_END();
        DPRINTF("%s: Found %d elements for credentials\n", PHPFUNC, j);
        creds[0].count = j;
        libvirt_virConnectAuth.cbdata = (void *)creds;
        conn->conn = virConnectOpenAuth(url, &libvirt_virConnectAuth, readonly ? VIR_CONNECT_RO : 0);
        for (i = 0; i < creds[0].count; i++)
            efree(creds[i].result);
        efree(creds);
    }

    if (conn->conn == NULL) {
        DPRINTF("%s: Cannot establish connection to %s\n", PHPFUNC, url);
        efree(conn);
        RETURN_FALSE;
    }

    resource_change_counter(INT_RESOURCE_CONNECTION, conn->conn, conn->conn, 1);
    DPRINTF("%s: Connection to %s established, returning %p\n", PHPFUNC, url, conn->conn);

    VIRT_REGISTER_RESOURCE(conn, le_libvirt_connection);
    conn->resource = VIRT_RESOURCE_HANDLE(return_value);
}

/*
 * Function name:   libvirt_connect_get_uri
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the connection URI. This is useful to check the hypervisor type of host machine when using "null" uri to libvirt_connect()
 * Arguments:       @conn [resource]: resource for connection
 * Returns:         connection URI string or FALSE for error
 */
PHP_FUNCTION(libvirt_connect_get_uri)
{
    zval *zconn;
    char *uri;
    php_libvirt_connection *conn = NULL;

    GET_CONNECTION_FROM_ARGS("r", &zconn);
    uri = virConnectGetURI(conn->conn);
    DPRINTF("%s: virConnectGetURI returned %s\n", PHPFUNC, uri);
    if (uri == NULL)
        RETURN_FALSE;

    VIRT_RETVAL_STRING(uri);
    VIR_FREE(uri);
}

/*
 * Function name:   libvirt_connect_get_hostname
 * Since version:   0.4.1(-1)
 * Description:     Function is used to get the hostname of the guest associated with the connection
 * Arguments:       @conn [resource]: resource for connection
 * Returns:         hostname of the host node or FALSE for error
 */
PHP_FUNCTION(libvirt_connect_get_hostname)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    char *hostname;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    hostname = virConnectGetHostname(conn->conn);
    DPRINTF("%s: virConnectGetHostname returned %s\n", PHPFUNC, hostname);
    if (hostname == NULL)
        RETURN_FALSE;

    VIRT_RETVAL_STRING(hostname);
    VIR_FREE(hostname);
}

/*
 * Function name:   libvirt_connect_get_hypervisor
 * Since version:   0.4.1(-2)
 * Description:     Function is used to get the information about the hypervisor on the connection identified by the connection pointer
 * Arguments:       @conn [resource]: resource for connection
 * Returns:         array of hypervisor information if available
 */
PHP_FUNCTION(libvirt_connect_get_hypervisor)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    unsigned long hvVer = 0;
    const char *type = NULL;
    char hvStr[64] = { 0 };

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    if (virConnectGetVersion(conn->conn, &hvVer) != 0)
        RETURN_FALSE;

    type = virConnectGetType(conn->conn);
    if (type == NULL)
        RETURN_FALSE;

    DPRINTF("%s: virConnectGetType returned %s\n", PHPFUNC, type);

    array_init(return_value);
    VIRT_ADD_ASSOC_STRING(return_value, "hypervisor", (char *)type);
    add_assoc_long(return_value, "major", (long)((hvVer/1000000) % 1000));
    add_assoc_long(return_value, "minor", (long)((hvVer/1000) % 1000));
    add_assoc_long(return_value, "release", (long)(hvVer % 1000));

    snprintf(hvStr, sizeof(hvStr), "%s %ld.%ld.%ld", type,
             (long)((hvVer/1000000) % 1000), (long)((hvVer/1000) % 1000), (long)(hvVer % 1000));
    VIRT_ADD_ASSOC_STRING(return_value, "hypervisor_string", hvStr);
}

/*
 * Function name:   libvirt_connect_get_capabilities
 * Since version:   0.4.1(-2)
 * Description:     Function is used to get the capabilities information from the connection
 * Arguments:       @conn [resource]: resource for connection
 *                  @xpath [string]: optional xPath query to be applied on the result
 * Returns:         capabilities XML from the connection or FALSE for error
 */
PHP_FUNCTION(libvirt_connect_get_capabilities)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    char *caps;
    char *xpath = NULL;
    size_t xpath_len;
    char *tmp = NULL;
    int retval = -1;

    GET_CONNECTION_FROM_ARGS("r|s", &zconn, &xpath, &xpath_len);

    caps = virConnectGetCapabilities(conn->conn);
    if (caps == NULL)
        RETURN_FALSE;

    tmp = get_string_from_xpath(caps, xpath, NULL, &retval);
    if ((tmp == NULL) || (retval < 0)) {
        VIRT_RETVAL_STRING(caps);
    } else {
        VIRT_RETVAL_STRING(tmp);
    }

    VIR_FREE(caps);
    VIR_FREE(tmp);
}

/*
 * Function name:   libvirt_connect_get_emulator
 * Since version:   0.4.5
 * Description:     Function is used to get the emulator for requested connection/architecture
 * Arguments:       @conn [resource]: libvirt connection resource
 *                  @arch [string]: optional architecture string, can be NULL to get default
 * Returns:         path to the emulator
 */
PHP_FUNCTION(libvirt_connect_get_emulator)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    char *arch = NULL;
    size_t arch_len;
    char *tmp;

    GET_CONNECTION_FROM_ARGS("r|s", &zconn, &arch, &arch_len);

    if ((arch == NULL) || (arch_len == 0))
        arch = NULL;

    tmp = connection_get_emulator(conn->conn, arch);
    if (tmp == NULL) {
        set_error("Cannot get emulator");
        RETURN_FALSE;
    }

    VIRT_RETVAL_STRING(tmp);
    VIR_FREE(tmp);
}

/*
 * Function name:   libvirt_connect_get_nic_models
 * Since version:   0.4.9
 * Description:     Function is used to get NIC models for requested connection/architecture
 * Arguments:       @conn [resource]: libvirt connection resource
 *                  @arch [string]: optional architecture string, can be NULL to get default
 * Returns:         array of models
 */
PHP_FUNCTION(libvirt_connect_get_nic_models)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    char *arch = NULL;
    size_t arch_len;
    char cmd[1024] = { 0 };
    char *reply = NULL;
    char *tmp = NULL;

    GET_CONNECTION_FROM_ARGS("r|s", &zconn, &arch, &arch_len);

    /* Disable getting it on remote connections */
    if (!is_local_connection(conn->conn)) {
        set_error("This function works only on local connections");
        RETURN_FALSE;
    }

    /* This approach is working only for QEMU driver so bails if not currently using it */
    if (strcmp(virConnectGetType(conn->conn), "QEMU") != 0) {
        set_error("This function can be used only for QEMU driver");
        RETURN_FALSE;
    }

#ifndef EXTWIN
    if ((arch == NULL) || (arch_len == 0))
        arch = NULL;

    tmp = connection_get_emulator(conn->conn, arch);
    if (tmp == NULL) {
        set_error("Cannot get emulator");
        RETURN_FALSE;
    }

    snprintf(cmd, sizeof(cmd), "%s -net nic,model=?", tmp);
    VIR_FREE(tmp);

    if (runCommand(cmd, &reply) < 0)
        RETURN_FALSE;

# define NEEDLE "Supported NIC models:\n"
    array_init(return_value);
    if ((tmp = strstr(reply, NEEDLE))) {
        size_t i;
        char num[16] = { 0 };
        tTokenizer t = tokenize(tmp + strlen(NEEDLE), "\n");

        for (i = 0; i < t.numTokens; i++) {
            snprintf(num, sizeof(num), "%zu", i);
            VIRT_ADD_ASSOC_STRING(return_value, num, t.tokens[i]);
        }
        free_tokens(t);
    }
    VIR_FREE(reply);
# undef NEEDLE
#endif
}

/*
 * Function name:   libvirt_connect_get_soundhw_models
 * Since version:   0.4.9
 * Description:     Function is used to get sound hardware models for requested connection/architecture
 * Arguments:       @conn [resource]: libvirt connection resource
 *                  @arch [string]: optional architecture string, can be NULL to get default
 *                  @flags [int]: flags for getting sound hardware. Can be either 0 or VIR_CONNECT_SOUNDHW_GET_NAMES
 * Returns:         array of models
 */
PHP_FUNCTION(libvirt_connect_get_soundhw_models)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    char *arch = NULL;
    size_t arch_len;
    char *tmp;
    zend_long flags = 0;

    GET_CONNECTION_FROM_ARGS("r|sl", &zconn, &arch, &arch_len, &flags);

    if ((arch == NULL) || (arch_len == 0))
        arch = NULL;

    /* Disable getting it on remote connections */
    if (!is_local_connection(conn->conn)) {
        set_error("This function works only on local connections");
        RETURN_FALSE;
    }

#ifndef EXTWIN
    /* This approach is working only for QEMU driver so bails if not currently using it */
    if (strcmp(virConnectGetType(conn->conn), "QEMU") != 0) {
        set_error("This function can be used only for QEMU driver");
        RETURN_FALSE;
    }

    tmp = connection_get_emulator(conn->conn, arch);
    if (tmp == NULL) {
        set_error("Cannot get emulator");
        RETURN_FALSE;
    }

    char cmd[4096] = { 0 };
    snprintf(cmd, sizeof(cmd), "%s -soundhw help 2>&1", tmp);
    VIR_FREE(tmp);

    FILE *fp = popen(cmd, "r");
    if (fp == NULL)
        RETURN_FALSE;

    short inFunc = 0;

    int n = 0;
    array_init(return_value);
    while (!feof(fp)) {
        memset(cmd, 0, sizeof(cmd));
        if (!fgets(cmd, sizeof(cmd), fp))
            break;

        if (strncmp(cmd, "Valid ", 6) == 0) {
            inFunc = 1;
            continue;
        } else
            if (strlen(cmd) < 2)
                inFunc = 0;

        if (inFunc) {
            int i = 0;
            char desc[1024] = { 0 };
            tTokenizer t = tokenize(cmd, " ");
            if (t.numTokens == 0)
                continue;

            if ((i > 0) && (flags & CONNECT_FLAG_SOUNDHW_GET_NAMES)) {
                zval *arr;
                memset(desc, 0, sizeof(desc));
                for (i = 1; i < t.numTokens; i++) {
                    strcat(desc, t.tokens[i]);
                    if (i < t.numTokens - 1)
                        strcat(desc, " ");
                }

                VIRT_ARRAY_INIT(arr);
                VIRT_ADD_ASSOC_STRING(arr, "name", t.tokens[0]);
                VIRT_ADD_ASSOC_STRING(arr, "description", desc);
                add_next_index_zval(return_value, arr);
            } else {
                char tmp2[16] = { 0 };
                snprintf(tmp2, sizeof(tmp2), "%d", n++);
                VIRT_ADD_ASSOC_STRING(return_value, tmp2, t.tokens[0]);
            }

            free_tokens(t);
        }
    }
    fclose(fp);
#endif
}

/*
 * Function name:   libvirt_connect_get_maxvcpus
 * Since version:   0.4.1(-2)
 * Description:     Function is used to get maximum number of VCPUs per VM on the hypervisor connection
 * Arguments:       @conn [resource]: resource for connection
 * Returns:         number of VCPUs available per VM on the connection or FALSE for error
 */
PHP_FUNCTION(libvirt_connect_get_maxvcpus)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    const char *type = NULL;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    type = virConnectGetType(conn->conn);
    if (type == NULL)
        RETURN_FALSE;

    RETURN_LONG(virConnectGetMaxVcpus(conn->conn, type));
}

/*
 * Function name:   libvirt_connect_get_sysinfo
 * Since version:   0.4.1(-2)
 * Description:     Function is used to get the system information from connection if available
 * Arguments:       @conn [resource]: resource for connection
 * Returns:         XML description of system information from the connection or FALSE for error
 */
PHP_FUNCTION(libvirt_connect_get_sysinfo)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    char *sysinfo;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    sysinfo = virConnectGetSysinfo(conn->conn, 0);
    if (sysinfo == NULL)
        RETURN_FALSE;

    VIRT_RETVAL_STRING(sysinfo);
    VIR_FREE(sysinfo);
}

/*
 * Function name:   libvirt_connect_get_encrypted
 * Since version:   0.4.1(-2)
 * Description:     Function is used to get the information whether the connection is encrypted or not
 * Arguments:       @conn [resource]: resource for connection
 * Returns:         1 if encrypted, 0 if not encrypted, -1 on error
 */
PHP_FUNCTION(libvirt_connect_get_encrypted)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    RETURN_LONG(virConnectIsEncrypted(conn->conn));
}

/*
 * Function name:   libvirt_connect_get_secure
 * Since version:   0.4.1(-2)
 * Description:     Function is used to get the information whether the connection is secure or not
 * Arguments:       @conn [resource]: resource for connection
 * Returns:         1 if secure, 0 if not secure, -1 on error
 */
PHP_FUNCTION(libvirt_connect_get_secure)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    RETURN_LONG(virConnectIsSecure(conn->conn));
}

/*
 * Function name:   libvirt_connect_get_information
 * Since version:   0.4.1(-2)
 * Description:     Function is used to get the information about the connection
 * Arguments:       @conn [resource]: resource for connection
 * Returns:         array of information about the connection
 */
PHP_FUNCTION(libvirt_connect_get_information)
{
    zval *zconn;
    char *tmp;
    unsigned long hvVer = 0;
    const char *type = NULL;
    char hvStr[64] = { 0 };
    int iTmp = -1, maxvcpus = -1;
    php_libvirt_connection *conn = NULL;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    tmp = virConnectGetURI(conn->conn);
    DPRINTF("%s: Got connection URI of %s...\n", PHPFUNC, tmp);
    array_init(return_value);
    VIRT_ADD_ASSOC_STRING(return_value, "uri", tmp ? tmp : "unknown");
    VIR_FREE(tmp);
    tmp = virConnectGetHostname(conn->conn);
    VIRT_ADD_ASSOC_STRING(return_value, "hostname", tmp ? tmp : "unknown");
    VIR_FREE(tmp);

    if ((virConnectGetVersion(conn->conn, &hvVer) == 0) && (type = virConnectGetType(conn->conn))) {
        VIRT_ADD_ASSOC_STRING(return_value, "hypervisor", (char *)type);
        add_assoc_long(return_value, "hypervisor_major", (long)((hvVer/1000000) % 1000));
        add_assoc_long(return_value, "hypervisor_minor", (long)((hvVer/1000) % 1000));
        add_assoc_long(return_value, "hypervisor_release", (long)(hvVer % 1000));
        snprintf(hvStr, sizeof(hvStr), "%s %ld.%ld.%ld", type,
                 (long)((hvVer/1000000) % 1000), (long)((hvVer/1000) % 1000), (long)(hvVer % 1000));
        VIRT_ADD_ASSOC_STRING(return_value, "hypervisor_string", hvStr);
    }

    if (strcmp(type, "QEMU") == 0) {
        /* For QEMU the value is not reliable so we return -1 instead */
        maxvcpus = -1;
    } else {
        maxvcpus = virConnectGetMaxVcpus(conn->conn, type);
    }

    add_assoc_long(return_value, "hypervisor_maxvcpus", maxvcpus);
    iTmp = virConnectIsEncrypted(conn->conn);
    if (iTmp == 1)
        VIRT_ADD_ASSOC_STRING(return_value, "encrypted", "Yes");
    else
        if (iTmp == 0)
            VIRT_ADD_ASSOC_STRING(return_value, "encrypted", "No");
        else
            VIRT_ADD_ASSOC_STRING(return_value, "encrypted", "unknown");

    iTmp = virConnectIsSecure(conn->conn);
    if (iTmp == 1)
        VIRT_ADD_ASSOC_STRING(return_value, "secure", "Yes");
    else
        if (iTmp == 0)
            VIRT_ADD_ASSOC_STRING(return_value, "secure", "No");
        else
            VIRT_ADD_ASSOC_STRING(return_value, "secure", "unknown");

    add_assoc_long(return_value, "num_inactive_domains", virConnectNumOfDefinedDomains(conn->conn));
    add_assoc_long(return_value, "num_inactive_interfaces", virConnectNumOfDefinedInterfaces(conn->conn));
    add_assoc_long(return_value, "num_inactive_networks", virConnectNumOfDefinedNetworks(conn->conn));
    add_assoc_long(return_value, "num_inactive_storagepools", virConnectNumOfDefinedStoragePools(conn->conn));

    add_assoc_long(return_value, "num_active_domains", virConnectNumOfDomains(conn->conn));
    add_assoc_long(return_value, "num_active_interfaces", virConnectNumOfInterfaces(conn->conn));
    add_assoc_long(return_value, "num_active_networks", virConnectNumOfNetworks(conn->conn));
    add_assoc_long(return_value, "num_active_storagepools", virConnectNumOfStoragePools(conn->conn));

    add_assoc_long(return_value, "num_total_domains", virConnectNumOfDomains(conn->conn) + virConnectNumOfDefinedDomains(conn->conn));
    add_assoc_long(return_value, "num_total_interfaces", virConnectNumOfInterfaces(conn->conn) + virConnectNumOfDefinedInterfaces(conn->conn));
    add_assoc_long(return_value, "num_total_networks", virConnectNumOfNetworks(conn->conn) + virConnectNumOfDefinedNetworks(conn->conn));
    add_assoc_long(return_value, "num_total_storagepools", virConnectNumOfStoragePools(conn->conn) +  virConnectNumOfDefinedStoragePools(conn->conn));

    add_assoc_long(return_value, "num_secrets", virConnectNumOfSecrets(conn->conn));
    add_assoc_long(return_value, "num_nwfilters", virConnectNumOfNWFilters(conn->conn));
}

/*
 * Function name:   libvirt_connect_get_machine_types
 * Since version:   0.4.9
 * Description:     Function is used to get machine types supported by hypervisor on the connection
 * Arguments:       @conn [resource]: resource for connection
 * Returns:         array of machine types for the connection incl. maxCpus if appropriate
 */
PHP_FUNCTION(libvirt_connect_get_machine_types)
{
    zval *zconn;
    php_libvirt_connection *conn = NULL;
    char *caps = NULL;
    char **ret = NULL;
    int i, num = -1;

    GET_CONNECTION_FROM_ARGS("r", &zconn);

    caps = virConnectGetCapabilities(conn->conn);
    if (caps == NULL)
        RETURN_FALSE;

    array_init(return_value);

    ret = get_array_from_xpath(caps, "//capabilities/guest/arch/@name", &num);
    if (ret != NULL) {
        for (i = 0; i < num; i++) {
            int num2, j;
            char tmp[1024] = { 0 };

            snprintf(tmp, sizeof(tmp), "//capabilities/guest/arch[@name=\"%s\"]/domain/@type", ret[i]);
            char **ret2 = get_array_from_xpath(caps, tmp, &num2);
            if (ret2 != NULL) {
                zval *arr2;
                VIRT_ARRAY_INIT(arr2);

                for (j = 0; j < num2; j++) {
                    int num3, k;
                    char tmp2[1024] = { 0 };
                    zval *arr3;

                    VIRT_ARRAY_INIT(arr3);

                    snprintf(tmp2, sizeof(tmp2), "//capabilities/guest/arch[@name=\"%s\"]/machine",
                             ret[i]);

                    char **ret3 = get_array_from_xpath(caps, tmp2, &num3);
                    if (ret3 != NULL) {
                        for (k = 0; k < num3; k++) {
                            char *numTmp = NULL;
                            char key[8] = { 0 };
                            char tmp3[2048] = { 0 };

                            snprintf(key, sizeof(key), "%d", k);
                            //VIRT_ADD_ASSOC_STRING(arr2, key, ret3[k]);

                            snprintf(tmp3, sizeof(tmp3), "//capabilities/guest/arch[@name=\"%s\"]/machine[text()=\"%s\"]/@maxCpus",
                                     ret[i], ret3[k]);

                            numTmp = get_string_from_xpath(caps, tmp3, NULL, NULL);
                            if (numTmp == NULL) {
                                VIRT_ADD_ASSOC_STRING(arr2, key, ret3[k]);
                            } else {
                                zval *arr4;
                                VIRT_ARRAY_INIT(arr4);
                                VIRT_ADD_ASSOC_STRING(arr4, "name", ret3[k]);
                                VIRT_ADD_ASSOC_STRING(arr4, "maxCpus", numTmp);

                                VIRT_ADD_ASSOC_ZVAL_EX(arr2, key, arr4);
                                VIR_FREE(numTmp);
                            }

                            VIR_FREE(ret3[k]);
                        }
                    }

                    /* Domain type specific */
                    snprintf(tmp2, sizeof(tmp2), "//capabilities/guest/arch[@name=\"%s\"]/domain[@type=\"%s\"]/machine",
                             ret[i], ret2[j]);

                    ret3 = get_array_from_xpath(caps, tmp2, &num3);
                    if (ret3 != NULL) {
                        for (k = 0; k < num3; k++) {
                            char key[8] = { 0 };
                            char tmp3[2048] = { 0 };
                            char *numTmp = NULL;

                            snprintf(key, sizeof(key), "%d", k);
                            snprintf(tmp3, sizeof(tmp3),
                                     "//capabilities/guest/arch[@name=\"%s\"]/domain[@type=\"%s\"]/machine[text()=\"%s\"]/@maxCpus",
                                     ret[i], ret2[j], ret3[k]);

                            numTmp = get_string_from_xpath(caps, tmp3, NULL, NULL);
                            if (numTmp == NULL) {
                                VIRT_ADD_ASSOC_STRING(arr3, key, ret3[k]);
                            } else {
                                zval *arr4;
                                VIRT_ARRAY_INIT(arr4);

                                VIRT_ADD_ASSOC_STRING(arr4, "name", ret3[k]);
                                VIRT_ADD_ASSOC_STRING(arr4, "maxCpus", numTmp);

                                VIRT_ADD_ASSOC_ZVAL_EX(arr3, key, arr4);
                                VIR_FREE(numTmp);
                            }

                            VIR_FREE(ret3[k]);
                        }

                        VIRT_ADD_ASSOC_ZVAL_EX(arr2, ret2[j], arr3);
                    }
                }
                //free(ret2[j]);

                VIRT_ADD_ASSOC_ZVAL_EX(return_value, ret[i], arr2);
            }
            VIR_FREE(ret[i]);
        }
    }
}

/*
 * Function name:   libvirt_connect_get_all_domain_stats
 * Since version:   0.5.1(-1)
 * Description:     Query statistics for all domains on a given connection
 * Arguments:       @conn [resource]: resource for connection
 *                  @stats [int]: the statistic groups from VIR_DOMAIN_STATS_*
 *                  @flags [int]: the filter flags from VIR_CONNECT_GET_ALL_DOMAINS_STATS_*
 * Returns:         assoc array with statistics or false on error
 */
PHP_FUNCTION(libvirt_connect_get_all_domain_stats)
{
    php_libvirt_connection *conn = NULL;
    zval *zconn;
    int retval = -1;
    zend_long flags = 0;
    zend_long stats = 0;
    const char *name = NULL;
    int i;
    int j;
    virTypedParameter params;
    virDomainStatsRecordPtr *retstats = NULL;

    GET_CONNECTION_FROM_ARGS("r|ll", &zconn, &stats, &flags);

    retval = virConnectGetAllDomainStats(conn->conn, stats, &retstats, flags);

    array_init(return_value);
    if (retval < 0)
        RETURN_FALSE;

    for (i = 0; i < retval; i++) {
        zval *arr2;
        VIRT_ARRAY_INIT(arr2);

        for (j = 0; j < retstats[i]->nparams; j++) {
            params = retstats[i]->params[j];
            switch (params.type) {
            case VIR_TYPED_PARAM_INT:
                add_assoc_long(arr2, params.field, params.value.i);
                break;
            case VIR_TYPED_PARAM_UINT:
                add_assoc_long(arr2, params.field, params.value.ui);
                break;
            case VIR_TYPED_PARAM_LLONG:
                add_assoc_long(arr2, params.field, params.value.l);
                break;
            case VIR_TYPED_PARAM_ULLONG:
                add_assoc_long(arr2, params.field, params.value.ul);
                break;
            case VIR_TYPED_PARAM_DOUBLE:
                add_assoc_double(arr2, params.field, params.value.d);
                break;
            case VIR_TYPED_PARAM_BOOLEAN:
                add_assoc_bool(arr2, params.field, params.value.b);
                break;
            case VIR_TYPED_PARAM_STRING:
                VIRT_ADD_ASSOC_STRING(arr2, params.field, params.value.s);
                break;
            }
        }
        name = virDomainGetName(retstats[i]->dom);
        zend_hash_update(Z_ARRVAL_P(return_value), zend_string_init(name, strlen(name), 0), arr2);
    }

    virDomainStatsRecordListFree(retstats);
}
