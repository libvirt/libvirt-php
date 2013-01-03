<?php
	require_once('functions.phpt');

	$conn = libvirt_connect('test:///default');
	if (!is_resource($conn))
		bail('Connection to default hypervisor failed');

	unset($conn);

	success( basename(__FILE__) );
?>
