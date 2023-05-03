/*
 * libvirt-snapshot.c: The PHP bindings to libvirt domain snapshot API
 *
 * See COPYING for the license of this software
 */

#include <config.h>

#include <libvirt/libvirt.h>

#include "libvirt-php.h"
#include "libvirt-snapshot.h"

DEBUG_INIT("snapshot");

int le_libvirt_snapshot;

void php_libvirt_snapshot_dtor(zend_resource *rsrc)
{
    php_libvirt_snapshot *snapshot = (php_libvirt_snapshot *)rsrc->ptr;
    int rv = 0;

    if (snapshot != NULL) {
        if (snapshot->snapshot != NULL) {
            if (!check_resource_allocation(NULL, INT_RESOURCE_SNAPSHOT, snapshot->snapshot)) {
                snapshot->snapshot = NULL;
                efree(snapshot);
                return;
            }
            rv = virDomainSnapshotFree(snapshot->snapshot);
            if (rv != 0) {
                DPRINTF("%s: virDomainSnapshotFree(%p) returned %d\n", __FUNCTION__, snapshot->snapshot, rv);
                php_error_docref(NULL, E_WARNING, "virDomainSnapshotFree failed with %i on destructor: %s", rv, LIBVIRT_G(last_error));
            } else {
                DPRINTF("%s: virDomainSnapshotFree(%p) completed successfully\n", __FUNCTION__, snapshot->snapshot);
                resource_change_counter(INT_RESOURCE_SNAPSHOT, snapshot->domain->conn->conn, snapshot->snapshot, 0);
            }
            snapshot->snapshot = NULL;
        }
        efree(snapshot);
    }
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
    size_t name_len;
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
    resource_change_counter(INT_RESOURCE_SNAPSHOT, domain->conn->conn, res_snapshot->snapshot, 1);

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
    resource_change_counter(INT_RESOURCE_SNAPSHOT, domain->conn->conn, res_snapshot->snapshot, 1);

    VIRT_REGISTER_RESOURCE(res_snapshot, le_libvirt_snapshot);
}

/*
 * Function name:   libvirt_domain_snapshot_create_xml
 * Since version:   0.5.8
 * Description:     This function creates the domain snapshot for the domain identified by it's resource
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @xml [string]: XML description of the snapshot.
 *                  @flags [int]: libvirt snapshot flags
 * Returns:         domain snapshot resource
 */
PHP_FUNCTION(libvirt_domain_snapshot_create_xml)
{
    php_libvirt_domain *domain = NULL;
    php_libvirt_snapshot *res_snapshot;
    zval *zdomain;
    char *xml = NULL;
    size_t xml_len = 0;
    zend_long flags = 0;
    virDomainSnapshotPtr snapshot = NULL;

    GET_DOMAIN_FROM_ARGS("rs|l", &zdomain, &xml, &xml_len, &flags);

    snapshot = virDomainSnapshotCreateXML(domain->domain, xml, flags);
    DPRINTF("%s: virDomainSnapshotCreateXML(%p, %s, %ld) returned %p\n",
            PHPFUNC, domain->domain, xml, flags, snapshot);
    if (snapshot == NULL)
        RETURN_FALSE;

    res_snapshot = (php_libvirt_snapshot *)emalloc(sizeof(php_libvirt_snapshot));
    res_snapshot->domain = domain;
    res_snapshot->snapshot = snapshot;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_snapshot->snapshot);
    resource_change_counter(INT_RESOURCE_SNAPSHOT, domain->conn->conn, res_snapshot->snapshot, 1);

    VIRT_REGISTER_RESOURCE(res_snapshot, le_libvirt_snapshot);
}

/*
 * Function name:   libvirt_domain_snapshot_current
 * Since version:   0.5.6
 * Description:     Function is used to lookup the current snapshot for given domain
 * Arguments:       @res [resource]: libvirt domain resource
 *                  @flags [int]: libvirt snapshot flags
 * Returns:         domain snapshot resource
 */
PHP_FUNCTION(libvirt_domain_snapshot_current)
{
    php_libvirt_domain *domain = NULL;
    zval *zdomain;
    zend_long flags = 0;
    php_libvirt_snapshot *res_snapshot;
    virDomainSnapshotPtr snapshot = NULL;

    GET_DOMAIN_FROM_ARGS("r|l", &zdomain, &flags);

    snapshot = virDomainSnapshotCurrent(domain->domain, flags);
    if (snapshot == NULL)
        RETURN_FALSE;

    res_snapshot = (php_libvirt_snapshot *)emalloc(sizeof(php_libvirt_snapshot));
    res_snapshot->domain = domain;
    res_snapshot->snapshot = snapshot;

    DPRINTF("%s: returning %p\n", PHPFUNC, res_snapshot->snapshot);
    resource_change_counter(INT_RESOURCE_SNAPSHOT, domain->conn->conn, res_snapshot->snapshot, 1);

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
    VIR_FREE(xml);
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
            VIR_FREE(names[i]);
        }
    }
    efree(names);
}
