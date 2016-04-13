--TEST--
libvirt_connect_get_capabilities
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php

	echo "# libvirt_connect\n";
	var_dump($conn = libvirt_connect('test:///default', false));
	if (!is_resource($conn))
		die('Connection to default hypervisor failed');

	echo "# libvirt_connect_get_capabilities\n";
	var_dump($res = libvirt_connect_get_emulator($conn));
	if (!$res) {
		die('Connect get capabilities failed with error: '.libvirt_get_last_error());
	}

	unset($conn);
?>
Done
--EXPECTF--
# libvirt_connect
resource(%d) of type (Libvirt connection)
# libvirt_connect_get_capabilities
string(%d) "%s"
Done
