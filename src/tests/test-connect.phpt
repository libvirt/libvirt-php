--TEST--
libvirt_version
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
	$conn = libvirt_connect('test:///default');
    var_dump($conn);
	unset($conn);
?>
Done
--EXPECTF--
resource(%d) of type (Libvirt connection)
Done
