<?php
	require_once('functions.phpt');

	ob_start();
	PHPInfo();
	$s = ob_get_contents();
	ob_end_clean();

	$s = strstr($s, 'Max. connections =>');
	$tmp = explode("\n", $s);
	$tmp = explode('=>', $tmp[0]);
	$num_ini = (int)$tmp[1];

	$num = $num_ini + 1;

	for ($i = 0; $i < $num; $i++)
		$conn[] = @libvirt_connect('test:///default', false);

	$tmp = libvirt_print_binding_resources();
	if (sizeof($tmp) > $num_ini)
		bail('Allocated '.sizeof($tmp).' connection resources but limits seems to be set to '.$num_ini.' resources');

	for ($i = 0; $i < $num; $i++)
		unset($conn[$i]);

	success( basename(__FILE__) );
?>
