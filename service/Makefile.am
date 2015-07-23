bindir = ${libexecdir}
bin_PROGRAMS = fleet-commander-client
fleet_commander_client_SOURCES =       \
	fleet-commander-client.vala    \
	fc-cache-data.vala             \
	fc-config-reader.vala          \
	fc-dconf-db-writer.vala        \
	fc-profile-cache-manager.vala  \
	fc-source-manager.vala         \
	fc-user-index.vala             \
	fc-user-session-handler.vala   \
	fc-utils.vala                  \
	logind-manager.vala            \
	fleet-commander-posix-helper.c \
	fleet-commander-posix-helper.h \
	fleet-commander-posix-helper.vapi

fleet_commander_client_CFLAGS  = $(JSON_GLIB_CFLAGS) $(GIO_CFLAGS) $(LIBSOUP_CFLAGS) -g
fleet_commander_client_LDADD   = $(JSON_GLIB_LIBS) $(GIO_LIBS) $(LIBSOUP_LIBS)
fleet_commander_client_VALAFLAGS = --pkg gio-2.0 --pkg libsoup-2.4 --pkg json-glib-1.0 --pkg posix
