<?php
	function bail($msg, $error_code = 1)
	{
		printf("[Error] $msg\n");
		exit($error_code);
	}

	function success($name = false) {
		if ($name == false)
			bail("Invalid test name!");

		printf("Test $name has been completed successfully\n");
		exit(0);
	}
?>
