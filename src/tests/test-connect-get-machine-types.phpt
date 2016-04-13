--TEST--
libvirt_connect_get_machine_types
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php

	echo "# libvirt_connect\n";
	var_dump($conn = libvirt_connect('test:///default', false));
	if (!is_resource($conn))
		die('Connection to default hypervisor failed');

	echo "# libvirt_connect_get_machine_types\n";
	$res = libvirt_connect_get_machine_types($conn);
	if (!$res) {
            die('Get machine types failed with error: '.libvirt_get_last_error());
        }
	if (!is_array($res)) {
            die("libvirt_connect_get_machine_types returned unexpected value: " . print_r($res, true) .
                "\nFailed with error: ".libvirt_get_last_error());
        }
	unset($res);
	unset($conn);
?>
Done
--EXPECTF--
# libvirt_connect
resource(%d) of type (Libvirt connection)
# libvirt_connect_get_machine_types
Done
