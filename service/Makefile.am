NULL =

libexec_PROGRAMS = fcmdr-service

$(dbus_built_sources) : Makefile.am org.gnome.FleetCommander.xml
	$(AM_V_GEN) gdbus-codegen \
	--interface-prefix org.gnome.FleetCommander \
	--c-namespace FCmdr \
	--generate-c-code fcmdr-generated \
	--generate-docbook fcmdr-generated-doc \
	$(top_srcdir)/service/org.gnome.FleetCommander.xml \
	$(NULL)

dbus_built_sources = \
	fcmdr-generated.c \
	fcmdr-generated.h \
	fcmdr-generated-doc-org.gnome.FleetCommander.Main.xml \
	$(NULL)

BUILT_SOURCES = \
	$(dbus_built_sources) \
	$(NULL)

fcmdr_service_CFLAGS = \
	-DG_LOG_DOMAIN=\"fleet-commander\" \
	$(GIO_CFLAGS) \
	$(NULL)

fcmdr_service_LDADD = \
	$(GIO_LIBS) \
	$(NULL)

fcmdr_service_SOURCES = \
	$(BUILT_SOURCES) \
	fcmdr-login-monitor.c \
	fcmdr-login-monitor.h \
	fcmdr-logind-monitor.c \
	fcmdr-logind-monitor.h \
	fcmdr-service.c \
	fcmdr-service.h \
	main.c \
	$(NULL)

CLEANFILES = $(BUILT_SOURCES)

DISTCLEANFILES = $(dbusservice_DATA)

EXTRA_DIST = \
	org.gnome.FleetCommander.xml \
	$(NULL)

-include $(top_srcdir)/git.mk
