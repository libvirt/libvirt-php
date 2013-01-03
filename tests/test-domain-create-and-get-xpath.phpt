<?php
	require_once('functions.phpt');

	$conn = libvirt_connect('test:///default', false);
	if (!is_resource($conn))
		bail('Connection to default hypervisor failed');

	$curdir = getcwd();
	$xml = file_get_contents($curdir.'/data/example-no-disk-and-media.xml');

	$res = libvirt_domain_create_xml($conn, $xml);
	if (!is_resource($res))
		bail('Domain definition failed with error: '.libvirt_get_last_error());

	$info = libvirt_domain_xml_xpath($res, '/domain/name');
	if (!$info)
		bail('Cannot get domain XML and/or xPath expression');

	if (!array_key_exists('num', $info))
		bail('Num element is missing in the output array');

	for ($i = 0; $i < $info['num']; $i++)
		if (!array_key_exists($i, $info))
			bail("Element $i doesn\'t exist in the output array");

	if (!libvirt_domain_destroy($res))
		bail('Domain destroy failed with error: '.libvirt_get_last_error());

	unset($res);
	unset($conn);

	success( basename(__FILE__) );
?>
