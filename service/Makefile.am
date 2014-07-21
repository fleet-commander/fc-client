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
	fcmdr-generated-doc-org.gnome.FleetCommander.DConf.xml \
	fcmdr-generated-doc-org.gnome.FleetCommander.Profiles.xml \
	$(NULL)

BUILT_SOURCES = \
	$(dbus_built_sources) \
	$(NULL)

fcmdr_service_CFLAGS = \
	-DG_LOG_DOMAIN=\"fcmdr-service\" \
	-DLOCALSTATEDIR=\""$(localstatedir)"\" \
	$(JSON_GLIB_CFLAGS) \
	$(LIBSOUP_CFLAGS) \
	$(GIO_CFLAGS) \
	$(NULL)

fcmdr_service_LDADD = \
	$(JSON_GLIB_LIBS) \
	$(LIBSOUP_LIBS) \
	$(GIO_LIBS) \
	$(NULL)

fcmdr_service_SOURCES = \
	$(BUILT_SOURCES) \
	fcmdr-extensions.c \
	fcmdr-extensions.h \
	fcmdr-http-profile-source.c \
	fcmdr-http-profile-source.h \
	fcmdr-profile.c \
	fcmdr-profile.h \
	fcmdr-profile-applies.c \
	fcmdr-profile-applies.h \
	fcmdr-profile-source.c \
	fcmdr-profile-source.h \
	fcmdr-service.c \
	fcmdr-service.h \
	fcmdr-service-backend.c \
	fcmdr-service-backend.h \
	fcmdr-gsettings-backend.c \
	fcmdr-gsettings-backend.h \
	fcmdr-utils.c \
	fcmdr-utils.h \
	fcmdr-yum-backend.c \
	fcmdr-yum-backend.h \
	main.c \
	$(NULL)

CLEANFILES = $(BUILT_SOURCES)

DISTCLEANFILES = $(dbusservice_DATA)

EXTRA_DIST = \
	org.gnome.FleetCommander.xml \
	$(NULL)

-include $(top_srcdir)/git.mk
