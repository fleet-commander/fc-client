fc_client_confadapt_pydir = ${fcpythondir}/fleetcommanderclient/configadapters
fc_client_confadapt_py_SCRIPTS = \
	fleetcommanderclient/configadapters/__init__.py \
	fleetcommanderclient/configadapters/base.py \
	fleetcommanderclient/configadapters/goa.py \
	fleetcommanderclient/configadapters/chromium.py \
	fleetcommanderclient/configadapters/firefox.py \
	fleetcommanderclient/configadapters/networkmanager.py \
	fleetcommanderclient/configadapters/dconf.py

fc_client_adapters_pydir = ${fcpythondir}/fleetcommanderclient/adapters
fc_client_adapters_py_SCRIPTS = \
	fleetcommanderclient/adapters/__init__.py \
	fleetcommanderclient/adapters/base.py \
	fleetcommanderclient/adapters/goa.py \
	fleetcommanderclient/adapters/chromium.py \
	fleetcommanderclient/adapters/firefox.py \
	fleetcommanderclient/adapters/nm.py \
	fleetcommanderclient/adapters/dconf.py


fc_client_pydir = ${fcpythondir}/fleetcommanderclient
fc_client_py_SCRIPTS = \
	fleetcommanderclient/__init__.py \
	fleetcommanderclient/configloader.py \
	fleetcommanderclient/mergers.py \
	fleetcommanderclient/settingscompiler.py \
	fleetcommanderclient/fcadclient.py \
	fleetcommanderclient/fcclient.py


EXTRA_DIST = \
	$(fc_client_confadapt_py_SCRIPTS) \
	$(fc_client_adapters_py_SCRIPTS) \
	$(fc_client_py_SCRIPTS)

#CLEANFILESM = \
#	$(fc_client_consts_DATA)
