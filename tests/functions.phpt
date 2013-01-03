<?php
	function bail($msg, $error_code = 1)
	{
		printf("[Error $error_code in %s] $msg\n", basename($_SERVER['SCRIPT_NAME']));
		exit($error_code);
	}

	function success($name = false) {
		if ($name == false)
			bail("Invalid test name!");

		printf("Test $name has been completed successfully\n");
		exit(0);
	}

	function skip($name = false) {
		if ($name == false)
			bail("Invalid test name!");

		printf("Test $name SKIPPED\n");
		exit(1);
	}

	if (!extension_loaded('libvirt')) {
		if (!dl('../src/libvirt-php.so'))
			bail('Cannot load libvirt-php extension. Please install libvirt-php first (using `make install`)');
	}
?>
