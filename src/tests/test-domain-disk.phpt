--TEST--
libvirt_domain_disk
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php

$disk_devname = 'hda'; // Disk that will be added to existing guest config
$disk_devtype = 'ide';
$disk_iodriver = 'raw';
$disk_image = '/tmp/test-libvirt-php.img';
touch($disk_image);

$logfile = __DIR__ . '/test.log';
@unlink($logfile);
echo "# libvirt_logfile_set\n";
if (!libvirt_logfile_set($logfile, 100)) { // Log file will be 100 KB max
    die('Logfile set failed with error: '.libvirt_get_last_error());
}

echo "# libvirt_connect\n";
var_dump($conn = libvirt_connect('qemu:///session',  false));
if (!is_resource($conn))
    die('Connection to default hypervisor failed. Is libvirt started? What about the QEMU driver?');

// libvirt_domain_disk_add needs a persistent guest loaded from a file
$xml = file_get_contents(__DIR__.'/qemu-no-disk-and-media.xml');
echo "# libvirt_domain_define_xml\n";
var_dump($dom = libvirt_domain_define_xml($conn, $xml));
if (!is_resource($dom)) {
    die('Domain definition failed with error: '.libvirt_get_last_error());
}

echo "# libvirt_domain_disk_add\n";
// The doc is a wrong. It says the function returns a domain resource.
// It actually returns a boolean!
var_dump($ret = libvirt_domain_disk_add($dom, $disk_image, $disk_devname, $disk_devtype, $disk_iodriver, 0));
if (!$ret) {
    die('Domain disk add failed with error: '.libvirt_get_last_error());
}

echo "# libvirt_domain_get_disk_devices\n";
$info = libvirt_domain_get_disk_devices($dom);
var_dump(is_array($info));
if (!array_key_exists('num', $info))
    die('libvirt_domain_get_disk_devices: Num element is missing in the output array');

echo "# libvirt_domain_undefine\n";
var_dump($ret = libvirt_domain_undefine($dom));
if (!$ret) {
    die('Domain undefine failed with error: '.libvirt_get_last_error());
}

unset($dom);
unset($conn);

?>
Done
--EXPECTF--
# libvirt_logfile_set
# libvirt_connect
resource(%d) of type (Libvirt connection)
# libvirt_domain_define_xml
resource(%d) of type (Libvirt domain)
# libvirt_domain_disk_add
bool(true)
# libvirt_domain_get_disk_devices
bool(true)
# libvirt_domain_undefine
bool(true)
Done