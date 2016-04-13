--TEST--
libvirt_domain_lookup_by_name
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php

	echo "# libvirt_connect\n";
	var_dump($conn = libvirt_connect('test:///default', false));
	if (!is_resource($conn))
		die('Connection to default hypervisor failed');
	// Loads domain of type 'test', name 'test-guest-no-disk-and-media'
	$xml = file_get_contents(__DIR__.'/example-no-disk-and-media.xml');

	echo "# libvirt_domain_create_xml\n";
	var_dump($res = libvirt_domain_create_xml($conn, $xml));
	if (!is_resource($res))
		die('Domain definition failed with error: '.libvirt_get_last_error());

	echo "# libvirt_domain_lookup_by_name\n";
	var_dump($dom = libvirt_domain_lookup_by_name($conn, "test"));

	unset($dom);
	unset($res);
	unset($conn);
?>
Done
--EXPECTF--
# libvirt_connect
resource(%d) of type (Libvirt connection)
# libvirt_domain_create_xml
resource(%d) of type (Libvirt domain)
# libvirt_domain_lookup_by_name
resource(%d) of type (Libvirt domain)
Done
