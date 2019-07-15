TESTS_ENVIRONMENT = export PATH=$(abs_top_srcdir)/tests/tools:$(abs_top_srcdir)/tests:$(PATH); export TOPSRCDIR=$(abs_top_srcdir); export PYTHON=@PYTHON@; export FC_TESTING=true;
TESTS = 01_mergers.py 02_settingscompiler.py 03_configadapter_goa.py 04_configadapter_nm.py 05_configadapter_dconf.py 06_configadapter_chromium.py 07_configadapter_firefox.py 08_configloader.py 09_fcclient.sh 10_fcadretriever.py 11_adapter_chromium.py 12_adapter_firefox.py 13_adapter_goa.py 14_adapter_dconf.py 15_adapter_nm.py 16_fcclientad.sh

EXTRA_DIST = \
	$(TESTS) \
	python-wrapper.sh \
	_fcclient_tests.py \
	_fcclientad_tests.py \
	test_fcclient_service.py \
	test_fcclientad_service.py \
	ldapmock.py \
	smbmock.py \
	data/test_config_file.conf \
	data/sampleprofiledata/0050-0050-0000-0000-0000-Test1.profile \
	data/sampleprofiledata/0060-0060-0000-0000-0000-Test2.profile \
	data/sampleprofiledata/0070-0070-0000-0000-0000-Invalid.profile \
	data/sampleprofiledata/0090-0090-0000-0000-0000-Test3.profile \
	tools/dconf
