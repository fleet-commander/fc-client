fc_client_confadapt_pydir = ${fcpythondir}/fleetcommanderclient/configadapters
fc_client_confadapt_py_SCRIPTS = \
	fleetcommanderclient/configadapters/__init__.py \
	fleetcommanderclient/configadapters/base.py \
	fleetcommanderclient/configadapters/goa.py \
	fleetcommanderclient/configadapters/chromium.py \
	fleetcommanderclient/configadapters/networkmanager.py \
	fleetcommanderclient/configadapters/dconf.py

fc_client_pydir = ${fcpythondir}/fleetcommanderclient
fc_client_py_SCRIPTS = \
	fleetcommanderclient/__init__.py \
	fleetcommanderclient/configloader.py \
	fleetcommanderclient/mergers.py \
	fleetcommanderclient/settingscompiler.py \
	fleetcommanderclient/fcclient.py

#fc_client_constsdir = ${fcpythondir}/fleetcommander
#fc_client_consts_in_files = fleetcommander/constants.py.in
#fc_client_consts_DATA = fleetcommander/constants.py

EXTRA_DIST = \
	$(fc_client_confadapt_py_SCRIPTS) \
	$(fc_client_py_SCRIPTS)

#CLEANFILESM = \
#	$(fc_client_consts_DATA)
