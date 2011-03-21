#!/bin/bash

php test-connect.phpt				|| exit 1
php test-version-check.phpt			|| exit 1
php test-version-get.phpt			|| exit 1
php test-domain-define-undefine.phpt		|| exit 1
php test-domain-define-create-destroy.phpt	|| exit 1
php test-domain-create.phpt			|| exit 1
php test-domain-create-and-get-xpath.phpt	|| exit 1
php test-domain-create-and-coredump.phpt	|| exit 1

echo "All tests passed successfully"
exit 0
