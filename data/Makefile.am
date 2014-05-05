NULL =

dbusconfdir = $(sysconfdir)/dbus-1/system.d
dbusconf_DATA = org.gnome.FleetCommander.conf

dbusservicedir = $(datadir)/dbus-1/system-services
dbusservice_DATA = org.gnome.FleetCommander.service

profiledir = $(sysconfdir)/profile.d
profile_DATA = fleet-commander.sh fleet-commander.csh

org.gnome.FleetCommander.service: Makefile
	$(AM_V_GEN) (echo '[D-BUS Service]'; \
	             echo 'Name=org.gnome.FleetCommander'; \
	             echo 'Exec=${libexecdir}/fcmdr-service'; \
	             echo 'SystemdService=fleet-commander.service'; \
	             echo 'User=root') > $@.tmp && \
	            mv $@.tmp $@

if HAVE_SYSTEMD
systemdservicedir = $(systemdsystemunitdir)
systemdservice_DATA = fleet-commander.service

fleet-commander.service: Makefile
	$(AM_V_GEN) (echo '[Unit]'; \
	             echo 'Description=Settings Lockdown'; \
	             echo; \
	             echo '[Service]'; \
	             echo 'Type=dbus'; \
	             echo 'BusName=org.gnome.FleetCommander'; \
	             echo 'ExecStart=${libexecdir}/fcmdr-service') > $@.tmp && \
	            mv $@.tmp $@
endif

BUILT_SOURCES = \
	$(dbusservice_DATA) \
	$(systemdservice_DATA) \
	$(NULL)

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	$(dbusconf_DATA) \
	$(profile_DATA) \
	$(NULL)

-include $(top_srcdir)/git.mk
