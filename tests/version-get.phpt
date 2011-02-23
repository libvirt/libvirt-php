<?php
	require_once('functions.phpt');

	if (!is_array( libvirt_version() ) )
		bail("Libvirt version doesn't return an array");

	success( basename(__FILE__) );
?>
