--TEST--
libvirt_domain_nic
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php

$mac = '00:11:22:33:44:55'; // Parms for defining a NIC
$network = 'default';
$model = 'e1000';

echo "# libvirt_connect\n";
var_dump($conn = libvirt_connect('qemu:///session',  false));
if (!is_resource($conn))
    die('Connection to default hypervisor failed. Is libvirt started? What about the QEMU driver?');

// libvirt_domain_nic_add needs a persistent guest loaded from a file
$xml = file_get_contents(__DIR__.'/qemu-no-disk-and-media.xml');
echo "# libvirt_domain_define_xml\n";
var_dump($dom = libvirt_domain_define_xml($conn, $xml));
if (!is_resource($dom)) {
    die('Domain definition failed with error: '.libvirt_get_last_error());
}

echo "# libvirt_domain_nic_add\n";
// The doc is wrong. It says the function returns a domain resource.
// It actually returns a boolean!
var_dump($ret = libvirt_domain_nic_add($dom, $mac, $network, $model, 0));
if (!$ret) {
    die('Domain nic add failed with error: '.libvirt_get_last_error());
}

echo "# libvirt_domain_get_interface_devices\n";
$info = libvirt_domain_get_interface_devices($dom);
var_dump(is_array($info));
if (!array_key_exists('num', $info))
    die('libvirt_domain_get_interface_devices: Num element is missing in the output array');

unset($conn);

?>
Done
--EXPECTF--
# libvirt_connect
resource(%d) of type (Libvirt connection)
# libvirt_domain_define_xml
resource(%d) of type (Libvirt domain)
# libvirt_domain_nic_add
bool(true)
# libvirt_domain_get_interface_devices
bool(true)
Done