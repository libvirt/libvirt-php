--TEST--
libvirt_domain_define_xml
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php

	echo "# libvirt_connect\n";
	var_dump($conn = libvirt_connect('test:///default',  false));
	if (!is_resource($conn))
		die('Connection to default hypervisor failed');

	$xml = file_get_contents(__DIR__.'/example-no-disk-and-media.xml');

	echo "# libvirt_domain_define_xml\n";
	var_dump($dom = libvirt_domain_define_xml($conn, $xml));
	if (!is_resource($dom))
		die('Domain definition failed with error: '.libvirt_get_last_error());

	echo "# libvirt_domain_undefine\n";
	var_dump($ret = libvirt_domain_undefine($dom));
	if (!$ret) {
		unlink($name);
		die('Domain undefine failed with error: '.libvirt_get_last_error());
	}

	unset($dom);
	unset($conn);
?>
Done
--EXPECTF--
# libvirt_connect
resource(%d) of type (Libvirt connection)
# libvirt_domain_define_xml
resource(%d) of type (Libvirt domain)
# libvirt_domain_undefine
bool(true)
Done
