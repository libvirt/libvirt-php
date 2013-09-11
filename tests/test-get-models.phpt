<?php
	require_once('functions.phpt');

	$conn = libvirt_connect(NULL);
	if (!is_resource($conn))
		bail('Connection to default hypervisor failed');

	$soundhw = libvirt_connect_get_soundhw_models($conn, NULL, VIR_CONNECT_FLAG_SOUNDHW_GET_NAMES);
	$nics = libvirt_connect_get_nic_models($conn);

	if ((is_bool($soundhw)) || (is_bool($nics)))
		bail('Cannot get sound hardware or network card models');

	unset($conn);

	success( basename(__FILE__) );
?>
