<?php
	require_once('functions.phpt');

	$conn = libvirt_connect('test:///default');
	if (!is_resource($conn))
		bail('Connection to default hypervisor failed');

	$tmp = libvirt_connect_get_emulator($conn);
	if (!is_string($tmp))
		bail('Cannot get default emulator');

	$tmp = libvirt_connect_get_emulator($conn, 'i686');
	if (!is_string($tmp))
		bail('Cannot get emulator for i686 architecture');


	unset($tmp);
	unset($conn);

	success( basename(__FILE__) );
?>
