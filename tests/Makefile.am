TESTS_ENVIRONMENT = export PATH=$(abs_top_srcdir)/tests/tools:$(PATH); export TOPSRCDIR=$(abs_top_srcdir); export FC_TESTING=true;
TESTS = 01_mergers.py 02_settingscompiler.py 03_configadapter_goa.py 04_configadapter_nm.py 05_configadapter_dconf.py 06_configadapter_chromium.py 07_configloader.py 08_fcclient.sh

EXTRA_DIST = \
	$(TESTS) \
	_fcclient_tests.py \
	test_fcclient_service.py \
	data/test_config_file.conf \
	data/sampleprofiledata/0050-0050-0000-0000-0000-Test1.profile \
	data/sampleprofiledata/0060-0060-0000-0000-0000-Test2.profile \
	data/sampleprofiledata/0070-0070-0000-0000-0000-Invalid.profile \
	data/sampleprofiledata/0090-0090-0000-0000-0000-Test3.profile \
	tools/dconf
