<?php
	$name = 'test';
	$image = '/tmp/test-libvirt-php.tmp';
	$local_test = true;
	$show_vnc_location = false;
	$memory = 64;

	require_once('functions.phpt');

	$conn = libvirt_connect('null', false); /* Enable read-write connection */
	if (!is_resource($conn))
		bail('Connection to default hypervisor failed');

	$disks = array(
			array( 'path' => '/tmp/test-libvirt-php.img', 'driver' => 'raw', 'bus' => 'ide', 'dev' => 'hda', 'size' => '1M',
				'flags' => VIR_DOMAIN_DISK_FILE | VIR_DOMAIN_DISK_ACCESS_ALL )
		);

	$networks = array(
			array( 'mac' => '00:11:22:33:44:55', 'network' => 'default', 'model' => 'e1000')
		);

	$networks = array();

	$flags = VIR_DOMAIN_FLAG_FEATURE_ACPI | VIR_DOMAIN_FLAG_FEATURE_APIC | VIR_DOMAIN_FLAG_FEATURE_PAE;

	if ($local_test)
		$flags |= VIR_DOMAIN_FLAG_TEST_LOCAL_VNC;

	$res = libvirt_domain_new($conn, $name, false, $memory, $memory, 1, $image, $disks, $networks, $flags);
	$ok = is_resource($res);

	$vncloc = libvirt_domain_new_get_vnc();

	if (!libvirt_domain_destroy($res))
		bail('Domain destroy failed with error: '.libvirt_get_last_error());

	unset($res);

	$res = libvirt_domain_lookup_by_name($conn, $name);
	if (is_resource($res))
		libvirt_domain_undefine($res);

	unset($res);
	unset($conn);

	@unlink('/tmp/test-libvirt-php.img');

	if ($ok) {
		if ($show_vnc_location)
			echo "VNC Server location is: $vncloc\n";
		success( basename(__FILE__) );
	}
	else
		bail('Invalid domain resource');
?>
