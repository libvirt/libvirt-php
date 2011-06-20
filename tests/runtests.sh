#!/bin/bash

php test-connect.phpt				|| exit 1
php test-version-check.phpt			|| exit 1
php test-version-get.phpt			|| exit 1
php test-domain-define-undefine.phpt		|| exit 1
php test-domain-define-create-destroy.phpt	|| exit 1
php test-domain-create.phpt			|| exit 1
php test-domain-create-and-get-xpath.phpt	|| exit 1
php test-domain-create-and-coredump.phpt	|| exit 1
php test-logging.phpt				|| exit 1

qemu-img create -f qcow2 /tmp/example-test.qcow2 1M > /dev/null
php test-domain-snapshot.phpt			|| exit 1
rm -f /tmp/example-test.qcow2

echo "All tests passed successfully"
exit 0
