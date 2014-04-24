NULL =

dbusconfdir = $(sysconfdir)/dbus-1/system.d
dbusconf_DATA = org.gnome.FleetCommander.conf

dbusservicedir = $(datadir)/dbus-1/system-services
dbusservice_DATA = org.gnome.FleetCommander.service

org.gnome.FleetCommander.service: Makefile
	$(AM_V_GEN) (echo '[D-Bus Service]'; \
	             echo 'Name=org.gnome.FleetCommander'; \
	             echo 'Exec=${libexecdir}/fcmdr-service'; \
	             echo 'User=root') > $@.tmp && \
	            mv $@.tmp $@

BUILT_SOURCES = $(dbusservice_DATA)

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	$(dbusconf_DATA) \
	$(NULL)

-include $(top_srcdir)/git.mk
