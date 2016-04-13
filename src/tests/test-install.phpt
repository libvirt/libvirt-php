--TEST--
libvirt_domain_new
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
	$logfile = __DIR__ . '/test-install.log';
	@unlink($logfile);
	libvirt_logfile_set($logfile, 10);

	$name = 'test';
	$image = __DIR__.'/test-libvirt-php.img';
	$disk_image = '/tmp/test-libvirt-php.img';
	$local_test = true;
	$show_vnc_location = false;
	$memory = 64;

	echo "# libvirt_connect\n";
	$conn = libvirt_connect('test:///default', false); /* Enable read-write connection */
	var_dump($conn);

	//cleaning
	if ($res = libvirt_domain_lookup_by_name($conn, "test")) {
		libvirt_domain_destroy($res);
		libvirt_domain_undefine($res);
	}

	$disks = array(
			array( 'path' => $disk_image, 'driver' => 'raw', 'bus' => 'ide', 'dev' => 'hda', 'size' => '1M',
				'flags' => VIR_DOMAIN_DISK_FILE | VIR_DOMAIN_DISK_ACCESS_ALL )
		);

	$networks = array(
			array( 'mac' => '00:11:22:33:44:55', 'network' => 'default', 'model' => 'e1000')
		);

	$networks = array();

	$flags = VIR_DOMAIN_FLAG_FEATURE_ACPI | VIR_DOMAIN_FLAG_FEATURE_APIC | VIR_DOMAIN_FLAG_FEATURE_PAE;

	if ($local_test)
		$flags |= VIR_DOMAIN_FLAG_TEST_LOCAL_VNC;

	echo "# libvirt_domain_new\n";
	$res = libvirt_domain_new($conn, $name, false, $memory, $memory, 1, $image, $disks, $networks, $flags);
	var_dump($res);
	if ($res == false)
		echo 'Installation test failed with error: '.libvirt_get_last_error().'. More info saved into '.$logfile;

	echo "# libvirt_domain_get_name\n";
	var_dump(libvirt_domain_get_name($res));

	echo "# libvirt_domain_get_id\n";
	var_dump(libvirt_domain_get_id($res));

	echo "# libvirt_domain_get_uuid\n";
	var_dump(bin2hex(libvirt_domain_get_uuid($res)));

	echo "# libvirt_domain_get_uuid_string\n";
	var_dump(libvirt_domain_get_uuid_string($res));

	echo "# libvirt_domain_get_info\n";
	var_dump(libvirt_domain_get_info($res));

	$ok = is_resource($res);

	echo "# libvirt_domain_new_get_vnc\n";
	$vncloc = libvirt_domain_new_get_vnc();
	var_dump($vncloc);

	echo "libvirt_domain_destroy\n";
	var_dump($ret = libvirt_domain_destroy($res));
	if (!$ret)
		echo 'Domain destroy failed with error: '.libvirt_get_last_error().'. More info saved into '.$logfile;

	unset($res);

	echo "# libvirt_domain_lookup_by_name\n";
	$res = libvirt_domain_lookup_by_name($conn, $name);
	var_dump($res);
	if (is_resource($res)) {
		echo "# libvirt_domain_undefine\n";
		var_dump(libvirt_domain_undefine($res));
	}
	unset($res);

	echo "# libvirt_domain_lookup_by_name\n";
	$res = libvirt_domain_lookup_by_name($conn, $name);
	var_dump($res);

	unset($res);
	unset($conn);

	@unlink($disk_image);
	@unlink($logfile);
?>
Done
--EXPECTF--
# libvirt_connect
resource(%d) of type (Libvirt connection)
# libvirt_domain_new
resource(%d) of type (Libvirt domain)
# libvirt_domain_get_name
string(12) "test-install"
# libvirt_domain_get_id
int(%d)
# libvirt_domain_get_uuid
string(%d) "%s"
# libvirt_domain_get_uuid_string
string(%d) "%s"
# libvirt_domain_get_info
array(5) {
  ["maxMem"]=>
  int(65536)
  ["memory"]=>
  int(65536)
  ["state"]=>
  int(1)
  ["nrVirtCpu"]=>
  int(1)
  ["cpuUsed"]=>
  float(%s)
}
# libvirt_domain_new_get_vnc
string(%d) "%s"
libvirt_domain_destroy
bool(true)
# libvirt_domain_lookup_by_name
resource(%d) of type (Libvirt domain)
# libvirt_domain_undefine
bool(true)
# libvirt_domain_lookup_by_name

%s Domain not found %s
bool(false)
Done
