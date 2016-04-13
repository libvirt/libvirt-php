--TEST--
libvirt_version
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
var_dump(libvirt_version());
?>
Done
--EXPECTF--
array(7) {
  ["libvirt.release"]=>
  int(%d)
  ["libvirt.minor"]=>
  int(%d)
  ["libvirt.major"]=>
  int(%d)
  ["connector.version"]=>
  string(%d) "%s"
  ["connector.major"]=>
  int(%d)
  ["connector.minor"]=>
  int(%d)
  ["connector.release"]=>
  int(%d)
}
Done
