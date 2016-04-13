--TEST--
libvirt_domain_get_xml
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php

echo "# libvirt_connect\n";
var_dump($conn = libvirt_connect('test:///default',  false));
if (!is_resource($conn))
    die('Connection to default hypervisor failed');

// We need a running domain to be able to pull the id
$xml = file_get_contents(__DIR__.'/example-no-disk-and-media.xml');
echo "# libvirt_domain_create_xml\n";
var_dump($dom = libvirt_domain_create_xml($conn, $xml));
if (!is_resource($dom)) {
    die('Domain definition failed with error: '.libvirt_get_last_error());
}

echo "# libvirt_domain_get_name\n";
var_dump($dom_name = libvirt_domain_get_name($dom));

echo "# libvirt_domain_get_uuid_string\n";
var_dump($dom_uuid_str = libvirt_domain_get_uuid_string($dom));

echo "# libvirt_domain_get_uuid\n";
var_dump($dom_uuid = libvirt_domain_get_uuid($dom));

echo "# libvirt_domain_get_id\n";
var_dump($dom_id = libvirt_domain_get_id($dom));

echo "# libvirt_domain_lookup_by_uuid_string\n";
var_dump($dom2 = libvirt_domain_lookup_by_uuid_string($conn, $dom_uuid_str));
if (!is_resource($dom2)) {
    die('Domain lookup by UUID string failed with error: '.libvirt_get_last_error());
}
unset($dom2);

echo "# libvirt_domain_lookup_by_id\n";
var_dump($dom2 = libvirt_domain_lookup_by_id($conn, $dom_id));
if (!is_resource($dom2)) {
    die('Domain lookup by ID failed with error: '.libvirt_get_last_error());
}
unset($dom2);

echo "# libvirt_domain_is_persistent\n";
var_dump(libvirt_domain_is_persistent($dom) !== -1);

echo "# libvirt_domain_get_xml_desc\n";
$xmlstr = libvirt_domain_get_xml_desc($dom, NULL);

$xmlok = (strpos($xmlstr, "<name>{$dom_name}</name>") &&
          strpos($xmlstr, "<uuid>{$dom_uuid_str}</uuid>"));
var_dump($xmlok);

echo "# libvirt_domain_shutdown\n";
var_dump($ret = libvirt_domain_shutdown($dom));
if (!$ret) {
    die('Domain shutdown failed with error: '.libvirt_get_last_error());
}

unset($res);
unset($dom);
unset($conn);
?>
Done
--EXPECTF--
# libvirt_connect
resource(%d) of type (Libvirt connection)
# libvirt_domain_create_xml
resource(%d) of type (Libvirt domain)
# libvirt_domain_get_name
string(%d) "test-guest-no-disk-and-media"
# libvirt_domain_get_uuid_string
string(36) "%s"
# libvirt_domain_get_uuid
string(%d) "%s"
# libvirt_domain_get_id
int(%d)
# libvirt_domain_lookup_by_uuid_string
resource(%d) of type (Libvirt domain)
# libvirt_domain_lookup_by_id
resource(%d) of type (Libvirt domain)
# libvirt_domain_is_persistent
bool(true)
# libvirt_domain_get_xml_desc
bool(true)
# libvirt_domain_shutdown
bool(true)
Done
