<?php
	require_once('functions.phpt');

	$conn = libvirt_connect('test:///default');
	if (!is_resource($conn))
		bail('Connection to default hypervisor failed');

	$nicstestok = false;
	$soundhwtestok = false;
	$soundhw = @libvirt_connect_get_soundhw_models($conn, NULL, VIR_CONNECT_FLAG_SOUNDHW_GET_NAMES);
	if (is_bool($soundhw)) {
		if (libvirt_get_last_error())
			$soundhwtestok = true;
	}
	$nics = @libvirt_connect_get_nic_models($conn);
	if (is_bool($nics)) {
		if (libvirt_get_last_error())
			$nicstestok = true;
	}

	if ((!$soundhwtestok) || (!$nicstestok))
		bail('Module seems to be able to get NICs models and/or sound hardware types but it shouldn\'t. Failing test...');

	unset($conn);

	success( basename(__FILE__) );
?>
