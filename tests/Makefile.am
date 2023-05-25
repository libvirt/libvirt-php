test_scripts = \
	runtests.sh \
	check_parameters.sh

TESTS = $(test_scripts)

EXTRA_DIST = \
	check_parameters.sh.in \
	check_parameters.php \
	runtests.sh.in \
	runtests-report.sh \
	data \
	functions.phpt.in \
	php.ini \
	test-connect.phpt \
	test-conn-limit.phpt \
	test-domain-create-and-coredump.phpt \
	test-domain-create-and-get-xpath.phpt \
	test-domain-create-get-metadata.phpt \
	test-domain-create.phpt \
	test-domain-define-create-destroy.phpt \
	test-domain-define-undefine.phpt \
	test-domain-snapshot.phpt \
	test-get-emulator.phpt \
	test-get-models.phpt \
	test-get-models-unsupported-hw.phpt \
	test-install.phpt \
	test-logging.phpt \
	test-version-get.phpt \
	test-version-check.phpt

runtests.sh: functions.phpt

functions.phpt: functions.phpt.in
	$(AM_V_GEN)sed \
		-e 's|[@]abs_srcdir[@]|$(abs_srcdir)|g' \
	< $< > $@-t && \
	chmod a+x $@-t && \
	mv $@-t $@

TESTS_ENVIRONMENT = \
  abs_builddir="$(abs_builddir)" \
  abs_srcdir="$(abs_srcdir)" \
  LC_ALL=C

CLEANFILES = functions.phpt
