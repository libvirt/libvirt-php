--TEST--
libvirt_version
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--INI--
libvirt.max_connections=3
--FILE--
<?php

    $num_ini = ini_get("libvirt.max_connections");
    var_dump($num_ini);

	$num = $num_ini + 1;

	for ($i = 0; $i < $num; $i++)
		$conn[] = libvirt_connect('test:///default', false);
    var_dump($conn);

	$tmp = libvirt_print_binding_resources();
    var_dump($tmp);

	for ($i = 0; $i < $num; $i++)
		unset($conn[$i]);
?>
Done
--EXPECTF--
string(1) "3"

%s Maximum number of connections allowed exceeded %s
array(4) {
  [0]=>
  resource(%d) of type (Libvirt connection)
  [1]=>
  resource(%d) of type (Libvirt connection)
  [2]=>
  resource(%d) of type (Libvirt connection)
  [3]=>
  bool(false)
}
array(3) {
  [0]=>
  string(%d) "Libvirt connection resource at %s"
  [1]=>
  string(%d) "Libvirt connection resource at %s"
  [2]=>
  string(%d) "Libvirt connection resource at %s"
}
Done
