fc_client_dbus_servicedir = ${datarootdir}/dbus-1/system-services/
fc_client_dbus_service_in_files = org.freedesktop.FleetCommanderClient.service.in
fc_client_dbus_service_DATA = org.freedesktop.FleetCommanderClient.service

fc_client_systemd_servicedir = $(systemdsystemunitdir)
fc_client_systemd_service_in_files = fleet-commander-client.service.in
fc_client_systemd_service_DATA = fleet-commander-client.service

fc_client_ad_systemd_servicedir = $(systemduserunitdir)
fc_client_ad_systemd_service_in_files = fleet-commander-adclient.service.in
fc_client_ad_systemd_service_DATA = fleet-commander-adclient.service

fc_client_dbus_configdir = ${sysconfdir}/dbus-1/system.d/
fc_client_dbus_config_DATA = org.freedesktop.FleetCommanderClient.conf

fc_client_configdir = ${sysconfdir}/xdg/
fc_client_config_DATA = fleet-commander-client.conf

EXTRA_DIST = \
	$(fc_client_dbus_service_in_files) \
	$(fc_client_ad_systemd_service_in_files) \
	$(fc_client_dbus_config_DATA) \
	$(fc_client_systemd_service_in_files) \
	$(fc_client_config_DATA)

CLEANFILES = \
	$(fc_client_dbus_service_DATA) \
	$(fc_client_systemd_service_DATA) \
	$(fc_client_ad_systemd_service_DATA)
