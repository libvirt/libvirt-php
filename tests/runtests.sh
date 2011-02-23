#!/bin/bash

php test-connect.phpt  || exit 1
php version-check.phpt || exit 1
php version-get.phpt   || exit 1

echo "All tests passed successfully"
exit 0
