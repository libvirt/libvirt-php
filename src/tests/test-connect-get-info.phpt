--TEST--
libvirt_connect_get_information
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php

echo "# libvirt_connect\n";
var_dump($conn = libvirt_connect('test:///default', false));
if (!is_resource($conn)) {
    die('Connection to default hypervisor failed');
}

echo "# libvirt_node_get_info\n";
$res = libvirt_node_get_info($conn);

if (!is_array($res)) {
    die("libvirt_node_get_info returned unexpected value: " . print_r($res, true) .
	"\nFailed with error: ".libvirt_get_last_error());
}

foreach (array('memory', 'cpus') as $k) {
    if (!array_key_exists($k, $res) || !is_numeric($res[$k])) {
	die("libvirt_node_get_info: Absent or incorrect key \'$k\'. Value: " . print_r($res, true) .
	    "\nFailed with error: ".libvirt_get_last_error());
    }
}

unset($res);

echo "# libvirt_connect_get_uri\n";
var_dump(libvirt_connect_get_uri($conn));

echo "# libvirt_connect_get_hostname\n";
var_dump(libvirt_connect_get_hostname($conn));

echo "# libvirt_domain_get_counts\n";
$counts = libvirt_domain_get_counts($conn);

if (!is_array($counts)) {
    die("libvirt_domain_get_counts returned unexpected value: " . print_r($counts, true) .
	"\nFailed with error: ".libvirt_get_last_error());
}

foreach (array("total", "active", "inactive") as $k) {
    if (!array_key_exists($k, $counts) || !is_numeric($counts[$k])) {
	die("libvirt_domain_get_counts: Absent or incorrect key \'$k\'. Value: " . print_r($counts, true) .
	    "\nFailed with error: ".libvirt_get_last_error());
    }
}

echo "# libvirt_list_active_domains\n";
var_dump(count(libvirt_list_active_domains($conn)) == $counts['active']);

echo "# libvirt_list_inactive_domains\n";
var_dump(count(libvirt_list_inactive_domains($conn)) == $counts['inactive']);

unset($count);

echo "# libvirt_connect_get_hypervisor\n";
$res = libvirt_connect_get_hypervisor($conn);
if (!is_array($res)) {
    die("libvirt_connect_get_hypervisor returned unexpected value: " . print_r($res, true) .
	"\nFailed with error: ".libvirt_get_last_error());
}
unset($res);

echo "# libvirt_connect_get_maxvcpus\n";
var_dump(is_numeric(libvirt_connect_get_maxvcpus($conn)));

echo "# libvirt_connect_get_information\n";
$res = @libvirt_connect_get_information($conn); // Need to suppress driver-dependent warning messages
if (!is_array($res)) {
    die("libvirt_connect_get_information returned unexpected value: " . print_r($res, true) .
	"\nFailed with error: ".libvirt_get_last_error());
}
unset($res);

echo "# libvirt_connect_get_encrypted\n";
var_dump(libvirt_connect_get_encrypted($conn) != -1);

echo "# libvirt_connect_get_secure\n";
var_dump(libvirt_connect_get_secure($conn) != -1);

unset($conn);

?>
Done
--EXPECTF--
# libvirt_connect
resource(%d) of type (Libvirt connection)
# libvirt_node_get_info
# libvirt_connect_get_uri
string(%d) "%s"
# libvirt_connect_get_hostname
string(%d) "%s"
# libvirt_domain_get_counts
# libvirt_list_active_domains
bool(true)
# libvirt_list_inactive_domains
bool(true)
# libvirt_connect_get_hypervisor
# libvirt_connect_get_maxvcpus
bool(true)
# libvirt_connect_get_information
# libvirt_connect_get_encrypted
bool(true)
# libvirt_connect_get_secure
bool(true)
Done
