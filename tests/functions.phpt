<?php
	function bail($msg, $error_code = 1)
	{
		printf("[Error $error_code] $msg\n");
		exit($error_code);
	}

	function success($name = false) {
		if ($name == false)
			bail("Invalid test name!");

		printf("Test $name has been completed successfully\n");
		exit(0);
	}

	if (!extension_loaded('libvirt')) {
		if (!dl('../src/libvirt-php.so'))
			bail('Cannot load libvirt-php extension. Please install libvirt-php first (using `make install`)');
	}
?>
