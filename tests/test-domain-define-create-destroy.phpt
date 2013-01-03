<?php
	require_once('functions.phpt');

	$conn = libvirt_connect('test:///default', false);
	if (!is_resource($conn))
		bail('Connection to default hypervisor failed');

	$curdir = getcwd();
	$xml = file_get_contents($curdir.'/data/example-no-disk-and-media.xml');

	$res = libvirt_domain_define_xml($conn, $xml);
	if (!is_resource($res))
		bail('Domain definition failed with error: '.libvirt_get_last_error());

	if (!libvirt_domain_create($res))
		bail('Domain creation failed with error: '.libvirt_get_last_error());

	$info = libvirt_domain_get_info($res);
	if (!$info)
		bail('Getting domain information failed with error: '.libvirt_get_last_error());

	$cpuUsed = $info['cpuUsed'];

	sleep(5);

	$info = libvirt_domain_get_info($res);
	if (!$info)
		bail('Getting domain information failed with error: '.libvirt_get_last_error());

	if ($cpuUsed == $info['cpuUsed'])
		bail('No time difference between CPU times after 5 seconds');

	if (!libvirt_domain_destroy($res))
		bail('Domain destroy failed with error: '.libvirt_get_last_error());

	if (!libvirt_domain_undefine($res))
		bail('Domain undefinition failed with error: '.libvirt_get_last_error());

	unset($res);
	unset($conn);

	success( basename(__FILE__) );
?>
