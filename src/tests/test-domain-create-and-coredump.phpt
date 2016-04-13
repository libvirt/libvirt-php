--TEST--
libvirt_domain_core_dump
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
	echo "# libvirt_connect\n";
	var_dump($conn = libvirt_connect('test:///default',  false));
	if (!is_resource($conn))
		die('Connection to default hypervisor failed');

	$xml = file_get_contents(__DIR__.'/example-no-disk-and-media.xml');

	echo "# libvirt_domain_create_xml\n";
	var_dump($res = libvirt_domain_create_xml($conn, $xml));
	if (!is_resource($res))
		die('Domain definition failed with error: '.libvirt_get_last_error());

	$name = tempnam(sys_get_temp_dir(), 'guestdumptest');
	echo "# libvirt_domain_core_dump\n";
	var_dump($ret = libvirt_domain_core_dump($res, $name));
	if (!$ret)
		die('Cannot dump core of the guest: '.libvirt_get_last_error());
	var_dump(file_exists($name));

	echo "# libvirt_domain_destroy\n";
	var_dump($ret = libvirt_domain_destroy($res));
	if (!$ret) {
		unlink($name);
		die('Domain destroy failed with error: '.libvirt_get_last_error());
	}

	unset($res);
	unset($conn);
	unlink($name);
?>
Done
--EXPECTF--
# libvirt_connect
resource(%d) of type (Libvirt connection)
# libvirt_domain_create_xml
resource(%d) of type (Libvirt domain)
# libvirt_domain_core_dump
bool(true)
bool(true)
# libvirt_domain_destroy
bool(true)
Done
