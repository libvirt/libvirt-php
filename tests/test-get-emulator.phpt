<?php
	$logfile = '/tmp/test.log';
	require_once('functions.phpt');

	unlink($logfile);
	if (!libvirt_logfile_set($logfile, 1))
		bail('Cannot enable debug logging to test.log file');

	$conn = libvirt_connect('null');
	if (!is_resource($conn))
		bail('Connection to default hypervisor failed');

	$tmp = libvirt_connect_get_emulator($conn);
	if (!is_string($tmp))
		bail('Cannot get default emulator');

	$tmp = libvirt_connect_get_emulator($conn, 'i686');
	if (!is_string($tmp))
		bail('Cannot get emulator for i686 architecture');

	$tmp = libvirt_connect_get_emulator($conn, 'x86_64');
	if (!is_string($tmp))
		bail('Cannot get emulator for x86_64 architecture');

	unset($tmp);
	unset($conn);

	success( basename(__FILE__) );
?>
