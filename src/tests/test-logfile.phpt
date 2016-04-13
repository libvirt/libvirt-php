--TEST--
libvirt_logfile_set
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php

$logfile = 'test.log';
@unlink($logfile);

echo "# libvirt_logfile_set\n";
if (!libvirt_logfile_set($logfile, 1)) {
    die('Logfile set failed with error: '.libvirt_get_last_error());
}

echo "# libvirt_connect\n";
var_dump($conn = libvirt_connect('test:///default', false));
if (!is_resource($conn)) {
    die('Connection to default hypervisor failed');
}

unset($res);
unset($conn);

$fp = fopen($logfile, 'r');
$log = fread($fp, filesize($logfile));
fclose($fp);

$logok = (strpos($log, 'libvirt_connect: Connection') &&
	  strpos($log, 'libvirt_connection_dtor: virConnectClose'));

unlink($logfile);

var_dump($logok);

?>
Done
--EXPECTF--
# libvirt_logfile_set
# libvirt_connect
resource(%d) of type (Libvirt connection)
bool(true)
Done
