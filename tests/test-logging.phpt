<?php
	$logfile = 'test.log';

	require_once('functions.phpt');

	@unlink($logfile);
	if (!libvirt_logfile_set($logfile, 1))
		bail('Cannot enable debug logging to test.log file');

	$conn = libvirt_connect('test:///default');
	if (!is_resource($conn))
		bail('Connection to default hypervisor failed');

	$res = libvirt_node_get_info($conn);
	if (!is_array($res))
		bail('Node get info doesn\'t return an array');

	if (!is_numeric($res['memory']))
		bail('Invalid memory size');
	if (!is_numeric($res['cpus']))
		bail('Invalid CPU core count');
	unset($res);

	if (!libvirt_connect_get_uri($conn))
		bail('Invalid URI value');

	if (!libvirt_connect_get_hostname($conn))
		bail('Invalid hostname value');

	if (!($res = libvirt_domain_get_counts($conn)))
		bail('Invalid domain count');

	if ($res['active'] != count( libvirt_list_active_domains($conn)))
		bail('Numbers of active domains mismatch');

	if ($res['inactive'] != count( libvirt_list_inactive_domains($conn)))
		bail('Numbers of inactive domains mismatch');

	if (libvirt_connect_get_hypervisor($conn) == false)
		echo "Warning: Getting hypervisor information failed!\n";

	if (libvirt_connect_get_maxvcpus($conn) == false)
		echo "Warning: Cannot get the maximum number of VCPUs per VM!\n";

	if (libvirt_connect_get_capabilities($conn) == false)
		bail('Invalid capabilities on the hypervisor connection');

	if (@libvirt_connect_get_information($conn) == false)
		bail('No information on the connection are available');

	unset($res);
	unset($conn);

	$fp = fopen($logfile, 'r');
	$log = fread($fp, filesize($logfile));
	fclose($fp);

	$ok = ((strpos($log, 'libvirt_connect: Connection')) &&
		(strpos($log, 'libvirt_connection_dtor: virConnectClose')));

	unlink($logfile);

	if (!$ok)
		bail('Missing entries in the log file');

	success( basename(__FILE__) );
?>
