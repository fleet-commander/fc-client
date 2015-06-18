bin_PROGRAMS = fleet-commander-client
fleet_commander_client_SOURCES = main.vala
fleet_commander_client_CFLAGS  = $(JSON_GLIB_CFLAGS) $(GIO_CFLAGS) $(LIBSOUP_CFLAGS)
fleet_commander_client_LDADD   = $(JSON_GLIB_LIBS) $(GIO_LIBS) $(LIBSOUP_LIBS)
fleet_commander_client_VALAFLAGS = --pkg gio-2.0 --pkg libsoup-2.4 --pkg json-glib-1.0
