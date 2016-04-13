--TEST--
libvirt_domain_xml_xpath
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
	echo "# libvirt_connect\n";
	var_dump($conn = libvirt_connect('test:///default', false));
	if (!is_resource($conn))
		die('Connection to default hypervisor failed');

	$xml = file_get_contents(__DIR__.'/example-no-disk-and-media.xml');

	echo "# libvirt_domain_create_xml\n";
	var_dump($res = libvirt_domain_create_xml($conn, $xml));
	if (!is_resource($res))
		die('Domain definition failed with error: '.libvirt_get_last_error());

	$info = libvirt_domain_xml_xpath($res, '/domain/name');
	if (!$info)
		die('Cannot get domain XML and/or xPath expression');
	var_dump($info);

	if (!array_key_exists('num', $info))
		die('Num element is missing in the output array');

	for ($i = 0; $i < $info['num']; $i++)
		if (!array_key_exists($i, $info))
			die("Element $i doesn\'t exist in the output array");

	echo "# libvirt_domain_destroy\n";
	var_dump($ret = libvirt_domain_destroy($res));
	if (!$ret) {
		die('Domain destroy failed with error: '.libvirt_get_last_error());
	}
	unset($res);
	unset($conn);
?>
Done
--EXPECTF--
# libvirt_connect
resource(%d) of type (Libvirt connection)
# libvirt_domain_create_xml
resource(%d) of type (Libvirt domain)
array(3) {
  [0]=>
  string(28) "test-guest-no-disk-and-media"
  ["num"]=>
  int(1)
  ["xpath"]=>
  string(12) "/domain/name"
}
# libvirt_domain_destroy
bool(true)
Done
