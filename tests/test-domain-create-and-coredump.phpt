<?php
	require_once('functions.phpt');

	$conn = libvirt_connect('test:///default',  false);
	if (!is_resource($conn))
		bail('Connection to default hypervisor failed');

	$curdir = getcwd();
	$xml = file_get_contents($curdir.'/data/example-no-disk-and-media.xml');

	$res = libvirt_domain_create_xml($conn, $xml);
	if (!is_resource($res))
		bail('Domain definition failed with error: '.libvirt_get_last_error());

	$name = tempnam(sys_get_temp_dir(), 'guestdumptest');
	if (!libvirt_domain_core_dump($res, $name))
		bail('Cannot dump core of the guest: '.libvirt_get_last_error());

	if (!libvirt_domain_destroy($res)) {
		unlink($name);
		bail('Domain destroy failed with error: '.libvirt_get_last_error());
	}

	unset($res);
	unset($conn);
	unlink($name);

	success( basename(__FILE__) );
?>
