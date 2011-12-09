<?php
	require_once('functions.phpt');

 	$res = libvirt_version();
	$virt_major = $res['libvirt.major'];
	$virt_minor = $res['libvirt.minor'];
	$virt_micro = $res['libvirt.release'];

	$bind_major = $res['connector.major'];
	$bind_minor = $res['connector.minor'];
	$bind_micro = $res['connector.release'];
	unset($res);
 
	if (libvirt_check_version($virt_major, $virt_minor, $virt_micro + 1, VIR_VERSION_LIBVIRT))
		bail("Checking against release version currently installed libvirt version + 1 failed");

	if (!libvirt_check_version($virt_major, $virt_minor, $virt_micro, VIR_VERSION_LIBVIRT))
		bail("Checking against currently installed libvirt version failed");

	if (libvirt_check_version($bind_major, $bind_minor, $bind_micro + 1, VIR_VERSION_BINDING))
		bail("Checking against release version currently installed libvirt-php version + 1 failed");

	if (!libvirt_check_version($bind_major, $bind_minor, $bind_micro, VIR_VERSION_BINDING))
		bail("Checking against currently installed libvirt-php version failed");

        if (libvirt_check_version($bind_major, $bind_minor, $bind_micro, VIR_VERSION_BINDING))
        	success( basename(__FILE__) );
        else    bail("Bad parameters analyse");
?>
