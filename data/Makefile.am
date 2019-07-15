fc_client_dbus_servicedir = ${datarootdir}/dbus-1/system-services/
fc_client_dbus_service_in_files = org.freedesktop.FleetCommanderClient.service.in
fc_client_dbus_service_DATA = org.freedesktop.FleetCommanderClient.service

fc_client_dbus_configdir = ${sysconfdir}/dbus-1/system.d/
fc_client_dbus_config_DATA = org.freedesktop.FleetCommanderClient.conf

fc_client_systemd_servicedir = $(systemdsystemunitdir)
fc_client_systemd_service_in_files = fleet-commander-client.service.in
fc_client_systemd_service_DATA = fleet-commander-client.service


fc_client_ad_dbus_servicedir = ${datarootdir}/dbus-1/system-services/
fc_client_ad_dbus_service_in_files = org.freedesktop.FleetCommanderClientAD.service.in
fc_client_ad_dbus_service_DATA = org.freedesktop.FleetCommanderClientAD.service

fc_client_ad_dbus_configdir = ${sysconfdir}/dbus-1/system.d/
fc_client_ad_dbus_config_DATA = org.freedesktop.FleetCommanderClientAD.conf

fc_client_ad_systemd_servicedir = $(systemdsystemunitdir)
fc_client_ad_systemd_service_in_files = fleet-commander-clientad.service.in
fc_client_ad_systemd_service_DATA = fleet-commander-clientad.service


fc_client_adretriever_systemd_servicedir = $(systemduserunitdir)
fc_client_adretriever_systemd_service_in_files = fleet-commander-adretriever.service.in
fc_client_adretriever_systemd_service_DATA = fleet-commander-adretriever.service


fc_client_configdir = ${sysconfdir}/xdg/
fc_client_config_DATA = fleet-commander-client.conf

EXTRA_DIST = \
	$(fc_client_dbus_service_DATA) \
	$(fc_client_dbus_config_DATA) \
	$(fc_client_systemd_service_DATA) \
	$(fc_client_ad_dbus_service_DATA) \
	$(fc_client_ad_dbus_config_DATA) \
	$(fc_client_ad_systemd_service_DATA) \
	$(fc_client_adretriever_systemd_service_DATA) \
	$(fc_client_config_DATA)

CLEANFILES = \
	$(fc_client_dbus_service_DATA) \
	$(fc_client_systemd_service_DATA) \
	$(fc_client_ad_dbus_service_DATA) \
	$(fc_client_ad_systemd_service_DATA) \
	$(fc_client_adretriever_systemd_service_DATA)