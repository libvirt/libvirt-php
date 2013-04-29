<?php
	require_once('functions.phpt');

	$conn = @libvirt_connect(NULL, false);
	if (!is_resource($conn))
		skip( basename(__FILE__) );

	//cleaning
	if ($res = @libvirt_domain_lookup_by_name($conn, "test-guest-qcow")) {
		@libvirt_domain_destroy($res);
		@libvirt_domain_undefine($res);
	}

	$curdir = getcwd();
	$xml = file_get_contents($curdir.'/data/example-qcow2-disk.xml');

	/* This is applicable only for QEMU/KVM so check whether we're on QEMU/KVM */
	$e = libvirt_connect_get_emulator($conn);
	$t = explode('/', $e);
	if (substr( $t[ sizeof($t) - 1], 0, 4) != 'qemu') {
		echo "Not running on KVM hypervisor. Skipping ...\n";
		success( basename(__FILE__) );
	}

	$res = libvirt_domain_create_xml($conn, $xml);
	if (!is_resource($res))
		bail('Domain definition failed with error: '.libvirt_get_last_error());

	if (!($snapshot = libvirt_domain_has_current_snapshot($res)) && !is_null(libvirt_get_last_error()))
		bail('An error occured while getting domain snapshot: '.libvirt_get_last_error());

	if (!is_resource($snapshot_res = libvirt_domain_snapshot_create($res)))
		bail('Error on creating snapshot: '.libvirt_get_last_error());

	if (!($xml = libvirt_domain_snapshot_get_xml($snapshot_res)))
		bail('Error on getting the snapshot XML description: '.libvirt_get_last_error());

	if (!$xml)
		bail('Empty XML description string');

	if (!libvirt_domain_has_current_snapshot($res))
		bail('Domain should be having current snapshot but it\'s not having it');

	if (!libvirt_domain_snapshot_revert($snapshot_res))
		bail('Cannot revert to the domain snapshot taken now: '.libvirt_get_last_error());

	if (!($snapshots=libvirt_list_domain_snapshots($res)))
		bail('Domain snapshots listing query failed: '.libvirt_get_last_error());

	for ($i = 0; $i < sizeof($snapshots); $i++) {
		$cur = libvirt_domain_snapshot_lookup_by_name($res, $snapshots[$i]);
		libvirt_domain_snapshot_delete($cur);
		unset($cur);
	}

	unset($snapshot_res);

	$snapshot_res = libvirt_domain_snapshot_create($res);
	if (!libvirt_domain_snapshot_delete($snapshot_res))
		bail('Cannot delete snapshot with children: '.libvirt_get_last_error());

	if (!libvirt_domain_destroy($res))
		bail('Domain destroy failed with error: '.libvirt_get_last_error());

	unset($res);
	unset($conn);

	success( basename(__FILE__) );
?>
